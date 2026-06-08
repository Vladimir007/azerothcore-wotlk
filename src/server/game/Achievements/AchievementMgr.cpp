#include "AchievementMgr.h"
#include "ArenaTeam.h"
#include "ArenaTeamMgr.h"
#include "Battleground.h"
#include "CellImpl.h"
#include "Chat.h"
#include "ChatTextBuilder.h"
#include "Common.h"
#include "DatabaseEnv.h"
#include "DBCDefines.h"
#include "DisableMgr.h"
#include "Duration.h"
#include "GameEventMgr.h"
#include "GameTime.h"
#include "GridNotifiersImpl.h"
#include "Guild.h"
#include "GuildMgr.h"
#include "InstanceScript.h"
#include "Language.h"
#include "Map.h"
#include "MapMgr.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "RaceMgr.h"
#include "ReputationMgr.h"
#include "ScriptMgr.h"
#include "SpellMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSessionMgr.h"

bool AchievementCriteriaData::IsValid(const AchievementCriteriaEntry* criteria)
{
    const auto tableName = "world_achievement_criteria_data";
    if (dataType >= MAX_ACHIEVEMENT_CRITERIA_DATA_TYPE)
    {
        LOG_ERROR("sql.sql", "Table `{}` for criteria (Entry: {}) has wrong data type ({}), ignored.", tableName, criteria->ID, dataType);
        return false;
    }

    switch (criteria->RequiredType)
    {
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
        case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:  // Only hardcoded list
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_ARENA:
        case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:
        case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL:
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT:
        case ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
        case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST:  // Only Children's Week achievements
        case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:  // Only Children's Week achievements
        case ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS:
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
        case ACHIEVEMENT_CRITERIA_TYPE_ON_LOGIN:
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_EPIC_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_RECEIVE_EPIC_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_OWN_RANK:
            break;
        default:
            if (dataType != ACHIEVEMENT_CRITERIA_DATA_TYPE_SCRIPT)
            {
                LOG_ERROR("sql.sql", "Table `{}` has data for non-supported criteria type (Entry: {} Type: {}), ignored.",
                    tableName, criteria->ID, criteria->RequiredType);
                return false;
            }
            break;
    }

    switch (dataType)
    {
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_NONE:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_INSTANCE_SCRIPT:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_NTH_BIRTHDAY:
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_CREATURE:
            if (!creature.id || !sObjectMgr->GetCreatureTemplate(creature.id))
            {
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) for data type {} ({}) has non-existing creature id in value1 ({}), ignored.",
                    tableName, criteria->ID, criteria->RequiredType, "ACHIEVEMENT_CRITERIA_DATA_TYPE_CREATURE", dataType, creature.id);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_CLASS_RACE:
            if (classRace.classID && ((1 << (classRace.classID - 1)) & CLASS_MASK_ALL_PLAYABLE) == 0)
            {
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) for data type {} ({}) has non-existing class in value1 ({}), ignored.",
                    tableName, criteria->ID, criteria->RequiredType, "ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_CLASS_RACE", dataType, classRace.classID);
                return false;
            }
            if (classRace.raceID && ((1 << (classRace.raceID - 1)) & sRaceMgr->GetPlayableRaceMask()) == 0)
            {
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) for data type {} ({}) has non-existing race in value2 ({}), ignored.",
                    tableName, criteria->ID, criteria->RequiredType, "ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_CLASS_RACE", dataType, classRace.raceID);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_LESS_HEALTH:
            if (health.percent < 1 || health.percent > 100)
            {
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) for data type {} ({}) has wrong percent value in value1 ({}), ignored.",
                    tableName, criteria->ID, criteria->RequiredType, "ACHIEVEMENT_CRITERIA_DATA_TYPE_PLAYER_LESS_HEALTH", dataType, health.percent);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_DEAD:
            if (player_dead.ownTeamFlag > 1)
            {
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) for data type {} ({}) has wrong boolean value1 ({}).",
                    tableName, criteria->ID, criteria->RequiredType, "ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_DEAD", dataType, player_dead.ownTeamFlag);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_AURA:
            {
                SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(aura.spellID);
                if (!spellInfo)
                {
                    LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) for data type {} ({}) has wrong spell id in value1 ({}), ignored.",
                        tableName, criteria->ID, criteria->RequiredType,
                        dataType == ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA ? "ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA" : "ACHIEVEMENT_CRITERIA_DATA_TYPE_T_AURA",
                        dataType, aura.spellID);
                    return false;
                }
                if (aura.effectIDX >= 3)
                {
                    LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) for data type {} ({}) has wrong spell effect index in value2 ({}), ignored.",
                        tableName, criteria->ID, criteria->RequiredType,
                        dataType == ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA ? "ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA" : "ACHIEVEMENT_CRITERIA_DATA_TYPE_T_AURA",
                        dataType, aura.effectIDX);
                    return false;
                }
                if (!spellInfo->Effects[aura.effectIDX].ApplyAuraName)
                {
                    LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) for data type {} ({}) has non-aura spell effect (ID: {} Effect: {}), ignores.",
                        tableName, criteria->ID, criteria->RequiredType,
                        dataType == ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA ? "ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA" : "ACHIEVEMENT_CRITERIA_DATA_TYPE_T_AURA",
                        dataType, aura.spellID, aura.effectIDX);
                    return false;
                }
                return true;
            }
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AREA:
            if (!sAreaTableStore.LookupEntry(area.id))
            {
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) for data type {} ({}) has wrong area id in value1 ({}), ignored.",
                    tableName, criteria->ID, criteria->RequiredType, "ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AREA", dataType, area.id);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_VALUE:
            if (value.compType >= COMP_TYPE_MAX)
            {
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) for data type {} ({}) has wrong ComparisonType in value2 ({}), ignored.",
                    tableName, value.compType, criteria->RequiredType, "ACHIEVEMENT_CRITERIA_DATA_TYPE_VALUE", dataType, value.value);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_LEVEL:
            if (level.minLevel > STRONG_MAX_LEVEL)
            {
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) for data type {} ({}) has wrong minLevel in value1 ({}), ignored.",
                    tableName, criteria->ID, criteria->RequiredType, "ACHIEVEMENT_CRITERIA_DATA_TYPE_T_LEVEL", dataType, level.minLevel);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_GENDER:
            if (gender.gender > GENDER_NONE)
            {
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) for data type {} ({}) has wrong gender in value1 ({}), ignored.",
                    tableName, criteria->ID, criteria->RequiredType, "ACHIEVEMENT_CRITERIA_DATA_TYPE_T_GENDER", dataType, gender.gender);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_SCRIPT:
            if (!ScriptId)
            {
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) for data type {} ({}) does not have ScriptName set, ignored.",
                    tableName, criteria->ID, criteria->RequiredType, "ACHIEVEMENT_CRITERIA_DATA_TYPE_SCRIPT", dataType);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_DIFFICULTY:
            if (difficulty.difficulty >= MAX_DIFFICULTY)
            {
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) for data type {} ({}) has wrong difficulty in value1 ({}), ignored.",
                    tableName, criteria->ID, criteria->RequiredType, "ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_DIFFICULTY", dataType, difficulty.difficulty);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_PLAYER_COUNT:
            if (map_players.maxCount <= 0)
            {
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) for data type {} ({}) has wrong max players count in value1 ({}), ignored.",
                    tableName, criteria->ID, criteria->RequiredType, "ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_PLAYER_COUNT", dataType, map_players.maxCount);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_TEAM:
            if (team.team != ALLIANCE && team.team != HORDE)
            {
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) for data type {} ({}) has unknown team in value1 ({}), ignored.",
                    tableName, criteria->ID, criteria->RequiredType, "ACHIEVEMENT_CRITERIA_DATA_TYPE_T_TEAM", dataType, team.team);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_DRUNK:
            if (drunk.state >= MAX_DRUNKEN)
            {
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) for data type {} ({}) has unknown drunken state in value1 ({}), ignored.",
                    tableName, criteria->ID, criteria->RequiredType, "ACHIEVEMENT_CRITERIA_DATA_TYPE_S_DRUNK", dataType, drunk.state);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_HOLIDAY:
            if (!sHolidaysStore.LookupEntry(holiday.id))
            {
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) for data type {} ({}) has unknown holiday in value1 ({}), ignored.",
                    tableName, criteria->ID, criteria->RequiredType, "ACHIEVEMENT_CRITERIA_DATA_TYPE_HOLIDAY", dataType, holiday.id);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_BG_LOSS_TEAM_SCORE:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_BG_TEAMS_SCORES:
            return true;  // Not check correctness node indexes
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_EQUIPPED_ITEM:
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_ITEM_QUALITY:
            if (equipped_item.itemQuality >= MAX_ITEM_QUALITY)
            {
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) for requirement {} ({}) has unknown quality state in value1 ({}), ignored.",
                    tableName, criteria->ID, criteria->RequiredType, "ACHIEVEMENT_CRITERIA_REQUIRE_S_EQUIPPED_ITEM", dataType, equipped_item.itemQuality);
                return false;
            }
            return true;
        case ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_ID:
            if (!sMapStore.LookupEntry(map_id.mapID))
            {
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) for requirement {} ({}) has unknown map id in value1 ({}), ignored.",
                    tableName, criteria->ID, criteria->RequiredType, "ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_ID", dataType, map_id.mapID);
                return false;
            }
            return true;
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_PLAYER_CLASS_RACE:
        if (!classRace.classID && !classRace.raceID)
        {
            LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) for data type {} ({}) must not have 0 in either value field, ignored.",
                      tableName, criteria->ID, criteria->RequiredType, "ACHIEVEMENT_CRITERIA_DATA_TYPE_S_PLAYER_CLASS_RACE", dataType);
            return false;
        }
        if (classRace.classID && ((1 << (classRace.classID - 1)) & CLASS_MASK_ALL_PLAYABLE) == 0)
        {
            LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) for data type {} ({}) has non-existing class in value1 ({}), ignored.",
                      tableName, criteria->ID, criteria->RequiredType, "ACHIEVEMENT_CRITERIA_DATA_TYPE_S_PLAYER_CLASS_RACE", dataType, classRace.classID);
            return false;
        }
        if (classRace.raceID && ((1 << (classRace.raceID - 1)) & sRaceMgr->GetPlayableRaceMask()) == 0)
        {
            LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) for data type {} ({}) has non-existing race in value2 ({}), ignored.",
                      tableName, criteria->ID, criteria->RequiredType, "ACHIEVEMENT_CRITERIA_DATA_TYPE_S_PLAYER_CLASS_RACE", dataType, classRace.raceID);
            return false;
        }
        return true;
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_KNOWN_TITLE:
        {
            if (!sCharTitlesStore.LookupEntry(known_title.titleID))
            {
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) for requirement {} ({}) have unknown title_id in value1 ({}), ignore.",
                          tableName, criteria->ID, criteria->RequiredType, "ACHIEVEMENT_CRITERIA_DATA_TYPE_S_KNOWN_TITLE", dataType, known_title.titleID);
                return false;
            }
            return true;
        }
    default:
        LOG_ERROR("sql.sql", "Table `{}` (Entry: {} Type: {}) has data for non-supported data type ({}), ignored.",
                  tableName, criteria->ID, criteria->RequiredType, dataType);
        return false;
    }
}

bool AchievementCriteriaData::Meets(uint32 criteria_id, Player const* source, Unit const* target, const uint32 misc_value1 /*= 0*/) const
{
    switch (dataType)
    {
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_NONE:
        return true;
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_CREATURE:
        if (!target || !target->IsCreature())
            return false;
        return target->GetEntry() == creature.id;
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_CLASS_RACE:
        if (!target || !target->IsPlayer())
            return false;
        if (classRace.classID && classRace.classID != target->ToPlayer()->getClass())
            return false;
        if (classRace.raceID && classRace.raceID != target->ToPlayer()->getRace())
            return false;
        return true;
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_PLAYER_CLASS_RACE:
        if (!source || !source->IsPlayer())
            return false;
        if (classRace.classID && classRace.classID != source->ToPlayer()->getClass())
            return false;
        if (classRace.raceID && classRace.raceID != source->ToPlayer()->getRace())
            return false;
        return true;
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_LESS_HEALTH:
        if (!target || !target->IsPlayer())
            return false;
        return !target->HealthAbovePct(health.percent);
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_DEAD:
        if (target && !target->IsAlive())
            if (const Player* player = target->ToPlayer())
                if (player->GetDeathTimer() != 0)
                    // flag set == must be same team, not set == different team
                    return (player->GetTeamId() == source->GetTeamId()) == (player_dead.ownTeamFlag != 0);
        return false;
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA:
        return source->HasAuraEffect(aura.spellID, aura.effectIDX);
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AREA:
        {
            uint32 zone_id, area_id;
            source->GetZoneAndAreaId(zone_id, area_id);
            return area.id == zone_id || area.id == area_id;
        }
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_AURA:
        return target && target->HasAuraEffect(aura.spellID, aura.effectIDX);
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_VALUE:
        return CompareValues(static_cast<ComparisonType>(value.compType), misc_value1, value.value);
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_LEVEL:
        if (!target)
            return false;
        return target->GetLevel() >= level.minLevel;
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_GENDER:
        if (!target)
            return false;
        return target->getGender() == gender.gender;
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_SCRIPT:
        return sScriptMgr->OnCriteriaCheck(ScriptId, const_cast<Player*>(source), const_cast<Unit*>(target), criteria_id);
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_DIFFICULTY:
        {
            if (source->GetMap()->IsRaid())
                if (source->GetMap()->Is25ManRaid() != ((difficulty.difficulty & RAID_DIFFICULTY_MASK_25MAN) != 0))
                    return false;

            const AchievementCriteriaEntry* criteria = sAchievementCriteriaStore.LookupEntry(criteria_id);
            const uint8 spawnMode = source->GetMap()->GetSpawnMode();
            // Dungeons completed on heroic mode count towards both in general achievement, but not in statistics.
            return sAchievementMgr->IsStatisticCriteria(criteria) ? spawnMode == difficulty.difficulty : spawnMode >= difficulty.difficulty;
        }
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_PLAYER_COUNT:
        return source->GetMap()->GetPlayersCountExceptGMs() <= map_players.maxCount;
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_T_TEAM:
        {
            if (!target || !target->IsPlayer())
                return false;

            // DB data compatibility...
            const uint32 teamOld = target->ToPlayer()->GetTeamId() == TEAM_ALLIANCE ? ALLIANCE : HORDE;
            return teamOld == team.team;
        }
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_DRUNK:
        return Player::GetDrunkenstateByValue(source->GetDrunkValue()) >= static_cast<DrunkenState>(drunk.state);
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_HOLIDAY:
        return IsHolidayActive(static_cast<HolidayIds>(holiday.id));
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_BG_LOSS_TEAM_SCORE:
        {
            const Battleground* bg = source->GetBattleground();
            if (!bg)
                return false;

            const uint32 score = bg->GetTeamScore(source->GetTeamId() == TEAM_ALLIANCE ? TEAM_HORDE : TEAM_ALLIANCE);
            return score >= bg_loss_team_score.minScore && score <= bg_loss_team_score.maxScore;
        }
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_INSTANCE_SCRIPT:
        {
            if (!source->IsInWorld())
                return false;
            Map* map = source->GetMap();
            if (!map->IsDungeon())
            {
                LOG_ERROR("sql.sql", "Achievement system call {} ({}) for achievement criteria {} for non-dungeon/non-raid map {}",
                          "ACHIEVEMENT_CRITERIA_DATA_TYPE_INSTANCE_SCRIPT", ACHIEVEMENT_CRITERIA_DATA_TYPE_INSTANCE_SCRIPT, criteria_id, map->GetId());
                return false;
            }
            InstanceScript* instance = map->ToInstanceMap()->GetInstanceScript();
            if (!instance)
            {
                LOG_ERROR("sql.sql", "Achievement system call {} ({}) for achievement criteria {} for map {} but map does not have a instance script",
                          "ACHIEVEMENT_CRITERIA_DATA_TYPE_INSTANCE_SCRIPT", ACHIEVEMENT_CRITERIA_DATA_TYPE_INSTANCE_SCRIPT, criteria_id, map->GetId());
                return false;
            }
            return instance->CheckAchievementCriteriaMeet(criteria_id, source, target, misc_value1);
        }
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_EQUIPPED_ITEM:
        {
            ItemTemplate const* pProto = sObjectMgr->GetItemTemplate(misc_value1);
            if (!pProto)
                return false;
            return pProto->ItemLevel >= equipped_item.itemLevel && pProto->Quality >= equipped_item.itemQuality;
        }
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_ID:
        return source->GetMapId() == map_id.mapID;
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_NTH_BIRTHDAY:
        {
            tm birthday_tm = Acore::Time::TimeBreakdown(sWorld->getIntConfig(CONFIG_BIRTHDAY_TIME));

            // Exactly N birthday
            birthday_tm.tm_year += birthday_login.nthBirthday;

            const time_t birthday = mktime(&birthday_tm);
            const time_t now = GameTime::GetGameTime().count();
            return now <= birthday + DAY && now >= birthday;
        }
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_KNOWN_TITLE:
        {
            if (CharTitlesEntry const* titleInfo = sCharTitlesStore.LookupEntry(known_title.titleID))
                return source && source->HasTitle(titleInfo->BitIndex);
            return false;
        }
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_S_ITEM_QUALITY:
        {
            ItemTemplate const* pProto = sObjectMgr->GetItemTemplate(misc_value1);
            if (!pProto)
                return false;
            return pProto->Quality == item.itemQuality;
        }
    case ACHIEVEMENT_CRITERIA_DATA_TYPE_BG_TEAMS_SCORES:
        {
            const Battleground* bg = source->GetBattleground();
            if (!bg)
                return false;

            const TeamID winnerTeam = GetTeamId(bg->GetWinner());
            if (winnerTeam == TEAM_NEUTRAL)
                return false;

            const uint32 winnerScore = bg->GetTeamScore(winnerTeam);
            const uint32 loserScore = bg->GetTeamScore(static_cast<TeamID>(!static_cast<uint32>(winnerTeam)));
            return source->GetTeamId() == winnerTeam && winnerScore == teams_scores.winnerScore && loserScore == teams_scores.loserScore;
        }
    default:
        break;
    }
    return false;
}

bool AchievementCriteriaDataSet::Meets(Player const* source, Unit const* target, const uint32 misc_value /*= 0*/) const
{
    for (auto itr = _storage.begin(); itr != _storage.end(); ++itr)
        if (!itr->Meets(_criteria_id, source, target, misc_value))
            return false;
    return true;
}

AchievementMgr::AchievementMgr(Player* player)
{
    _player = player;
    _offlineUpdatesDelayTimer = 0;
}

AchievementMgr::~AchievementMgr()
{
}

void AchievementMgr::Reset()
{
    for (CompletedAchievementMap::const_iterator iter = _completedAchievements.begin(); iter != _completedAchievements.end(); ++iter)
    {
        WorldPacket data(SMSG_ACHIEVEMENT_DELETED, 4);
        data << iter->first;
        _player->SendDirectMessage(&data);
    }

    for (CriteriaProgressMap::const_iterator iter = _criteriaProgress.begin(); iter != _criteriaProgress.end(); ++iter)
    {
        WorldPacket data(SMSG_CRITERIA_DELETED, 4);
        data << iter->first;
        _player->SendDirectMessage(&data);
    }

    _completedAchievements.clear();
    _criteriaProgress.clear();
    DeleteFromDB(_player->GetGUID().GetCounter());

    // Re-fill data
    CheckAllAchievementCriteria();
}

void AchievementMgr::DeleteFromDB(const ObjectGuid::LowType lowGuid)
{
    const CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_ACHIEVEMENT);
    stmt->SetData(0, lowGuid);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_ACHIEVEMENT_PROGRESS);
    stmt->SetData(0, lowGuid);
    trans->Append(stmt);

    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_ACHIEVEMENT_OFFLINE_UPDATES);
    stmt->SetData(0, lowGuid);
    trans->Append(stmt);

    CharacterDatabase.CommitTransaction(trans);
}

void AchievementMgr::LoadFromDB(const QueryResult& achievementResult, const QueryResult& criteriaResult, const QueryResult& offlineUpdatesResult)
{
    if (achievementResult)
    {
        do
        {
            const Field* fields = achievementResult->Fetch();
            uint32 achievementID = fields[0].Get<uint16>();

            // Must not happen: cleanup at server startup in sAchievementMgr->LoadCompletedAchievements()
            const AchievementEntry* achievement = sAchievementStore.LookupEntry(achievementID);
            if (!achievement)
                continue;

            auto& [date, changed] = _completedAchievements[achievementID];
            date = static_cast<time_t>(fields[1].Get<uint32>());
            changed = false;

            // Title achievement rewards are retroactive
            if (const AchievementReward* reward = sAchievementMgr->GetAchievementReward(achievement))
                if (const uint32 titleId = reward->titleId[Player::TeamIdForRace(GetPlayer()->getRace())])
                    if (const CharTitlesEntry* titleEntry = sCharTitlesStore.LookupEntry(titleId))
                        if (!GetPlayer()->HasTitle(titleEntry))
                            GetPlayer()->SetTitle(titleEntry);
        } while (achievementResult->NextRow());
    }

    if (criteriaResult)
    {
        do
        {
            const Field* fields = criteriaResult->Fetch();
            uint32 id = fields[0].Get<uint16>();
            const uint32 counter = fields[1].Get<uint32>();
            const time_t date = fields[2].Get<uint32>();

            const AchievementCriteriaEntry* criteria = sAchievementCriteriaStore.LookupEntry(id);
            if (!criteria)
            {
                // We will remove not existed criteria for all characters
                LOG_ERROR("achievement", "Non-existing achievement criteria {} data removed from table `character_achievement_progress`.", id);
                CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_INVALID_ACHIEV_PROGRESS_CRITERIA);
                stmt->SetData(0, static_cast<uint16>(id));
                CharacterDatabase.Execute(stmt);
                continue;
            }

            if (criteria->TimeLimit && date + criteria->TimeLimit < GameTime::GetGameTime().count())
                continue;

            auto& [_counter, _date, _changed] = _criteriaProgress[id];
            _counter = counter;
            _date = date;
            _changed = false;
        } while (criteriaResult->NextRow());
    }

    if (offlineUpdatesResult)
    {
        uint32 count = 0;
        do
        {
            const Field* fields = offlineUpdatesResult->Fetch();

            AchievementOfflinePlayerUpdate update;
            update.updateType = static_cast<AchievementOfflinePlayerUpdateType>(fields[0].Get<uint8>());
            update.arg1 = fields[1].Get<uint32>();
            update.arg2 = fields[2].Get<uint32>();
            update.arg3 = fields[3].Get<uint32>();
            _offlineUpdatesQueue.push_back(update);

            ++count;
        } while (offlineUpdatesResult->NextRow());

        if (count > 0)
            _offlineUpdatesDelayTimer = 5 * SECOND * IN_MILLISECONDS;
    }
}

void AchievementMgr::SaveToDB(const CharacterDatabaseTransaction& trans)
{
    if (!_completedAchievements.empty())
    {
        for (auto iter = _completedAchievements.begin(); iter != _completedAchievements.end(); ++iter)
        {
            if (!iter->second.changed)
                continue;

            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_ACHIEVEMENT_BY_ACHIEVEMENT);
            stmt->SetData(0, iter->first);
            stmt->SetData(1, GetPlayer()->GetGUID().GetCounter());
            trans->Append(stmt);

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHAR_ACHIEVEMENT);
            stmt->SetData(0, GetPlayer()->GetGUID().GetCounter());
            stmt->SetData(1, iter->first);
            stmt->SetData(2, static_cast<uint32>(iter->second.date));
            trans->Append(stmt);

            iter->second.changed = false;

            sScriptMgr->OnPlayerAchievementSave(trans, GetPlayer(), iter->first, iter->second);
        }
    }

    if (!_criteriaProgress.empty())
    {
        for (auto iter = _criteriaProgress.begin(); iter != _criteriaProgress.end(); ++iter)
        {
            if (!iter->second.changed)
                continue;

            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_ACHIEVEMENT_PROGRESS_BY_CRITERIA);
            stmt->SetData(0, GetPlayer()->GetGUID().GetCounter());
            stmt->SetData(1, iter->first);
            trans->Append(stmt);

            // Insert only for (counter != 0) is very important! This is how criteria of completed achievements gets deleted from db (by setting counter to 0).
            if (iter->second.counter)
            {
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHAR_ACHIEVEMENT_PROGRESS);
                stmt->SetData(0, GetPlayer()->GetGUID().GetCounter());
                stmt->SetData(1, iter->first);
                stmt->SetData(2, iter->second.counter);
                stmt->SetData(3, static_cast<uint32>(iter->second.date));
                trans->Append(stmt);
            }

            iter->second.changed = false;

            sScriptMgr->OnPlayerCriteriaSave(trans, GetPlayer(), iter->first, iter->second);
        }
    }
}

void AchievementMgr::ResetAchievementCriteria(AchievementCriteriaCondition condition, uint32 value, bool evenIfCriteriaComplete)
{
    // Disable for game masters with GM-mode enabled
    if (_player->IsGameMaster())
        return;

    LOG_DEBUG("achievement", "AchievementMgr::ResetAchievementCriteria({}, {}, {})", condition, value, evenIfCriteriaComplete);

    const AchievementCriteriaEntryList* achievementCriteriaList = sAchievementMgr->GetAchievementCriteriaByCondition(condition, value);
    if (!achievementCriteriaList)
        return;

    for (auto i = achievementCriteriaList->begin(); i != achievementCriteriaList->end(); ++i)
    {
        const AchievementCriteriaEntry* achievementCriteria = *i;
        const AchievementEntry* achievement = sAchievementStore.LookupEntry(achievementCriteria->ReferredAchievement);
        if (!achievement)
            continue;

        // Don't update already completed criteria if not forced or achievement already complete
        if ((IsCompletedCriteria(achievementCriteria, achievement) && !evenIfCriteriaComplete) || HasAchieved(achievement->ID))
            continue;

        RemoveCriteriaProgress(achievementCriteria);
    }
}

// Called at player login. The player might have fulfilled some achievements when the achievement system wasn't working yet.
void AchievementMgr::CheckAllAchievementCriteria()
{
    // Suppress sending packets
    for (uint32 i = 0; i < ACHIEVEMENT_CRITERIA_TYPE_TOTAL; ++i)
        UpdateAchievementCriteria(static_cast<AchievementCriteriaTypes>(i));
}

void AchievementMgr::SendAchievementEarned(const AchievementEntry* achievement) const
{
    if (GetPlayer()->GetSession()->PlayerLoading())
        return;

    // Don't send for achievements with ACHIEVEMENT_FLAG_TRACKING
    if (achievement->Flags & ACHIEVEMENT_FLAG_HIDDEN)
        return;

    LOG_DEBUG("achievement", "AchievementMgr::SendAchievementEarned({})", achievement->ID);

    Guild* guild = sGuildMgr->GetGuildById(GetPlayer()->GetGuildId());
    if (guild)
    {
        Acore::BroadcastTextBuilder _builder(GetPlayer(), CHAT_MSG_GUILD_ACHIEVEMENT, BROADCAST_TEXT_ACHIEVEMENT_EARNED, GetPlayer()->getGender(), GetPlayer(), achievement->ID);
        Acore::LocalizedPacketDo _localizer(_builder);
        guild->BroadcastWorker(_localizer, GetPlayer());
    }

    if (achievement->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_KILL | ACHIEVEMENT_FLAG_REALM_FIRST_REACH))
    {
        // If guild exists - send its name to the server
        // If guild does not exist - send player's name to the server
        if (achievement->Flags & ACHIEVEMENT_FLAG_REALM_FIRST_KILL && guild)
        {
            WorldPacket data(SMSG_SERVER_FIRST_ACHIEVEMENT, guild->GetName().size() + 1 + 8 + 4 + 4);
            data << guild->GetName();
            data << GetPlayer()->GetGUID();
            data << achievement->ID;
            data << static_cast<uint32>(0);  // Display name as plain string in chat (always 0 for guild)
            sWorldSessionMgr->SendGlobalMessage(&data);
        }
        else
        {
            const TeamID teamId = GetPlayer()->GetTeamId();

            // Broadcast realm first reached
            WorldPacket data(SMSG_SERVER_FIRST_ACHIEVEMENT, GetPlayer()->GetName().size() + 1 + 8 + 4 + 4);
            data << GetPlayer()->GetName();
            data << GetPlayer()->GetGUID();
            data << achievement->ID;
            const std::size_t linkTypePos = data.wpos();
            data << static_cast<uint32>(1);  // Display name as clickable link in chat
            sWorldSessionMgr->SendGlobalMessage(&data, nullptr, teamId);

            data.put<uint32>(linkTypePos, 0);  // Display name as plain string in chat
            sWorldSessionMgr->SendGlobalMessage(&data, nullptr, teamId == TEAM_ALLIANCE ? TEAM_HORDE : TEAM_ALLIANCE);
        }
    }

    // If player is in world he can tell his friends about new achievement
    else if (GetPlayer()->IsInWorld())
    {
        Acore::BroadcastTextBuilder _builder(GetPlayer(), CHAT_MSG_ACHIEVEMENT, BROADCAST_TEXT_ACHIEVEMENT_EARNED, GetPlayer()->getGender(), GetPlayer(), achievement->ID);
        Acore::LocalizedPacketDo _localizer(_builder);
        Acore::PlayerDistWorker _worker(GetPlayer(), sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY), _localizer);
        Cell::VisitObjects(GetPlayer(), _worker, sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY));
    }

    WorldPacket data(SMSG_ACHIEVEMENT_EARNED, 8 + 4 + 8);
    data << GetPlayer()->GetPackGUID();
    data << achievement->ID;
    data.AppendPackedTime(GameTime::GetGameTime().count());
    data << static_cast<uint32>(0);
    GetPlayer()->SendMessageToSetInRange(&data, sWorld->getFloatConfig(CONFIG_LISTEN_RANGE_SAY), true);
}

void AchievementMgr::SendCriteriaUpdate(AchievementCriteriaEntry const* entry, CriteriaProgress const* progress, const uint32 timeElapsed, const bool timedCompleted) const
{
    WorldPacket data(SMSG_CRITERIA_UPDATE, 8 + 4 + 8);
    data << entry->ID;

    // The counter is packed like a packed Guid
    data.appendPackGUID(progress->counter);

    data << GetPlayer()->GetPackGUID();
    if (!entry->TimeLimit)
        data << static_cast<uint32>(0);
    else
        data << static_cast<uint32>(timedCompleted ? 0 : 1); // 1 is for keeping the counter at 0 in client
    data.AppendPackedTime(progress->date);
    data << timeElapsed;  // Time elapsed in seconds

    if (sAchievementMgr->IsAverageCriteria(entry))
        data << static_cast<uint32>(GameTime::GetGameTime().count() - GetPlayer()->GetCreationTime().count());  // For average achievements
    else
        data << timeElapsed;  // Time elapsed in seconds

    GetPlayer()->SendDirectMessage(&data);
}

static constexpr uint32 achievementIdByArenaSlot[MAX_ARENA_SLOT] = { 1057, 1107, 1108 };
static const uint32 achievementIdForDungeon[][4] =
{
    // ach_cr_id, is_dungeon, is_raid, is_heroic_dungeon
    { 321,     true,       true,    true  },
    { 916,     false,      true,    false },
    { 917,     false,      true,    false },
    { 918,     true,       false,   false },
    { 2219,    false,      false,   true  },
    { 0,       false,      false,   false }
};

// This function will be called whenever the user might have done a criteria relevant action
void AchievementMgr::UpdateAchievementCriteria(AchievementCriteriaTypes type, uint32 miscValue1 /*= 0*/, uint32 miscValue2 /*= 0*/, Unit* unit /*= nullptr*/)
{
    // Disable for game masters with GM-mode enabled
    if (_player->IsGameMaster())
        return;

    if (type >= ACHIEVEMENT_CRITERIA_TYPE_TOTAL)
    {
        LOG_DEBUG("achievement", "UpdateAchievementCriteria: Wrong criteria type {}", type);
        return;
    }

    LOG_DEBUG("achievement", "AchievementMgr::UpdateAchievementCriteria({}, {}, {})", type, miscValue1, miscValue2);

    const AchievementCriteriaEntryList* achievementCriteriaList = nullptr;

    switch (type)
    {
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND:
        case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:
        case ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
        case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL_AT_AREA:
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL:
        case ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL:
        case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA:
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REPUTATION:
        case ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS:
        case ACHIEVEMENT_CRITERIA_TYPE_HK_RACE:
        case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:
        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT:
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
        case ACHIEVEMENT_CRITERIA_TYPE_FISH_IN_GAMEOBJECT:
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS:
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE:
            if (miscValue1)
            {
                achievementCriteriaList = sAchievementMgr->GetSpecialAchievementCriteriaByType(type, miscValue1);
                break;
            }
            achievementCriteriaList = sAchievementMgr->GetAchievementCriteriaByType(type);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM:
            if (miscValue2)
            {
                achievementCriteriaList = sAchievementMgr->GetSpecialAchievementCriteriaByType(type, miscValue2);
                break;
            }
            achievementCriteriaList = sAchievementMgr->GetAchievementCriteriaByType(type);
            break;
        default:
            achievementCriteriaList = sAchievementMgr->GetAchievementCriteriaByType(type);
            break;
    }

    if (!achievementCriteriaList)
        return;

    sScriptMgr->OnBeforeCheckCriteria(this, achievementCriteriaList);

    for (auto i = achievementCriteriaList->begin(); i != achievementCriteriaList->end(); ++i)
    {
        AchievementCriteriaEntry const* achievementCriteria = *i;
        AchievementEntry const* achievement = sAchievementStore.LookupEntry(achievementCriteria->ReferredAchievement);
        if (!achievement)
            continue;

        if (!CanUpdateCriteria(achievementCriteria, achievement))
            continue;

        if (!sScriptMgr->CanCheckCriteria(this, achievementCriteria))
            continue;

        switch (type)
        {
            // std. case: increment at 1
            case ACHIEVEMENT_CRITERIA_TYPE_NUMBER_OF_TALENT_RESETS:
            case ACHIEVEMENT_CRITERIA_TYPE_LOSE_DUEL:
            case ACHIEVEMENT_CRITERIA_TYPE_CREATE_AUCTION:
            case ACHIEVEMENT_CRITERIA_TYPE_WON_AUCTIONS:
            case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED:
            case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED:
            case ACHIEVEMENT_CRITERIA_TYPE_ROLL_DISENCHANT:
            case ACHIEVEMENT_CRITERIA_TYPE_QUEST_ABANDONED:
            case ACHIEVEMENT_CRITERIA_TYPE_FLIGHT_PATHS_TAKEN:
            case ACHIEVEMENT_CRITERIA_TYPE_ACCEPTED_SUMMONINGS:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscValue1)
                    continue;
                SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                break;
            // std case: increment at miscValue1
            case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_VENDORS:
            case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TALENTS:
            case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD:
            case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TRAVELLING:
            case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_AT_BARBER:
            case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL:
            case ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY:
            case ACHIEVEMENT_CRITERIA_TYPE_GOLD_EARNED_BY_AUCTIONS:
            case ACHIEVEMENT_CRITERIA_TYPE_TOTAL_DAMAGE_RECEIVED:
            case ACHIEVEMENT_CRITERIA_TYPE_TOTAL_HEALING_RECEIVED:
            case ACHIEVEMENT_CRITERIA_TYPE_USE_LFD_TO_GROUP_WITH_PLAYERS:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscValue1)
                    continue;
                SetCriteriaProgress(achievementCriteria, miscValue1, PROGRESS_ACCUMULATE);
                break;
            // std case: high value at miscValue1
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_BID:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_SOLD:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HIT_DEALT:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HIT_RECEIVED:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEAL_CASTED:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEALING_RECEIVED:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscValue1)
                    continue;
                SetCriteriaProgress(achievementCriteria, miscValue1, PROGRESS_HIGHEST);
                break;

            // specialized cases
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST:
                {
                    // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                    if (!miscValue1)
                        continue;

                    if (achievement->CategoryID == CATEGORY_CHILDRENS_WEEK)
                    {
                        if (const AchievementCriteriaDataSet* data = sAchievementMgr->GetCriteriaDataSet(achievementCriteria);
                            !data || !data->Meets(GetPlayer(), nullptr))
                            continue;
                    }

                    SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
                {
                    // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                    if (!miscValue1)
                        continue;
                    if (achievementCriteria->WinBG.MapID != GetPlayer()->GetMapId())
                        continue;

                    // Those requirements couldn't be found in the dbc
                    if (const AchievementCriteriaDataSet* data = sAchievementMgr->GetCriteriaDataSet(achievementCriteria);
                        !data || !data->Meets(GetPlayer(), unit))
                        continue;

                    SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
                {
                    // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                    if (!miscValue1)
                        continue;
                    if (achievementCriteria->KillCreature.CreatureID != miscValue1)
                        continue;

                    // those requirements couldn't be found in the dbc
                    const AchievementCriteriaDataSet* data = sAchievementMgr->GetCriteriaDataSet(achievementCriteria);
                    if (!data || !data->Meets(GetPlayer(), unit))
                        continue;

                    SetCriteriaProgress(achievementCriteria, miscValue2, PROGRESS_ACCUMULATE);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE:
                {
                    // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                    if (!miscValue2)
                        continue;

                    // those requirements couldn't be found in the dbc
                    if (const AchievementCriteriaDataSet* data = sAchievementMgr->GetCriteriaDataSet(achievementCriteria);
                        !data || !data->Meets(GetPlayer(), unit, miscValue1))
                        continue;

                    SetCriteriaProgress(achievementCriteria, miscValue2, PROGRESS_ACCUMULATE);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
                if (const AchievementCriteriaDataSet* data = sAchievementMgr->GetCriteriaDataSet(achievementCriteria))
                    if (!data->Meets(GetPlayer(), unit))
                        continue;
                SetCriteriaProgress(achievementCriteria, GetPlayer()->GetLevel());
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL:
                // Update at loading or specific skill update
                if (miscValue1 && miscValue1 != achievementCriteria->ReachSkillLevel.SkillID)
                    continue;
                if (uint32 skillValue = GetPlayer()->GetBaseSkillValue(achievementCriteria->ReachSkillLevel.SkillID))
                    SetCriteriaProgress(achievementCriteria, skillValue);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL:
                // Update at loading or specific skill update
                if (miscValue1 && miscValue1 != achievementCriteria->LearnSkillLevel.SkillID)
                    continue;
                if (uint32 maxSkillValue = GetPlayer()->GetPureMaxSkillValue(achievementCriteria->LearnSkillLevel.SkillID))
                    SetCriteriaProgress(achievementCriteria, maxSkillValue);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
                if ((miscValue1 && achievementCriteria->CompleteAchievement.LinkedAchievement == miscValue1) ||
                    (!miscValue1 && GetPlayer()->HasAchieved(achievementCriteria->CompleteAchievement.LinkedAchievement)))
                    SetCriteriaProgress(achievementCriteria, 1);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST_COUNT:
                {
                    SetCriteriaProgress(achievementCriteria, GetPlayer()->GetRewardedQuestCount());
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY:
                {
                    Seconds nextDailyResetTime = sWorld->GetNextDailyQuestsResetTime();
                    CriteriaProgress* progress = GetCriteriaProgress(achievementCriteria);

                    if (!miscValue1) // Login case.
                    {
                        // Reset if player missed one day.
                        if (progress && Seconds(progress->date) < nextDailyResetTime - 2_days)
                            SetCriteriaProgress(achievementCriteria, 0, PROGRESS_SET);
                        continue;
                    }

                    ProgressType progressType;
                    if (!progress)
                        // 1st time. Start count.
                        progressType = PROGRESS_SET;
                    else if (Seconds(progress->date) < nextDailyResetTime - 2_days)
                        // Last progress is older than 2 days. Player missed 1 day => Restart count.
                        progressType = PROGRESS_RESET;
                    else if (Seconds(progress->date) < (nextDailyResetTime - 1_days))
                        // Last progress is between 1 and 2 days. => 1st time of the day.
                        progressType = PROGRESS_ACCUMULATE;
                    else
                        // Last progress is within the day before the reset => Already counted today.
                        continue;

                    SetCriteriaProgress(achievementCriteria, 1, progressType);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE:
                {
                    // Speedup for non-login case
                    if (miscValue1 && miscValue1 != achievementCriteria->CompleteQuestsInZone.ZoneID)
                        continue;

                    uint32 counter = 0;

                    const RewardedQuestSet& rewQuests = GetPlayer()->getRewardedQuests();
                    for (auto itr = rewQuests.begin(); itr != rewQuests.end(); ++itr)
                    {
                        if (const Quest* quest = sObjectMgr->GetQuestTemplate(*itr); quest && quest->GetZoneOrSort() >= 0 &&
                            static_cast<uint32>(quest->GetZoneOrSort()) == achievementCriteria->CompleteQuestsInZone.ZoneID &&
                            !quest->HasSpecialFlag(QUEST_SPECIAL_FLAGS_NO_LOREMASTER_COUNT))
                            ++counter;
                    }
                    SetCriteriaProgress(achievementCriteria, counter);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscValue1)
                    continue;
                if (GetPlayer()->GetMapId() != achievementCriteria->CompleteBattleground.MapID)
                    continue;
                SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscValue1)
                    continue;
                if (GetPlayer()->GetMapId() != achievementCriteria->DeathAtMap.MapID)
                    continue;
                SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_DEATH:
                {
                    // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                    if (!miscValue1)
                        continue;
                    // Skip wrong arena achievements, if not achievementIdByArenaSlot then normal total death counter
                    bool notFit = false;
                    for (int j = 0; j < MAX_ARENA_SLOT; ++j)
                    {
                        if (achievementIdByArenaSlot[j] == achievement->ID)
                        {
                            if (Battleground* bg = GetPlayer()->GetBattleground(); !bg || !bg->isArena() || ArenaTeam::GetSlotByType(bg->GetArenaType()) != j)
                                notFit = true;
                            break;
                        }
                    }
                    if (notFit)
                        continue;

                    SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_DEATH_IN_DUNGEON:
                {
                    // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                    if (!miscValue1)
                        continue;

                    const Map* map = GetPlayer()->IsInWorld() ? GetPlayer()->GetMap() : sMapMgr->FindMap(GetPlayer()->GetMapId(), GetPlayer()->GetInstanceId());
                    if (!map || !map->IsDungeon())
                        continue;

                    // Search case
                    bool found = false;
                    for (int j = 0; achievementIdForDungeon[j][0]; ++j)
                    {
                        if (achievementIdForDungeon[j][0] == achievement->ID)
                        {
                            if (map->IsRaid())
                            {
                                // If raid accepted (ignore difficulty)
                                if (!achievementIdForDungeon[j][2])
                                    break;
                            }
                            else if (GetPlayer()->GetDungeonDifficulty() == DUNGEON_DIFFICULTY_NORMAL)
                            {
                                // Dungeon in normal mode accepted
                                if (!achievementIdForDungeon[j][1])
                                    break;
                            }
                            else
                            {
                                // Dungeon in heroic mode accepted
                                if (!achievementIdForDungeon[j][3])
                                    break;
                            }

                            found = true;
                            break;
                        }
                    }
                    if (!found)
                        continue;

                    // FIXME: work only for instances where max == min for players
                    if (map->ToInstanceMap()->GetMaxPlayers() != achievementCriteria->DeathInDungeon.ManLimit)
                        continue;
                    SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_RAID:
                {
                    // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                    if (!miscValue1)
                        continue;

                    const Map* map = GetPlayer()->IsInWorld() ? GetPlayer()->GetMap() : sMapMgr->FindMap(GetPlayer()->GetMapId(), GetPlayer()->GetInstanceId());
                    if (!map || !map->IsDungeon())
                        continue;

                    if (map->ToInstanceMap()->GetMaxPlayers() != achievementCriteria->CompleteRaid.GroupSize)
                        continue;
                    SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscValue1)
                    continue;
                if (miscValue1 != achievementCriteria->KilledByCreature.CreatureEntry)
                    continue;
                SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_PLAYER:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscValue1)
                    continue;

                // If team check required: must kill by opposition faction
                if (achievement->ID == 318 && miscValue2 == GetPlayer()->GetTeamId())
                    continue;

                SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
                {
                    // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                    if (!miscValue1)
                        continue;

                    // those requirements couldn't be found in the dbc
                    if (const AchievementCriteriaDataSet* data = sAchievementMgr->GetCriteriaDataSet(achievementCriteria);
                        !data || !data->Meets(GetPlayer(), unit))
                        continue;

                    // miscValue1 is the ingame (fallHeight * 100) as stored in dbc
                    SetCriteriaProgress(achievementCriteria, miscValue1);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_DEATHS_FROM:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscValue1)
                    continue;
                if (miscValue2 != achievementCriteria->DeathFrom.Type)
                    continue;
                SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:
                {
                    // If miscValues != 0, it contains the questID.
                    if (miscValue1)
                    {
                        if (miscValue1 != achievementCriteria->CompleteQuest.QuestID)
                            continue;
                    }
                    else
                    {
                        // Login case.
                        if (!GetPlayer()->GetQuestRewardStatus(achievementCriteria->CompleteQuest.QuestID))
                            continue;
                    }

                    if (const AchievementCriteriaDataSet* data = sAchievementMgr->GetCriteriaDataSet(achievementCriteria))
                        if (!data->Meets(GetPlayer(), unit))
                            continue;

                    SetCriteriaProgress(achievementCriteria, 1);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
            case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
                {
                    if (!miscValue1 || miscValue1 != achievementCriteria->BeSpellTarget.SpellID)
                        continue;

                    // Those requirements couldn't be found in the dbc
                    const AchievementCriteriaDataSet* data = sAchievementMgr->GetCriteriaDataSet(achievementCriteria);
                    if (!data)
                        continue;

                    if (!data->Meets(GetPlayer(), unit))
                        continue;

                    SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:
            case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:
                {
                    if (!miscValue1 || miscValue1 != achievementCriteria->CastSpell.SpellID)
                        continue;

                    // Those requirements couldn't be found in the dbc
                    const AchievementCriteriaDataSet* data = sAchievementMgr->GetCriteriaDataSet(achievementCriteria);
                    if (!data)
                        continue;

                    if (!data->Meets(GetPlayer(), unit))
                        continue;

                    SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL:
                if (miscValue1 && miscValue1 != achievementCriteria->LearnSpell.SpellID)
                    continue;

                if (GetPlayer()->HasSpell(achievementCriteria->LearnSpell.SpellID))
                    SetCriteriaProgress(achievementCriteria, 1);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:
                {
                    // miscValue1 = loot_type (note: 0 = LOOT_CORPSE, and then it ignored)
                    // miscValue2 = count of item loot
                    if (!miscValue1 || !miscValue2)
                        continue;
                    if (miscValue1 != achievementCriteria->LootType.LootType)
                        continue;

                    // Zone specific
                    if (achievementCriteria->LootType.LootTypeCount == 1)
                    {
                        // Those requirements couldn't be found in the dbc
                        if (const AchievementCriteriaDataSet* data = sAchievementMgr->GetCriteriaDataSet(achievementCriteria);
                            !data || !data->Meets(GetPlayer(), unit))
                            continue;
                    }

                    SetCriteriaProgress(achievementCriteria, miscValue2, PROGRESS_ACCUMULATE);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM:
                // Speedup for non-login case
                if (miscValue1 && achievementCriteria->OwnItem.ItemID != miscValue1)
                    continue;
                SetCriteriaProgress(achievementCriteria, miscValue2, PROGRESS_ACCUMULATE);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA:
                if (!miscValue1)  // No update at login
                    continue;

                // Additional requirements
                if (achievementCriteria->AdditionalRequirements[0].AdditionalRequirementType == ACHIEVEMENT_CRITERIA_CONDITION_NO_LOSE)
                {
                    // Those requirements couldn't be found in the dbc
                    if (const AchievementCriteriaDataSet* data = sAchievementMgr->GetCriteriaDataSet(achievementCriteria);
                        !data || !data->Meets(GetPlayer(), unit, miscValue1))
                    {
                        // reset the progress as we have a win without the requirement.
                        SetCriteriaProgress(achievementCriteria, 0);
                        continue;
                    }
                }

                SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscValue1)
                    continue;

                if (achievementCriteria->UseItem.ItemID != miscValue1)
                    continue;

                // Children's Week achievements have extra requirements
                // if (achievement->categoryId == CATEGORY_CHILDRENS_WEEK || achievement->ID == 1291) // Lonely?
                {
                    // Skip progress only if data exists and is not meet
                    if (const AchievementCriteriaDataSet* data = sAchievementMgr->GetCriteriaDataSet(achievementCriteria);
                        data && !data->Meets(GetPlayer(), nullptr))
                        continue;
                }

                SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM:
                // You _have_ to loot that item, just owning it when logging in does _not_ count!
                if (!miscValue1)
                    continue;
                if (miscValue1 != achievementCriteria->OwnItem.ItemID)
                    continue;
                SetCriteriaProgress(achievementCriteria, miscValue2, PROGRESS_ACCUMULATE);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA:
                {
                    const WorldMapOverlayEntry* worldOverlayEntry = sWorldMapOverlayStore.LookupEntry(achievementCriteria->ExploreArea.AreaReference);
                    if (!worldOverlayEntry)
                        break;

                    bool matchFound = false;
                    for (int j = 0; j < MAX_WORLD_MAP_OVERLAY_AREA_IDX; ++j)
                    {
                        AreaTableEntry const* area = sAreaTableStore.LookupEntry(worldOverlayEntry->AreaID[j]);
                        if (!area)
                            break;

                        uint32 playerIndexOffset = static_cast<uint32>(area->ExploreFlag) / 32;
                        if (playerIndexOffset >= PLAYER_EXPLORED_ZONES_SIZE)
                            continue;

                        if (const uint32 mask = 1 << (static_cast<uint32>(area->ExploreFlag) % 32);
                            GetPlayer()->GetUInt32Value(PLAYER_EXPLORED_ZONES_1 + playerIndexOffset) & mask)
                        {
                            matchFound = true;
                            break;
                        }
                    }

                    if (matchFound)
                        SetCriteriaProgress(achievementCriteria, 1);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT:
                SetCriteriaProgress(achievementCriteria, GetPlayer()->GetBankBagSlotCount());
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REPUTATION:
                {
                    // Skip faction check only at loading
                    if (miscValue1 && miscValue1 != achievementCriteria->GainReputation.FactionID)
                        continue;

                    if (int32 reputation = GetPlayer()->GetReputationMgr().GetReputation(achievementCriteria->GainReputation.FactionID); reputation > 0)
                        SetCriteriaProgress(achievementCriteria, reputation);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_GAIN_EXALTED_REPUTATION:
                {
                    SetCriteriaProgress(achievementCriteria, GetPlayer()->GetReputationMgr().GetExaltedFactionCount());
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP:
                {
                    // skip for login case
                    if (!miscValue1)
                        continue;
                    SetCriteriaProgress(achievementCriteria, 1);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM:
                {
                    // miscValue1 = itemid
                    // miscValue2 = itemSlot
                    if (!miscValue1)
                        continue;

                    if (miscValue2 != achievementCriteria->EquipEpicItem.ItemSlot)
                        continue;

                    // Check item level and quality via world_achievement_criteria_data
                    if (const AchievementCriteriaDataSet* data = sAchievementMgr->GetCriteriaDataSet(achievementCriteria);
                        !data || !data->Meets(GetPlayer(), nullptr, miscValue1))
                        continue;

                    SetCriteriaProgress(achievementCriteria, 1);
                    break;
                }

            case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT:
            case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT:
                {
                    // miscValue1 = ItemID
                    // miscValue2 = diced value
                    if (!miscValue1)
                        continue;
                    if (miscValue2 != achievementCriteria->RollGreedOnLoot.RollValue)
                        continue;

                    ItemTemplate const* pProto = sObjectMgr->GetItemTemplate(miscValue1);
                    if (!pProto)
                        continue;

                    // Check item level via world_achievement_criteria_data
                    if (const AchievementCriteriaDataSet* data = sAchievementMgr->GetCriteriaDataSet(achievementCriteria);
                        !data || !data->Meets(GetPlayer(), nullptr, pProto->ItemLevel))
                        continue;

                    SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:
                {
                    // miscValue1 = emote
                    if (!miscValue1)
                        continue;
                    if (miscValue1 != achievementCriteria->DoEmote.EmoteID)
                        continue;
                    if (achievementCriteria->DoEmote.Count)
                    {
                        // Those requirements couldn't be found in the dbc
                        if (const AchievementCriteriaDataSet* data = sAchievementMgr->GetCriteriaDataSet(achievementCriteria);
                            !data || !data->Meets(GetPlayer(), unit))
                            continue;
                    }

                    SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE:
            case ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE:
                {
                    if (!miscValue1)
                        continue;

                    if (achievementCriteria->AdditionalRequirements[0].AdditionalRequirementType == ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP)
                    {
                        if (GetPlayer()->GetMapId() != achievementCriteria->AdditionalRequirements[0].AdditionalRequirementValue)
                            continue;

                        // Map-specific case (BG in fact) expected player targeted damage/heal
                        if (!unit || !unit->IsPlayer())
                            continue;
                    }

                    SetCriteriaProgress(achievementCriteria, miscValue1, PROGRESS_ACCUMULATE);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM:
                // miscValue1 = item_id
                if (!miscValue1)
                    continue;
                if (miscValue1 != achievementCriteria->EquipItem.ItemID)
                    continue;

                SetCriteriaProgress(achievementCriteria, 1);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT:
                // miscValue1 = go entry
                if (!miscValue1)
                    continue;
                if (miscValue1 != achievementCriteria->UseGameObject.GameObject)
                    continue;

                SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_FISH_IN_GAMEOBJECT:
                if (!miscValue1)
                    continue;
                if (miscValue1 != achievementCriteria->FishInGameObject.GameObject)
                    continue;

                SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS:
                {
                    if (miscValue1 && miscValue1 != achievementCriteria->LearnSkillLineSpell.SkillLine)
                        continue;

                    uint32 spellCount = 0;
                    for (PlayerSpellMap::const_iterator spellIter = GetPlayer()->GetSpellMap().begin(); spellIter != GetPlayer()->GetSpellMap().end(); ++spellIter)
                    {
                        SkillLineAbilityMapBounds bounds = sSpellMgr->GetSkillLineAbilityMapBounds(spellIter->first);
                        for (auto skillIter = bounds.first; skillIter != bounds.second; ++skillIter)
                        {
                            if (skillIter->second->SkillLine == achievementCriteria->LearnSkillLineSpell.SkillLine)
                            {
                                // Do not add counter twice if by any chance skill is listed twice in dbc (e.g. skill 777 and spell 22717)
                                ++spellCount;
                                break;
                            }
                        }
                    }

                    SetCriteriaProgress(achievementCriteria, spellCount);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL:
                // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                if (!miscValue1)
                    continue;

                if (achievementCriteria->WinDuel.DuelCount)
                {
                    // those requirements couldn't be found in the dbc
                    const AchievementCriteriaDataSet* data = sAchievementMgr->GetCriteriaDataSet(achievementCriteria);
                    if (!data)
                        continue;

                    if (!data->Meets(GetPlayer(), unit))
                        continue;
                }

                SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REVERED_REPUTATION:
                SetCriteriaProgress(achievementCriteria, GetPlayer()->GetReputationMgr().GetReveredFactionCount());
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_GAIN_HONORED_REPUTATION:
                SetCriteriaProgress(achievementCriteria, GetPlayer()->GetReputationMgr().GetHonoredFactionCount());
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_KNOWN_FACTIONS:
                SetCriteriaProgress(achievementCriteria, GetPlayer()->GetReputationMgr().GetVisibleFactionCount());
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_LOOT_EPIC_ITEM:
            case ACHIEVEMENT_CRITERIA_TYPE_RECEIVE_EPIC_ITEM:
                {
                    // AchievementMgr::UpdateAchievementCriteria might also be called on login - skip in this case
                    if (!miscValue1)
                        continue;
                    if (const ItemTemplate* proto = sObjectMgr->GetItemTemplate(miscValue1); !proto || proto->Quality < ITEM_QUALITY_EPIC)
                        continue;
                    if (const AchievementCriteriaDataSet* data = sAchievementMgr->GetCriteriaDataSet(achievementCriteria); !data || !data->Meets(GetPlayer(), unit, miscValue1))
                        continue;
                    SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE:
                {
                    if (miscValue1 && miscValue1 != achievementCriteria->LearnSkillLine.SkillLine)
                        continue;

                    uint32 spellCount = 0;
                    for (PlayerSpellMap::const_iterator spellIter = GetPlayer()->GetSpellMap().begin(); spellIter != GetPlayer()->GetSpellMap().end(); ++spellIter)
                    {
                        SkillLineAbilityMapBounds bounds = sSpellMgr->GetSkillLineAbilityMapBounds(spellIter->first);
                        for (auto skillIter = bounds.first; skillIter != bounds.second; ++skillIter)
                        {
                            if (skillIter->second->SkillLine == achievementCriteria->LearnSkillLine.SkillLine)
                            {
                                // Do not add counter twice if by any chance skill is listed twice in dbc (e.g. skill 777 and spell 22717)
                                ++spellCount;
                                break;
                            }
                        }
                    }

                    SetCriteriaProgress(achievementCriteria, spellCount);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_EARN_HONORABLE_KILL:
                SetCriteriaProgress(achievementCriteria, GetPlayer()->GetUInt32Value(PLAYER_FIELD_LIFETIME_HONORABLE_KILLS));
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS:
                if (!miscValue1 || miscValue1 != achievementCriteria->HKClass.ClassID)
                    continue;

                SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_HK_RACE:
                if (!miscValue1 || miscValue1 != achievementCriteria->HKRace.RaceID)
                    continue;

                SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_GOLD_VALUE_OWNED:
                SetCriteriaProgress(achievementCriteria, GetPlayer()->GetMoney(), PROGRESS_HIGHEST);
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS:
                {
                    if (!miscValue1)
                    {
                        uint32 points = 0;
                        for (auto itr = _completedAchievements.begin(); itr != _completedAchievements.end(); ++itr)
                            if (AchievementEntry const* pAchievement = sAchievementStore.LookupEntry(itr->first))
                                points += pAchievement->Points;
                        SetCriteriaProgress(achievementCriteria, points, PROGRESS_SET);
                    }
                    else
                        SetCriteriaProgress(achievementCriteria, miscValue1, PROGRESS_ACCUMULATE);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
                {
                    if (!miscValue1 || miscValue1 != achievementCriteria->BGObjective.ObjectiveID)
                        continue;

                    // Those requirements couldn't be found in the dbc
                    if (const AchievementCriteriaDataSet* data = sAchievementMgr->GetCriteriaDataSet(achievementCriteria); !data || !data->Meets(GetPlayer(), unit))
                        continue;

                    SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL:
            case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:
            case ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS:
                {
                    // skip login update
                    if (!miscValue1)
                        continue;

                    // Those requirements couldn't be found in the dbc
                    if (const AchievementCriteriaDataSet* data = sAchievementMgr->GetCriteriaDataSet(achievementCriteria); !data || !data->Meets(GetPlayer(), unit))
                        continue;

                    SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL_AT_AREA:
                {
                    if (!miscValue1 || miscValue1 != achievementCriteria->HonorableKillAtArea.AreaID)
                        continue;

                    SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_TEAM_RATING:
                {
                    uint32 reqTeamType = achievementCriteria->HighestTeamRating.TeamType;

                    if (miscValue1)
                    {
                        if (miscValue2 != reqTeamType)
                            continue;

                        SetCriteriaProgress(achievementCriteria, miscValue1, PROGRESS_HIGHEST);
                    }
                    else  // Login case
                    {
                        for (uint32 arena_slot = 0; arena_slot < MAX_ARENA_SLOT; ++arena_slot)
                        {
                            uint32 arenaTeamId = GetPlayer()->GetArenaTeamId(arena_slot);
                            if (!arenaTeamId)
                                continue;

                            ArenaTeam* team = sArenaTeamMgr->GetArenaTeamById(arenaTeamId);
                            if (!team || team->GetType() != reqTeamType)
                                continue;

                            SetCriteriaProgress(achievementCriteria, team->GetStats().Rating, PROGRESS_HIGHEST);
                            break;
                        }
                    }

                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_PERSONAL_RATING:
                {
                    uint32 reqTeamType = achievementCriteria->HighestPersonalRating.TeamType;

                    if (miscValue1)
                    {
                        if (miscValue2 != reqTeamType)
                            continue;

                        SetCriteriaProgress(achievementCriteria, miscValue1, PROGRESS_HIGHEST);
                    }
                    else  // Login case
                    {
                        for (uint32 arena_slot = 0; arena_slot < MAX_ARENA_SLOT; ++arena_slot)
                        {
                            uint32 arenaTeamId = GetPlayer()->GetArenaTeamId(arena_slot);
                            if (!arenaTeamId)
                                continue;

                            ArenaTeam* team = sArenaTeamMgr->GetArenaTeamById(arenaTeamId);
                            if (!team || team->GetType() != reqTeamType)
                                continue;

                            if (ArenaTeamMember const* member = team->GetMember(GetPlayer()->GetGUID()))
                            {
                                SetCriteriaProgress(achievementCriteria, member->PersonalRating, PROGRESS_HIGHEST);
                                break;
                            }
                        }
                    }

                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_ON_LOGIN:
                {
                    // These criteria are only called directly after login - with expected miscValue1 == 1
                    if (!miscValue1)
                        continue;

                    // They have no proper requirements in dbc
                    if (const AchievementCriteriaDataSet* data = sAchievementMgr->GetCriteriaDataSet(achievementCriteria); !data || !data->Meets(GetPlayer(), unit))
                        continue;

                    SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_PLAY_ARENA:
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_ARENA: // This also behaves like ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA
                {
                    // Those requirements couldn't be found in the dbc
                    if (const AchievementCriteriaDataSet* data = sAchievementMgr->GetCriteriaDataSet(achievementCriteria); !data || !data->Meets(GetPlayer(), nullptr))
                        continue;

                    // Check map id requirement
                    if (miscValue1 == achievementCriteria->WinArena.MapID)
                        SetCriteriaProgress(achievementCriteria, 1, PROGRESS_ACCUMULATE);
                    break;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_OWN_RANK:
                {
                    // Those requirements couldn't be found in the dbc
                    if (const AchievementCriteriaDataSet* data = sAchievementMgr->GetCriteriaDataSet(achievementCriteria); !data || !data->Meets(GetPlayer(), nullptr))
                        continue;

                    SetCriteriaProgress(achievementCriteria, 1);
                    break;
                }
            // Not exist in DBC, not triggered in code as result
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEALTH:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_SPELL_POWER:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_ARMOR:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_POWER:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_STAT:
            case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_RATING:
                break;
            // FIXME: not triggered in code as result, need to implement
            case ACHIEVEMENT_CRITERIA_TYPE_TOTAL:
                break;  // Not implemented yet :(
        }

        if (IsCompletedCriteria(achievementCriteria, achievement))
            CompletedCriteriaFor(achievement);

        // Check again the completeness for SUMM and REQ COUNT achievements,
        // as they don't depend on the completed criteria but on the sum of the progress of each individual criteria
        if (achievement->Flags & ACHIEVEMENT_FLAG_SUMM)
            if (IsCompletedAchievement(achievement))
                CompletedAchievement(achievement);

        if (AchievementEntryList const* achRefList = sAchievementMgr->GetAchievementByReferencedId(achievement->ID))
            for (auto itr = achRefList->begin(); itr != achRefList->end(); ++itr)
                if (IsCompletedAchievement(*itr))
                    CompletedAchievement(*itr);
    }
}

bool AchievementMgr::IsCompletedCriteria(const AchievementCriteriaEntry* achievementCriteria, const AchievementEntry* achievement)
{
    // counter can never complete
    if (achievement->Flags & ACHIEVEMENT_FLAG_COUNTER)
        return false;

    if (achievement->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
    {
        // someone on this realm has already completed that achievement
        if (sAchievementMgr->IsRealmCompleted(achievement))
            return false;

        // A character may only have 1 race-specific 'Realm First!' achievement
        // prevent clever use of the race/faction change service to obtain multiple 'Realm First!' achievements
        constexpr std::array<uint32, 9> raceSpecificRealmFirstAchievements { 1405, 1406, 1407, 1408, 1409, 1410, 1411, 1412, 1413 };
        if (std::ranges::find(raceSpecificRealmFirstAchievements, achievement->ID) != std::ranges::end(raceSpecificRealmFirstAchievements))
            for (const uint32 raceAchievementId : raceSpecificRealmFirstAchievements)
                if (raceAchievementId != achievement->ID && HasAchieved(raceAchievementId))
                    return false;
    }

    // Progress will be deleted after getting the achievement (optimization).
    // Finished achievement should indicate criteria completed, since not finding progress would start some timed achievements and probably other things
    if (HasAchieved(achievement->ID))
    {
        bool completed = true;

        // Completed only after all referenced achievements are also completed
        if (const AchievementEntryList* achRefList = sAchievementMgr->GetAchievementByReferencedId(achievement->ID))
            for (auto itr = achRefList->begin(); itr != achRefList->end(); ++itr)
                if (!IsCompletedAchievement(*itr))
                {
                    completed = false;
                    break;
                }

        if (completed)
            return true;
    }

    const CriteriaProgress* progress = GetCriteriaProgress(achievementCriteria);
    if (!progress)
        return false;

    if (!sScriptMgr->IsCompletedCriteria(this, achievementCriteria, achievement, progress))
        return false;

    switch (achievementCriteria->RequiredType)
    {
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
            return progress->counter >= achievementCriteria->WinBG.WinCount;
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
            return progress->counter >= achievementCriteria->KillCreature.CreatureCount;
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL:
            return progress->counter >= achievementCriteria->ReachLevel.Level;
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL:
            return progress->counter >= achievementCriteria->ReachSkillLevel.SkillLevel;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
            return progress->counter >= 1;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST_COUNT:
            return progress->counter >= achievementCriteria->CompleteQuestCount.TotalQuestCount;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY:
            return progress->counter >= achievementCriteria->CompleteDailyQuestDaily.NumberOfDays;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE:
            return progress->counter >= achievementCriteria->CompleteQuestsInZone.QuestCount;
        case ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE:
        case ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE:
            return progress->counter >= achievementCriteria->HealingDone.Count;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST:
            return progress->counter >= achievementCriteria->CompleteDailyQuest.QuestCount;
        case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
            return progress->counter >= achievementCriteria->FallWithoutDying.FallHeight;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:
            return progress->counter >= 1;
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
            return progress->counter >= achievementCriteria->BeSpellTarget.SpellCount;
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:
            return progress->counter >= achievementCriteria->CastSpell.CastCount;
        case ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
            return progress->counter >= achievementCriteria->BGObjective.CompleteCount;
        case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL_AT_AREA:
            return progress->counter >= achievementCriteria->HonorableKillAtArea.KillCount;
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL:
            return progress->counter >= 1;
        case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL:
        case ACHIEVEMENT_CRITERIA_TYPE_EARN_HONORABLE_KILL:
            return progress->counter >= achievementCriteria->HonorableKill.KillCount;
        case ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM:
            return progress->counter >= achievementCriteria->OwnItem.ItemCount;
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA:
            return progress->counter >= achievementCriteria->WinRatedArena.Count;
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_PERSONAL_RATING:
            return progress->counter >= achievementCriteria->HighestPersonalRating.PersonalRating;
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL:
            return progress->counter >= (achievementCriteria->LearnSkillLevel.SkillLevel * 75);
        case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:
            return progress->counter >= achievementCriteria->UseItem.ItemCount;
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM:
            return progress->counter >= achievementCriteria->LootItem.ItemCount;
        case ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA:
            return progress->counter >= 1;
        case ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT:
            return progress->counter >= achievementCriteria->BuyBankSlot.NumberOfSlots;
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REPUTATION:
            return progress->counter >= achievementCriteria->GainReputation.ReputationAmount;
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_EXALTED_REPUTATION:
            return progress->counter >= achievementCriteria->GainExaltedReputation.NumberOfExaltedFactions;
        case ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP:
            return progress->counter >= achievementCriteria->VisitBarber.NumberOfVisits;
        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM:
            return progress->counter >= achievementCriteria->EquipEpicItem.Count;
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT:
            return progress->counter >= achievementCriteria->RollGreedOnLoot.Count;
        case ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS:
            return progress->counter >= achievementCriteria->HKClass.Count;
        case ACHIEVEMENT_CRITERIA_TYPE_HK_RACE:
            return progress->counter >= achievementCriteria->HKRace.Count;
        case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:
            return progress->counter >= achievementCriteria->DoEmote.Count;
        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM:
            return progress->counter >= achievementCriteria->EquipItem.Count;
        case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD:
            return progress->counter >= achievementCriteria->QuestRewardMoney.GoldInCopper;
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY:
            return progress->counter >= achievementCriteria->LootMoney.GoldInCopper;
        case ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT:
            return progress->counter >= achievementCriteria->UseGameObject.UseCount;
        case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:
            return progress->counter >= achievementCriteria->SpecialPvPKill.KillCount;
        case ACHIEVEMENT_CRITERIA_TYPE_FISH_IN_GAMEOBJECT:
            return progress->counter >= achievementCriteria->FishInGameObject.LootCount;
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS:
            return progress->counter >= achievementCriteria->LearnSkillLineSpell.SpellCount;
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL:
            return progress->counter >= achievementCriteria->WinDuel.DuelCount;
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:
            return progress->counter >= achievementCriteria->LootType.LootTypeCount;
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE:
            return progress->counter >= achievementCriteria->LearnSkillLine.SpellCount;
        case ACHIEVEMENT_CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS:
            return progress->counter >= 9000;
        case ACHIEVEMENT_CRITERIA_TYPE_USE_LFD_TO_GROUP_WITH_PLAYERS:
            return progress->counter >= achievementCriteria->UseLFG.DungeonsComplete;
        case ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS:
            return progress->counter >= achievementCriteria->GetKillingBlow.KillCount;
        case ACHIEVEMENT_CRITERIA_TYPE_ON_LOGIN:
            return true;
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_ARENA:
            return achievementCriteria->WinArena.Count && progress->counter >= achievementCriteria->WinArena.Count;
        case ACHIEVEMENT_CRITERIA_TYPE_OWN_RANK:
            return true;
        // Handle all statistic-only criteria here
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND:
        case ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP:
        case ACHIEVEMENT_CRITERIA_TYPE_DEATH:
        case ACHIEVEMENT_CRITERIA_TYPE_DEATH_IN_DUNGEON:
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_RAID:
        case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE:
        case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_PLAYER:
        case ACHIEVEMENT_CRITERIA_TYPE_DEATHS_FROM:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_TEAM_RATING:
        case ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_VENDORS:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_TALENTS:
        case ACHIEVEMENT_CRITERIA_TYPE_NUMBER_OF_TALENT_RESETS:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_AT_BARBER:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_SPENT_FOR_MAIL:
        case ACHIEVEMENT_CRITERIA_TYPE_LOSE_DUEL:
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE:
        case ACHIEVEMENT_CRITERIA_TYPE_GOLD_EARNED_BY_AUCTIONS:
        case ACHIEVEMENT_CRITERIA_TYPE_CREATE_AUCTION:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_BID:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_AUCTION_SOLD:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_GOLD_VALUE_OWNED:
        case ACHIEVEMENT_CRITERIA_TYPE_WON_AUCTIONS:
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REVERED_REPUTATION:
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_HONORED_REPUTATION:
        case ACHIEVEMENT_CRITERIA_TYPE_KNOWN_FACTIONS:
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_EPIC_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_RECEIVE_EPIC_ITEM:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED:
        case ACHIEVEMENT_CRITERIA_TYPE_ROLL_DISENCHANT:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_HEALTH:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_SPELL_POWER:
        case ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_ARMOR:
        case ACHIEVEMENT_CRITERIA_TYPE_QUEST_ABANDONED:
        case ACHIEVEMENT_CRITERIA_TYPE_FLIGHT_PATHS_TAKEN:
        case ACHIEVEMENT_CRITERIA_TYPE_ACCEPTED_SUMMONINGS:
        case ACHIEVEMENT_CRITERIA_TYPE_PLAY_ARENA:
        default:
            break;
    }
    return false;
}

void AchievementMgr::CompletedCriteriaFor(const AchievementEntry* achievement)
{
    // Counter can never complete
    if (achievement->Flags & ACHIEVEMENT_FLAG_COUNTER)
        return;

    // Already completed and stored
    if (HasAchieved(achievement->ID))
        return;

    if (IsCompletedAchievement(achievement))
        CompletedAchievement(achievement);
}

bool AchievementMgr::IsCompletedAchievement(const AchievementEntry* entry)
{
    // Counter can never complete
    if (entry->Flags & ACHIEVEMENT_FLAG_COUNTER)
        return false;

    // For achievement with referenced achievement criteria get from referenced and counter from self
    const uint32 achievementForTestId = entry->RefAchievement ? entry->RefAchievement : entry->ID;
    const uint32 achievementForTestCount = entry->Count;

    const AchievementCriteriaEntryList* cList = sAchievementMgr->GetAchievementCriteriaByAchievement(achievementForTestId);
    if (!cList)
        return false;
    uint32 count = 0;

    // For SUMM achievements, we have to count the progress of each criterion of the achievement.
    // Oddly, the target count is NOT contained in the achievement, but in each individual criteria
    if (entry->Flags & ACHIEVEMENT_FLAG_SUMM)
    {
        for (auto itr = cList->begin(); itr != cList->end(); ++itr)
        {
            const AchievementCriteriaEntry* criteria = *itr;

            const CriteriaProgress* progress = GetCriteriaProgress(criteria);
            if (!progress)
                continue;

            count += progress->counter;

            // For counters, field4 contains the main count requirement
            if (count >= criteria->Raw.Count)
                return true;
        }
        return false;
    }

    // Default case - need complete all or
    bool completed_all = true;
    for (auto itr = cList->begin(); itr != cList->end(); ++itr)
    {
        // Found an uncompleted criteria, but DON'T return false yet - there might be a completed criteria with ACHIEVEMENT_CRITERIA_COMPLETE_FLAG_ALL
        if (const AchievementCriteriaEntry* criteria = *itr; IsCompletedCriteria(criteria, entry))
            ++count;
        else
            completed_all = false;

        // Completed as have req. count of completed criteria
        if (achievementForTestCount > 0 && achievementForTestCount <= count)
            return true;
    }

    // All criteria completed requirement
    if (completed_all && achievementForTestCount == 0)
        return true;

    return false;
}

CriteriaProgress* AchievementMgr::GetCriteriaProgress(AchievementCriteriaEntry const* entry)
{
    const auto iter = _criteriaProgress.find(entry->ID);
    if (iter == _criteriaProgress.end())
        return nullptr;
    return &iter->second;
}

void AchievementMgr::SetCriteriaProgress(AchievementCriteriaEntry const* entry, uint32 changeValue, ProgressType ptype)
{
    // Don't allow to cheat - doing timed achievements without timer active
    const auto timedIter = _timedAchievements.find(entry->ID);
    if (entry->TimeLimit && timedIter == _timedAchievements.end())
        return;

    if (!sScriptMgr->OnPlayerBeforeCriteriaProgress(GetPlayer(), entry))
        return;

    LOG_DEBUG("achievement", "AchievementMgr::SetCriteriaProgress({}, {}) for {}", entry->ID, changeValue, _player->GetGUID().ToString());

    CriteriaProgress* progress = GetCriteriaProgress(entry);
    if (!progress)
    {
        // Do not create record for 0 counter but allow it for timed achievements.
        // We will need to send 0 progress to client to start the timer.
        if (changeValue == 0 && !entry->TimeLimit)
            return;
        progress = &_criteriaProgress[entry->ID];
        progress->counter = changeValue;
    }
    else
    {
        uint32 newValue = 0;
        switch (ptype)
        {
            case PROGRESS_SET:
            case PROGRESS_RESET:
                newValue = changeValue;
                break;
            case PROGRESS_ACCUMULATE:
                {
                    // Avoid overflow
                    constexpr uint32 max_value = std::numeric_limits<uint32>::max();
                    newValue = max_value - progress->counter > changeValue ? progress->counter + changeValue : max_value;
                    break;
                }
            case PROGRESS_HIGHEST:
                newValue = progress->counter < changeValue ? changeValue : progress->counter;
                break;
        }

        // Do not update (not mark as changed) if counter will have same value
        if (ptype != PROGRESS_RESET && progress->counter == newValue && !entry->TimeLimit)
            return;

        progress->counter = newValue;
    }

    progress->changed = true;
    progress->date = GameTime::GetGameTime().count(); // set the date to the latest update.

    uint32 timeElapsed = 0;

    if (entry->TimeLimit)
    {
        // Has to exist else we wouldn't be here
        const bool timedCompleted = IsCompletedCriteria(entry, sAchievementStore.LookupEntry(entry->ReferredAchievement));
        // Client expects this in packet
        timeElapsed = entry->TimeLimit - (timedIter->second / IN_MILLISECONDS);

        // Remove the timer, we won't need it anymore
        if (timedCompleted)
            _timedAchievements.erase(timedIter);
    }

    SendCriteriaUpdate(entry, progress, timeElapsed, true);

    sScriptMgr->OnPlayerCriteriaProgress(GetPlayer(), entry);
}

void AchievementMgr::RemoveCriteriaProgress(const AchievementCriteriaEntry* entry)
{
    const auto criteriaProgress = _criteriaProgress.find(entry->ID);
    if (criteriaProgress == _criteriaProgress.end())
        return;

    WorldPacket data(SMSG_CRITERIA_DELETED, 4);
    data << entry->ID;
    _player->SendDirectMessage(&data);

    _criteriaProgress.erase(criteriaProgress);
}

void AchievementMgr::Update(const uint32 timeDiff)
{
    if (_offlineUpdatesDelayTimer > 0)
    {
        if (timeDiff >= _offlineUpdatesDelayTimer)
        {
            _offlineUpdatesDelayTimer = 0;
            ProcessOfflineUpdatesQueue();
        }
        else
            _offlineUpdatesDelayTimer -= timeDiff;
    }

    UpdateTimedAchievements(timeDiff);
}

void AchievementMgr::UpdateTimedAchievements(const uint32 timeDiff)
{
    if (!_timedAchievements.empty())
    {
        for (auto itr = _timedAchievements.begin(); itr != _timedAchievements.end();)
        {
            // Time is up, remove timer and reset progress
            if (itr->second <= timeDiff)
            {
                AchievementCriteriaEntry const* entry = sAchievementCriteriaStore.LookupEntry(itr->first);
                RemoveCriteriaProgress(entry);
                _timedAchievements.erase(itr++);
            }
            else
            {
                itr->second -= timeDiff;
                ++itr;
            }
        }
    }
}

void AchievementMgr::StartTimedAchievement(const AchievementCriteriaTimedTypes type, const uint32 entry, const uint32 timeLost /*= 0*/)
{
    const AchievementCriteriaEntryList& achievementCriteriaList = sAchievementMgr->GetTimedAchievementCriteriaByType(type);
    for (auto i = achievementCriteriaList.begin(); i != achievementCriteriaList.end(); ++i)
    {
        if ((*i)->TimerStartEvent != entry)
            continue;

        if (const AchievementEntry* achievement = sAchievementStore.LookupEntry((*i)->ReferredAchievement);
            !_timedAchievements.contains((*i)->ID) && !IsCompletedCriteria(*i, achievement))
        {
            // Start the timer
            if ((*i)->TimeLimit * IN_MILLISECONDS > timeLost)
            {
                _timedAchievements[(*i)->ID] = (*i)->TimeLimit * IN_MILLISECONDS - timeLost;

                // And at client too
                SetCriteriaProgress(*i, 0, PROGRESS_SET);
            }
        }
    }
}

void AchievementMgr::RemoveTimedAchievement(const AchievementCriteriaTimedTypes type, const uint32 entry)
{
    const AchievementCriteriaEntryList& achievementCriteriaList = sAchievementMgr->GetTimedAchievementCriteriaByType(type);
    for (auto i = achievementCriteriaList.begin(); i != achievementCriteriaList.end(); ++i)
    {
        if ((*i)->TimerStartEvent != entry)
            continue;

        auto timedIter = _timedAchievements.find((*i)->ID);
        // We don't have timer for this achievement
        if (timedIter == _timedAchievements.end())
            continue;

        // Remove progress
        RemoveCriteriaProgress(*i);

        // Remove the timer
        _timedAchievements.erase(timedIter);
    }
}

void AchievementMgr::CompletedAchievement(const AchievementEntry* entry)
{
    // Disable for game masters with GM-mode enabled
    if (_player->IsGameMaster())
    {
        LOG_INFO("achievement", "Not available in GM mode.");
        ChatHandler(_player->GetSession()).PSendSysMessage("Not available in GM mode");
        return;
    }

    if (!sScriptMgr->OnPlayerBeforeAchievementComplete(GetPlayer(), entry))
    {
        return;
    }

    if (entry->Flags & ACHIEVEMENT_FLAG_COUNTER || HasAchieved(entry->ID))
        return;

    LOG_DEBUG("achievement", "AchievementMgr::CompletedAchievement({})", entry->ID);

    SendAchievementEarned(entry);
    auto& [date, changed] = _completedAchievements[entry->ID];
    date = GameTime::GetGameTime().count();
    changed = true;

    sScriptMgr->OnPlayerAchievementComplete(GetPlayer(), entry);

    // Set all progress counters to 0, so progress will be deleted from db during save
    {
        bool allRefsCompleted = true;
        const uint32 achiCheckId = entry->RefAchievement ? entry->RefAchievement : entry->ID;

        if (AchievementEntryList const* achRefList = sAchievementMgr->GetAchievementByReferencedId(achiCheckId))
            for (auto itr = achRefList->begin(); itr != achRefList->end(); ++itr)
                if (!IsCompletedAchievement(*itr))
                {
                    allRefsCompleted = false;
                    break;
                }

        if (allRefsCompleted)
            if (AchievementCriteriaEntryList const* cList = sAchievementMgr->GetAchievementCriteriaByAchievement(achiCheckId))
                for (auto itr = cList->begin(); itr != cList->end(); ++itr)
                    if (CriteriaProgress* progress = GetCriteriaProgress(*itr))
                    {
                        progress->changed = true;
                        progress->counter = 0;
                    }
    }

    if (entry->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL) && !_player->GetSession()->IsGameMaster())
        sAchievementMgr->SetRealmCompleted(entry);

    UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT, entry->ID);
    UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_EARN_ACHIEVEMENT_POINTS, entry->Points);

    // Reward items and titles if any
    AchievementReward const* reward = sAchievementMgr->GetAchievementReward(entry);

    // No rewards
    if (!reward)
        return;

    // Titles
    //! Currently there's only one achievement that deals with gender-specific titles.
    //! Since no common attributes were found, (not even in titleRewardFlags field)
    //! we explicitly check by ID. Maybe in the future we could move the world_achievement_reward
    //! condition fields to the condition system.
    if (const uint32 titleId = reward->titleId[entry->ID == 1793 ? GetPlayer()->getGender() : static_cast<uint8>(GetPlayer()->GetTeamId())])
        if (const CharTitlesEntry* titleEntry = sCharTitlesStore.LookupEntry(titleId))
            GetPlayer()->SetTitle(titleEntry);

    // Mail
    if (reward->sender)
    {
        MailDraft draft(reward->mailTemplate);

        if (!reward->mailTemplate)
            draft = MailDraft(reward->subject, reward->text);

        const CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

        if (Item* item = reward->itemId ? Item::CreateItem(reward->itemId, 1, GetPlayer()) : nullptr)
        {
            // Save new item before send
            item->SaveToDB(trans);  // Save for prevent lost at next mail load, if send fail then item will be deleted

            // Item
            draft.AddItem(item);
        }

        draft.SendMailTo(trans, GetPlayer(), MailSender(MAIL_CREATURE, reward->sender));
        CharacterDatabase.CommitTransaction(trans);
    }
}

void AchievementMgr::SendAllAchievementData() const
{
    WorldPacket data(SMSG_ALL_ACHIEVEMENT_DATA, _completedAchievements.size() * 8 + 4 + _criteriaProgress.size() * 38 + 4);
    BuildAllDataPacket(&data);
    GetPlayer()->SendDirectMessage(&data);
}

void AchievementMgr::SendRespondInspectAchievements(const Player* player) const
{
    WorldPacket data(SMSG_RESPOND_INSPECT_ACHIEVEMENTS, 9 + _completedAchievements.size() * 8 + 4 + _criteriaProgress.size() * 38 + 4);
    data << GetPlayer()->GetPackGUID();
    BuildAllDataPacket(&data);
    player->SendDirectMessage(&data);
}

// Used by SMSG_RESPOND_INSPECT_ACHIEVEMENT and SMSG_ALL_ACHIEVEMENT_DATA
void AchievementMgr::BuildAllDataPacket(WorldPacket* data) const
{
    for (auto iter = _completedAchievements.begin(); iter != _completedAchievements.end(); ++iter)
    {
        // Skip hidden achievements
        if (const AchievementEntry* achievement = sAchievementStore.LookupEntry(iter->first); !achievement || achievement->Flags & ACHIEVEMENT_FLAG_HIDDEN)
            continue;

        *data << iter->first;
        data->AppendPackedTime(iter->second.date);
    }

    *data << -1;
    const time_t now = GameTime::GetGameTime().count();

    for (auto iter = _criteriaProgress.begin(); iter != _criteriaProgress.end(); ++iter)
    {
        *data << iter->first;
        data->appendPackGUID(iter->second.counter);
        *data << GetPlayer()->GetPackGUID();
        *data << static_cast<uint32>(0);  // TODO: This should be 1 if it is a failed timed criteria
        data->AppendPackedTime(iter->second.date);
        *data << static_cast<uint32>(now - iter->second.date);

        if (sAchievementMgr->IsAverageCriteria(sAchievementCriteriaStore.LookupEntry(iter->first)))
            *data << static_cast<uint32>(now - GetPlayer()->GetCreationTime().count());  // For average achievements
        else
            *data << static_cast<uint32>(now - iter->second.date);
    }

    *data << -1;
}

bool AchievementMgr::HasAchieved(const uint32 achievementId) const
{
    return _completedAchievements.contains(achievementId);
}

bool AchievementMgr::CanUpdateCriteria(const AchievementCriteriaEntry* criteria, const AchievementEntry* achievement)
{
    if (sDisableMgr->IsDisabledFor(DISABLE_TYPE_ACHIEVEMENT_CRITERIA, criteria->ID, nullptr))
        return false;

    if (achievement->MapID != -1 && GetPlayer()->GetMapId() != static_cast<uint32>(achievement->MapID))
        return false;

    if ((achievement->RequiredFaction == ACHIEVEMENT_FACTION_HORDE    && GetPlayer()->GetTeamId(true) != TEAM_HORDE) ||
        (achievement->RequiredFaction == ACHIEVEMENT_FACTION_ALLIANCE && GetPlayer()->GetTeamId(true) != TEAM_ALLIANCE))
        return false;

    for (uint32 i = 0; i < MAX_CRITERIA_REQUIREMENTS; ++i)
    {
        if (!criteria->AdditionalRequirements[i].AdditionalRequirementType)
            continue;

        switch (criteria->AdditionalRequirements[i].AdditionalRequirementType)
        {
            case ACHIEVEMENT_CRITERIA_CONDITION_BG_MAP:
                if (GetPlayer()->GetMapId() != criteria->AdditionalRequirements[i].AdditionalRequirementValue)
                    return false;
                break;
            case ACHIEVEMENT_CRITERIA_CONDITION_NOT_IN_GROUP:
                if (GetPlayer()->GetGroup())
                    return false;
                break;
            default:
                break;
        }
    }

    // Don't update already completed criteria
    if (IsCompletedCriteria(criteria, achievement))
        return false;

    return true;
}

const CompletedAchievementMap& AchievementMgr::GetCompletedAchievements()
{
    return _completedAchievements;
}

void AchievementMgr::ProcessOfflineUpdatesQueue()
{
    if (_offlineUpdatesQueue.empty())
        return;

    for (auto const& update : _offlineUpdatesQueue)
        ProcessOfflineUpdate(update);

    _offlineUpdatesQueue.clear();

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_ACHIEVEMENT_OFFLINE_UPDATES);
    stmt->SetData(0, GetPlayer()->GetGUID().GetCounter());
    CharacterDatabase.Execute(stmt);
}

void AchievementMgr::ProcessOfflineUpdate(const AchievementOfflinePlayerUpdate& update)
{
    switch (update.updateType)
    {
        case ACHIEVEMENT_OFFLINE_PLAYER_UPDATE_TYPE_COMPLETE_ACHIEVEMENT:
        {
            AchievementEntry const* achievement = sAchievementStore.LookupEntry(update.arg1);

            ASSERT(achievement, "Not found achievement to complete for offline achievements update. Wrong arg1 ({}) value?", update.arg1);

            CompletedAchievement(achievement);
            break;
        }
        case ACHIEVEMENT_OFFLINE_PLAYER_UPDATE_TYPE_UPDATE_CRITERIA:
        {
            const auto criteriaType = static_cast<AchievementCriteriaTypes>(update.arg1);
            UpdateAchievementCriteria(criteriaType, update.arg2, update.arg3);
            break;
        }
        default:
            ASSERT(false, "Unknown offline achievement update type ({}) for player - {}", update.updateType, GetPlayer()->GetGUID().GetCounter());
            break;
    }
}

AchievementGlobalMgr* AchievementGlobalMgr::instance()
{
    static AchievementGlobalMgr instance;
    return &instance;
}

bool AchievementGlobalMgr::IsStatisticCriteria(const AchievementCriteriaEntry* achievementCriteria)
{
    return IsStatisticAchievement(sAchievementStore.LookupEntry(achievementCriteria->ReferredAchievement));
}

bool AchievementGlobalMgr::IsStatisticAchievement(const AchievementEntry* achievement)
{
    if (!achievement)
        return false;

    const AchievementCategoryEntry* cat = sAchievementCategoryStore.LookupEntry(achievement->CategoryID);
    do
    {
        switch (cat->ID)
        {
            case ACHIEVEMENT_CATEGORY_STATISTICS:
                return true;
            case ACHIEVEMENT_CATEGORY_GENERAL:
                return false;
            default:
                cat = sAchievementCategoryStore.LookupEntry(cat->ParentCategory);
                break;
        }
    } while (cat);

    return false;
}

bool AchievementGlobalMgr::IsAverageCriteria(const AchievementCriteriaEntry* criteria) const
{
    const auto referencedAchievement = sAchievementStore.LookupEntry(criteria->ReferredAchievement);
    if (!referencedAchievement)
        return false;

    if (referencedAchievement->Flags & ACHIEVEMENT_FLAG_AVERAGE)
        return true;

    if (AchievementEntryList const* achRefList = GetAchievementByReferencedId(referencedAchievement->ID))
        for (auto itr = achRefList->begin(); itr != achRefList->end(); ++itr)
            if ((*itr)->Flags & ACHIEVEMENT_FLAG_AVERAGE)
                return true;

    return false;
}

bool AchievementGlobalMgr::IsRealmCompleted(const AchievementEntry* achievement) const
{
    const auto itr = _allCompletedAchievements.find(achievement->ID);
    if (itr == _allCompletedAchievements.end())
        return false;

    if (itr->second == SystemTimePoint::min())
        return false;

    if (!sScriptMgr->IsRealmCompleted(this, achievement, itr->second))
        return false;

    if (itr->second == SystemTimePoint::max())
        return true;

    // Allow completing the realm first kill for configurable time window after first person did it.
    // It may allow more than one group to achieve it (highly unlikely), but apparently this is how blizz handles it as well.
    if (achievement->Flags & ACHIEVEMENT_FLAG_REALM_FIRST_KILL)
    {
        const auto windowSeconds = Seconds(sWorld->getIntConfig(CONFIG_ACHIEVEMENT_REALM_FIRST_KILL_WINDOW));
        if (windowSeconds == 0s)
            return true;
        return GameTime::GetSystemTime() - itr->second > windowSeconds;
    }

    sScriptMgr->SetRealmCompleted(achievement);
    return true;
}

void AchievementGlobalMgr::SetRealmCompleted(const AchievementEntry* achievement)
{
    if (IsRealmCompleted(achievement))
        return;
    _allCompletedAchievements[achievement->ID] = GameTime::GetSystemTime();
}

void AchievementGlobalMgr::LoadAchievementCriteriaList()
{
    const uint32 oldMSTime = getMSTime();

    if (sAchievementCriteriaStore.GetNumRows() == 0)
    {
        LOG_WARN("server.loading", ">> Loaded 0 achievement criteria.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 loaded = 0;
    for (uint32 entryId = 0; entryId < sAchievementCriteriaStore.GetNumRows(); ++entryId)
    {
        AchievementCriteriaEntry const* criteria = sAchievementCriteriaStore.LookupEntry(entryId);
        if (!criteria)
            continue;

        if (!GetAchievement(criteria->ReferredAchievement))
        {
            LOG_DEBUG("server.loading", "Achievement {} referenced by criteria {} doesn't exist, criteria not loaded.", criteria->ReferredAchievement, criteria->ID);
            continue;
        }

        _achievementCriteriaByType[criteria->RequiredType].push_back(criteria);
        _achievementCriteriaListByAchievement[criteria->ReferredAchievement].push_back(criteria);

        if (criteria->AdditionalRequirements[0].AdditionalRequirementType != ACHIEVEMENT_CRITERIA_CONDITION_NONE)
            _achievementCriteriaByCondition[criteria->AdditionalRequirements[0].AdditionalRequirementType][criteria->AdditionalRequirements[0].AdditionalRequirementValue].push_back(criteria);
        if (criteria->AdditionalRequirements[1].AdditionalRequirementType != ACHIEVEMENT_CRITERIA_CONDITION_NONE &&
                criteria->AdditionalRequirements[1].AdditionalRequirementType != criteria->AdditionalRequirements[0].AdditionalRequirementType)
            _achievementCriteriaByCondition[criteria->AdditionalRequirements[1].AdditionalRequirementType][criteria->AdditionalRequirements[1].AdditionalRequirementValue].push_back(criteria);

        switch (criteria->RequiredType)
        {
        case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
            _specialList[criteria->RequiredType][criteria->KillCreature.CreatureID].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
            _specialList[criteria->RequiredType][criteria->WinBG.MapID].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL:
            _specialList[criteria->RequiredType][criteria->ReachSkillLevel.SkillID].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT:
            _specialList[criteria->RequiredType][criteria->CompleteAchievement.LinkedAchievement].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE:
            _specialList[criteria->RequiredType][criteria->CompleteQuestsInZone.ZoneID].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND:
            _specialList[criteria->RequiredType][criteria->CompleteBattleground.MapID].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE:
            _specialList[criteria->RequiredType][criteria->KilledByCreature.CreatureEntry].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:
            _specialList[criteria->RequiredType][criteria->CompleteQuest.QuestID].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
            _specialList[criteria->RequiredType][criteria->BeSpellTarget.SpellID].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:
            _specialList[criteria->RequiredType][criteria->CastSpell.SpellID].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
            _specialList[criteria->RequiredType][criteria->BGObjective.ObjectiveID].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL_AT_AREA:
            _specialList[criteria->RequiredType][criteria->HonorableKillAtArea.AreaID].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL:
            _specialList[criteria->RequiredType][criteria->LearnSpell.SpellID].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM:
            _specialList[criteria->RequiredType][criteria->OwnItem.ItemID].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL:
            _specialList[criteria->RequiredType][criteria->LearnSkillLevel.SkillID].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:
            _specialList[criteria->RequiredType][criteria->UseItem.ItemID].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM:
            _specialList[criteria->RequiredType][criteria->OwnItem.ItemID].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA:
            {
                WorldMapOverlayEntry const* worldOverlayEntry = sWorldMapOverlayStore.LookupEntry(criteria->ExploreArea.AreaReference);
                if (!worldOverlayEntry)
                    break;

                for (uint8 j = 0; j < MAX_WORLD_MAP_OVERLAY_AREA_IDX; ++j)
                    if (worldOverlayEntry->AreaID[j])
                    {
                        bool valid = true;
                        for (uint8 i = 0; i < j; ++i)
                            if (worldOverlayEntry->AreaID[j] == worldOverlayEntry->AreaID[i])
                                valid = false;
                        if (valid)
                            _specialList[criteria->RequiredType][worldOverlayEntry->AreaID[j]].push_back(criteria);
                    }
            }
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_GAIN_REPUTATION:
            _specialList[criteria->RequiredType][criteria->GainReputation.FactionID].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM:
            _specialList[criteria->RequiredType][criteria->EquipEpicItem.ItemSlot].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS:
            _specialList[criteria->RequiredType][criteria->HKClass.ClassID].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_HK_RACE:
            _specialList[criteria->RequiredType][criteria->HKRace.RaceID].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:
            _specialList[criteria->RequiredType][criteria->DoEmote.EmoteID].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM:
            _specialList[criteria->RequiredType][criteria->EquipItem.ItemID].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT:
            _specialList[criteria->RequiredType][criteria->UseGameObject.GameObject].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
            _specialList[criteria->RequiredType][criteria->BeSpellTarget.SpellID].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_FISH_IN_GAMEOBJECT:
            _specialList[criteria->RequiredType][criteria->FishInGameObject.GameObject].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS:
            _specialList[criteria->RequiredType][criteria->LearnSkillLineSpell.SkillLine].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:
            _specialList[criteria->RequiredType][criteria->LootType.LootType].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:
            _specialList[criteria->RequiredType][criteria->CastSpell.SpellID].push_back(criteria);
            break;
        case ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE:
            _specialList[criteria->RequiredType][criteria->LearnSkillLine.SkillLine].push_back(criteria);
            break;
        default:
            break;
        }

        if (criteria->TimeLimit)
            _achievementCriteriaByTimedType[criteria->TimedType].push_back(criteria);
        ++loaded;
    }

    LOG_INFO("server.loading", ">> Loaded {} achievement criteria in {} ms", loaded, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void AchievementGlobalMgr::LoadAchievementReferenceList()
{
    const uint32 oldMSTime = getMSTime();

    if (sAchievementStore.GetNumRows() == 0)
    {
        LOG_WARN("server.loading", ">> Loaded 0 achievement references.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    for (uint32 entryId = 0; entryId < sAchievementStore.GetNumRows(); ++entryId)
    {
        AchievementEntry const* achievement = sAchievementStore.LookupEntry(entryId);
        if (!achievement || !achievement->RefAchievement)
            continue;

        _achievementListByReferencedId[achievement->RefAchievement].push_back(achievement);
        ++count;
    }

    LOG_INFO("server.loading", ">> Loaded {} achievement references in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void AchievementGlobalMgr::LoadAchievementCriteriaData()
{
    const uint32 oldMSTime = getMSTime();

    _criteriaDataMap.clear();  // Need for reload case

    const QueryResult result = WorldDatabase.Query("SELECT criteria, type, value1, value2, script_name FROM world_achievement_criteria_data");
    const auto tableName = "world_achievement_criteria_data";

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 additional achievement criteria data. DB table `{}` is empty.", tableName);
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();
        uint32 criteria_id = fields[0].Get<uint32>();

        const AchievementCriteriaEntry* criteria = sAchievementCriteriaStore.LookupEntry(criteria_id);

        if (!criteria)
        {
            LOG_ERROR("sql.sql", "Table `{}` has data for non-existing criteria (Entry: {}), ignore.", tableName, criteria_id);
            continue;
        }

        uint32 dataType = fields[1].Get<uint8>();
        auto scriptName = fields[4].Get<std::string>();
        uint32 scriptId = 0;
        if (scriptName.length()) // not empty
        {
            if (dataType != ACHIEVEMENT_CRITERIA_DATA_TYPE_SCRIPT)
                LOG_ERROR("sql.sql", "Table `{}` has ScriptName set for non-scripted data type (Entry: {}, type {}), useless data.", tableName, criteria_id, dataType);
            else
                scriptId = sObjectMgr->GetScriptID(scriptName);
        }

        AchievementCriteriaData data(dataType, fields[2].Get<uint32>(), fields[3].Get<uint32>(), scriptId);

        if (!data.IsValid(criteria))
            continue;

        // This will allocate empty data set storage
        AchievementCriteriaDataSet& dataSet = _criteriaDataMap[criteria_id];
        dataSet.SetCriteriaId(criteria_id);

        // Add real data only for not NONE data types
        if (data.dataType != ACHIEVEMENT_CRITERIA_DATA_TYPE_NONE)
            dataSet.Add(data);

        // Counting data by and data types
        ++count;
    } while (result->NextRow());

    // Post loading checks
    for (uint32 entryId = 0; entryId < sAchievementCriteriaStore.GetNumRows(); ++entryId)
    {
        const AchievementCriteriaEntry* criteria = sAchievementCriteriaStore.LookupEntry(entryId);
        if (!criteria)
            continue;

        switch (criteria->RequiredType)
        {
            case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE:
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_BG:
            case ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING:
            case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET:
            case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL:
            case ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE:
            case ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL:
            case ACHIEVEMENT_CRITERIA_TYPE_OWN_RANK:
            case ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM:
            case ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT:
            case ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT:
            case ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS:
            case ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2:
            case ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL:
            case ACHIEVEMENT_CRITERIA_TYPE_ON_LOGIN:
            case ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE_TYPE:
            case ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2:
                // Achievement requires db data
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST:
                {
                    AchievementEntry const* achievement = sAchievementStore.LookupEntry(criteria->ReferredAchievement);
                    if (!achievement)
                        continue;

                    // Exist many achievements with this criterion, use at this moment hardcoded check to skil simple case
                    if (achievement->ID == 1282)
                        break;
                    continue;
                }
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA:  // Need skip generic cases
                if (criteria->AdditionalRequirements[0].AdditionalRequirementType != ACHIEVEMENT_CRITERIA_CONDITION_NO_LOSE)
                    continue;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE:  // Need skip generic cases
                if (criteria->DoEmote.Count == 0)
                    continue;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL:  // Skip statistics
                if (criteria->WinDuel.DuelCount == 0)
                    continue;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE:  // Need skip generic cases
                if (criteria->LootType.LootTypeCount != 1)
                    continue;
                break;
            case ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST:
            case ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM:  // Only Children's Week achievements
                {
                    AchievementEntry const* achievement = sAchievementStore.LookupEntry(criteria->ReferredAchievement);
                    if (!achievement)
                        continue;
                    if (achievement->CategoryID != CATEGORY_CHILDRENS_WEEK)
                        continue;
                    break;
                }
            default:  // Type not use DB data, ignore
                continue;
        }

        if (!GetCriteriaDataSet(criteria) && !sDisableMgr->IsDisabledFor(DISABLE_TYPE_ACHIEVEMENT_CRITERIA, entryId, nullptr))
            LOG_ERROR("sql.sql", "Table `{}` does not have expected data for criteria (Entry: {} Type: {}) for achievement {}.",
                tableName, criteria->ID, criteria->RequiredType, criteria->ReferredAchievement);
    }

    LOG_INFO("server.loading", ">> Loaded {} additional achievement criteria data in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void AchievementGlobalMgr::LoadCompletedAchievements()
{
    const uint32 oldMSTime = getMSTime();

    const QueryResult result = CharacterDatabase.Query("SELECT achievement FROM character_achievement GROUP BY achievement");

    // Populate _allCompletedAchievements with all realm first achievement ids to make multithreaded access safer
    // while it will not prevent races, it will prevent crashes that happen because std::unordered_map key was added
    // instead the only potential race will happen on value associated with the key
    for (uint32 i = 0; i < sAchievementStore.GetNumRows(); ++i)
        if (AchievementEntry const* achievement = sAchievementStore.LookupEntry(i))
            if (achievement->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
                _allCompletedAchievements[achievement->ID] = SystemTimePoint::min();

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 completed achievements. DB table `character_achievement` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    do
    {
        const Field* fields = result->Fetch();

        uint16 achievementId = fields[0].Get<uint16>();
        if (const AchievementEntry* achievement = sAchievementStore.LookupEntry(achievementId); !achievement)
        {
            // Remove non-existent achievements from all characters
            LOG_ERROR("achievement", "Non-existing achievement {} data removed from table `character_achievement`.", achievementId);

            CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_INVALID_ACHIEVMENT);

            stmt->SetData(0, achievementId);
            CharacterDatabase.Execute(stmt);
        }
        else if (achievement->Flags & (ACHIEVEMENT_FLAG_REALM_FIRST_REACH | ACHIEVEMENT_FLAG_REALM_FIRST_KILL))
            _allCompletedAchievements[achievementId] =  SystemTimePoint::max();
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} completed achievements in {} ms", _allCompletedAchievements.size(), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void AchievementGlobalMgr::LoadRewards()
{
    const uint32 oldMSTime = getMSTime();

    _achievementRewards.clear();  // need for reload case
    const QueryResult result = WorldDatabase.Query("SELECT id, title_alliance, title_horde, item, sender, subject, body, mail_template FROM world_achievement_reward");
    const auto tableName = "world_achievement_reward";

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 achievement rewards. DB table `{}` is empty.", tableName);
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();
        uint32 entry = fields[0].Get<uint32>();
        AchievementEntry const* achievement = sAchievementStore.LookupEntry(entry);
        if (!achievement)
        {
            LOG_ERROR("sql.sql", "Table `{}` has wrong achievement (Entry: {}). Ignoring.", tableName, entry);
            continue;
        }

        AchievementReward reward;
        reward.titleId[0]   = fields[1].Get<uint32>(); // Alliance title
        reward.titleId[1]   = fields[2].Get<uint32>(); // Horde title
        reward.itemId       = fields[3].Get<uint32>();
        reward.sender       = fields[4].Get<uint32>(); // The sender of the mail (a creature from creature_template)
        reward.subject      = fields[5].Get<std::string>();
        reward.text         = fields[6].Get<std::string>();
        reward.mailTemplate = fields[7].Get<uint32>();

        // Must reward a title or send a mail else, skip it.
        if (!reward.titleId[0] && !reward.titleId[1] && !reward.sender)
        {
            LOG_ERROR("sql.sql", "Table `{}` (Entry: {}) does not have any title or item reward data. Ignoring.", tableName, entry);
            continue;
        }

        if (achievement->RequiredFaction == ACHIEVEMENT_FACTION_ANY && (!reward.titleId[0] ^ !reward.titleId[1]))
            LOG_DEBUG("achievement", "Table `{}` (Entry: {}) has title (A: {} H: {}) set for only one team.", tableName, entry, reward.titleId[0], reward.titleId[1]);

        if (reward.titleId[0])
        {
            if (const CharTitlesEntry* titleEntry = sCharTitlesStore.LookupEntry(reward.titleId[0]); !titleEntry)
            {
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {}) has invalid title id ({}) in `title_alliance`. Setting it to 0.", tableName, entry, reward.titleId[0]);
                reward.titleId[0] = 0;
            }
        }

        if (reward.titleId[1])
        {
            if (const CharTitlesEntry* titleEntry = sCharTitlesStore.LookupEntry(reward.titleId[1]); !titleEntry)
            {
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {}) has invalid title id ({}) in `title_horde`. Setting it to 0.", tableName, entry, reward.titleId[1]);
                reward.titleId[1] = 0;
            }
        }

        // Check mail data before item for report including wrong item case
        if (reward.sender)
        {
            if (!sObjectMgr->GetCreatureTemplate(reward.sender))
            {
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {}) has invalid creature_template entry {} as sender. Will not send the mail reward.", tableName, entry, reward.sender);
                reward.sender = 0;
            }
        }
        else
        {
            if (reward.itemId)
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {}) has item reward set but does not have sender data set. Item will not be sent.", tableName, entry);

            if (!reward.subject.empty())
                // Maybe add "Mail will not be sent." ?
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {}) has mail subject but does not have sender data set.", tableName, entry);

            if (!reward.text.empty())
                // Maybe add "Mail will not be sent." ?
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {}) has mail body set but does not have sender data set.", tableName, entry);

            if (reward.mailTemplate)
                // Maybe add "Mail will not be sent." ?
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {}) has mail_template set does not have sender data set.", tableName, entry);
        }

        if (reward.mailTemplate)
        {
            if (!sMailTemplateStore.LookupEntry(reward.mailTemplate))
            {
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {}) has invalid mail_template ({}) (check the DBC).", tableName, entry, reward.mailTemplate);
                reward.mailTemplate = 0;
            }
            else if (!reward.subject.empty() || !reward.text.empty())
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {}) has mail_template ({}) and mail subject/body. To use the column mail_template, subject and body must be empty.", tableName, entry, reward.mailTemplate);
        }

        if (reward.itemId)
        {
            if (!sObjectMgr->GetItemTemplate(reward.itemId))
            {
                // Not sure if it's an error, it's probably an outDebug instead, because we can simply send a mail with no reward, right?
                LOG_ERROR("sql.sql", "Table `{}` (Entry: {}) has invalid item_template id {}. Reward mail will not contain any item.", tableName, entry, reward.itemId);
                reward.itemId = 0;
            }
        }

        _achievementRewards[entry] = reward;
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} achievement rewards in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

const AchievementEntry* AchievementGlobalMgr::GetAchievement(const uint32 achievementId)
{
    return sAchievementStore.LookupEntry(achievementId);
}

void AchievementGlobalMgr::CompletedAchievementForOfflinePlayer(const ObjectGuid::LowType playerLowGuid, const AchievementEntry* entry)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHAR_ACHIEVEMENT_OFFLINE_UPDATES);
    stmt->SetData(0, playerLowGuid);
    stmt->SetData(1, ACHIEVEMENT_OFFLINE_PLAYER_UPDATE_TYPE_COMPLETE_ACHIEVEMENT);
    stmt->SetData(2, entry->ID);
    stmt->SetData(3, 0);
    stmt->SetData(4, 0);
    CharacterDatabase.Execute(stmt);
}

void AchievementGlobalMgr::UpdateAchievementCriteriaForOfflinePlayer(
    const ObjectGuid::LowType playerLowGuid, const AchievementCriteriaTypes type, const uint32 miscValue1, const uint32 miscValue2)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHAR_ACHIEVEMENT_OFFLINE_UPDATES);
    stmt->SetData(0, playerLowGuid);
    stmt->SetData(1, ACHIEVEMENT_OFFLINE_PLAYER_UPDATE_TYPE_UPDATE_CRITERIA);
    stmt->SetData(2, type);
    stmt->SetData(3, miscValue1);
    stmt->SetData(4, miscValue2);
    CharacterDatabase.Execute(stmt);
}
