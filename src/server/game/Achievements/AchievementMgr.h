#ifndef NCORE_ACHIEVEMENT_MGR_H
#define NCORE_ACHIEVEMENT_MGR_H

#include <map>
#include <string>
#include "DatabaseEnv.h"
#include "DBCDefines.h"
#include "DBCStores.h"
#include "ObjectGuid.h"

typedef std::list<const AchievementCriteriaEntry*> AchievementCriteriaEntryList;
typedef std::list<const AchievementEntry*> AchievementEntryList;

typedef std::unordered_map<uint32, AchievementCriteriaEntryList> AchievementCriteriaListByAchievement;
typedef std::map<uint32, AchievementEntryList> AchievementListByReferencedId;

enum AchievementOfflinePlayerUpdateType
{
    ACHIEVEMENT_OFFLINE_PLAYER_UPDATE_TYPE_COMPLETE_ACHIEVEMENT = 1,
    ACHIEVEMENT_OFFLINE_PLAYER_UPDATE_TYPE_UPDATE_CRITERIA      = 2,
};

struct AchievementOfflinePlayerUpdate
{
    AchievementOfflinePlayerUpdateType updateType;
    uint32 arg1;
    uint32 arg2;
    uint32 arg3;
};

struct CriteriaProgress
{
    uint32 counter;
    time_t date;  // Latest update time.
    bool changed;
};

enum AchievementCriteriaDataType
{
    //                                                           (value1, value2) comment
    ACHIEVEMENT_CRITERIA_DATA_TYPE_NONE                 = 0,  // ()
    ACHIEVEMENT_CRITERIA_DATA_TYPE_T_CREATURE           = 1,  // (creature_id,)
    ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_CLASS_RACE  = 2,  // (class_id, race_id)
    ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_LESS_HEALTH = 3,  // (health_percent, 0)
    ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_DEAD        = 4,  // (own_team, 0)          Not corpse (not released body), own_team == false if enemy team expected
    ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA               = 5,  // (spell_id, effect_idx)
    ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AREA               = 6,  // (area_id, 0)
    ACHIEVEMENT_CRITERIA_DATA_TYPE_T_AURA               = 7,  // (spell_id, effect_idx)
    ACHIEVEMENT_CRITERIA_DATA_TYPE_VALUE                = 8,  // (min_value, )           Value provided with achievement update must be not less that limit
    ACHIEVEMENT_CRITERIA_DATA_TYPE_T_LEVEL              = 9,  // (min_level, )           Target min_level
    ACHIEVEMENT_CRITERIA_DATA_TYPE_T_GENDER             = 10, // (gender, )              0=male; 1=female
    ACHIEVEMENT_CRITERIA_DATA_TYPE_SCRIPT               = 11, // scripted requirement
    ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_DIFFICULTY       = 12, // (difficulty, )                   normal/heroic difficulty for current event map
    ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_PLAYER_COUNT     = 13, // (count, )                        "with less than %u people in the zone"
    ACHIEVEMENT_CRITERIA_DATA_TYPE_T_TEAM               = 14, // (team, )                         HORDE(67), ALLIANCE(469)
    ACHIEVEMENT_CRITERIA_DATA_TYPE_S_DRUNK              = 15, // (drunken_state, 0)             (enum DrunkenState) of player
    ACHIEVEMENT_CRITERIA_DATA_TYPE_HOLIDAY              = 16, // (holiday_id, 0)             event in holiday time
    ACHIEVEMENT_CRITERIA_DATA_TYPE_BG_LOSS_TEAM_SCORE   = 17, // (min_score, max_score)     player's team win bg and opposition team have team score in range
    ACHIEVEMENT_CRITERIA_DATA_TYPE_INSTANCE_SCRIPT      = 18, // (0, 0)             maker instance script call for check current criteria requirements fit
    ACHIEVEMENT_CRITERIA_DATA_TYPE_S_EQUIPPED_ITEM      = 19, // (item_level, item_quality)  for equipped item in slot to check item level and quality
    ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_ID               = 20, // (map_id, 0)             player must be on map with id in map_id
    ACHIEVEMENT_CRITERIA_DATA_TYPE_S_PLAYER_CLASS_RACE  = 21, // (class_id, race_id)
    ACHIEVEMENT_CRITERIA_DATA_TYPE_NTH_BIRTHDAY         = 22, // (N, )                            login on day of N-th Birthday
    ACHIEVEMENT_CRITERIA_DATA_TYPE_S_KNOWN_TITLE        = 23, // (title_id, )                     known (pvp) title, values from dbc
    ACHIEVEMENT_CRITERIA_DATA_TYPE_BG_TEAMS_SCORES      = 24, // (winner_score, loser_score)   player's team win bg and their teams have exact scores
    ACHIEVEMENT_CRITERIA_DATA_TYPE_S_ITEM_QUALITY       = 25  // (item_quality,)
};
#define MAX_ACHIEVEMENT_CRITERIA_DATA_TYPE                26 // maximum value in AchievementCriteriaDataType enum

enum AchievementCommonCategories
{
    ACHIEVEMENT_CATEGORY_GENERAL                        = -1,
    ACHIEVEMENT_CATEGORY_STATISTICS                     =  1
};

class Player;
class Unit;

struct AchievementCriteriaData
{
    AchievementCriteriaDataType dataType{ACHIEVEMENT_CRITERIA_DATA_TYPE_NONE};
    union
    {
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_NONE                  = 0 (no data)
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_T_CREATURE            = 1
        struct
        {
            uint32 id;
        } creature;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_CLASS_RACE   = 2
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_S_PLAYER_CLASS_RACE   = 21
        struct
        {
            uint32 classID;
            uint32 raceID;
        } classRace;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_LESS_HEALTH  = 3
        struct
        {
            uint32 percent;
        } health;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_T_PLAYER_DEAD         = 4
        struct
        {
            uint32 ownTeamFlag;
        } player_dead;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AURA                = 5
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_T_AURA                = 7
        struct
        {
            uint32 spellID;
            uint32 effectIDX;
        } aura;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_S_AREA                = 6
        struct
        {
            uint32 id;
        } area;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_VALUE                 = 8
        struct
        {
            uint32 value;
            uint32 compType;
        } value;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_T_LEVEL               = 9
        struct
        {
            uint32 minLevel;
        } level;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_T_GENDER              = 10
        struct
        {
            uint32 gender;
        } gender;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_SCRIPT                = 11 (no data)
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_DIFFICULTY        = 12
        struct
        {
            uint32 difficulty;
        } difficulty;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_PLAYER_COUNT      = 13
        struct
        {
            uint32 maxCount;
        } map_players;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_T_TEAM                = 14
        struct
        {
            uint32 team;
        } team;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_S_DRUNK               = 15
        struct
        {
            uint32 state;
        } drunk;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_HOLIDAY               = 16
        struct
        {
            uint32 id;
        } holiday;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_BG_LOSS_TEAM_SCORE    = 17
        struct
        {
            uint32 minScore;
            uint32 maxScore;
        } bg_loss_team_score;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_INSTANCE_SCRIPT       = 18 (no data)
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_S_EQUIPPED_ITEM       = 19
        struct
        {
            uint32 itemLevel;
            uint32 itemQuality;
        } equipped_item;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_MAP_ID                = 20
        struct
        {
            uint32 mapID;
        } map_id;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_NTH_BIRTHDAY          = 22
        struct
        {
            uint32 nthBirthday;
        } birthday_login;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_KNOWN_TITLE           = 23
        struct
        {
            uint32 titleID;
        } known_title;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_BG_TEAMS_SCORES       = 24
        struct
        {
            uint32 winnerScore;
            uint32 loserScore;
        } teams_scores;
        // ACHIEVEMENT_CRITERIA_DATA_TYPE_S_ITEM_QUALITY        = 25
        struct
        {
            uint32 itemQuality;
        } item;
        // ...
        struct
        {
            uint32 value1;
            uint32 value2;
        } raw;
    };
    uint32 ScriptId;

    AchievementCriteriaData()
    {
        raw.value1 = 0;
        raw.value2 = 0;
        ScriptId = 0;
    }

    AchievementCriteriaData(uint32 _dataType, uint32 _value1, uint32 _value2, uint32 _scriptId) :
        dataType(static_cast<AchievementCriteriaDataType>(_dataType))
    {
        raw.value1 = _value1;
        raw.value2 = _value2;
        ScriptId = _scriptId;
    }

    bool IsValid(AchievementCriteriaEntry const* criteria);
    bool Meets(uint32 criteria_id, Player const* source, Unit const* target, uint32 misc_value1 = 0) const;
};

struct AchievementCriteriaDataSet
{
    AchievementCriteriaDataSet()  = default;
    typedef std::vector<AchievementCriteriaData> Storage;
    void Add(AchievementCriteriaData const& data) { _storage.push_back(data); }
    bool Meets(Player const* source, Unit const* target, uint32 misc_value = 0) const;
    void SetCriteriaId(const uint32 id) { _criteria_id = id; }
private:
    uint32 _criteria_id{0};
    Storage _storage;
};

typedef std::map<uint32, AchievementCriteriaDataSet> AchievementCriteriaDataMap;

struct AchievementReward
{
    uint32 titleId[2];
    uint32 itemId;
    uint32 sender;
    std::string subject;
    std::string text;
    uint32 mailTemplate;
};

typedef std::map<uint32, AchievementReward> AchievementRewards;

struct CompletedAchievementData
{
    time_t date;
    bool changed;
};

typedef std::unordered_map<uint32, CriteriaProgress> CriteriaProgressMap;
typedef std::unordered_map<uint32, CompletedAchievementData> CompletedAchievementMap;

class WorldPacket;

class AchievementMgr
{
public:
    explicit AchievementMgr(Player* player);
    ~AchievementMgr();

    void Reset();
    static void DeleteFromDB(ObjectGuid::LowType lowGuid);
    void LoadFromDB(const QueryResult& achievementResult, const QueryResult& criteriaResult, const QueryResult& offlineUpdatesResult);
    void SaveToDB(const CharacterDatabaseTransaction& trans);
    void ResetAchievementCriteria(AchievementCriteriaCondition condition, uint32 value, bool evenIfCriteriaComplete = false);
    void UpdateAchievementCriteria(AchievementCriteriaTypes type, uint32 miscValue1 = 0, uint32 miscValue2 = 0, Unit* unit = nullptr);
    void CompletedAchievement(const AchievementEntry* entry);
    void CheckAllAchievementCriteria();
    void SendAllAchievementData() const;
    void SendRespondInspectAchievements(const Player* player) const;
    [[nodiscard]] bool HasAchieved(uint32 achievementId) const;
    [[nodiscard]] Player* GetPlayer() const { return _player; }

    void Update(uint32 timeDiff);
    void StartTimedAchievement(AchievementCriteriaTimedTypes type, uint32 entry, uint32 timeLost = 0);
    void RemoveTimedAchievement(AchievementCriteriaTimedTypes type, uint32 entry);  // Used for quest and scripted timed achievements

    void RemoveCriteriaProgress(const AchievementCriteriaEntry* entry);
    CriteriaProgress* GetCriteriaProgress(const AchievementCriteriaEntry* entry);
    CompletedAchievementMap const& GetCompletedAchievements();

private:
    enum ProgressType { PROGRESS_SET, PROGRESS_ACCUMULATE, PROGRESS_HIGHEST, PROGRESS_RESET };
    void SendAchievementEarned(const AchievementEntry* achievement) const;
    void SendCriteriaUpdate(const AchievementCriteriaEntry* entry, CriteriaProgress const* progress, uint32 timeElapsed, bool timedCompleted) const;
    void SetCriteriaProgress(const AchievementCriteriaEntry* entry, uint32 changeValue, ProgressType ptype = PROGRESS_SET);
    void CompletedCriteriaFor(const AchievementEntry* achievement);
    bool IsCompletedCriteria(const AchievementCriteriaEntry* achievementCriteria, AchievementEntry const* achievement);
    bool IsCompletedAchievement(const AchievementEntry* entry);
    bool CanUpdateCriteria(const AchievementCriteriaEntry* criteria, AchievementEntry const* achievement);
    void BuildAllDataPacket(WorldPacket* data) const;

    void UpdateTimedAchievements(uint32 timeDiff);

    // Handles updates when character was offline.
    void ProcessOfflineUpdate(const AchievementOfflinePlayerUpdate& update);
    void ProcessOfflineUpdatesQueue();

    Player* _player;
    CriteriaProgressMap _criteriaProgress;
    CompletedAchievementMap _completedAchievements;
    typedef std::map<uint32, uint32> TimedAchievementMap;
    TimedAchievementMap _timedAchievements;  // Criteria id/time left in MS

    // Offline updates cannot be processed while players are loading,
    // as the player will not be notified of the changes.
    // To ensure proper notification, introduce a delay before processing.
    uint32 _offlineUpdatesDelayTimer;
    std::vector<AchievementOfflinePlayerUpdate> _offlineUpdatesQueue;
};

class AchievementGlobalMgr
{
    AchievementGlobalMgr() = default;
    ~AchievementGlobalMgr() = default;

public:
    static AchievementGlobalMgr* instance();

    static bool IsStatisticCriteria(const AchievementCriteriaEntry* achievementCriteria);
    static bool IsStatisticAchievement(const AchievementEntry* achievement);
    bool IsAverageCriteria(const AchievementCriteriaEntry* criteria) const;

    [[nodiscard]] const AchievementCriteriaEntryList* GetAchievementCriteriaByType(const AchievementCriteriaTypes type) const
    {
        return &_achievementCriteriaByType[type];
    }

    const AchievementCriteriaEntryList* GetSpecialAchievementCriteriaByType(const AchievementCriteriaTypes type, const uint32 val)
    {
        if (_specialList[type].contains(val))
            return &_specialList[type][val];
        return nullptr;
    }

    const AchievementCriteriaEntryList* GetAchievementCriteriaByCondition(const AchievementCriteriaCondition condition, const uint32 val)
    {
        if (_achievementCriteriaByCondition[condition].contains(val))
            return &_achievementCriteriaByCondition[condition][val];
        return nullptr;
    }

    [[nodiscard]] const AchievementCriteriaEntryList& GetTimedAchievementCriteriaByType(const AchievementCriteriaTimedTypes type) const
    {
        return _achievementCriteriaByTimedType[type];
    }

    [[nodiscard]] const AchievementCriteriaEntryList* GetAchievementCriteriaByAchievement(const uint32 id) const
    {
        const auto itr = _achievementCriteriaListByAchievement.find(id);
        return itr != _achievementCriteriaListByAchievement.end() ? &itr->second : nullptr;
    }

    [[nodiscard]] AchievementEntryList const* GetAchievementByReferencedId(const uint32 id) const
    {
        const auto itr = _achievementListByReferencedId.find(id);
        return itr != _achievementListByReferencedId.end() ? &itr->second : nullptr;
    }

    AchievementReward const* GetAchievementReward(const AchievementEntry* achievement) const
    {
        const auto iter = _achievementRewards.find(achievement->ID);
        return iter != _achievementRewards.end() ? &iter->second : nullptr;
    }

    AchievementCriteriaDataSet const* GetCriteriaDataSet(const AchievementCriteriaEntry* achievementCriteria) const
    {
        const auto iter = _criteriaDataMap.find(achievementCriteria->ID);
        return iter != _criteriaDataMap.end() ? &iter->second : nullptr;
    }

    bool IsRealmCompleted(const AchievementEntry* achievement) const;
    void SetRealmCompleted(const AchievementEntry* achievement);

    void LoadAchievementCriteriaList();
    void LoadAchievementCriteriaData();
    void LoadAchievementReferenceList();
    void LoadCompletedAchievements();
    void LoadRewards();

    [[nodiscard]] static const AchievementEntry* GetAchievement(uint32 achievementId);

    static void CompletedAchievementForOfflinePlayer(ObjectGuid::LowType playerLowGuid, const AchievementEntry* entry);
    static void UpdateAchievementCriteriaForOfflinePlayer(ObjectGuid::LowType playerLowGuid, AchievementCriteriaTypes type, uint32 miscValue1 = 0, uint32 miscValue2 = 0);
private:
    AchievementCriteriaDataMap _criteriaDataMap;

    // Store achievement criteria by type to speed up lookup
    AchievementCriteriaEntryList _achievementCriteriaByType[ACHIEVEMENT_CRITERIA_TYPE_TOTAL];
    AchievementCriteriaEntryList _achievementCriteriaByTimedType[ACHIEVEMENT_TIMED_TYPE_MAX];
    // Store achievement criteria by achievement to speed up lookup
    AchievementCriteriaListByAchievement _achievementCriteriaListByAchievement;
    // Store achievements by referenced achievement id to speed up lookup
    AchievementListByReferencedId _achievementListByReferencedId;

    typedef std::unordered_map<uint32 /*achievementId*/, SystemTimePoint /*completionTime*/> AllCompletedAchievements;
    AllCompletedAchievements _allCompletedAchievements;

    AchievementRewards _achievementRewards;

    std::map<uint32, AchievementCriteriaEntryList> _specialList[ACHIEVEMENT_CRITERIA_TYPE_TOTAL];
    std::map<uint32, AchievementCriteriaEntryList> _achievementCriteriaByCondition[ACHIEVEMENT_CRITERIA_CONDITION_TOTAL];
};

#define sAchievementMgr AchievementGlobalMgr::instance()

#endif
