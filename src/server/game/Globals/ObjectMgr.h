#ifndef OBJECT_MGR_H
#define OBJECT_MGR_H

#include <map>
#include <memory>
#include <string>

#include "AhoCorasick.h"
#include "Bag.h"
#include "ConditionMgr.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "GameObject.h"
#include "ItemTemplate.h"
#include "Log.h"
#include "Mail.h"
#include "Map.h"
#include "NPCHandler.h"
#include "Object.h"
#include "ObjectAccessor.h"
#include "ObjectDefines.h"
#include "QuestDef.h"
#include "TemporarySummon.h"
#include "Trainer.h"
#include "VehicleDefines.h"

class Item;
struct DungeonProgressionRequirements;
struct PlayerClassInfo;
struct PlayerClassLevelInfo;
struct PlayerInfo;
struct PlayerLevelInfo;

// GCC have alternative #pragma pack(N) syntax and old gcc version not support pack(push, N), also any gcc version not support it at some platform
#if defined(__GNUC__)
#pragma pack(1)
#else
#pragma pack(push, 1)
#endif

struct PageText
{
    std::string Text;
    uint32 NextPage;
};

/// Key for storing temp summon data in TempSummonDataContainer
struct TempSummonGroupKey
{
    TempSummonGroupKey(const uint32 summonerEntry, const SummonerType summonerType, const uint8 group)
        : _summonerEntry(summonerEntry), _summonerType(summonerType), _summonGroup(group) {}

    bool operator<(const TempSummonGroupKey& rhs) const
    {
        return std::tie(_summonerEntry, _summonerType, _summonGroup) < std::tie(rhs._summonerEntry, rhs._summonerType, rhs._summonGroup);
    }

private:
    uint32 _summonerEntry;      ///< Summoner's entry
    SummonerType _summonerType; ///< Summoner's type, see SummonerType for available types
    uint8 _summonGroup;         ///< Summon's group id
};

// GCC have alternative #pragma pack() syntax and old gcc version not support pack(pop), also any gcc version not support it at some platform
#if defined(__GNUC__)
#pragma pack()
#else
#pragma pack(pop)
#endif

// DB scripting commands
enum ScriptCommands
{
    SCRIPT_COMMAND_TALK                  = 0,                // source/target = Creature, target = any, data_i[0] = talk type (0=say, 1=whisper, 2=yell, 3=emote text, 4=boss emote text), data_i[1] & 1 = player talk (instead of creature), data_i[2] = string_id
    SCRIPT_COMMAND_EMOTE                 = 1,                // source/target = Creature, data_i[0] = emote id, data_i[1] = 0: set emote state; > 0: play emote state
    SCRIPT_COMMAND_FIELD_SET             = 2,                // source/target = Creature, data_i[0] = field id, datalog2 = value
    SCRIPT_COMMAND_MOVE_TO               = 3,                // source/target = Creature, data_i[1] = time to reach, x/y/z = destination
    SCRIPT_COMMAND_FLAG_SET              = 4,                // source/target = Creature, data_i[0] = field id, datalog2 = bitmask
    SCRIPT_COMMAND_FLAG_REMOVE           = 5,                // source/target = Creature, data_i[0] = field id, datalog2 = bitmask
    SCRIPT_COMMAND_TELEPORT_TO           = 6,                // source/target = Creature/Player (see data_i[1]), data_i[0] = map_id, data_i[1] = 0: Player; 1: Creature, x/y/z = destination, o = orientation
    SCRIPT_COMMAND_QUEST_EXPLORED        = 7,                // target/source = Player, target/source = GO/Creature, data_i[0] = quest id, data_i[1] = distance or 0
    SCRIPT_COMMAND_KILL_CREDIT           = 8,                // target/source = Player, data_i[0] = creature entry, data_i[1] = 0: personal credit, 1: group credit
    SCRIPT_COMMAND_RESPAWN_GAME_OBJECT    = 9,                // source = WorldObject (summoner), data_i[0] = GO guid, data_i[1] = despawn delay
    SCRIPT_COMMAND_TEMP_SUMMON_CREATURE  = 10,               // source = WorldObject (summoner), data_i[0] = creature entry, data_i[1] = despawn delay, x/y/z = summon position, o = orientation
    SCRIPT_COMMAND_OPEN_DOOR             = 11,               // source = Unit, data_i[0] = GO guid, data_i[1] = reset delay (min 15)
    SCRIPT_COMMAND_CLOSE_DOOR            = 12,               // source = Unit, data_i[0] = GO guid, data_i[1] = reset delay (min 15)
    SCRIPT_COMMAND_ACTIVATE_OBJECT       = 13,               // source = Unit, target = GO
    SCRIPT_COMMAND_REMOVE_AURA           = 14,               // source (data_i[1] != 0) or target (data_i[1] == 0) = Unit, data_i[0] = spell id
    SCRIPT_COMMAND_CAST_SPELL            = 15,               // source and/or target = Unit, data_i[1] = cast direction (0: s->t 1: s->s 2: t->t 3: t->s 4: s->creature with data_i[2] entry), data_i[2] & 1 = triggered flag
    SCRIPT_COMMAND_PLAY_SOUND            = 16,               // source = WorldObject, target = none/Player, data_i[0] = sound id, data_i[1] (bitmask: 0/1=anyone/player, 0/2=without/with distance dependency, so 1|2 = 3 is target with distance dependency)
    SCRIPT_COMMAND_CREATE_ITEM           = 17,               // target/source = Player, data_i[0] = item entry, data_i[1] = amount
    SCRIPT_COMMAND_DESPAWN_SELF          = 18,               // target/source = Creature, data_i[0] = despawn delay

    SCRIPT_COMMAND_LOAD_PATH             = 20,               // source = Unit, data_i[0] = path id, data_i[1] = is repeatable
    SCRIPT_COMMAND_CALLSCRIPT_TO_UNIT    = 21,               // source = WorldObject (if present used as a search center), data_i[0] = script id, data_i[1] = unit lowGUID, data_i[2] = script table to use (see ScriptsType)
    SCRIPT_COMMAND_KILL                  = 22,               // source/target = Creature, data_i[2] = remove corpse attribute

    // NordCore only
    SCRIPT_COMMAND_ORIENTATION           = 30,               // source = Unit, target (data_i[0] > 0) = Unit, data_i[0] = > 0 turn source to face target, o = orientation
    SCRIPT_COMMAND_EQUIP                 = 31,               // source = Creature, data_i[0] = equipment id
    SCRIPT_COMMAND_MODEL                 = 32,               // source = Creature, data_i[0] = model id
    SCRIPT_COMMAND_CLOSE_GOSSIP          = 33,               // source = Player
    SCRIPT_COMMAND_PLAY_MOVIE            = 34,               // source = Player, data_i[0] = movie id
    SCRIPT_COMMAND_MOVEMENT              = 35                // source = Creature, data_i[0] = MovementType, data_i[1] = MovementDistance (wander_distance f.ex.), data_i[2] = pathID
};

// Benchmarked: Faster than std::unordered_map (insert/find)
typedef std::map<uint32, PageText> PageTextContainer;

// Benchmarked: Faster than std::map (insert/find)
typedef std::unordered_map<uint16, InstanceTemplate> InstanceTemplateContainer;

struct GameTele
{
    float  PositionX;
    float  PositionY;
    float  PositionZ;
    float  Orientation;
    uint32 MapID;
    std::string Name;
    std::wstring WNameLow;
};

typedef std::unordered_map<uint32, GameTele > GameTeleContainer;

enum ScriptsType
{
    SCRIPTS_FIRST = 1,

    SCRIPTS_SPELL = SCRIPTS_FIRST,
    SCRIPTS_EVENT,
    SCRIPTS_WAYPOINT,

    SCRIPTS_LAST
};

enum eScriptFlags
{
    // Talk Flags
    SF_TALK_USE_PLAYER          = 0x1,

    // Emote flags
    SF_EMOTE_USE_STATE          = 0x1,

    // TeleportTo flags
    SF_TELEPORT_USE_CREATURE    = 0x1,

    // KillCredit flags
    SF_KILL_CREDIT_REWARD_GROUP  = 0x1,

    // RemoveAura flags
    SF_REMOVE_AURA_REVERSE       = 0x1,

    // CastSpell flags
    SF_CAST_SPELL_SOURCE_TO_TARGET = 0,
    SF_CAST_SPELL_SOURCE_TO_SOURCE = 1,
    SF_CAST_SPELL_TARGET_TO_TARGET = 2,
    SF_CAST_SPELL_TARGET_TO_SOURCE = 3,
    SF_CAST_SPELL_SEARCH_CREATURE  = 4,
    SF_CAST_SPELL_TRIGGERED      = 0x1,

    // Playsound flags
    SF_PLAY_SOUND_TARGET_PLAYER  = 0x1,
    SF_PLAY_SOUND_DISTANCE_SOUND = 0x2,
    SF_PLAY_SOUND_DISTANCE_RADIUS = 0x4,

    // Orientation flags
    SF_ORIENTATION_FACE_TARGET  = 0x1,
};

struct ScriptInfo
{
    ScriptsType type;
    uint32 id;
    uint32 delay;
    ScriptCommands command;

    union
    {
        struct
        {
            uint32 nData[3];
            float  fData[4];
        } Raw;

        struct                      // SCRIPT_COMMAND_TALK (0)
        {
            uint32 ChatType;        // data_i[0]
            uint32 Flags;           // data_i[1]
            int32  TextID;          // data_i[2]
        } Talk;

        struct                      // SCRIPT_COMMAND_EMOTE (1)
        {
            uint32 EmoteID;         // data_i[0]
            uint32 Flags;           // data_i[1]
        } Emote;

        struct                      // SCRIPT_COMMAND_FIELD_SET (2)
        {
            uint32 FieldID;         // data_i[0]
            uint32 FieldValue;      // data_i[1]
        } FieldSet;

        struct                      // SCRIPT_COMMAND_MOVE_TO (3)
        {
            uint32 Unused1;         // data_i[0]
            uint32 TravelTime;      // data_i[1]
            int32  Unused2;         // data_i[2]

            float DestX;
            float DestY;
            float DestZ;
        } MoveTo;

        struct                      // SCRIPT_COMMAND_FLAG_SET (4)
        // SCRIPT_COMMAND_FLAG_REMOVE (5)
        {
            uint32 FieldID;         // data_i[0]
            uint32 FieldValue;      // data_i[1]
        } FlagToggle;

        struct                      // SCRIPT_COMMAND_TELEPORT_TO (6)
        {
            uint32 MapID;           // data_i[0]
            uint32 Flags;           // data_i[1]
            int32  Unused1;         // data_i[2]

            float DestX;
            float DestY;
            float DestZ;
            float Orientation;
        } TeleportTo;

        struct                      // SCRIPT_COMMAND_QUEST_EXPLORED (7)
        {
            uint32 QuestID;         // data_i[0]
            uint32 Distance;        // data_i[1]
        } QuestExplored;

        struct                      // SCRIPT_COMMAND_KILL_CREDIT (8)
        {
            uint32 CreatureEntry;   // data_i[0]
            uint32 Flags;           // data_i[1]
        } KillCredit;

        struct                      // SCRIPT_COMMAND_RESPAWN_GAME_OBJECT (9)
        {
            uint32 GOGuid;          // data_i[0]
            uint32 DespawnDelay;    // data_i[1]
        } RespawnGameObject;

        struct                      // SCRIPT_COMMAND_TEMP_SUMMON_CREATURE (10)
        {
            uint32 CreatureEntry;   // data_i[0]
            uint32 DespawnDelay;    // data_i[1]
            uint32 CheckIfExists;   // data_i[2]

            float PosX;
            float PosY;
            float PosZ;
            float Orientation;
        } TempSummonCreature;

        struct                      // SCRIPT_COMMAND_CLOSE_DOOR (12)
        // SCRIPT_COMMAND_OPEN_DOOR (11)
        {
            uint32 GOGuid;          // data_i[0]
            uint32 ResetDelay;      // data_i[1]
        } ToggleDoor;

        // SCRIPT_COMMAND_ACTIVATE_OBJECT (13)

        struct                      // SCRIPT_COMMAND_REMOVE_AURA (14)
        {
            uint32 SpellID;         // data_i[0]
            uint32 Flags;           // data_i[1]
        } RemoveAura;

        struct                      // SCRIPT_COMMAND_CAST_SPELL (15)
        {
            uint32 SpellID;         // data_i[0]
            uint32 Flags;           // data_i[1]
            int32  CreatureEntry;   // data_i[2]

            float SearchRadius;
        } CastSpell;

        struct                      // SCRIPT_COMMAND_PLAY_SOUND (16)
        {
            uint32 SoundID;         // data_i[0]
            uint32 Flags;           // data_i[1]
            int32  Radius;          // data_i[2]
        } Playsound;

        struct                      // SCRIPT_COMMAND_CREATE_ITEM (17)
        {
            uint32 ItemEntry;       // data_i[0]
            uint32 Amount;          // data_i[1]
        } CreateItem;

        struct                      // SCRIPT_COMMAND_DESPAWN_SELF (18)
        {
            uint32 DespawnDelay;    // data_i[0]
        } DespawnSelf;

        struct                      // SCRIPT_COMMAND_LOAD_PATH (20)
        {
            uint32 PathID;          // data_i[0]
            uint32 IsRepeatable;    // data_i[1]
        } LoadPath;

        struct                      // SCRIPT_COMMAND_CALLSCRIPT_TO_UNIT (21)
        {
            uint32 CreatureEntry;   // data_i[0]
            uint32 ScriptID;        // data_i[1]
            uint32 ScriptType;      // data_i[2]
        } CallScript;

        struct                      // SCRIPT_COMMAND_KILL (22)
        {
            uint32 Unused1;         // data_i[0]
            uint32 Unused2;         // data_i[1]
            int32  RemoveCorpse;    // data_i[2]
        } Kill;

        struct                      // SCRIPT_COMMAND_ORIENTATION (30)
        {
            uint32 Flags;           // data_i[0]
            uint32 Unused1;         // data_i[1]
            int32  Unused2;         // data_i[2]

            float Unused3;
            float Unused4;
            float Unused5;
            float Orientation;
        } Orientation;

        struct                      // SCRIPT_COMMAND_EQUIP (31)
        {
            uint32 EquipmentID;     // data_i[0]
        } Equip;

        struct                      // SCRIPT_COMMAND_MODEL (32)
        {
            uint32 ModelID;         // data_i[0]
        } Model;

        // SCRIPT_COMMAND_CLOSE_GOSSIP (33)

        struct                      // SCRIPT_COMMAND_PLAY_MOVIE (34)
        {
            uint32 MovieID;         // data_i[0]
        } PlayMovie;

        struct                       // SCRIPT_COMMAND_MOVEMENT (35)
        {
            uint32 MovementType;     // data_i[0]
            uint32 MovementDistance; // data_i[1]
            int32  Path;             // data_i[2]
        } Movement;
    };

    [[nodiscard]] std::string GetDebugInfo() const;
};

typedef std::multimap<uint32, ScriptInfo> ScriptMap;
typedef std::map<uint32, ScriptMap > ScriptMapMap;
typedef std::multimap<uint32, uint32> SpellScriptsContainer;
typedef std::pair<SpellScriptsContainer::iterator, SpellScriptsContainer::iterator> SpellScriptsBounds;
extern ScriptMapMap sSpellScripts;
extern ScriptMapMap sEventScripts;
extern ScriptMapMap sWaypointScripts;

ScriptMapMap* GetScriptsMapByType(ScriptsType type);
std::string GetScriptCommandName(ScriptCommands command);

struct SpellClickInfo
{
    uint32 spellID;
    uint8 castFlags;
    SpellClickUserTypes userType;

    // helpers
    bool IsFitToRequirements(const Unit* clicker, const Unit* activator) const;
};

typedef std::multimap<uint32, SpellClickInfo> SpellClickInfoContainer;
typedef std::pair<SpellClickInfoContainer::const_iterator, SpellClickInfoContainer::const_iterator> SpellClickInfoMapBounds;

struct AreaTriggerTeleport
{
    uint32 targetMapID;
    float targetX;
    float targetY;
    float targetZ;
    float targetOrientation;
};

struct AreaTrigger
{
    uint32 entry;
    uint32 map;
    float x;
    float y;
    float z;
    float radius;
    float length;
    float width;
    float height;
    float orientation;
};

struct BroadcastText
{
    BroadcastText()
    {
        MaleText.resize(DEFAULT_LOCALE + 1);
        FemaleText.resize(DEFAULT_LOCALE + 1);
    }

    uint32 Id{0};
    uint32 LanguageID{0};
    std::string MaleText;
    std::string FemaleText;

    [[nodiscard]] const std::string& GetText(const uint8 gender = GENDER_MALE, const bool forceGender = false) const
    {
        if (gender == GENDER_FEMALE && (forceGender || !FemaleText.empty()))
            return FemaleText;
        return MaleText;
    }
};

typedef std::unordered_map<uint32, BroadcastText> BroadcastTextContainer;

typedef std::set<ObjectGuid::LowType> CellGuidSet;

struct CellObjectGuids
{
    CellGuidSet creatures;
    CellGuidSet gameObjects;
};

typedef std::unordered_map<uint32/*cell_id*/, CellObjectGuids> CellObjectGuidsMap;
typedef std::unordered_map<uint32/*(mapid, spawnMode) pair*/, CellObjectGuidsMap> MapObjectGuids;
typedef std::map<ObjectGuid, ObjectGuid> LinkedRespawnContainer;
typedef std::unordered_map<ObjectGuid::LowType, CreatureData> CreatureDataContainer;
typedef std::unordered_map<ObjectGuid::LowType, GameObjectData> GameObjectDataContainer;
typedef std::unordered_map<uint32, SpawnGroupTemplateData> SpawnGroupDataContainer;
typedef std::multimap<uint32, const SpawnData*> SpawnGroupLinkContainer;
typedef std::map<TempSummonGroupKey, std::vector<TempSummonData> > TempSummonDataContainer;
typedef std::map<TempSummonGroupKey, std::vector<GameObjectSummonData> > GameObjectSummonDataContainer;
typedef std::unordered_map<uint32, std::string> NcoreStringContainer;
typedef std::unordered_map<uint32, VehicleSeatAddon> VehicleSeatAddonContainer;
typedef std::multimap<uint32, uint32> QuestRelations;
typedef std::pair<QuestRelations::const_iterator, QuestRelations::const_iterator> QuestRelationBounds;

struct PetLevelInfo
{
    PetLevelInfo()
    {
        stats.fill(0);
    }

    std::array<uint32, MAX_STATS> stats = { };
    uint32 health{0};
    uint32 mana{0};
    uint32 armor{0};
    uint32 min_dmg{0};
    uint32 max_dmg{0};
};

struct MailLevelReward
{
    MailLevelReward()  = default;
    MailLevelReward(const uint32 _raceMask, const uint32 _mailTemplateId, const uint32 _senderEntry) :
        raceMask(_raceMask), mailTemplateId(_mailTemplateId), senderEntry(_senderEntry) {}

    uint32 raceMask{0};
    uint32 mailTemplateId{0};
    uint32 senderEntry{0};
};

typedef std::list<MailLevelReward> MailLevelRewardList;
typedef std::unordered_map<uint8, MailLevelRewardList> MailLevelRewardContainer;

// We assume the rate is in general the same for all three types below, but chose to keep three for scalability and customization
struct RepRewardRate
{
    float questRate;            // We allow rate = 0.0 in database. For this case, it means that
    float questDailyRate;
    float questWeeklyRate;
    float questMonthlyRate;
    float questRepeatableRate;
    float creatureRate;         // no reputation are given at all for this faction/rate type.
    float spellRate;
};

struct ReputationOnKillEntry
{
    uint32 RepFaction1;
    uint32 RepFaction2;
    uint32 ReputationMaxCap1;
    float RepValue1;
    uint32 ReputationMaxCap2;
    float RepValue2;
    bool IsTeamAward1;
    bool IsTeamAward2;
    bool TeamDependent;
};

struct RepSpilloverTemplate
{
    uint32 faction[MAX_SPILLOVER_FACTIONS];
    float factionRate[MAX_SPILLOVER_FACTIONS];
    uint32 factionRank[MAX_SPILLOVER_FACTIONS];
};

struct PointOfInterest
{
    uint32 ID;
    float PositionX;
    float PositionY;
    uint32 Icon;
    uint32 Flags;
    uint32 Importance;
    std::string Name;
};

struct QuestGreeting
{
    uint16 EmoteType;
    uint32 EmoteDelay;
    std::string Greeting;
};

struct GossipMenuItems
{
    uint32          MenuID;
    uint32          OptionID;
    uint8           OptionIcon;
    std::string     OptionText;
    uint32          OptionBroadcastTextID;
    uint32          OptionType;
    uint32          OptionNpcFlag;
    uint32          ActionMenuID;
    uint32          ActionPoiID;
    bool            BoxCoded;
    uint32          BoxMoney;
    std::string     BoxText;
    ConditionList   Conditions;
    uint32          BoxBroadcastTextID;
};

struct GossipMenus
{
    uint32          MenuID;
    uint32          TextID;
    ConditionList   Conditions;
};

typedef std::multimap<uint32, GossipMenus> GossipMenusContainer;
typedef std::pair<GossipMenusContainer::const_iterator, GossipMenusContainer::const_iterator> GossipMenusMapBounds;
typedef std::pair<GossipMenusContainer::iterator, GossipMenusContainer::iterator> GossipMenusMapBoundsNonConst;
typedef std::multimap<uint32, GossipMenuItems> GossipMenuItemsContainer;
typedef std::pair<GossipMenuItemsContainer::const_iterator, GossipMenuItemsContainer::const_iterator> GossipMenuItemsMapBounds;
typedef std::pair<GossipMenuItemsContainer::iterator, GossipMenuItemsContainer::iterator> GossipMenuItemsMapBoundsNonConst;

struct QuestPOIPoint
{
    int32 x{0};
    int32 y{0};

    QuestPOIPoint() = default;
    QuestPOIPoint(const int32 _x, const int32 _y) : x(_x), y(_y) {}
};

struct QuestPOI
{
    uint32 ID{0};
    int32 ObjectiveIndex{0};
    uint32 MapID{0};
    uint32 AreaID{0};
    uint32 FloorID{0};
    uint32 Priority{0};
    uint32 Flags{0};
    std::vector<QuestPOIPoint> points;

    QuestPOI() = default;
    QuestPOI(const uint32 id, const int32 objIndex, const uint32 mapID, const uint32 areaID, const uint32 floorID, const uint32 priority, const uint32 flags)
        : ID(id), ObjectiveIndex(objIndex), MapID(mapID), AreaID(areaID), FloorID(floorID), Priority(priority), Flags(flags) {}
};

typedef std::vector<QuestPOI> QuestPOIVector;
typedef std::unordered_map<uint32, QuestPOIVector> QuestPOIContainer;
typedef std::map<std::pair<uint32, uint8>, QuestGreeting> QuestGreetingContainer;
typedef std::unordered_map<uint32, VendorItemData> CacheVendorItemContainer;
typedef std::vector<uint32> CreatureCustomIDsContainer;

enum SkillRangeType
{
    SKILL_RANGE_LANGUAGE,                                   // 300..300
    SKILL_RANGE_LEVEL,                                      // 1...max skill for level
    SKILL_RANGE_MONO,                                       // 1...1, gray monolith bar
    SKILL_RANGE_RANK,                                       // 1...skill for known rank
    SKILL_RANGE_NONE,                                       // 0..0 always
};

SkillRangeType GetSkillRangeType(const SkillRaceClassInfoEntry* rcEntry);

#define MAX_PLAYER_NAME          12                         // max allowed by client name length
#define MAX_INTERNAL_PLAYER_NAME 15                         // max server internal player name length (> MAX_PLAYER_NAME for support declined names)
#define MAX_PET_NAME             12                         // max allowed by client name length
#define MAX_CHARTER_NAME         24                         // max allowed by client name length
#define MAX_CHANNEL_NAME         50

bool normalizePlayerName(std::string& name);

struct LanguageDesc
{
    Language langID;
    uint32   spellID;
    uint32   skillID;
};

extern LanguageDesc lang_description[LANGUAGES_COUNT];
const LanguageDesc* GetLanguageDescByID(uint32 lang);

struct DungeonEncounter
{
    DungeonEncounter(const DungeonEncounterEntry* _dbcEntry, const EncounterCreditType _creditType, const uint32 _creditEntry, const uint32 _lastEncounterDungeon)
        : dbcEntry(_dbcEntry), creditType(_creditType), creditEntry(_creditEntry), lastEncounterDungeon(_lastEncounterDungeon) { }

    const DungeonEncounterEntry* dbcEntry;
    EncounterCreditType creditType;
    uint32 creditEntry;
    uint32 lastEncounterDungeon;
};

typedef std::list<const DungeonEncounter*> DungeonEncounterList;
typedef std::unordered_map<uint32, DungeonEncounterList> DungeonEncounterContainer;

typedef std::map<std::pair<SummonSlot /*TotemSlot*/, Races /*RaceId*/>, uint32 /*DisplayId*/> PlayerTotemModelMap;

typedef std::map<std::tuple<ShapeshiftForm /*ShapeshiftID*/, uint8 /*RaceID*/, uint8 /*CustomizationID*/, uint8 /*GenderID*/>, uint32 /*ModelID*/> PlayerShapeshiftModelMap;

static constexpr uint32 MAX_QUEST_MONEY_REWARDS = 10;
typedef std::array<uint32, MAX_QUEST_MONEY_REWARDS> QuestMoneyRewardArray;
typedef std::unordered_map<uint32, QuestMoneyRewardArray> QuestMoneyRewardStore;

class ObjectMgr
{
    ObjectMgr();
    ~ObjectMgr();

public:
    static ObjectMgr* instance();

    typedef std::unordered_map<uint32, Item*> ItemMap;
    typedef std::unordered_map<uint32, Quest*> QuestMap;
    typedef std::unordered_map<uint32, AreaTrigger> AreaTriggerContainer;
    typedef std::unordered_map<uint32, AreaTriggerTeleport> AreaTriggerTeleportContainer;
    typedef std::unordered_map<uint32, uint32> AreaTriggerScriptContainer;
    typedef std::unordered_map<uint32, std::unordered_map<uint8, DungeonProgressionRequirements*>> DungeonProgressionRequirementsContainer;
    typedef std::unordered_map<uint32, RepRewardRate > RepRewardRateContainer;
    typedef std::unordered_map<uint32, ReputationOnKillEntry> RepOnKillContainer;
    typedef std::unordered_map<uint32, RepSpilloverTemplate> RepSpilloverTemplateContainer;
    typedef std::unordered_map<uint32, PointOfInterest> PointOfInterestContainer;
    typedef std::vector<std::string> ScriptNameContainer;
    typedef std::map<uint32, uint32> CharacterConversionMap;
    typedef std::unordered_map<ObjectGuid::LowType, std::vector<float>> CreatureSparringContainer;

    const GameObjectTemplate* GetGameObjectTemplate(uint32 entry);
    bool IsGameObjectStaticTransport(uint32 entry);
    [[nodiscard]] const GameObjectTemplateContainer* GetGameObjectTemplates() const { return &_gameObjectTemplateStore; }
    int LoadReferenceVendor(uint32 vendor, int32 item, std::set<uint32>* skip_vendors);

    void LoadGameObjectTemplate();
    void LoadGameObjectTemplateAddons();

    const CreatureTemplate* GetCreatureTemplate(uint32 entry) const;
    [[nodiscard]] const CreatureTemplateContainer* GetCreatureTemplates() const { return &_creatureTemplateStore; }
    const CreatureModelInfo* GetCreatureModelInfo(uint32 modelId) const;
    const CreatureModelInfo* GetCreatureModelRandomGender(CreatureModel* model, const CreatureTemplate* creatureTemplate) const;
    static const CreatureModel* ChooseDisplayId(const CreatureTemplate* cinfo, const CreatureData* data = nullptr);
    static void ChooseCreatureFlags(const CreatureTemplate* cinfo, uint32& npcFlag, uint32& unit_flags, uint32& dynamicFlags, const CreatureData* data = nullptr);
    const EquipmentInfo* GetEquipmentInfo(uint32 entry, int8& id);
    const CreatureAddon* GetCreatureAddon(ObjectGuid::LowType lowGUID);
    const GameObjectAddon* GetGameObjectAddon(ObjectGuid::LowType lowGUID);
    [[nodiscard]] const GameObjectTemplateAddon* GetGameObjectTemplateAddon(uint32 entry) const;
    const CreatureAddon* GetCreatureTemplateAddon(uint32 entry);
    const CreatureMovementData* GetCreatureMovementOverride(ObjectGuid::LowType spawnId) const;
    const ItemTemplate* GetItemTemplate(uint32 entry) const;
    [[nodiscard]] const ItemTemplateContainer* GetItemTemplateStore() const { return &_itemTemplateStore; }
    [[nodiscard]] const std::vector<ItemTemplate*>* GetItemTemplateStoreFast() const { return &_itemTemplateStoreFast; }

    uint32 GetModelForTotem(SummonSlot totemSlot, Races race) const;

    uint32 GetModelForShapeshift(ShapeshiftForm form, const Player* player) const;

    const ItemSetNameEntry* GetItemSetNameEntry(const uint32 itemID)
    {
        const auto itr = _itemSetNameStore.find(itemID);
        if (itr != _itemSetNameStore.end())
            return &itr->second;
        return nullptr;
    }

    const InstanceTemplate* GetInstanceTemplate(uint32 mapID);

    [[nodiscard]] const PetLevelInfo* GetPetLevelInfo(uint32 creatureID, uint8 level) const;

    [[nodiscard]] const PlayerClassInfo* GetPlayerClassInfo(const uint32 class_) const
    {
        if (class_ >= MAX_CLASSES)
            return nullptr;
        return _playerClassInfo[class_];
    }
    void GetPlayerClassLevelInfo(uint32 class_, uint8 level, PlayerClassLevelInfo* info) const;

    [[nodiscard]] const PlayerInfo* GetPlayerInfo(uint32 race, uint32 class_) const;

    void GetPlayerLevelInfo(uint32 race, uint32 class_, uint8 level, PlayerLevelInfo* info) const;

    static uint32 GetNearestTaxiNode(float x, float y, float z, uint32 mapID, uint32 teamID);
    static uint32 GetNearestTaxiNode(const WorldLocation& loc, uint32 teamID);
    static void GetTaxiPath(uint32 source, uint32 destination, uint32& path, uint32& cost);
    uint32 GetTaxiMountDisplayID(uint32 id, TeamID teamID, bool allowedAltTeam = false) const;

    [[nodiscard]] const GameObjectQuestItemList* GetGameObjectQuestItemList(uint32 id) const
    {
        const auto itr = _gameObjectQuestItemStore.find(id);
        if (itr != _gameObjectQuestItemStore.end())
            return &itr->second;
        return nullptr;
    }
    [[nodiscard]] const GameObjectQuestItemMap* GetGameObjectQuestItemMap() const { return &_gameObjectQuestItemStore; }

    [[nodiscard]] const CreatureQuestItemList* GetCreatureQuestItemList(const uint32 id) const
    {
        const auto itr = _creatureQuestItemStore.find(id);
        if (itr != _creatureQuestItemStore.end())
            return &itr->second;
        return nullptr;
    }
    [[nodiscard]] const CreatureQuestItemMap* GetCreatureQuestItemMap() const { return &_creatureQuestItemStore; }

    [[nodiscard]] const Quest* GetQuestTemplate(const uint32 questID) const
    {
        return questID < _questTemplatesFast.size() ? _questTemplatesFast[questID] : nullptr;
    }

    [[nodiscard]] const QuestMap& GetQuestTemplates() const { return _questTemplates; }

    [[nodiscard]] uint32 GetQuestForAreaTrigger(const uint32 triggerID) const
    {
        if (const auto itr = _questAreaTriggerStore.find(triggerID); itr != _questAreaTriggerStore.end())
            return itr->second;
        return 0;
    }

    [[nodiscard]] bool IsTavernAreaTrigger(const uint32 triggerID, const uint32 faction) const
    {
        if (const auto itr = _tavernAreaTriggerStore.find(triggerID); itr != _tavernAreaTriggerStore.end())
            return (itr->second & faction) != 0;
        return false;
    }

    [[nodiscard]] const GossipText* GetGossipText(uint32 textID) const;

    [[nodiscard]] const AreaTrigger* GetAreaTrigger(const uint32 trigger) const
    {
        const auto itr = _areaTriggerStore.find(trigger);
        if (itr != _areaTriggerStore.end())
            return &itr->second;
        return nullptr;
    }

    [[nodiscard]] const AreaTriggerTeleport* GetAreaTriggerTeleport(const uint32 trigger) const
    {
        const auto itr = _areaTriggerTeleportStore.find(trigger);
        if (itr != _areaTriggerTeleportStore.end())
            return &itr->second;
        return nullptr;
    }

    [[nodiscard]] const DungeonProgressionRequirements* GetAccessRequirement(const uint32 mapID, const Difficulty difficulty) const
    {
        if (const auto itr = _accessRequirementStore.find(mapID); itr != _accessRequirementStore.end())
        {
            std::unordered_map<uint8, DungeonProgressionRequirements*> difficultiesProgressionRequirements = itr->second;
            if (const auto difficultiesItr = difficultiesProgressionRequirements.find(difficulty); difficultiesItr != difficultiesProgressionRequirements.end())
                return difficultiesItr->second;
        }
        return nullptr;
    }

    [[nodiscard]] const AreaTriggerTeleport* GetGoBackTrigger(uint32 mapID) const;
    [[nodiscard]] const AreaTriggerTeleport* GetMapEntranceTrigger(uint32 mapID) const;

    [[nodiscard]] const AreaTriggerScriptContainer& GetAllAreaTriggerScriptData() const { return _areaTriggerScriptStore; }
    uint32 GetAreaTriggerScriptId(uint32 triggerID);
    SpellScriptsBounds GetSpellScriptsBounds(uint32 spellID);

    [[nodiscard]] const RepRewardRate* GetRepRewardRate(const uint32 factionID) const
    {
        const auto itr = _repRewardRateStore.find(factionID);
        if (itr != _repRewardRateStore.end())
            return &itr->second;

        return nullptr;
    }

    [[nodiscard]] const ReputationOnKillEntry* GetReputationOnKilEntry(const uint32 id) const
    {
        const auto itr = _repOnKillStore.find(id);
        if (itr != _repOnKillStore.end())
            return &itr->second;
        return nullptr;
    }

    static int32 GetBaseReputationOf(const FactionEntry* factionEntry, uint8 race, uint8 playerClass);

    [[nodiscard]] const RepSpilloverTemplate* GetRepSpilloverTemplate(const uint32 factionID) const
    {
        const auto itr = _repSpilloverTemplateStore.find(factionID);
        if (itr != _repSpilloverTemplateStore.end())
            return &itr->second;
        return nullptr;
    }

    [[nodiscard]] const PointOfInterest* GetPointOfInterest(const uint32 id) const
    {
        const auto itr = _pointsOfInterestStore.find(id);
        if (itr != _pointsOfInterestStore.end())
            return &itr->second;
        return nullptr;
    }

    const QuestPOIVector* GetQuestPOIVector(const uint32 questID)
    {
        const auto itr = _questPOIStore.find(questID);
        if (itr != _questPOIStore.end())
            return &itr->second;
        return nullptr;
    }

    const VehicleAccessoryList* GetVehicleAccessoryList(const Vehicle* veh) const;

    const DungeonEncounterList* GetDungeonEncounterList(const uint32 mapID, const Difficulty difficulty)
    {
        const auto itr = _dungeonEncounterStore.find(MAKE_PAIR32(mapID, difficulty));
        if (itr != _dungeonEncounterStore.end())
            return &itr->second;
        return nullptr;
    }

    void LoadQuests();
    void LoadQuestMoneyRewards();
    void LoadQuestStartersAndEnders()
    {
        LOG_INFO("server.loading", "Loading GO Start Quest Data...");
        LoadGameObjectQuestStarters();
        LOG_INFO("server.loading", "Loading GO End Quest Data...");
        LoadGameObjectQuestEnders();
        LOG_INFO("server.loading", "Loading Creature Start Quest Data...");
        LoadCreatureQuestStarters();
        LOG_INFO("server.loading", "Loading Creature End Quest Data...");
        LoadCreatureQuestEnders();
    }
    void LoadGameObjectQuestStarters();
    void LoadGameObjectQuestEnders();
    void LoadCreatureQuestStarters();
    void LoadCreatureQuestEnders();

    QuestRelations* GetGOQuestRelationMap()
    {
        return &_goQuestRelations;
    }

    QuestRelations* GetGOQuestInvolvedRelationMap()
    {
        return &_goQuestInvolvedRelations;
    }

    QuestRelationBounds GetGOQuestRelationBounds(const uint32 goEntry)
    {
        return _goQuestRelations.equal_range(goEntry);
    }

    QuestRelationBounds GetGOQuestInvolvedRelationBounds(const uint32 goEntry)
    {
        return _goQuestInvolvedRelations.equal_range(goEntry);
    }

    QuestRelations* GetCreatureQuestRelationMap()
    {
        return &_creatureQuestRelations;
    }

    QuestRelations* GetCreatureQuestInvolvedRelationMap()
    {
        return &_creatureQuestInvolvedRelations;
    }

    QuestRelationBounds GetCreatureQuestRelationBounds(const uint32 creatureEntry)
    {
        return _creatureQuestRelations.equal_range(creatureEntry);
    }

    QuestRelationBounds GetCreatureQuestInvolvedRelationBounds(const uint32 creatureEntry)
    {
        return _creatureQuestInvolvedRelations.equal_range(creatureEntry);
    }

    void LoadEventScripts();
    void LoadSpellScripts();
    void LoadWaypointScripts();

    void LoadSpellScriptNames();
    void ValidateSpellScripts();
    static void InitializeSpellInfoPrecomputedData();

    bool LoadAcoreStrings();
    void LoadBroadcastTexts();
    void LoadCreatureClassLevelStats();
    void LoadCreatureTemplates();
    void LoadCreatureTemplate(const Field* fields, bool triggerHook = false);
    void LoadCreatureTemplateModels() const;
    void LoadCreatureTemplateAddons();
    void LoadCreatureTemplateResistances();
    void LoadCreatureTemplateSpells();
    void LoadCreatureCustomIDs();
    void CheckCreatureTemplate(const CreatureTemplate* cInfo);
    static void CheckCreatureMovement(const char* table, uint64 id, CreatureMovementData& creatureMovement);
    void LoadGameObjectQuestItems();
    void LoadCreatureQuestItems();
    void LoadTempSummons();
    void LoadGameObjectSummons();
    void LoadSpawnGroupTemplates();
    void LoadSpawnGroups();
    void LoadCreatures();
    void LoadCreatureSparring();
    void LoadLinkedRespawn();
    bool SetCreatureLinkedRespawn(ObjectGuid::LowType guidLow, ObjectGuid::LowType linkedGuidLow);
    void LoadCreatureAddons();
    void LoadGameObjectAddons();
    void LoadCreatureModelInfo();
    void LoadPlayerTotemModels();
    void LoadPlayerShapeshiftModels();
    void LoadEquipmentTemplates();
    void LoadCreatureMovementOverrides();
    void LoadGameObjects();
    void LoadItemTemplates();
    void LoadItemSetNames();
    void LoadInstanceTemplate();
    void LoadInstanceEncounters();
    void LoadMailLevelRewards();
    void LoadVehicleTemplateAccessories();
    void LoadVehicleAccessories();
    void LoadVehicleSeatAddon();

    void LoadGossipText();

    void LoadAreaTriggers();
    void LoadAreaTriggerTeleports();
    void LoadAccessRequirements();
    void LoadQuestAreaTriggers();
    void LoadQuestGreetings();
    void LoadAreaTriggerScripts();
    void LoadTavernAreaTriggers();
    void LoadGameObjectForQuests() const;

    void LoadPageTexts();
    const PageText* GetPageText(uint32 pageEntry);

    void LoadPlayerInfo();
    void LoadPetLevelInfo();
    void LoadExplorationBaseXP();
    void LoadPetNames();
    void LoadPetNumber();
    void LoadFishingBaseSkillLevel();
    void ChangeFishingBaseSkillLevel(uint32 entry, int32 skill);

    void LoadReputationRewardRate();
    void LoadReputationOnKill();
    void LoadReputationSpilloverTemplate();

    void LoadPointsOfInterest();
    void LoadQuestPOI();

    void LoadNPCSpellClickSpells();

    void LoadGameTele();

    void LoadGossipMenu();
    void LoadGossipMenuItems();

    void LoadVendors();
    void LoadTrainers();
    void LoadCreatureDefaultTrainers();

    std::string GeneratePetName(uint32 entry);
    uint32 GetBaseXP(uint8 level);
    [[nodiscard]] uint32 GetXPForLevel(uint8 level) const;

    [[nodiscard]] int32 GetFishingBaseSkillLevel(const uint32 entry) const
    {
        const auto itr = _fishingBaseForAreaStore.find(entry);
        return itr != _fishingBaseForAreaStore.end() ? itr->second : 0;
    }

    static void ReturnOrDeleteOldMails(bool serverUp);

    const CreatureBaseStats* GetCreatureBaseStats(uint8 level, uint8 unitClass);

    void SetHighestGUIDs();

    template<HighGuid type>
    ObjectGuidGeneratorBase& GetGenerator()
    {
        static_assert(ObjectGuidTraits<type>::Global, "Only global guid can be generated in ObjectMgr context");
        return GetGuidSequenceGenerator<type>();
    }

    uint32 GenerateAuctionID();
    uint64 GenerateEquipmentSetGUID();
    uint32 GenerateMailID();
    uint32 GeneratePetNumber();
    ObjectGuid::LowType GenerateCreatureSpawnID();
    ObjectGuid::LowType GenerateGameObjectSpawnID();

    typedef std::multimap<int32, uint32> ExclusiveQuestGroups;
    typedef std::pair<ExclusiveQuestGroups::const_iterator, ExclusiveQuestGroups::const_iterator> ExclusiveQuestGroupsBounds;

    ExclusiveQuestGroups mExclusiveQuestGroups;

    typedef std::unordered_map<uint32, std::vector<uint32>> BreadcrumbQuestMap;
    BreadcrumbQuestMap _breadcrumbsForQuest;

    [[nodiscard]] const std::vector<uint32>* GetBreadcrumbsForQuest(const uint32 questID) const
    {
        const auto itr = _breadcrumbsForQuest.find(questID);
        if (itr != _breadcrumbsForQuest.end())
            return &itr->second;

        return nullptr;
    }

    const MailLevelReward* GetMailLevelReward(const uint32 level, const uint32 raceMask)
    {
        const auto mapItr = _mailLevelRewardStore.find(level);
        if (mapItr == _mailLevelRewardStore.end())
            return nullptr;

        for (const auto & set_itr : mapItr->second)
            if (set_itr.raceMask & raceMask)
                return &set_itr;

        return nullptr;
    }

    const CellObjectGuids& GetGridObjectGuids(const uint16 mapID, const uint8 spawnMode, const uint32 gridID)
    {
        if (const auto itr1 = _mapObjectGuidsStore.find(MAKE_PAIR32(mapID, spawnMode)); itr1 != _mapObjectGuidsStore.end())
            if (const auto itr2 = itr1->second.find(gridID); itr2 != itr1->second.end())
                return itr2->second;
        return _emptyCellObjectGuids;
    }

    const CellObjectGuidsMap& GetMapObjectGuids(const uint16 mapID, const uint8 spawnMode)
    {
        if (const auto itr1 = _mapObjectGuidsStore.find(MAKE_PAIR32(mapID, spawnMode)); itr1 != _mapObjectGuidsStore.end())
            return itr1->second;
        return _emptyCellObjectGuidsMap;
    }

    /**
     * Gets temp summon data for all creatures of specified group.
     *
     * @param summonerID   Summoner's entry.
     * @param summonerType Summoner's type, see SummonerType for available types.
     * @param group        ID of required group.
     *
     * @return null if group was not found, otherwise reference to the creature group data
     */
    [[nodiscard]] const std::vector<TempSummonData>* GetSummonGroup(const uint32 summonerID, const SummonerType summonerType, const uint8 group) const
    {
        const auto itr = _tempSummonDataStore.find(TempSummonGroupKey(summonerID, summonerType, group));
        if (itr != _tempSummonDataStore.end())
            return &itr->second;
        return nullptr;
    }

    [[nodiscard]] const std::vector<GameObjectSummonData>* GetGameObjectSummonGroup(const uint32 summonerID, const SummonerType summonerType, const uint8 group) const
    {
        const auto itr = _goSummonDataStore.find(TempSummonGroupKey(summonerID, summonerType, group));
        if (itr != _goSummonDataStore.end())
            return &itr->second;
        return nullptr;
    }

    [[nodiscard]] const BroadcastText* GetBroadcastText(const uint32 id) const
    {
        const auto itr = _broadcastTextStore.find(id);
        if (itr != _broadcastTextStore.end())
            return &itr->second;
        return nullptr;
    }
    [[nodiscard]] const CreatureDataContainer& GetAllCreatureData() const { return _creatureDataStore; }
    [[nodiscard]] const CreatureData* GetCreatureData(const ObjectGuid::LowType spawnID) const
    {
        const auto itr = _creatureDataStore.find(spawnID);
        if (itr == _creatureDataStore.end())
            return nullptr;
        return &itr->second;
    }

    [[nodiscard]] const CreatureSparringContainer& GetSparringData() const { return _creatureSparringStore; }

    CreatureData& NewOrExistCreatureData(const ObjectGuid::LowType spawnID) { return _creatureDataStore[spawnID]; }
    /**
     * @brief Loads a single creature spawn entry from the database into the data store cache.
     *
     * This is needed as a prerequisite for Creature::LoadCreatureFromDB(), which reads
     * from the in-memory cache (via GetCreatureData()) rather than querying the DB itself.
     * For spawns not loaded during server startup, this method populates the cache so that
     * Creature::LoadCreatureFromDB() can then create the live entity.
     *
     * Returns the cached data if already loaded, or nullptr if the spawn doesn't exist
     * or fails validation.
     *
     * @param spawnID The creature spawn GUID to load.
     * @return Pointer to the cached CreatureData, or nullptr on failure.
     */
    const CreatureData* LoadCreatureDataFromDB(ObjectGuid::LowType spawnID);
    void DeleteCreatureData(ObjectGuid::LowType guid);
    [[nodiscard]] ObjectGuid GetLinkedRespawnGuid(const ObjectGuid guid) const
    {
        const auto itr = _linkedRespawnStore.find(guid);
        if (itr == _linkedRespawnStore.end())
            return ObjectGuid::Empty;
        return itr->second;
    }

    [[nodiscard]] const GameObjectDataContainer& GetAllGOData() const { return _gameObjectDataStore; }
    [[nodiscard]] const GameObjectData* GetGameObjectData(const ObjectGuid::LowType spawnID) const
    {
        const auto itr = _gameObjectDataStore.find(spawnID);
        if (itr == _gameObjectDataStore.end()) return nullptr;
            return &itr->second;
    }
    [[nodiscard]] const SpawnData* GetSpawnData(SpawnObjectType type, ObjectGuid::LowType spawnID) const;

    [[nodiscard]] const SpawnGroupTemplateData* GetSpawnGroupData(const uint32 groupID) const
    {
        const auto itr = _spawnGroupDataStore.find(groupID);
        return itr != _spawnGroupDataStore.end() ? &itr->second : nullptr;
    }
    [[nodiscard]] const QuestGreeting* GetQuestGreeting(TypeID type, uint32 id) const;
    [[nodiscard]] SpawnGroupTemplateData const* GetDefaultSpawnGroup() const { return &_spawnGroupDataStore.at(0); }
    [[nodiscard]] SpawnGroupTemplateData const* GetLegacySpawnGroup() const { return &_spawnGroupDataStore.at(1); }
    std::pair<SpawnGroupLinkContainer::const_iterator, SpawnGroupLinkContainer::const_iterator> GetSpawnDataForGroup(uint32 groupId) const
    {
        return _spawnGroupMapStore.equal_range(groupId);
    }
    void OnDeleteSpawnData(SpawnData const* data);

    GameObjectData& NewGOData(const ObjectGuid::LowType guid) { return _gameObjectDataStore[guid]; }
    /**
     * @brief Loads a single GameObject spawn entry from the database into the data store cache.
     *
     * This is needed as a prerequisite for GameObject::LoadGameObjectFromDB(), which reads
     * from the in-memory cache (via GetGameObjectData()) rather than querying the DB itself.
     * For spawns not loaded during server startup, this method populates the cache so that
     * GameObject::LoadGameObjectFromDB() can then create the live entity.
     *
     * Returns the cached data if already loaded, or nullptr if the spawn doesn't exist
     * or fails validation.
     *
     * @param spawnID The GameObject spawn GUID to load.
     * @return Pointer to the cached GameObjectData, or nullptr on failure.
     */
    const GameObjectData* LoadGameObjectDataFromDB(ObjectGuid::LowType spawnID);
    void DeleteGOData(ObjectGuid::LowType guid);

    [[nodiscard]] std::string GetNcoreString(const uint32 entry) const
    {
        const auto itr = _acoreStringStore.find(entry);
        if (itr == _acoreStringStore.end())
            return "";
        return itr->second;
    }
    [[nodiscard]] LocaleConstant GetDBCLocaleIndex() const { return DBCLocaleIndex; }
    void SetDBCLocaleIndex(const LocaleConstant locale) { DBCLocaleIndex = locale; }

    // grid objects
    void AddCreatureToGrid(ObjectGuid::LowType guid, const CreatureData* data);
    void RemoveCreatureFromGrid(ObjectGuid::LowType guid, const CreatureData* data);
    void AddGameObjectToGrid(ObjectGuid::LowType guid, const GameObjectData* data);
    void RemoveGameObjectFromGrid(ObjectGuid::LowType guid, const GameObjectData* data);
    ObjectGuid::LowType AddGOData(uint32 entry, uint32 mapID, float x, float y, float z, float o, uint32 spawnTimeDelay = 0, float rotation0 = 0, float rotation1 = 0, float rotation2 = 0, float rotation3 = 0);
    ObjectGuid::LowType AddCreData(uint32 entry, uint32 mapID, float x, float y, float z, float o, uint32 spawnTimeDelay = 0);

    // reserved names
    void LoadReservedPlayerNamesDB();
    void LoadReservedPlayerNamesDBC();
    [[nodiscard]] bool IsReservedName(std::string_view name) const;
    void AddReservedPlayerName(const std::string& name);

    // profanity names
    void LoadProfanityNamesFromDB();
    void LoadProfanityNamesFromDBC();
    [[nodiscard]] bool IsProfanityName(std::string_view name) const;
    void AddProfanityPlayerName(const std::string& name);

    // chat filter (substring chat content filter)
    void LoadChatFilter();
    [[nodiscard]] bool IsChatFiltered(std::string_view text) const;

    // name with valid structure and symbols
    static uint8 CheckPlayerName(std::string_view name, bool create = false);
    static PetNameInvalidReason CheckPetName(std::string_view name);
    static bool IsValidCharterName(std::string_view name);
    static bool IsValidChannelName(const std::string& name);

    static bool CheckDeclinedNames(const std::wstring& wOwnName, const DeclinedName& names);

    [[nodiscard]] const GameTele* GetGameTele(uint32 id) const
    {
        const auto itr = _gameTeleStore.find(id);
        if (itr == _gameTeleStore.end()) return nullptr;
        return &itr->second;
    }
    [[nodiscard]] const GameTele* GetGameTele(std::string_view name, bool exactSearch = false) const;
    [[nodiscard]] const GameTeleContainer& GetGameTeleMap() const { return _gameTeleStore; }
    bool AddGameTele(GameTele& tele);
    bool DeleteGameTele(std::string_view name);

    Trainer::Trainer* GetTrainer(uint32 creatureId);
    const std::vector<const Trainer::Trainer*>& GetClassTrainers(const uint8 classID) const { return _classTrainers.at(classID); }

    [[nodiscard]] const VendorItemData* GetNpcVendorItemList(const uint32 entry) const
    {
        const auto iter = _cacheVendorItemStore.find(entry);
        if (iter == _cacheVendorItemStore.end())
            return nullptr;
        return &iter->second;
    }

    void AddVendorItem(uint32 entry, uint32 item, uint32 maxCount, uint32 incrTime, uint32 extendedCost, bool persist = true); // for event
    bool RemoveVendorItem(uint32 entry, uint32 item, bool persist = true); // for event
    bool IsVendorItemValid(uint32 vendorEntry, uint32 itemID, uint32 maxCount, uint32 incrTime, uint32 extendedCost,
        const Player* player = nullptr, std::set<uint32>* skip_vendors = nullptr, uint32 npcFlag = 0) const;

    void LoadScriptNames();
    ScriptNameContainer& GetScriptNames() { return _scriptNamesStore; }
    [[nodiscard]] const std::string& GetScriptName(uint32 id) const;
    uint32 GetScriptID(const std::string& name);

    [[nodiscard]] SpellClickInfoMapBounds GetSpellClickInfoMapBounds(const uint32 creatureID) const
    {
        return _spellClickInfoStore.equal_range(creatureID);
    }

    [[nodiscard]] GossipMenusMapBounds GetGossipMenusMapBounds(const uint32 uiMenuID) const
    {
        return _gossipMenusStore.equal_range(uiMenuID);
    }

    GossipMenusMapBoundsNonConst GetGossipMenusMapBoundsNonConst(const uint32 uiMenuID)
    {
        return _gossipMenusStore.equal_range(uiMenuID);
    }

    [[nodiscard]] GossipMenuItemsMapBounds GetGossipMenuItemsMapBounds(const uint32 uiMenuID) const
    {
        return _gossipMenuItemsStore.equal_range(uiMenuID);
    }
    GossipMenuItemsMapBoundsNonConst GetGossipMenuItemsMapBoundsNonConst(const uint32 uiMenuID)
    {
        return _gossipMenuItemsStore.equal_range(uiMenuID);
    }

    CharacterConversionMap FactionChangeAchievements;
    CharacterConversionMap FactionChangeItems;
    CharacterConversionMap FactionChangeQuests;
    CharacterConversionMap FactionChangeReputation;
    CharacterConversionMap FactionChangeSpells;
    CharacterConversionMap FactionChangeTitles;

    void LoadFactionChangeAchievements();
    void LoadFactionChangeItems();
    void LoadFactionChangeQuests();
    void LoadFactionChangeReputations();
    void LoadFactionChangeSpells();
    void LoadFactionChangeTitles();

    [[nodiscard]] bool IsTransportMap(const uint32 mapID) const { return _transportMaps.contains(mapID); }

    const VehicleSeatAddon* GetVehicleSeatAddon(const uint32 seatID) const
    {
        const auto itr = _vehicleSeatAddonStore.find(seatID);
        if (itr == _vehicleSeatAddonStore.end())
            return nullptr;
        return &itr->second;
    }

    [[nodiscard]] uint32 GetQuestMoneyReward(uint8 level, uint32 questMoneyDifficulty) const;
private:
    // First free id for selected id type
    uint32 _auctionID; // Accessed by a single thread
    uint64 _equipmentSetGUID; // Accessed by a single thread
    uint32 _mailID;
    std::mutex _mailIdMutex;
    uint32 _hiPetNumber;
    std::mutex _hiPetNumberMutex;

    ObjectGuid::LowType _creatureSpawnID;
    ObjectGuid::LowType _gameObjectSpawnID;

    // first free low guid for selected guid type
    template<HighGuid high>
    ObjectGuidGeneratorBase& GetGuidSequenceGenerator()
    {
        auto itr = _guidGenerators.find(high);
        if (itr == _guidGenerators.end())
            itr = _guidGenerators.insert(std::make_pair(high, std::unique_ptr<ObjectGuidGenerator<high>>(new ObjectGuidGenerator<high>()))).first;

        return *itr->second;
    }

    std::map<HighGuid, std::unique_ptr<ObjectGuidGeneratorBase>> _guidGenerators;

    QuestMap _questTemplates;
    std::vector<Quest*> _questTemplatesFast;

    typedef std::unordered_map<uint32, GossipText> GossipTextContainer;
    typedef std::unordered_map<uint32, uint32> QuestAreaTriggerContainer;
    typedef std::unordered_map<uint32, uint32> TavernAreaTriggerContainer;

    QuestAreaTriggerContainer _questAreaTriggerStore;
    TavernAreaTriggerContainer _tavernAreaTriggerStore;
    GossipTextContainer _gossipTextStore;
    QuestGreetingContainer _questGreetingStore;
    AreaTriggerContainer _areaTriggerStore;
    AreaTriggerTeleportContainer _areaTriggerTeleportStore;
    AreaTriggerScriptContainer _areaTriggerScriptStore;
    DungeonProgressionRequirementsContainer _accessRequirementStore;
    DungeonEncounterContainer _dungeonEncounterStore;

    RepRewardRateContainer _repRewardRateStore;
    RepOnKillContainer _repOnKillStore;
    RepSpilloverTemplateContainer _repSpilloverTemplateStore;

    GossipMenusContainer _gossipMenusStore;
    GossipMenuItemsContainer _gossipMenuItemsStore;
    PointOfInterestContainer _pointsOfInterestStore;

    QuestPOIContainer _questPOIStore;

    QuestRelations _goQuestRelations;
    QuestRelations _goQuestInvolvedRelations;
    QuestRelations _creatureQuestRelations;
    QuestRelations _creatureQuestInvolvedRelations;

    //character reserved names
    typedef std::set<std::wstring> ReservedNamesContainer;
    ReservedNamesContainer _reservedNamesStore;

    //character profanity names
    typedef std::set<std::wstring> ProfanityNamesContainer;
    ProfanityNamesContainer _profanityNamesStore;

    //chat filter (Aho-Corasick automaton; matches any banned word as substring of input)
    std::unique_ptr<Acore::AhoCorasick<wchar_t>> _chatFilterAutomaton;

    GameTeleContainer _gameTeleStore;
    ScriptNameContainer _scriptNamesStore;
    SpellClickInfoContainer _spellClickInfoStore;
    SpellScriptsContainer _spellScriptsStore;
    VehicleAccessoryContainer _vehicleTemplateAccessoryStore;
    VehicleAccessoryContainer _vehicleAccessoryStore;
    VehicleSeatAddonContainer _vehicleSeatAddonStore;

    LocaleConstant DBCLocaleIndex;

    PageTextContainer _pageTextStore;
    InstanceTemplateContainer _instanceTemplateStore;

    CreatureSparringContainer _creatureSparringStore;

    void LoadScripts(ScriptsType type);
    void LoadQuestRelationsHelper(QuestRelations& map, const std::string& table, bool starter, bool go) const;
    void PlayerCreateInfoAddItemHelper(uint32 race_, uint32 class_, uint32 itemID, int32 count) const;

    void LoadPlayerCreateInfo();
    void LoadPlayerCreateInfoItems() const;
    void LoadPlayerCreateInfoSkills() const;
    void LoadPlayerCreateInfoSpells() const;
    void LoadPlayerCreateInfoCastSpell() const;
    void LoadPlayerCreateInfoActions() const;
    void LoadPlayerCreateInfoStats();
    void LoadPlayerCreateInfoXP();

    void LoadQuestDetails();
    void LoadQuestRequestItems();
    void LoadQuestOfferRewards();
    void LoadQuestTemplateAddons();

    MailLevelRewardContainer _mailLevelRewardStore;

    CreatureBaseStatsContainer _creatureBaseStatsStore;

    typedef std::map<uint32, PetLevelInfo*> PetLevelInfoContainer;
    // PetLevelInfoContainer[creature_id][level]
    PetLevelInfoContainer _petInfoStore;                            // [creature_id][level]

    PlayerClassInfo* _playerClassInfo[MAX_CLASSES]{};

    void BuildPlayerLevelInfo(uint8 race, uint8 class_, uint8 level, PlayerLevelInfo* plInfo) const;

    std::vector<std::vector<PlayerInfo*>> _playerInfo;

    typedef std::vector<uint32> PlayerXPPerLevel;  // [level]
    PlayerXPPerLevel _playerXPPerLevel;

    typedef std::map<uint32, uint32> BaseXPContainer;  // [area level][base xp]
    BaseXPContainer _baseXPTable;

    typedef std::map<uint32, int32> FishingBaseSkillContainer; // [areaID][base skill level]
    FishingBaseSkillContainer _fishingBaseForAreaStore;

    typedef std::map<uint32, std::vector<std::string>> HalfNameContainer;
    HalfNameContainer _petHalfName0;
    HalfNameContainer _petHalfName1;

    typedef std::unordered_map<uint32, ItemSetNameEntry> ItemSetNameContainer;
    ItemSetNameContainer _itemSetNameStore;

    MapObjectGuids _mapObjectGuidsStore;
    CellObjectGuidsMap _emptyCellObjectGuidsMap;
    CellObjectGuids _emptyCellObjectGuids;
    CreatureDataContainer _creatureDataStore;
    CreatureTemplateContainer _creatureTemplateStore;
    CreatureCustomIDsContainer _creatureCustomIDsStore;
    std::vector<CreatureTemplate*> _creatureTemplateStoreFast;
    CreatureModelContainer _creatureModelStore;
    CreatureAddonContainer _creatureAddonStore;
    CreatureAddonContainer _creatureTemplateAddonStore;
    std::unordered_map<ObjectGuid::LowType, CreatureMovementData> _creatureMovementOverrides;
    GameObjectAddonContainer _gameObjectAddonStore;
    GameObjectQuestItemMap _gameObjectQuestItemStore;
    CreatureQuestItemMap _creatureQuestItemStore;
    EquipmentInfoContainer _equipmentInfoStore;
    LinkedRespawnContainer _linkedRespawnStore;
    GameObjectDataContainer _gameObjectDataStore;
    SpawnGroupDataContainer _spawnGroupDataStore;
    SpawnGroupLinkContainer _spawnGroupMapStore;
    GameObjectTemplateContainer _gameObjectTemplateStore;
    GameObjectTemplateAddonContainer _gameObjectTemplateAddonStore;
    /// Stores temp summon data grouped by summoner's entry, summoner's type and group id
    TempSummonDataContainer _tempSummonDataStore;
    /// Stores GameObject summon data grouped by summoner's entry, summoner's type and group id
    GameObjectSummonDataContainer _goSummonDataStore;

    BroadcastTextContainer _broadcastTextStore;
    ItemTemplateContainer _itemTemplateStore;
    std::vector<ItemTemplate*> _itemTemplateStoreFast;
    NcoreStringContainer _acoreStringStore;

    CacheVendorItemContainer _cacheVendorItemStore;
    std::unordered_map<uint32, Trainer::Trainer> _trainers;
    std::unordered_map<uint8, std::vector<const Trainer::Trainer*>> _classTrainers;
    std::unordered_map<uint32, uint32> _creatureDefaultTrainers;

    std::set<uint32> _difficultyEntries[MAX_DIFFICULTY - 1]; // already loaded difficulty 1 value in creatures, used in CheckCreatureTemplate
    std::set<uint32> _hasDifficultyEntries[MAX_DIFFICULTY - 1]; // already loaded creatures with difficulty 1 values, used in CheckCreatureTemplate

    enum CreatureLinkedRespawnType
    {
        CREATURE_TO_CREATURE,
        CREATURE_TO_GO,         // Creature is dependent on GO
        GO_TO_GO,
        GO_TO_CREATURE,         // GO is dependent on creature
    };

    std::set<uint32> _transportMaps; // Helper container storing map ids that are for transports only, loaded from world_game_object_template

    PlayerTotemModelMap _playerTotemModel;

    PlayerShapeshiftModelMap _playerShapeshiftModel;

    QuestMoneyRewardStore _questMoneyRewards;
};

#define sObjectMgr ObjectMgr::instance()

#endif
