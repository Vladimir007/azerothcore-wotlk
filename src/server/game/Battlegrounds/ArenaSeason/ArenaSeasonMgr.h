#ifndef ARENA_SEASON_MGR_H
#define ARENA_SEASON_MGR_H

#include <unordered_map>
#include <vector>
#include "ArenaTeamFilter.h"

enum ArenaSeasonState
{
    ARENA_SEASON_STATE_DISABLED    = 0,
    ARENA_SEASON_STATE_IN_PROGRESS = 1
};

enum ArenaSeasonRewardType
{
    ARENA_SEASON_REWARD_TYPE_ITEM,
    ARENA_SEASON_REWARD_TYPE_ACHIEVEMENT
};

enum ArenaSeasonRewardGroupCriteriaType
{
    ARENA_SEASON_REWARD_CRITERIA_TYPE_PERCENT_VALUE,
    ARENA_SEASON_REWARD_CRITERIA_TYPE_ABSOLUTE_VALUE
};

// ArenaSeasonReward represents one reward, it can be an item or achievement.
struct ArenaSeasonReward
{
    ArenaSeasonReward() = default;

    // Item or achievement entry.
    uint32 entry{};

    ArenaSeasonRewardType type{ARENA_SEASON_REWARD_TYPE_ITEM};

    // Used in unit tests.
    bool operator==(const ArenaSeasonReward& other) const
    {
        return entry == other.entry && type == other.type;
    }
};

struct ArenaSeasonRewardGroup
{
    ArenaSeasonRewardGroup() = default;

    uint8 season{};

    ArenaSeasonRewardGroupCriteriaType criteriaType;

    float minCriteria{};
    float maxCriteria{};

    uint32 rewardMailTemplateID{};
    std::string rewardMailSubject{};
    std::string rewardMailBody{};
    uint32 goldReward{};

    std::vector<ArenaSeasonReward> itemRewards;
    std::vector<ArenaSeasonReward> achievementRewards;

    // Used in unit tests.
    bool operator==(const ArenaSeasonRewardGroup& other) const
    {
        return minCriteria == other.minCriteria &&
               maxCriteria == other.maxCriteria &&
               criteriaType == other.criteriaType &&
               itemRewards == other.itemRewards &&
               achievementRewards == other.achievementRewards;
    }
};

class ArenaSeasonMgr
{
public:
    static ArenaSeasonMgr* instance();

    using ArenaSeasonRewardGroupsBySeasonContainer = std::unordered_map<uint8, std::vector<ArenaSeasonRewardGroup>>;

    // Loading functions
    void LoadRewards();
    void LoadActiveSeason();

    // Season management functions
    void ChangeCurrentSeason(uint8 season);
    uint8 GetCurrentSeason() { return _currentSeason; }

    void SetSeasonState(ArenaSeasonState state);
    ArenaSeasonState GetSeasonState() { return _currentSeasonState; }

    // Season completion functions
    void RewardTeamsForTheSeason(const std::shared_ptr<ArenaTeamFilter>& teamsFilter);
    bool CanDeleteArenaTeams();
    void DeleteArenaTeams();

private:
    static uint16 GameEventForArenaSeason(uint8 season);
    static void BroadcastUpdatedWorldState();

    ArenaSeasonRewardGroupsBySeasonContainer _arenaSeasonRewardGroupsStore;

    uint8 _currentSeason{};
    ArenaSeasonState _currentSeasonState{};
};

#define sArenaSeasonMgr ArenaSeasonMgr::instance()

#endif
