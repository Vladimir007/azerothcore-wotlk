#include "ArenaSeasonMgr.h"
#include "ArenaSeasonRewardsDistributor.h"
#include "ArenaTeamMgr.h"
#include "BattlegroundMgr.h"
#include "GameEventMgr.h"
#include "MapMgr.h"
#include "Player.h"

ArenaSeasonMgr* ArenaSeasonMgr::instance()
{
    static ArenaSeasonMgr instance;
    return &instance;
}

void ArenaSeasonMgr::LoadRewards()
{
    const uint32 oldMSTime = getMSTime();

    std::unordered_map<std::string, ArenaSeasonRewardGroupCriteriaType> stringToArenaSeasonRewardGroupCriteriaType = {
        {"pct", ARENA_SEASON_REWARD_CRITERIA_TYPE_PERCENT_VALUE},
        {"abs", ARENA_SEASON_REWARD_CRITERIA_TYPE_ABSOLUTE_VALUE}
    };

    QueryResult result = WorldDatabase.Query("SELECT id, arena_season, criteria_type, min_criteria, max_criteria, mail_template, mail_subject, mail_body, gold_reward FROM world_arena_season_reward_group");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 arena season rewards. DB table `arena_season_reward_group` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    std::unordered_map<uint32, ArenaSeasonRewardGroup> groupsMap;

    do
    {
        const Field* fields = result->Fetch();
        uint32 id = fields[0].Get<uint32>();

        ArenaSeasonRewardGroup group;
        group.season               = fields[1].Get<uint8>();
        group.criteriaType         = stringToArenaSeasonRewardGroupCriteriaType[fields[2].Get<std::string>()];
        group.minCriteria          = fields[3].Get<float>();
        group.maxCriteria          = fields[4].Get<float>();
        group.rewardMailTemplateID = fields[5].Get<uint32>();
        group.rewardMailSubject    = fields[6].Get<std::string>();
        group.rewardMailBody       = fields[7].Get<std::string>();
        group.goldReward           = fields[8].Get<uint32>();

        groupsMap[id] = group;
    } while (result->NextRow());

    std::unordered_map<std::string, ArenaSeasonRewardType> stringToArenaSeasonRewardType = {
        {"achievement", ARENA_SEASON_REWARD_TYPE_ACHIEVEMENT},
        {"item", ARENA_SEASON_REWARD_TYPE_ITEM}
    };

    result = WorldDatabase.Query("SELECT group, type, entry FROM world_arena_season_reward");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 arena season rewards. DB table `arena_season_reward` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    do
    {
        const Field* fields = result->Fetch();
        uint32 groupId = fields[0].Get<uint32>();

        ArenaSeasonReward reward;
        reward.type  = stringToArenaSeasonRewardType[fields[1].Get<std::string>()];
        reward.entry = fields[2].Get<uint32>();

        auto itr = groupsMap.find(groupId);
        ASSERT(itr != groupsMap.end(), "Unknown arena_season_reward_group ({}) in arena_season_reward", groupId);

        (reward.type == ARENA_SEASON_REWARD_TYPE_ITEM) ?
            groupsMap[groupId].itemRewards.push_back(reward) :
            groupsMap[groupId].achievementRewards.push_back(reward);

    } while (result->NextRow());

    for (const auto& val : groupsMap | std::views::values)
        _arenaSeasonRewardGroupsStore[val.season].push_back(val);

    LOG_INFO("server.loading", ">> Loaded {} arena season rewards in {} ms", static_cast<uint32>(groupsMap.size()), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ArenaSeasonMgr::LoadActiveSeason()
{
    const QueryResult result = CharacterDatabase.Query("SELECT season_id, season_state FROM active_arena_season");
    ASSERT(result, "active_arena_season can't be empty");

    const Field* fields = result->Fetch();
    _currentSeason      = fields[0].Get<uint8>();
    _currentSeasonState = static_cast<ArenaSeasonState>(fields[1].Get<uint8>());

    const uint16 eventID = GameEventForArenaSeason(_currentSeason);
    sGameEventMgr->StartEvent(eventID, true);

    LOG_INFO("server.loading", "Arena Season {} loaded...", _currentSeason);
    LOG_INFO("server.loading", " ");
}

void ArenaSeasonMgr::RewardTeamsForTheSeason(const std::shared_ptr<ArenaTeamFilter>& teamsFilter)
{
    auto rewarder = ArenaSeasonTeamRewarderImpl();
    auto distributor = ArenaSeasonRewardDistributor(&rewarder);
    std::vector<ArenaSeasonRewardGroup> rewards = _arenaSeasonRewardGroupsStore[GetCurrentSeason()];
    ArenaTeamMgr::ArenaTeamContainer filteredTeams = teamsFilter->Filter(sArenaTeamMgr->GetArenaTeams());
    distributor.DistributeRewards(filteredTeams, rewards);
}

bool ArenaSeasonMgr::CanDeleteArenaTeams()
{
    if (_arenaSeasonRewardGroupsStore[GetCurrentSeason()].empty())
        return false;

    for (auto const& bg : sBattlegroundMgr->GetActiveBattlegrounds())
        if (bg->isRated())
            return false;

    return true;
}

void ArenaSeasonMgr::DeleteArenaTeams()
{
    if (!CanDeleteArenaTeams())
        return;

    // Cleanup queue first.
    std::vector arenasQueueTypes = {BATTLEGROUND_QUEUE_2v2, BATTLEGROUND_QUEUE_3v3, BATTLEGROUND_QUEUE_5v5};
    for (const BattlegroundQueueTypeId queueType : arenasQueueTypes)
    {
        for (auto queue = sBattlegroundMgr->GetBattlegroundQueue(queueType); const auto& playerGUID : queue.m_QueuedPlayers | std::views::keys)
            queue.RemovePlayer(playerGUID, true);
    }

    sArenaTeamMgr->DeleteAllArenaTeams();
}

void ArenaSeasonMgr::ChangeCurrentSeason(const uint8 season)
{
    if (_currentSeason == season)
        return;

    const uint16 currentEventID = GameEventForArenaSeason(_currentSeason);
    sGameEventMgr->StopEvent(currentEventID, true);

    const uint16 newEventID = GameEventForArenaSeason(season);
    sGameEventMgr->StartEvent(newEventID, true);

    _currentSeason = season;
    _currentSeasonState = ARENA_SEASON_STATE_IN_PROGRESS;
    CharacterDatabase.Execute("UPDATE active_arena_season SET season_id=$1, season_state=$2", _currentSeason, _currentSeasonState);
    BroadcastUpdatedWorldState();
}

void ArenaSeasonMgr::SetSeasonState(const ArenaSeasonState state)
{
    if (_currentSeasonState == state)
        return;
    _currentSeasonState = state;
    CharacterDatabase.Execute("UPDATE active_arena_season SET season_state=$1", _currentSeasonState);
    BroadcastUpdatedWorldState();
}

uint16 ArenaSeasonMgr::GameEventForArenaSeason(uint8 season)
{
    const QueryResult result = WorldDatabase.Query("SELECT event FROM world_game_event_arena_season WHERE season=$1", season);

    if (!result)
    {
        LOG_ERROR("ArenaSeasonMgr", "ArenaSeason ({}) must be an existent Arena Season", season);
        return 0;
    }

    const Field* fields = result->Fetch();
    return fields[0].Get<uint16>();
}

void ArenaSeasonMgr::BroadcastUpdatedWorldState()
{
    sMapMgr->DoForAllMaps([](Map* map)
    {
        // Ignore instanceable maps, players will get a fresh state once they change the map.
        if (map->Instanceable())
            return;

        map->DoForAllPlayers([&](Player* player)
        {
            uint32 currZone, currArea;
            player->GetZoneAndAreaId(currZone, currArea);
            player->SendInitWorldStates(currZone, currArea);
        });
    });
}
