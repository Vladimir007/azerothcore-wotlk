#include "ObjectMgr.h"

#include <numeric>
#include <ranges>
#include <set>
#include <boost/algorithm/string.hpp>

#include "AchievementMgr.h"
#include "ArenaTeamMgr.h"
#include "CharacterCache.h"
#include "Chat.h"
#include "Common.h"
#include "Config.h"
#include "Containers.h"
#include "CreatureAIFactory.h"
#include "DatabaseEnv.h"
#include "DBCStructure.h"
#include "DisableMgr.h"
#include "GameEventMgr.h"
#include "GameObjectAIFactory.h"
#include "GameTime.h"
#include "GossipDef.h"
#include "GuildMgr.h"
#include "ItemEnchantmentMgr.h"
#include "LFGMgr.h"
#include "Log.h"
#include "MapMgr.h"
#include "Pet.h"
#include "PoolMgr.h"
#include "RaceMgr.h"
#include "Realm.h"
#include "ReputationMgr.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "StringConvert.h"
#include "Tokenize.h"
#include "Transport.h"
#include "Unit.h"
#include "Util.h"
#include "Vehicle.h"
#include "World.h"

ScriptMapMap sSpellScripts;
ScriptMapMap sEventScripts;
ScriptMapMap sWaypointScripts;

ScriptMapMap* GetScriptsMapByType(const ScriptsType type)
{
    ScriptMapMap* res = nullptr;
    switch (type)
    {
        case SCRIPTS_SPELL:
            res = &sSpellScripts;
            break;
        case SCRIPTS_EVENT:
            res = &sEventScripts;
            break;
        case SCRIPTS_WAYPOINT:
            res = &sWaypointScripts;
            break;
        default:
            break;
    }
    return res;
}

std::string GetScriptCommandName(ScriptCommands command)
{
    std::string res;
    switch (command)
    {
        case SCRIPT_COMMAND_TALK:
            res = "SCRIPT_COMMAND_TALK";
            break;
        case SCRIPT_COMMAND_EMOTE:
            res = "SCRIPT_COMMAND_EMOTE";
            break;
        case SCRIPT_COMMAND_FIELD_SET:
            res = "SCRIPT_COMMAND_FIELD_SET";
            break;
        case SCRIPT_COMMAND_MOVE_TO:
            res = "SCRIPT_COMMAND_MOVE_TO";
            break;
        case SCRIPT_COMMAND_FLAG_SET:
            res = "SCRIPT_COMMAND_FLAG_SET";
            break;
        case SCRIPT_COMMAND_FLAG_REMOVE:
            res = "SCRIPT_COMMAND_FLAG_REMOVE";
            break;
        case SCRIPT_COMMAND_TELEPORT_TO:
            res = "SCRIPT_COMMAND_TELEPORT_TO";
            break;
        case SCRIPT_COMMAND_QUEST_EXPLORED:
            res = "SCRIPT_COMMAND_QUEST_EXPLORED";
            break;
        case SCRIPT_COMMAND_KILL_CREDIT:
            res = "SCRIPT_COMMAND_KILL_CREDIT";
            break;
        case SCRIPT_COMMAND_RESPAWN_GAME_OBJECT:
            res = "SCRIPT_COMMAND_RESPAWN_GAME_OBJECT";
            break;
        case SCRIPT_COMMAND_TEMP_SUMMON_CREATURE:
            res = "SCRIPT_COMMAND_TEMP_SUMMON_CREATURE";
            break;
        case SCRIPT_COMMAND_OPEN_DOOR:
            res = "SCRIPT_COMMAND_OPEN_DOOR";
            break;
        case SCRIPT_COMMAND_CLOSE_DOOR:
            res = "SCRIPT_COMMAND_CLOSE_DOOR";
            break;
        case SCRIPT_COMMAND_ACTIVATE_OBJECT:
            res = "SCRIPT_COMMAND_ACTIVATE_OBJECT";
            break;
        case SCRIPT_COMMAND_REMOVE_AURA:
            res = "SCRIPT_COMMAND_REMOVE_AURA";
            break;
        case SCRIPT_COMMAND_CAST_SPELL:
            res = "SCRIPT_COMMAND_CAST_SPELL";
            break;
        case SCRIPT_COMMAND_PLAY_SOUND:
            res = "SCRIPT_COMMAND_PLAY_SOUND";
            break;
        case SCRIPT_COMMAND_CREATE_ITEM:
            res = "SCRIPT_COMMAND_CREATE_ITEM";
            break;
        case SCRIPT_COMMAND_DESPAWN_SELF:
            res = "SCRIPT_COMMAND_DESPAWN_SELF";
            break;
        case SCRIPT_COMMAND_LOAD_PATH:
            res = "SCRIPT_COMMAND_LOAD_PATH";
            break;
        case SCRIPT_COMMAND_CALLSCRIPT_TO_UNIT:
            res = "SCRIPT_COMMAND_CALLSCRIPT_TO_UNIT";
            break;
        case SCRIPT_COMMAND_KILL:
            res = "SCRIPT_COMMAND_KILL";
            break;
        // NordCore only
        case SCRIPT_COMMAND_ORIENTATION:
            res = "SCRIPT_COMMAND_ORIENTATION";
            break;
        case SCRIPT_COMMAND_EQUIP:
            res = "SCRIPT_COMMAND_EQUIP";
            break;
        case SCRIPT_COMMAND_MODEL:
            res = "SCRIPT_COMMAND_MODEL";
            break;
        case SCRIPT_COMMAND_CLOSE_GOSSIP:
            res = "SCRIPT_COMMAND_CLOSE_GOSSIP";
            break;
        case SCRIPT_COMMAND_PLAY_MOVIE:
            res = "SCRIPT_COMMAND_PLAY_MOVIE";
            break;
        default:
            {
                char sz[32];
                snprintf(sz, sizeof(sz), "Unknown command: %d", command);
                res = sz;
                break;
            }
    }
    return res;
}

std::string ScriptInfo::GetDebugInfo() const
{
    char sz[256];
    std::string tableName;
    switch (type)
    {
    case SCRIPTS_SPELL:
        tableName = "world_spell_script";
        break;
    case SCRIPTS_EVENT:
        tableName = "world_event_script";
        break;
    case SCRIPTS_WAYPOINT:
        tableName = "world_waypoint_script";
        break;
    default:
        break;
    }
    snprintf(sz, sizeof(sz), "%s ('%s' script id: %u)", GetScriptCommandName(command).c_str(), tableName.c_str(), id);
    return {sz};
}

bool normalizePlayerName(std::string& name)
{
    if (name.empty())
        return false;

    if (name.find(' ') != std::string::npos)
        return false;

    std::wstring tmp;
    if (!Utf8toWStr(name, tmp))
        return false;

    wstrToLower(tmp);
    if (!tmp.empty())
        tmp[0] = wcharToUpper(tmp[0]);

    if (!WStrToUtf8(tmp, name))
        return false;

    return true;
}

LanguageDesc lang_description[LANGUAGES_COUNT] =
{
    { LANG_ADDON,           0, 0                       },
    { LANG_UNIVERSAL,       0, 0                       },
    { LANG_ORCISH,        669, SKILL_LANG_ORCISH       },
    { LANG_DARNASSIAN,    671, SKILL_LANG_DARNASSIAN   },
    { LANG_TAURAHE,       670, SKILL_LANG_TAURAHE      },
    { LANG_DWARVISH,      672, SKILL_LANG_DWARVEN      },
    { LANG_COMMON,        668, SKILL_LANG_COMMON       },
    { LANG_DEMONIC,       815, SKILL_LANG_DEMON_TONGUE },
    { LANG_TITAN,         816, SKILL_LANG_TITAN        },
    { LANG_THALASSIAN,    813, SKILL_LANG_THALASSIAN   },
    { LANG_DRACONIC,      814, SKILL_LANG_DRACONIC     },
    { LANG_KALIMAG,       817, SKILL_LANG_OLD_TONGUE   },
    { LANG_GNOMISH,      7340, SKILL_LANG_GNOMISH      },
    { LANG_TROLL,        7341, SKILL_LANG_TROLL        },
    { LANG_GUTTERSPEAK, 17737, SKILL_LANG_GUTTERSPEAK  },
    { LANG_DRAENEI,     29932, SKILL_LANG_DRAENEI      },
    { LANG_ZOMBIE,          0, 0                       },
    { LANG_GNOMISH_BINARY,  0, 0                       },
    { LANG_GOBLIN_BINARY,   0, 0                       }
};

const LanguageDesc* GetLanguageDescByID(const uint32 lang)
{
    for (const auto & i : lang_description)
    {
        if (static_cast<uint32>(i.langID) == lang)
            return &i;
    }
    return nullptr;
}

bool SpellClickInfo::IsFitToRequirements(const Unit* clicker, const Unit* activator) const
{
    const Player* playerClicker = clicker->ToPlayer();
    if (!playerClicker)
        return true;

    const Unit* summoner = nullptr;
    // Check summoners for party
    if (activator->IsSummon())
        summoner = activator->ToTempSummon()->GetSummonerUnit();
    if (!summoner)
        summoner = activator;

    // This only applies to players
    switch (userType)
    {
        case SPELL_CLICK_USER_FRIEND:
            if (!playerClicker->IsFriendlyTo(summoner))
                return false;
            break;
        case SPELL_CLICK_USER_RAID:
            if (!playerClicker->IsInRaidWith(summoner))
                return false;
            break;
        case SPELL_CLICK_USER_PARTY:
            if (!playerClicker->IsInPartyWith(summoner))
                return false;
            break;
        default:
            break;
    }

    return true;
}

ObjectMgr::ObjectMgr():
    _auctionID(1),
    _equipmentSetGUID(1),
    _mailID(1),
    _hiPetNumber(1),
    _creatureSpawnID(1),
    _gameObjectSpawnID(1),
    DBCLocaleIndex(LOCALE_enUS)
{
    for (auto & i : _playerClassInfo)
        i = nullptr;

    // Initialize default spawn group
    _spawnGroupDataStore[0] = {0, "Default Group", 0, SPAWNGROUP_FLAG_SYSTEM};
}

ObjectMgr::~ObjectMgr()
{
    for (const auto &tmpl: _questTemplates | std::views::values)
        delete tmpl;

    for (const auto &petInfo: _petInfoStore | std::views::values)
        delete[] petInfo;

    // Free only if loaded
    for (const auto & class_ : _playerClassInfo)
    {
        if (class_)
            delete[] class_->levelInfo;
        delete class_;
    }

    for (int race = 0; race < sRaceMgr->GetMaxRaces(); ++race)
    {
        for (int class_ = 0; class_ < MAX_CLASSES; ++class_)
        {
            if (_playerInfo[race][class_])
                delete[] _playerInfo[race][class_]->levelInfo;
            delete _playerInfo[race][class_];
        }
    }

    for (auto &item: _cacheVendorItemStore | std::views::values)
        item.Clear();

    for (auto &encounters: _dungeonEncounterStore | std::views::values)
        for (const auto & encounter : encounters)
            delete encounter;

    for (const auto &req: _accessRequirementStore | std::views::values)
    {
        for (const auto &diff: req | std::views::values)
        {
            for (const auto & quest : diff->quests)
                delete quest;

            for (const auto & achievement : diff->achievements)
                delete achievement;

            for (const auto & item : diff->items)
                delete item;

            delete diff;
        }
    }
}

ObjectMgr* ObjectMgr::instance()
{
    static ObjectMgr instance;
    return &instance;
}

void ObjectMgr::LoadCreatureTemplates()
{
    const uint32 oldMSTime = getMSTime();

    const QueryResult result = WorldDatabase.Query(
        "SELECT id, difficulty, kill_credit, name, title, icon_name, gossip_menu, min_level, max_level, expansion, "
        "faction, npc_flag, flags_extra, speed_walk, speed_run, speed_swim, speed_flight, detection_range, rank, "
        "damage_school, damage_modifier, base_attack_time, range_attack_time, base_variance, range_variance, "
        "unit_class, unit_flags, unit_flags2, dynamic_flags, family, type, type_flags, loot, "
        "pickpocket_loot, skin_loot, pet_spell_data, vehicle, gold_min, gold_max, name_ai, movement_type, movement, "
        "ctm.ground, ctm.swim, ctm.flight, ctm.rooted, ctm.chase, ctm.random, ctm.interaction_pause_timer, "
        "hover_height, health_modifier, mana_modifier, armor_modifier, experience_modifier, racial_leader, regen_health, immunity, script_name "
        "FROM world_creature_template ct LEFT JOIN world_creature_template_movement ctm ON ct.id = ctm.creature ORDER BY id DESC");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creature template definitions. DB table `world_creature_template` is empty.");
        return;
    }

    _creatureTemplateStore.rehash(result->GetRowCount());
    _creatureTemplateStoreFast.clear();

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();
        LoadCreatureTemplate(fields);
        ++count;
    } while (result->NextRow());

    // We load the creature models after loading but before checking
    LoadCreatureTemplateModels();

    sScriptMgr->OnAfterDatabaseLoadCreatureTemplates(_creatureTemplateStoreFast);

    LoadCreatureTemplateResistances();
    LoadCreatureTemplateSpells();

    // Checking needs to be done after loading because of the difficulty self referencing
    for (auto &creatureTemplate: _creatureTemplateStore | std::views::values)
    {
        CheckCreatureTemplate(&creatureTemplate);
        creatureTemplate.InitializeQueryData();
    }

    LOG_INFO("server.loading", ">> Loaded {} Creature Definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadCreatureTemplate(const Field* fields, const bool triggerHook)
{
    /*
        id(0), difficulty, kill_credit, name, title, icon_name, gossip_menu, min_level, max_level, expansion, faction,
        npc_flag(11), flags_extra, speed_walk, speed_run, speed_swim, speed_flight, detection_range, rank,
        damage_school(19), damage_modifier, base_attack_time, range_attack_time, base_variance, range_variance,
        unit_class(25), unit_flags, unit_flags2, dynamic_flags, family, type, type_flags, loot,
        pickpocket_loot(33), skin_loot, pet_spell_data, vehicle, gold_min, gold_max, name_ai, movement_type, movement,
        ctm.ground(42), ctm.swim, ctm.flight, ctm.rooted, ctm.chase, ctm.random, ctm.interaction_pause_timer,
        hover_height(49), health_modifier, mana_modifier, armor_modifier, experience_modifier, racial_leader,
        regen_health(55), immunity, script_name
     */

    uint32 entry = fields[0].Get<uint32>();

    CreatureTemplate& creatureTemplate = _creatureTemplateStore[entry];

    // Enlarge the fast cache as necessary
    if (_creatureTemplateStoreFast.size() < entry + 1)
        _creatureTemplateStoreFast.resize(entry + 1, nullptr);

    // Load a pointer to this creatureTemplate into the fast cache
    _creatureTemplateStoreFast[entry] = &creatureTemplate;

    // Build the creatureTemplate
    creatureTemplate.Entry = entry;

    const auto difficultyArr = fields[1].GetArray<uint32, MAX_DIFFICULTY - 1>();
    for (uint8 i = 0; i < MAX_DIFFICULTY - 1; ++i)
        creatureTemplate.DifficultyEntry[i] = difficultyArr[i];

    const auto killCreditArr = fields[2].GetArray<uint32, MAX_KILL_CREDIT>();
    for (uint8 i = 0; i < MAX_KILL_CREDIT; ++i)
        creatureTemplate.KillCredit[i] = killCreditArr[i];

    creatureTemplate.Name             = fields[3].Get<std::string>();
    creatureTemplate.SubName          = fields[4].Get<std::string>();
    creatureTemplate.IconName         = fields[5].Get<std::string>();
    creatureTemplate.GossipMenuId     = fields[6].Get<uint32>();
    creatureTemplate.LevelMin         = fields[7].Get<uint8>();
    creatureTemplate.MaxLevel         = fields[8].Get<uint8>();
    creatureTemplate.Expansion        = fields[9].Get<uint32>();
    creatureTemplate.Faction          = fields[10].Get<uint32>();
    creatureTemplate.FlagNPC          = fields[11].Get<uint32>();
    creatureTemplate.FlagsExtra       = fields[12].Get<uint32>();
    creatureTemplate.SpeedWalk        = fields[13].Get<float>();
    creatureTemplate.SpeedRun         = fields[14].Get<float>();
    creatureTemplate.SpeedSwim        = fields[15].Get<float>();
    creatureTemplate.SpeedFlight      = fields[16].Get<float>();
    creatureTemplate.DetectionRange   = fields[17].Get<float>();
    creatureTemplate.Rank             = fields[18].Get<uint32>();
    creatureTemplate.DamageSchool     = fields[19].Get<uint32>();
    creatureTemplate.DamageModifier   = fields[20].Get<float>();
    creatureTemplate.BaseAttackTime   = fields[21].Get<uint32>();
    creatureTemplate.RangeAttackTime  = fields[22].Get<uint32>();
    creatureTemplate.BaseVariance     = fields[23].Get<float>();
    creatureTemplate.RangeVariance    = fields[24].Get<float>();
    creatureTemplate.UnitClass        = fields[25].Get<uint32>();
    creatureTemplate.UnitFlags        = fields[26].Get<uint32>();
    creatureTemplate.UnitFlags2       = fields[27].Get<uint32>();
    creatureTemplate.DynamicFlags     = fields[28].Get<uint32>();
    creatureTemplate.Family           = fields[29].Get<uint32>();
    creatureTemplate.type             = fields[30].Get<uint32>();
    creatureTemplate.TypeFlags        = fields[31].Get<uint32>();
    creatureTemplate.LootID           = fields[32].Get<uint32>();
    creatureTemplate.PickpocketLootID = fields[33].Get<uint32>();
    creatureTemplate.SkinLootID       = fields[34].Get<uint32>();

    for (uint8 i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
        creatureTemplate.resistance[i] = 0;

    for (unsigned int & spell : creatureTemplate.spells)
        spell = 0;

    creatureTemplate.PetSpellDataId = fields[35].Get<uint32>();
    creatureTemplate.VehicleId      = fields[36].Get<uint32>();
    creatureTemplate.GoldMin        = fields[37].Get<uint32>();
    creatureTemplate.GoldMax        = fields[38].Get<uint32>();
    creatureTemplate.AIName         = fields[39].Get<std::string>();
    creatureTemplate.MovementType   = fields[40].Get<uint32>();
    creatureTemplate.MovementID     = fields[41].Get<uint32>();

    if (!fields[42].IsNull())
        creatureTemplate.Movement.Ground = static_cast<CreatureGroundMovementType>(fields[42].Get<uint8>());
    if (!fields[43].IsNull())
        creatureTemplate.Movement.Swim = static_cast<bool>(fields[43].Get<uint8>());
    if (!fields[44].IsNull())
        creatureTemplate.Movement.Flight = static_cast<CreatureFlightMovementType>(fields[44].Get<uint8>());
    if (!fields[45].IsNull())
        creatureTemplate.Movement.Rooted = static_cast<bool>(fields[45].Get<uint8>());
    if (!fields[46].IsNull())
        creatureTemplate.Movement.Chase = static_cast<CreatureChaseMovementType>(fields[46].Get<uint8>());
    if (!fields[47].IsNull())
        creatureTemplate.Movement.Random = static_cast<CreatureRandomMovementType>(fields[47].Get<uint8>());
    if (!fields[48].IsNull())
        creatureTemplate.Movement.InteractionPauseTimer = fields[48].Get<uint32>();

    creatureTemplate.HoverHeight           = fields[49].Get<float>();
    creatureTemplate.ModHealth             = fields[50].Get<float>();
    creatureTemplate.ModMana               = fields[51].Get<float>();
    creatureTemplate.ModArmor              = fields[52].Get<float>();
    creatureTemplate.ModExperience         = fields[53].Get<float>();
    creatureTemplate.RacialLeader          = fields[54].Get<bool>();
    creatureTemplate.RegenHealth           = fields[55].Get<bool>();
    creatureTemplate.CreatureImmunitiesID  = fields[56].Get<int32>();
    creatureTemplate.ScriptID              = GetScriptID(fields[57].Get<std::string>());

    // Warn about deprecated immunity flags that should be moved to `world_creature_immunity` table
    if (creatureTemplate.FlagsExtra & CREATURE_FLAG_EXTRA_NO_TAUNT)
        LOG_WARN("server.loading", "Creature (Entry: {}) has deprecated flags_extra bit NO_TAUNT (0x100) set. This will be migrated to the `world_creature_immunity` table in a future update.", entry);
    if (creatureTemplate.FlagsExtra & CREATURE_FLAG_EXTRA_AVOID_AOE)
        LOG_WARN("server.loading", "Creature (Entry: {}) has deprecated flags_extra bit AVOID_AOE (0x400000) set. This will be migrated to the `world_creature_immunity` table in a future update.", entry);
    if (creatureTemplate.FlagsExtra & CREATURE_FLAG_EXTRA_IMMUNITY_KNOCKBACK)
        LOG_WARN("server.loading", "Creature (Entry: {}) has deprecated flags_extra bit IMMUNITY_KNOCKBACK (0x40000000) set. This will be migrated to the `world_creature_immunity` table in a future update.", entry);

    // Useful if the creature template load is being triggered from outside this class
    if (triggerHook)
        sScriptMgr->OnAfterDatabaseLoadCreatureTemplates(_creatureTemplateStoreFast);
}

void ObjectMgr::LoadCreatureTemplateModels() const {
    const uint32 oldMSTime = getMSTime();
    const QueryResult result = WorldDatabase.Query("SELECT creature, display, scale, probability FROM world_creature_template_model ORDER BY index ASC");

    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 creature template model definitions. DB table `world_creature_template_model` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        uint32 creatureID = fields[0].Get<uint32>();
        uint32 creatureDisplayID = fields[1].Get<uint32>();
        float displayScale = fields[2].Get<float>();
        float probability = fields[3].Get<float>();

        const CreatureTemplate* cInfo = GetCreatureTemplate(creatureID);
        if (!cInfo)
        {
            LOG_ERROR("sql.sql", "Creature template (Entry: {}) does not exist but has a record in `world_creature_template_model`", creatureID);
            continue;
        }

        if (!sCreatureDisplayInfoStore.LookupEntry(creatureDisplayID))
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}) lists non-existing `display` ({}), this can crash the client.", creatureID, creatureDisplayID);
            continue;
        }

        if (!GetCreatureModelInfo(creatureDisplayID))
            LOG_ERROR("sql.sql", "No model data exist for `display` = {} listed by creature (Entry: {}).", creatureDisplayID, creatureID);

        if (displayScale <= 0.0f)
            displayScale = 1.0f;

        const_cast<CreatureTemplate*>(cInfo)->Models.emplace_back(creatureDisplayID, displayScale, probability);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} creature template models in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadCreatureTemplateResistances()
{
    const uint32 oldMSTime = getMSTime();
    const QueryResult result = WorldDatabase.Query("SELECT creature, school, resistance FROM world_creature_template_resistance");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creature template resistance definitions. DB table `world_creature_template_resistance` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();

        uint32 creatureID = fields[0].Get<uint32>();
        uint8 school = fields[1].Get<uint8>();

        if (school == SPELL_SCHOOL_NORMAL || school >= MAX_SPELL_SCHOOL)
        {
            LOG_ERROR("sql.sql", "world_creature_template_resistance has resistance definitions for creature {} but this school {} doesn't exist", creatureID, school);
            continue;
        }

        auto itr = _creatureTemplateStore.find(creatureID);
        if (itr == _creatureTemplateStore.end())
        {
            LOG_ERROR("sql.sql", "world_creature_template_resistance has resistance definitions for creature {} but this creature doesn't exist", creatureID);
            continue;
        }

        CreatureTemplate& creatureTemplate = itr->second;
        creatureTemplate.resistance[school] = fields[2].Get<int16>();

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Creature Template Resistances in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadCreatureTemplateSpells()
{
    const uint32 oldMSTime = getMSTime();
    const QueryResult result = WorldDatabase.Query("SELECT creature, index, spell FROM world_creature_template_spell");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creature template spell definitions. DB table `world_creature_template_spell` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();

        uint32 creatureID = fields[0].Get<uint32>();
        uint8 index = fields[1].Get<uint8>();

        if (index >= MAX_CREATURE_SPELLS)
        {
            LOG_ERROR("sql.sql", "world_creature_template_spell has spell definitions for creature {} with a incorrect index {}", creatureID, index);
            continue;
        }

        auto itr = _creatureTemplateStore.find(creatureID);
        if (itr == _creatureTemplateStore.end())
        {
            LOG_ERROR("sql.sql", "world_creature_template_spell has spell definitions for creature {} but this creature doesn't exist", creatureID);
            continue;
        }

        CreatureTemplate& creatureTemplate = itr->second;
        creatureTemplate.spells[index] = fields[2].Get<uint32>();

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Creature Template Spells in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadCreatureTemplateAddons()
{
    const uint32 oldMSTime = getMSTime();
    const QueryResult result = WorldDatabase.Query("SELECT id, path, mount, bytes1, bytes2, emote, visibility_distance_type, auras FROM world_creature_template_addon");
    const auto tableName = "world_creature_template_addon";

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creature template addon definitions. DB table `{}` is empty.", tableName);
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        uint32 entry = fields[0].Get<uint32>();

        if (!GetCreatureTemplate(entry))
        {
            LOG_ERROR("sql.sql", "Creature template (Entry: {}) does not exist but has a record in `{}`", entry, tableName);
            continue;
        }

        CreatureAddon& creatureAddon = _creatureTemplateAddonStore[entry];

        creatureAddon.pathID = fields[1].Get<uint32>();
        creatureAddon.mount = fields[2].Get<uint32>();
        creatureAddon.bytes1 = fields[3].Get<uint32>();
        creatureAddon.bytes2 = fields[4].Get<uint32>();
        creatureAddon.emote = fields[5].Get<uint32>();
        creatureAddon.visibilityDistanceType = static_cast<VisibilityDistanceType>(fields[6].Get<uint8>());

        const auto auras = fields[7].GetVector<uint32>();
        for (auto spellID : auras)
        {
            const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(spellID);
            if (!spellInfo)
            {
                LOG_ERROR("sql.sql", "Creature (Entry: {}) has wrong spell '{}' defined in `auras` field in `{}`.", entry, spellID, tableName);
                continue;
            }

            if (std::ranges::find(creatureAddon.auras, spellInfo->ID) != creatureAddon.auras.end())
            {
                LOG_ERROR("sql.sql", "Creature (Entry: {}) has duplicate aura (spell {}) in `auras` field in `{}`.", entry, spellInfo->ID, tableName);
                continue;
            }

            if (spellInfo->GetDuration() > 0)
            {
                LOG_DEBUG/*ERROR*/("sql.sql", "Creature (Entry: {}) has temporary aura (spell {}) in `auras` field in `{}`.", entry, spellInfo->ID, tableName);
            }

            creatureAddon.auras.push_back(spellInfo->ID);
        }

        if (creatureAddon.mount)
        {
            if (!sCreatureDisplayInfoStore.LookupEntry(creatureAddon.mount))
            {
                LOG_ERROR("sql.sql", "Creature (Entry: {}) has invalid displayInfoId ({}) for mount defined in `{}`", entry, creatureAddon.mount, tableName);
                creatureAddon.mount = 0;
            }
        }

        if (!sEmotesStore.LookupEntry(creatureAddon.emote))
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}) has invalid emote ({}) defined in `world_creature_addon`.", entry, creatureAddon.emote);
            creatureAddon.emote = 0;
        }

        if (creatureAddon.visibilityDistanceType >= VisibilityDistanceType::Max)
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}) has invalid visibilityDistanceType ({}) defined in `{}`.", entry, AsUnderlyingType(creatureAddon.visibilityDistanceType), tableName);
            creatureAddon.visibilityDistanceType = VisibilityDistanceType::Normal;
        }
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Creature Template Addons in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadCreatureCustomIDs()
{
    // Hack for modules
    const auto stringCreatureIds = sConfigMgr->GetOption<std::string>("Creatures.CustomIDs", "190010,55005,999991,25462,98888,601014,34567,34568");
    std::vector<std::string_view> CustomCreatures = Acore::Tokenize(stringCreatureIds, ',', false);

    for (const auto& itr : CustomCreatures)
        _creatureCustomIDsStore.push_back(Acore::StringTo<uint32>(itr).value());
}

void ObjectMgr::CheckCreatureTemplate(const CreatureTemplate* cInfo)
{
    if (!cInfo)
        return;

    bool ok = true;  // bool to allow to continue outside of this loop
    for (uint32 diff = 0; diff < MAX_DIFFICULTY - 1 && ok; ++diff)
    {
        if (!cInfo->DifficultyEntry[diff])
            continue;
        ok = false;  // Will be set to true at the end of this loop again

        const CreatureTemplate* difficultyInfo = GetCreatureTemplate(cInfo->DifficultyEntry[diff]);
        if (!difficultyInfo)
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}) has `difficulty[{}]`={} but creature entry {} does not exist.",
                cInfo->Entry, diff, cInfo->DifficultyEntry[diff], cInfo->DifficultyEntry[diff]);
            continue;
        }

        bool ok2 = true;
        for (uint32 diff2 = 0; diff2 < MAX_DIFFICULTY - 1 && ok2; ++diff2)
        {
            ok2 = false;
            if (_difficultyEntries[diff2].contains(cInfo->Entry))
            {
                LOG_ERROR("sql.sql", "Creature (Entry: {}) is listed as `difficulty[{}]` of another creature, but itself lists {} in `difficulty[{}]`.",
                    cInfo->Entry, diff2, cInfo->DifficultyEntry[diff], diff);
                continue;
            }

            if (_difficultyEntries[diff2].contains(cInfo->DifficultyEntry[diff]))
            {
                LOG_ERROR("sql.sql", "Creature (Entry: {}) already listed as `difficulty[{}]` for another entry.", cInfo->DifficultyEntry[diff], diff2);
                continue;
            }

            if (_hasDifficultyEntries[diff2].contains(cInfo->DifficultyEntry[diff]))
            {
                LOG_ERROR("sql.sql", "Creature (Entry: {}) has `difficulty[{}]`={} but creature entry {} has itself a value in `difficulty[{}]`.",
                    cInfo->Entry, diff, cInfo->DifficultyEntry[diff], cInfo->DifficultyEntry[diff], diff2);
                continue;
            }
            ok2 = true;
        }
        if (!ok2)
            continue;

        if (cInfo->Expansion > difficultyInfo->Expansion)
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}, expansion {}) has different `expansion` in difficulty {} mode (Entry: {}, expansion {}).",
                cInfo->Entry, cInfo->Expansion, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->Expansion);
        }

        if (cInfo->Faction != difficultyInfo->Faction)
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}, faction {}) has different `faction` in difficulty {} mode (Entry: {}, faction {}).",
                cInfo->Entry, cInfo->Faction, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->Faction);
        }

        if (cInfo->UnitClass != difficultyInfo->UnitClass)
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}, class {}) has different `unit_class` in difficulty {} mode (Entry: {}, class {}).",
                cInfo->Entry, cInfo->UnitClass, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->UnitClass);
            continue;
        }

        if (cInfo->FlagNPC != difficultyInfo->FlagNPC)
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}) has different `npc_flag` in difficulty {} mode (Entry: {}).", cInfo->Entry, diff + 1, cInfo->DifficultyEntry[diff]);
            continue;
        }

        if (cInfo->Family != difficultyInfo->Family)
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}, family {}) has different `family` in difficulty {} mode (Entry: {}, family {}).",
                cInfo->Entry, cInfo->Family, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->Family);
        }

        if (cInfo->type != difficultyInfo->type)
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}, type {}) has different `type` in difficulty {} mode (Entry: {}, type {}).",
                cInfo->Entry, cInfo->type, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->type);
        }

        if (!cInfo->VehicleId && difficultyInfo->VehicleId)
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}, vehicle {}) has different `vehicle` in difficulty {} mode (Entry: {}, vehicle {}).",
                cInfo->Entry, cInfo->VehicleId, diff + 1, cInfo->DifficultyEntry[diff], difficultyInfo->VehicleId);
        }

        // Check damage school
        if (cInfo->DamageSchool != difficultyInfo->DamageSchool)
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}) has different `damage_school` in difficulty {} mode (Entry: {})", cInfo->Entry, diff + 1, cInfo->DifficultyEntry[diff]);
        }

        if (!difficultyInfo->AIName.empty())
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}) lists difficulty {} mode entry {} with `name_ai` filled in. `name_ai` of difficulty 0 mode creature is always used instead.",
                cInfo->Entry, diff + 1, cInfo->DifficultyEntry[diff]);
            continue;
        }

        if (difficultyInfo->ScriptID)
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}) lists difficulty {} mode entry {} with `script_name` filled in. `script_name` of difficulty 0 mode creature is always used instead.",
                cInfo->Entry, diff + 1, cInfo->DifficultyEntry[diff]);
            continue;
        }

        _hasDifficultyEntries[diff].insert(cInfo->Entry);
        _difficultyEntries[diff].insert(cInfo->DifficultyEntry[diff]);
        ok = true;
    }

    if (!cInfo->AIName.empty() && !sCreatureAIRegistry->HasItem(cInfo->AIName))
    {
        LOG_ERROR("sql.sql", "Creature (Entry: {}) has non-registered `name_ai` '{}' set, removing", cInfo->Entry, cInfo->AIName);
        const_cast<CreatureTemplate*>(cInfo)->AIName.clear();
    }

    if (!sFactionTemplateStore.LookupEntry(cInfo->Faction))
        LOG_ERROR("sql.sql", "Creature (Entry: {}) has non-existing faction template ({}).", cInfo->Entry, cInfo->Faction);

    for (int k = 0; k < MAX_KILL_CREDIT; ++k)
    {
        if (cInfo->KillCredit[k])
        {
            if (!GetCreatureTemplate(cInfo->KillCredit[k]))
            {
                LOG_ERROR("sql.sql", "Creature (Entry: {}) lists non-existing creature entry {} in `kill_credit[{}]`.", cInfo->Entry, cInfo->KillCredit[k], k);
                const_cast<CreatureTemplate*>(cInfo)->KillCredit[k] = 0;
            }
        }
    }

    if (cInfo->Models.empty())
        LOG_ERROR("sql.sql", "Creature (Entry: {}) does not have any existing display id in world_creature_template_model.", cInfo->Entry);
    else
    {
        const float totalProbability = std::accumulate(
            cInfo->Models.begin(), cInfo->Models.end(), 0.0f,
            [](const float sum, const CreatureModel& model) { return sum + model.Probability; });

        if (totalProbability <= 0.0f)
        {
            // There are many cases in official data of all models having a probability of 0. Believe to be treated equivalent to equal chance ONLY if all are zeroed
            if (totalProbability == 0.0f)
                LOG_DEBUG("sql.sql", "Creature (Entry: {}) has zero total chance for all models in world_creature_template_model. Setting all to 1.0.", cInfo->Entry);
            else // Custom, likely bad data
                LOG_ERROR("sql.sql", "Creature (Entry: {}) has less than zero total chance for all models in world_creature_template_model. Setting all to 1.0.", cInfo->Entry);

            for (auto& models = const_cast<CreatureTemplate*>(cInfo)->Models; auto& model : models)
                model.Probability = 1.0f;
        }
    }

    if (!cInfo->UnitClass || ((1 << (cInfo->UnitClass - 1)) & CLASS_MASK_ALL_CREATURES) == 0)
    {
        LOG_ERROR("sql.sql", "Creature (Entry: {}) has invalid unit_class ({}) in creature_template. Set to 1 (CLASS_WARRIOR).", cInfo->Entry, cInfo->UnitClass);
        const_cast<CreatureTemplate*>(cInfo)->UnitClass = CLASS_WARRIOR;
    }

    if (cInfo->DamageSchool >= MAX_SPELL_SCHOOL)
    {
        LOG_ERROR("sql.sql", "Creature (Entry: {}) has invalid spell school value ({}) in `damage_school`.", cInfo->Entry, cInfo->DamageSchool);
        const_cast<CreatureTemplate*>(cInfo)->DamageSchool = SPELL_SCHOOL_NORMAL;
    }

    if (cInfo->BaseAttackTime == 0)
        const_cast<CreatureTemplate*>(cInfo)->BaseAttackTime  = BASE_ATTACK_TIME;

    if (cInfo->RangeAttackTime == 0)
        const_cast<CreatureTemplate*>(cInfo)->RangeAttackTime = BASE_ATTACK_TIME;

    if (cInfo->SpeedWalk == 0.0f)
    {
        LOG_ERROR("sql.sql", "Creature (Entry: {}) has wrong value ({}) in speed_walk, set to 1.", cInfo->Entry, cInfo->SpeedWalk);
        const_cast<CreatureTemplate*>(cInfo)->SpeedWalk = 1.0f;
    }

    if (cInfo->SpeedRun == 0.0f)
    {
        LOG_ERROR("sql.sql", "Creature (Entry: {}) has wrong value ({}) in speed_run, set to 1.14286.", cInfo->Entry, cInfo->SpeedRun);
        const_cast<CreatureTemplate*>(cInfo)->SpeedRun = 1.14286f;
    }

    if (cInfo->type && !sCreatureTypeStore.LookupEntry(cInfo->type))
    {
        LOG_ERROR("sql.sql", "Creature (Entry: {}) has invalid creature type ({}) in `type`.", cInfo->Entry, cInfo->type);
        const_cast<CreatureTemplate*>(cInfo)->type = CREATURE_TYPE_HUMANOID;
    }

    // Must exist in DBC or use hidden miscellaneous family
    if (cInfo->Family && !sCreatureFamilyStore.LookupEntry(cInfo->Family) && cInfo->Family != CREATURE_FAMILY_NOT_SPECIFIED)
    {
        LOG_ERROR("sql.sql", "Creature (Entry: {}) has invalid creature family ({}) in `family`.", cInfo->Entry, cInfo->Family);
        const_cast<CreatureTemplate*>(cInfo)->Family = 0;
    }

    CheckCreatureMovement("world_creature_template_movement", cInfo->Entry, const_cast<CreatureTemplate*>(cInfo)->Movement);

    if (cInfo->HoverHeight < 0.0f)
    {
        LOG_ERROR("sql.sql", "Creature (Entry: {}) has wrong value ({}) in `hover_height`", cInfo->Entry, cInfo->HoverHeight);
        const_cast<CreatureTemplate*>(cInfo)->HoverHeight = 1.0f;
    }

    if (cInfo->VehicleId && !sVehicleStore.LookupEntry(cInfo->VehicleId))
    {
        LOG_ERROR("sql.sql", "Creature (Entry: {}) has a non-existing VehicleId ({}). This *WILL* cause the client to freeze!", cInfo->Entry, cInfo->VehicleId);
        const_cast<CreatureTemplate*>(cInfo)->VehicleId = 0;
    }

    if (cInfo->PetSpellDataId && !sCreatureSpellDataStore.LookupEntry(cInfo->PetSpellDataId))
    {
        LOG_ERROR("sql.sql", "Creature (Entry: {}) has non-existing `pet_spell_data` ({}).", cInfo->Entry, cInfo->PetSpellDataId);
    }

    for (uint8 j = 0; j < MAX_CREATURE_SPELLS; ++j)
    {
        if (cInfo->spells[j] && !sSpellMgr->GetSpellInfo(cInfo->spells[j]))
        {
            LOG_ERROR("sql.sql", "Creature (Entry: {}) has non-existing spell[{}] ({}), set to 0.", cInfo->Entry, j, cInfo->spells[j]);
            const_cast<CreatureTemplate*>(cInfo)->spells[j] = 0;
        }
    }

    if (cInfo->MovementType >= MAX_DB_MOTION_TYPE)
    {
        LOG_ERROR("sql.sql", "Creature (Entry: {}) has wrong movement generator type ({}), ignored and set to IDLE.", cInfo->Entry, cInfo->MovementType);
        const_cast<CreatureTemplate*>(cInfo)->MovementType = IDLE_MOTION_TYPE;
    }

    if (cInfo->Expansion > MAX_EXPANSIONS - 1)
    {
        LOG_ERROR("sql.sql", "Table `creature_template` lists creature (Entry: {}) with expansion {}. Ignored and set to 0.", cInfo->Entry, cInfo->Expansion);
        const_cast<CreatureTemplate*>(cInfo)->Expansion = 0;
    }

    if (uint32 badFlags = cInfo->FlagsExtra & ~CREATURE_FLAG_EXTRA_DB_ALLOWED)
    {
        LOG_ERROR("sql.sql", "Table `creature_template` lists creature (Entry: {}) with disallowed `flags_extra` {}, removing incorrect flag.", cInfo->Entry, badFlags);
        const_cast<CreatureTemplate*>(cInfo)->FlagsExtra &= CREATURE_FLAG_EXTRA_DB_ALLOWED;
    }

    const_cast<CreatureTemplate*>(cInfo)->DamageModifier *= Creature::_GetDamageMod(cInfo->Rank);

    if (cInfo->GossipMenuId && !(cInfo->FlagNPC & UNIT_NPC_FLAG_GOSSIP) && !(cInfo->FlagsExtra & CREATURE_FLAG_EXTRA_MODULE))
    {
        LOG_ERROR("sql.sql", "Creature (Entry: {}) has assigned gossip menu {}, but npc_flag does not include UNIT_NPC_FLAG_GOSSIP (1).", cInfo->Entry, cInfo->GossipMenuId);
    }
    else if (!cInfo->GossipMenuId && (cInfo->FlagNPC & UNIT_NPC_FLAG_GOSSIP) && !(cInfo->FlagsExtra & CREATURE_FLAG_EXTRA_MODULE))
    {
        LOG_INFO("sql.sql", "Creature (Entry: {}) has npc_flag UNIT_NPC_FLAG_GOSSIP (1), but gossip menu is unassigned.", cInfo->Entry);
    }
}

void ObjectMgr::CheckCreatureMovement(const char* table, uint64 id, CreatureMovementData& creatureMovement)
{
    if (creatureMovement.Ground >= CreatureGroundMovementType::Max)
    {
        LOG_ERROR("sql.sql", "`{}`.`Ground` wrong value ({}) for Id {}, setting to Run.", table, static_cast<uint32>(creatureMovement.Ground), id);
        creatureMovement.Ground = CreatureGroundMovementType::Run;
    }

    if (creatureMovement.Flight >= CreatureFlightMovementType::Max)
    {
        LOG_ERROR("sql.sql", "`{}`.`Flight` wrong value ({}) for Id {}, setting to None.", table, static_cast<uint32>(creatureMovement.Flight), id);
        creatureMovement.Flight = CreatureFlightMovementType::None;
    }

    if (creatureMovement.Chase >= CreatureChaseMovementType::Max)
    {
        LOG_ERROR("sql.sql", "`{}`.`Chase` wrong value ({}) for Id {}, setting to Run.", table, static_cast<uint32>(creatureMovement.Chase), id);
        creatureMovement.Chase = CreatureChaseMovementType::Run;
    }

    if (creatureMovement.Random >= CreatureRandomMovementType::Max)
    {
        LOG_ERROR("sql.sql", "`{}`.`Random` wrong value ({}) for Id {}, setting to Walk.", table, static_cast<uint32>(creatureMovement.Random), id);
        creatureMovement.Random = CreatureRandomMovementType::Walk;
    }
}

void ObjectMgr::LoadCreatureAddons()
{
    const uint32 oldMSTime = getMSTime();
    const QueryResult result = WorldDatabase.Query("SELECT guid, path, mount, bytes1, bytes2, emote, visibility_distance_type, auras FROM world_creature_addon");
    const auto tableName = "world_creature_addon";

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creature addon definitions. DB table `{}` is empty.", tableName);
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        ObjectGuid::LowType guid = fields[0].Get<uint32>();

        const CreatureData* creData = GetCreatureData(guid);
        if (!creData)
        {
            LOG_ERROR("sql.sql", "Creature (GUID: {}) does not exist but has a record in `{}`", guid, tableName);
            continue;
        }

        CreatureAddon& creatureAddon = _creatureAddonStore[guid];

        creatureAddon.pathID = fields[1].Get<uint32>();
        if (creData->movementType == WAYPOINT_MOTION_TYPE && !creatureAddon.pathID)
        {
            const_cast<CreatureData*>(creData)->movementType = IDLE_MOTION_TYPE;
            LOG_ERROR("sql.sql", "Creature (GUID {}) has movement type set to WAYPOINT_MOTION_TYPE but no path assigned", guid);
        }

        creatureAddon.mount   = fields[2].Get<uint32>();
        creatureAddon.bytes1  = fields[3].Get<uint32>();
        creatureAddon.bytes2  = fields[4].Get<uint32>();
        creatureAddon.emote   = fields[5].Get<uint32>();
        creatureAddon.visibilityDistanceType = static_cast<VisibilityDistanceType>(fields[6].Get<uint8>());
        const auto auras = fields[7].GetVector<uint32>();

        for (auto spellID : auras)
        {
            const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(spellID);
            if (!spellInfo)
            {
                LOG_ERROR("sql.sql", "Creature (GUID: {}) has wrong spell '{}' defined in `auras` field in `{}`.", guid, spellID, tableName);
                continue;
            }

            if (std::ranges::find(creatureAddon.auras, spellInfo->ID) != creatureAddon.auras.end())
            {
                LOG_ERROR("sql.sql", "Creature (GUID: {}) has duplicate aura (spell {}) in `auras` field in `{}`.", guid, spellInfo->ID, tableName);
                continue;
            }

            if (spellInfo->GetDuration() > 0)
            {
                LOG_DEBUG/*ERROR*/("sql.sql", "Creature (Entry: {}) has temporary aura (spell {}) in `auras` field in `world_creature_template_addon`.", guid, spellInfo->ID);
                // continue;
            }

            creatureAddon.auras.push_back(spellInfo->ID);
        }

        if (creatureAddon.mount)
        {
            if (!sCreatureDisplayInfoStore.LookupEntry(creatureAddon.mount))
            {
                LOG_ERROR("sql.sql", "Creature (GUID: {}) has invalid displayInfoId ({}) for mount defined in `{}`", guid, creatureAddon.mount, tableName);
                creatureAddon.mount = 0;
            }
        }

        if (!sEmotesStore.LookupEntry(creatureAddon.emote))
        {
            LOG_ERROR("sql.sql", "Creature (GUID: {}) has invalid emote ({}) defined in `{}`.", guid, creatureAddon.emote, tableName);
            creatureAddon.emote = 0;
        }

        if (creatureAddon.visibilityDistanceType >= VisibilityDistanceType::Max)
        {
            LOG_ERROR("sql.sql", "Creature (GUID: {}) has invalid visibilityDistanceType ({}) defined in `{}`.", guid, AsUnderlyingType(creatureAddon.visibilityDistanceType), tableName);
            creatureAddon.visibilityDistanceType = VisibilityDistanceType::Normal;
        }

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Creature Addons in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadGameObjectAddons()
{
    const uint32 oldMSTime = getMSTime();
    const QueryResult result = WorldDatabase.Query("SELECT guid, parent_rotation, invisibility_type, invisibility_value FROM world_game_object_addon");
    const auto tableName = "world_game_object_addon";

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 GameObject addon definitions. DB table `{}` is empty.", tableName);
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        ObjectGuid::LowType guid = fields[0].Get<uint32>();
        if (!GetGameObjectData(guid))
        {
            LOG_ERROR("sql.sql", "GameObject (GUID: {}) does not exist but has a record in `{}`", guid, tableName);
            continue;
        }

        auto& [parentRotation, invisibilityType, invisibilityValue] = _gameObjectAddonStore[guid];
        const auto rotation = fields[1].GetArray<float, 4>();
        parentRotation = QuaternionData(rotation[0], rotation[1], rotation[2], rotation[3]);
        invisibilityType = static_cast<InvisibilityType>(fields[2].Get<uint8>());
        invisibilityValue = fields[3].Get<uint32>();

        if (invisibilityType >= TOTAL_INVISIBILITY_TYPES)
        {
            LOG_ERROR("sql.sql", "GameObject (GUID: {}) has invalid InvisibilityType in `{}`", guid, tableName);
            invisibilityType = INVISIBILITY_GENERAL;
            invisibilityValue = 0;
        }

        if (invisibilityType && !invisibilityValue)
        {
            LOG_ERROR("sql.sql", "GameObject (GUID: {}) has InvisibilityType set but has no InvisibilityValue in `{}`, set to 1", guid, tableName);
            invisibilityValue = 1;
        }

        if (!parentRotation.IsUnit())
        {
            LOG_ERROR("sql.sql", "GameObject (GUID: {}) has invalid parent rotation in `{}`, set to default", guid, tableName);
            parentRotation = QuaternionData();
        }

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} GameObject Addons in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

const GameObjectAddon* ObjectMgr::GetGameObjectAddon(const ObjectGuid::LowType lowGUID)
{
    if (const auto itr = _gameObjectAddonStore.find(lowGUID); itr != _gameObjectAddonStore.end())
        return &itr->second;
    return nullptr;
}

const CreatureAddon* ObjectMgr::GetCreatureAddon(const ObjectGuid::LowType lowGUID)
{
    if (const auto itr = _creatureAddonStore.find(lowGUID); itr != _creatureAddonStore.end())
        return &itr->second;
    return nullptr;
}

const CreatureAddon* ObjectMgr::GetCreatureTemplateAddon(const uint32 entry)
{
    if (const auto itr = _creatureTemplateAddonStore.find(entry); itr != _creatureTemplateAddonStore.end())
        return &itr->second;
    return nullptr;
}

const CreatureMovementData* ObjectMgr::GetCreatureMovementOverride(const ObjectGuid::LowType spawnId) const
{
    return Acore::Containers::MapGetValuePtr(_creatureMovementOverrides, spawnId);
}

const EquipmentInfo* ObjectMgr::GetEquipmentInfo(const uint32 entry, int8& id)
{
    const auto itr = _equipmentInfoStore.find(entry);
    if (itr == _equipmentInfoStore.end())
        return nullptr;

    if (itr->second.empty())
        return nullptr;

    if (id == -1) // Select a random element
    {
        auto rItr = itr->second.begin();
        std::advance(rItr, urand(0u, itr->second.size() - 1));
        id = static_cast<int8>(std::distance(itr->second.begin(), rItr) + 1);
        return &rItr->second;
    }

    const auto itr2 = itr->second.find(id);
    if (itr2 != itr->second.end())
        return &itr2->second;

    return nullptr;
}

void ObjectMgr::LoadEquipmentTemplates()
{
    const uint32 oldMSTime = getMSTime();
    const QueryResult result = WorldDatabase.Query("SELECT creature, id, items FROM world_creature_equip_template");
    const auto tableName = "world_creature_equip_template";

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creature equipment templates. DB table `{}` is empty!", tableName);
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        uint32 entry = fields[0].Get<uint32>();

        if (!GetCreatureTemplate(entry))
        {
            LOG_ERROR("sql.sql", "Creature template (CreatureID: {}) does not exist but has a record in `{}`", entry, tableName);
            continue;
        }

        uint8 id = fields[1].Get<uint8>();
        if (!id)
        {
            LOG_ERROR("sql.sql", "Creature equipment template with id 0 found for creature {}, skipped.", entry);
            continue;
        }

        const auto itemsArray = fields[2].GetArray<uint32, 3>();
        auto& [ItemID] = _equipmentInfoStore[entry][id];
        ItemID[0] = itemsArray[0];
        ItemID[1] = itemsArray[1];
        ItemID[2] = itemsArray[2];

        for (uint8 i = 0; i < MAX_EQUIPMENT_ITEMS; ++i)
        {
            if (!ItemID[i])
                continue;

            const ItemEntry* dbcItem = sItemStore.LookupEntry(ItemID[i]);

            if (!dbcItem)
            {
                LOG_ERROR("sql.sql", "Unknown item (ID={}) in {}.items[{}] for CreatureID = {} and ID = {}, forced to 0.", ItemID[i], tableName, i, entry, id);
                ItemID[i] = 0;
                continue;
            }

            if (dbcItem->InventoryType != INVTYPE_WEAPON &&
                dbcItem->InventoryType != INVTYPE_SHIELD &&
                dbcItem->InventoryType != INVTYPE_RANGED &&
                dbcItem->InventoryType != INVTYPE_2HWEAPON &&
                dbcItem->InventoryType != INVTYPE_WEAPONMAINHAND &&
                dbcItem->InventoryType != INVTYPE_WEAPONOFFHAND &&
                dbcItem->InventoryType != INVTYPE_HOLDABLE &&
                dbcItem->InventoryType != INVTYPE_THROWN &&
                dbcItem->InventoryType != INVTYPE_RANGEDRIGHT)
            {
                LOG_ERROR("sql.sql", "Item (ID={}) in {}.items[{}] for CreatureID = {} and ID = {} is not equipable in a hand, forced to 0.", ItemID[i], tableName, i, entry, id);
                ItemID[i] = 0;
            }
        }

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Equipment Templates in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadCreatureMovementOverrides()
{
    const uint32 oldMSTime = getMSTime();

    _creatureMovementOverrides.clear();

    // Load the data from world_creature_movement_override and if NULL fallback to world_creature_template_movement
    const QueryResult result = WorldDatabase.Query(
        "SELECT cmo.guid, COALESCE(cmo.ground, ctm.ground), COALESCE(cmo.swim, ctm.swim), "
        "COALESCE(cmo.flight, ctm.flight), COALESCE(cmo.rooted, ctm.rooted), COALESCE(cmo.chase, ctm.chase), "
        "COALESCE(cmo.random, ctm.random), COALESCE(cmo.interaction_pause_timer, ctm.interaction_pause_timer) "
        "FROM world_creature_movement_override AS cmo "
        "LEFT JOIN world_creature AS c ON c.guid = cmo.guid "
        "LEFT JOIN world_creature_template_movement AS ctm ON ctm.creature = c.id1");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creature movement overrides. DB table `world_creature_movement_override` is empty!");
        return;
    }

    do
    {
        const Field* fields  = result->Fetch();
        ObjectGuid::LowType spawnId = fields[0].Get<uint32>();
        if (!GetCreatureData(spawnId))
        {
            LOG_ERROR("sql.sql", "Creature (GUID: {}) does not exist but has a record in `world_creature_movement_override`", spawnId);
            continue;
        }

        CreatureMovementData& movement = _creatureMovementOverrides[spawnId];
        if (!fields[1].IsNull())
            movement.Ground = static_cast<CreatureGroundMovementType>(fields[1].Get<uint8>());

        if (!fields[2].IsNull())
            movement.Swim = static_cast<bool>(fields[2].Get<uint8>());

        if (!fields[3].IsNull())
            movement.Flight = static_cast<CreatureFlightMovementType>(fields[3].Get<uint8>());

        if (!fields[4].IsNull())
            movement.Rooted = static_cast<bool>(fields[4].Get<uint8>());

        if (!fields[5].IsNull())
            movement.Chase = static_cast<CreatureChaseMovementType>(fields[5].Get<uint8>());

        if (!fields[6].IsNull())
            movement.Random = static_cast<CreatureRandomMovementType>(fields[6].Get<uint8>());

        if (!fields[7].IsNull())
            movement.InteractionPauseTimer = fields[7].Get<uint32>();

        CheckCreatureMovement("world_creature_movement_override", spawnId, movement);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Movement Overrides in {} ms", _creatureMovementOverrides.size(), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

const CreatureModelInfo* ObjectMgr::GetCreatureModelInfo(const uint32 modelId) const
{
    if (const auto itr = _creatureModelStore.find(modelId); itr != _creatureModelStore.end())
        return &itr->second;
    return nullptr;
}

const CreatureModel* ObjectMgr::ChooseDisplayId(const CreatureTemplate* cinfo, const CreatureData* data /*= nullptr*/)
{
    // Load creature model (display id)
    if (data && data->displayid)
        if (const CreatureModel* model = cinfo->GetModelWithDisplayId(data->displayid))
            return model;

    if (!cinfo->HasFlagsExtra(CREATURE_FLAG_EXTRA_TRIGGER))
        if (const CreatureModel* model = cinfo->GetRandomValidModel())
            return model;

    // Triggers by default receive the invisible model
    return cinfo->GetFirstInvisibleModel();
}

void ObjectMgr::ChooseCreatureFlags(const CreatureTemplate* cinfo, uint32& npcFlag, uint32& unit_flags, uint32& dynamicFlags, const CreatureData* data /*= nullptr*/)
{
    npcFlag = cinfo->FlagNPC;
    unit_flags = cinfo->UnitFlags;
    dynamicFlags = cinfo->DynamicFlags;

    if (data)
    {
        if (data->npcFlag)
            npcFlag = data->npcFlag;

        if (data->unitFlags)
            unit_flags = data->unitFlags;

        if (data->dynamicFlags)
            dynamicFlags = data->dynamicFlags;
    }
}

const CreatureModelInfo* ObjectMgr::GetCreatureModelRandomGender(CreatureModel* model, const CreatureTemplate* creatureTemplate) const
{
    const CreatureModelInfo* modelInfo = GetCreatureModelInfo(model->CreatureDisplayID);
    if (!modelInfo)
        return nullptr;

    // If a model for another gender exists, 50% chance to use it
    if (modelInfo->OtherGenderModelID != 0 && urand(0, 1) == 0)
    {
        if (const CreatureModelInfo* minfo_tmp = GetCreatureModelInfo(modelInfo->OtherGenderModelID))
        {
            // Model ID changed
            model->CreatureDisplayID = modelInfo->OtherGenderModelID;
            if (creatureTemplate)
            {
                const auto itr = std::ranges::find_if(creatureTemplate->Models, [&](const CreatureModel& templateModel)
                {
                    return templateModel.CreatureDisplayID == modelInfo->OtherGenderModelID;
                });
                if (itr != creatureTemplate->Models.end())
                    *model = *itr;
            }
            return minfo_tmp;
        }
        LOG_ERROR("sql.sql", "Model (Entry: {}) has other_gender_model_info {} not found in table `world_creature_model_info`. ", model->CreatureDisplayID, modelInfo->OtherGenderModelID);
    }

    return modelInfo;
}

void ObjectMgr::LoadCreatureModelInfo()
{
    const uint32 oldMSTime = getMSTime();
    const QueryResult result = WorldDatabase.Query("SELECT id, bounding_radius, combat_reach, gender, other_gender_model_info FROM world_creature_model_info");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creature model definitions. DB table `world_creature_model_info` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    _creatureModelStore.rehash(result->GetRowCount());
    uint32 count = 0;

    // List of ModelDataIDs that use Invisible models
    const std::set<uint32> triggerCreatureModelDataID {1731, 1752, 2206, 2296, 2372, 2382, 2481, 2512, 2513, 2611, 2636, 2790, 3230, 3274};

    do
    {
        const Field* fields = result->Fetch();

        uint32 modelID = fields[0].Get<uint32>();
        const CreatureDisplayInfoEntry* creatureDisplay = sCreatureDisplayInfoStore.LookupEntry(modelID);

        CreatureModelInfo& modelInfo = _creatureModelStore[modelID];

        modelInfo.BoundingRadius     = fields[1].Get<float>();
        modelInfo.CombatReach        = fields[2].Get<float>();
        modelInfo.Gender             = fields[3].Get<uint8>();
        modelInfo.OtherGenderModelID = fields[4].Get<uint32>();
        modelInfo.IsTrigger          = false;

        if (!sCreatureDisplayInfoStore.LookupEntry(modelID))
            LOG_ERROR("sql.sql", "Table `world_creature_model_info` has model for not existed display id ({}).", modelID);

        if (modelInfo.Gender > GENDER_NONE)
        {
            LOG_ERROR("sql.sql", "Table `world_creature_model_info` has wrong gender ({}) for display id ({}).", static_cast<uint32>(modelInfo.Gender), modelID);
            modelInfo.Gender = GENDER_MALE;
        }

        if (modelInfo.OtherGenderModelID && !sCreatureDisplayInfoStore.LookupEntry(modelInfo.OtherGenderModelID))
        {
            LOG_ERROR("sql.sql", "Table `world_creature_model_info` has not existed alt.gender model ({}) for existed display id ({}).", modelInfo.OtherGenderModelID, modelID);
            modelInfo.OtherGenderModelID = 0;
        }

        if (modelInfo.CombatReach < 0.1f)
            modelInfo.CombatReach = DEFAULT_COMBAT_REACH;

        if (const CreatureModelDataEntry* modelData = sCreatureModelDataStore.LookupEntry(creatureDisplay->ModelID))
            if (triggerCreatureModelDataID.contains(modelData->ID))
                modelInfo.IsTrigger = true;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Creature Model Based Info in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadPlayerTotemModels()
{
    const uint32 oldMSTime = getMSTime();
    const QueryResult result = WorldDatabase.Query("SELECT totem, race, model from world_player_totem_model");

    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 player totem model records. DB table `world_player_totem_model` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        auto totemSlot = static_cast<SummonSlot>(fields[0].Get<uint8>());
        uint8 race = fields[1].Get<uint8>();
        uint32 displayID = fields[2].Get<uint32>();

        if (totemSlot < SUMMON_SLOT_TOTEM_FIRE || totemSlot >= MAX_TOTEM_SLOT)
        {
            LOG_ERROR("sql.sql", "Wrong TotemSlot {} in `world_player_totem_model` table, skipped.", totemSlot);
            continue;
        }

        if (!sChrRacesStore.LookupEntry(race))
        {
            LOG_ERROR("sql.sql", "Race {} defined in `world_player_totem_model` does not exists, skipped.", static_cast<uint32>(race));
            continue;
        }

        if (!sCreatureDisplayInfoStore.LookupEntry(displayID))
        {
            LOG_ERROR("sql.sql", "TotemSlot: {} defined in `world_player_totem_model` has non-existing model ({}), skipped.", totemSlot, displayID);
            continue;
        }

        _playerTotemModel[std::make_pair(totemSlot, static_cast<Races>(race))] = displayID;
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} player totem model records in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

uint32 ObjectMgr::GetModelForTotem(SummonSlot totemSlot, Races race) const
{
    if (const auto itr = _playerTotemModel.find(std::make_pair(totemSlot, race)); itr != _playerTotemModel.end())
        return itr->second;

    LOG_ERROR("misc", "TotemSlot {} with RaceID ({}) have no totem model data defined, set to default model.", totemSlot, race);
    return 0;
}

void ObjectMgr::LoadPlayerShapeshiftModels()
{
    const uint32 oldMSTime = getMSTime();
    const QueryResult result = WorldDatabase.Query("SELECT shapeshift, race, customization, gender, model from world_player_shapeshift_model");

    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 player shapeshift model records. DB table `world_player_shapeshift_model` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        auto shapeshiftForm = static_cast<ShapeshiftForm>(fields[0].Get<uint8>());
        uint8 race = fields[1].Get<uint8>();
        uint8 customizationID = fields[2].Get<uint8>();
        uint8 genderID = static_cast<Gender>(fields[3].Get<uint8>());
        uint32 modelID = fields[4].Get<uint32>();

        if (!sChrRacesStore.LookupEntry(race))
        {
            LOG_ERROR("sql.sql", "Race {} defined in `world_player_shapeshift_model` does not exists, skipped.", static_cast<uint32>(race));
            continue;
        }

        if (!sCreatureDisplayInfoStore.LookupEntry(modelID))
        {
            LOG_ERROR("sql.sql", "ShapeshiftForm: {}, Race: {} defined in `world_player_shapeshift_model` has non-existing model ({}), skipped.", shapeshiftForm, race, modelID);
            continue;
        }

        _playerShapeshiftModel[std::make_tuple(shapeshiftForm, race, customizationID, genderID)] = modelID;
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} player totem model records in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

uint32 ObjectMgr::GetModelForShapeshift(ShapeshiftForm form, const Player* player) const
{
    uint8 customizationID;

    if (player->GetTeamId() == TEAM_ALLIANCE)
        customizationID = player->GetByteValue(PLAYER_BYTES, 3); // Use Hair Color
    else
        customizationID = player->GetByteValue(PLAYER_BYTES, 0); // Use Skin Color

    auto itr = _playerShapeshiftModel.find(std::make_tuple(form, player->getRace(), customizationID, player->getGender()));
    if (itr != _playerShapeshiftModel.end())
        return itr->second; // Explicit combination

    itr = _playerShapeshiftModel.find(std::make_tuple(form, player->getRace(), customizationID, GENDER_NONE));
    if (itr != _playerShapeshiftModel.end())
        return itr->second; // Combination applied to both genders

    itr = _playerShapeshiftModel.find(std::make_tuple(form, player->getRace(), 255, player->getGender()));
    if (itr != _playerShapeshiftModel.end())
        return itr->second; // Default gender-dependent model

    itr = _playerShapeshiftModel.find(std::make_tuple(form, player->getRace(), 255, GENDER_NONE));
    if (itr != _playerShapeshiftModel.end())
        return itr->second; // Last resort

    LOG_DEBUG("entities.player", "ShapeshiftForm {} with RaceID ({}) have no shapeshift model data defined, using fallback data.", form, player->getRace());
    return 0;
}

void ObjectMgr::LoadLinkedRespawn()
{
    const uint32 oldMSTime = getMSTime();

    _linkedRespawnStore.clear();
    const QueryResult result = WorldDatabase.Query("SELECT guid, linked_guid, link_type FROM world_linked_respawn ORDER BY guid ASC");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 linked respawns. DB table `world_linked_respawn` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    do
    {
        const Field* fields = result->Fetch();

        ObjectGuid::LowType guidLow = fields[0].Get<uint32>();
        ObjectGuid::LowType linkedGuidLow = fields[1].Get<uint32>();
        const uint8 linkType = fields[2].Get<uint8>();

        ObjectGuid guid, linkedGuid;
        bool error = false;
        switch (linkType)
        {
        case CREATURE_TO_CREATURE:
            {
                const CreatureData* slave = GetCreatureData(guidLow);
                if (!slave)
                {
                    LOG_ERROR("sql.sql", "LinkedRespawn: Creature (guid) {} not found in creature table", guidLow);
                    error = true;
                    break;
                }

                const CreatureData* master = GetCreatureData(linkedGuidLow);
                if (!master)
                {
                    LOG_ERROR("sql.sql", "LinkedRespawn: Creature (linked_guid) {} not found in creature table", linkedGuidLow);
                    error = true;
                    break;
                }

                if (const MapEntry* map = sMapStore.LookupEntry(master->mapID); !map || !map->InstanceAble() || master->mapID != slave->mapID)
                {
                    LOG_ERROR("sql.sql", "LinkedRespawn: Creature '{}' linking to Creature '{}' on an unpermitted map.", guidLow, linkedGuidLow);
                    error = true;
                    break;
                }

                if (!(master->spawnMask & slave->spawnMask))  // They must have a possibility to meet (normal/heroic difficulty)
                {
                    LOG_ERROR("sql.sql", "LinkedRespawn: Creature '{}' linking to Creature '{}' with not corresponding spawnMask", guidLow, linkedGuidLow);
                    error = true;
                    break;
                }

                guid = ObjectGuid::Create<HighGuid::Unit>(slave->id1, guidLow);
                linkedGuid = ObjectGuid::Create<HighGuid::Unit>(master->id1, linkedGuidLow);
                break;
            }
        case CREATURE_TO_GO:
            {
                const CreatureData* slave = GetCreatureData(guidLow);
                if (!slave)
                {
                    LOG_ERROR("sql.sql", "LinkedRespawn: Creature (guid) {} not found in world_creature table", guidLow);
                    error = true;
                    break;
                }

                const GameObjectData* master = GetGameObjectData(linkedGuidLow);
                if (!master)
                {
                    LOG_ERROR("sql.sql", "LinkedRespawn: GameObject (linked_guid) {} not found in world_game_object table", linkedGuidLow);
                    error = true;
                    break;
                }

                if (const MapEntry* map = sMapStore.LookupEntry(master->mapID); !map || !map->InstanceAble() || master->mapID != slave->mapID)
                {
                    LOG_ERROR("sql.sql", "LinkedRespawn: Creature '{}' linking to GameObject '{}' on an unpermitted map.", guidLow, linkedGuidLow);
                    error = true;
                    break;
                }

                if (!(master->spawnMask & slave->spawnMask))  // They must have a possibility to meet (normal/heroic difficulty)
                {
                    LOG_ERROR("sql.sql", "LinkedRespawn: Creature '{}' linking to GameObject '{}' with not corresponding spawnMask", guidLow, linkedGuidLow);
                    error = true;
                    break;
                }

                guid = ObjectGuid::Create<HighGuid::Unit>(slave->id1, guidLow);
                linkedGuid = ObjectGuid::Create<HighGuid::GameObject>(master->id, linkedGuidLow);
                break;
            }
        case GO_TO_GO:
            {
                const GameObjectData* slave = GetGameObjectData(guidLow);
                if (!slave)
                {
                    LOG_ERROR("sql.sql", "LinkedRespawn: GameObject (guid) {} not found in world_game_object table", guidLow);
                    error = true;
                    break;
                }

                const GameObjectData* master = GetGameObjectData(linkedGuidLow);
                if (!master)
                {
                    LOG_ERROR("sql.sql", "LinkedRespawn: GameObject (linked_guid) {} not found in world_game_object table", linkedGuidLow);
                    error = true;
                    break;
                }

                if (const MapEntry* map = sMapStore.LookupEntry(master->mapID); !map || !map->InstanceAble() || master->mapID != slave->mapID)
                {
                    LOG_ERROR("sql.sql", "LinkedRespawn: GameObject '{}' linking to GameObject '{}' on an unpermitted map.", guidLow, linkedGuidLow);
                    error = true;
                    break;
                }

                if (!(master->spawnMask & slave->spawnMask))  // They must have a possibility to meet (normal/heroic difficulty)
                {
                    LOG_ERROR("sql.sql", "LinkedRespawn: GameObject '{}' linking to GameObject '{}' with not corresponding spawnMask", guidLow, linkedGuidLow);
                    error = true;
                    break;
                }

                guid = ObjectGuid::Create<HighGuid::GameObject>(slave->id, guidLow);
                linkedGuid = ObjectGuid::Create<HighGuid::GameObject>(master->id, linkedGuidLow);
                break;
            }
        case GO_TO_CREATURE:
            {
                const GameObjectData* slave = GetGameObjectData(guidLow);
                if (!slave)
                {
                    LOG_ERROR("sql.sql", "LinkedRespawn: GameObject (guid) {} not found in world_game_object table", guidLow);
                    error = true;
                    break;
                }

                const CreatureData* master = GetCreatureData(linkedGuidLow);
                if (!master)
                {
                    LOG_ERROR("sql.sql", "LinkedRespawn: Creature (linked_guid) {} not found in world_creature table", linkedGuidLow);
                    error = true;
                    break;
                }

                if (const MapEntry* map = sMapStore.LookupEntry(master->mapID); !map || !map->InstanceAble() || master->mapID != slave->mapID)
                {
                    LOG_ERROR("sql.sql", "LinkedRespawn: GameObject '{}' linking to Creature '{}' on an unpermitted map.", guidLow, linkedGuidLow);
                    error = true;
                    break;
                }

                if (!(master->spawnMask & slave->spawnMask))  // they must have a possibility to meet (normal/heroic difficulty)
                {
                    LOG_ERROR("sql.sql", "LinkedRespawn: GameObject '{}' linking to Creature '{}' with not corresponding spawnMask", guidLow, linkedGuidLow);
                    error = true;
                    break;
                }

                guid = ObjectGuid::Create<HighGuid::GameObject>(slave->id, guidLow);
                linkedGuid = ObjectGuid::Create<HighGuid::Unit>(master->id1, linkedGuidLow);
                break;
            }
        default:
            {
                LOG_ERROR("sql.sql", "LinkedRespawn: Unknown type: {}", static_cast<uint32>(linkType));
                error = true;
                break;
            }
        }

        if (!error)
            _linkedRespawnStore[guid] = linkedGuid;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Linked Respawns In {} ms", _linkedRespawnStore.size(), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

bool ObjectMgr::SetCreatureLinkedRespawn(ObjectGuid::LowType guidLow, ObjectGuid::LowType linkedGuidLow)
{
    if (!guidLow)
        return false;

    const CreatureData* master = GetCreatureData(guidLow);
    const ObjectGuid guid = ObjectGuid::Create<HighGuid::Unit>(master->id1, guidLow);

    if (!linkedGuidLow) // We're removing the linking
    {
        _linkedRespawnStore.erase(guid);
        WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_CREATURE_LINKED_RESPAWN);
        stmt->SetData(0, guidLow);
        WorldDatabase.Execute(stmt);
        return true;
    }

    const CreatureData* slave = GetCreatureData(linkedGuidLow);
    if (!slave)
    {
        LOG_ERROR("sql.sql", "Creature '{}' linking to non-existent creature '{}'.", guidLow, linkedGuidLow);
        return false;
    }

    if (const MapEntry* map = sMapStore.LookupEntry(master->mapID); !map || !map->InstanceAble() || master->mapID != slave->mapID)
    {
        LOG_ERROR("sql.sql", "Creature '{}' linking to '{}' on an unpermitted map.", guidLow, linkedGuidLow);
        return false;
    }

    if (!(master->spawnMask & slave->spawnMask))  // They must have a possibility to meet (normal/heroic difficulty)
    {
        LOG_ERROR("sql.sql", "LinkedRespawn: Creature '{}' linking to '{}' with not corresponding spawnMask", guidLow, linkedGuidLow);
        return false;
    }

    _linkedRespawnStore[guid] = ObjectGuid::Create<HighGuid::Unit>(slave->id1, linkedGuidLow);
    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_REP_CREATURE_LINKED_RESPAWN);
    stmt->SetData(0, guidLow);
    stmt->SetData(1, linkedGuidLow);
    WorldDatabase.Execute(stmt);
    return true;
}

void ObjectMgr::LoadTempSummons()
{
    const uint32 oldMSTime = getMSTime();
    const QueryResult result = WorldDatabase.Query("SELECT summoner, summoner_type, group, entry, position, orientation, summon_type, summon_time FROM world_creature_summon_group");
    const auto tableName = "world_creature_summon_group";

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 temp summons. DB table `{}` is empty.", tableName);
        return;
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        uint32 summonerID = fields[0].Get<uint32>();
        auto summonerType = static_cast<SummonerType>(fields[1].Get<uint8>());
        uint8 group = fields[2].Get<uint8>();

        switch (summonerType)
        {
            case SUMMONER_TYPE_CREATURE:
                if (!GetCreatureTemplate(summonerID))
                {
                    LOG_ERROR("sql.sql", "Table `{}` has summoner with non existing entry {} for creature summoner type, skipped.", tableName, summonerID);
                    continue;
                }
                break;
            case SUMMONER_TYPE_GAMEOBJECT:
                if (!GetGameObjectTemplate(summonerID))
                {
                    LOG_ERROR("sql.sql", "Table `{}` has summoner with non existing entry {} for GameObject summoner type, skipped.", tableName, summonerID);
                    continue;
                }
                break;
            case SUMMONER_TYPE_MAP:
                if (!sMapStore.LookupEntry(summonerID))
                {
                    LOG_ERROR("sql.sql", "Table `{}` has summoner with non existing entry {} for map summoner type, skipped.", tableName, summonerID);
                    continue;
                }
                break;
            default:
                LOG_ERROR("sql.sql", "Table `{}` has unhandled summoner type {} for summoner {}, skipped.", tableName, summonerType, summonerID);
                continue;
        }

        TempSummonData data;
        data.entry = fields[3].Get<uint32>();

        if (!GetCreatureTemplate(data.entry))
        {
            LOG_ERROR("sql.sql", "Table `{}` has creature in group [Summoner ID: {}, Summoner Type: {}, Group ID: {}] with non existing creature entry {}, skipped.",
                tableName, summonerID, summonerType, group, data.entry);
            continue;
        }

        const auto position = fields[4].GetArray<float, 3>();
        const float orientation = fields[5].Get<float>();

        data.pos.Relocate(position[0], position[1], position[2], orientation);
        data.type = static_cast<TempSummonType>(fields[6].Get<uint8>());

        if (data.type > TEMPSUMMON_MANUAL_DESPAWN)
        {
            LOG_ERROR("sql.sql", "Table `{}` has unhandled temp summon type {} in group [Summoner ID: {}, Summoner Type: {}, Group ID: {}] for creature entry {}, skipped.",
                tableName, data.type, summonerID, summonerType, group, data.entry);
            continue;
        }

        data.time = fields[7].Get<uint32>();

        TempSummonGroupKey key(summonerID, summonerType, group);
        _tempSummonDataStore[key].push_back(data);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Temporary Summons in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadGameObjectSummons()
{
    const uint32 oldMSTime = getMSTime();

    _goSummonDataStore.clear();

    const QueryResult result = WorldDatabase.Query("SELECT summoner, summoner_type, group, entry, position, orientation, rotation, respawn_time FROM world_game_object_summon_group");
    const auto tableName = "world_game_object_summon_group";

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 GameObject summons. DB table `{}` is empty.", tableName);
        return;
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        uint32 summonerID = fields[0].Get<uint32>();
        auto summonerType = static_cast<SummonerType>(fields[1].Get<uint8>());
        uint8 group = fields[2].Get<uint8>();

        switch (summonerType)
        {
            case SUMMONER_TYPE_CREATURE:
                if (!GetCreatureTemplate(summonerID))
                {
                    LOG_ERROR("sql.sql", "Table `{}` has summoner with non existing entry {} for creature summoner type, skipped.", tableName, summonerID);
                    continue;
                }
                break;
            case SUMMONER_TYPE_GAMEOBJECT:
                if (!GetGameObjectTemplate(summonerID))
                {
                    LOG_ERROR("sql.sql", "Table `{}` has summoner with non existing entry {} for GameObject summoner type, skipped.", tableName, summonerID);
                    continue;
                }
                break;
            case SUMMONER_TYPE_MAP:
                if (!sMapStore.LookupEntry(summonerID))
                {
                    LOG_ERROR("sql.sql", "Table `{}` has summoner with non existing entry {} for map summoner type, skipped.", tableName, summonerID);
                    continue;
                }
                break;
            default:
                LOG_ERROR("sql.sql", "Table `{}` has unhandled summoner type {} for summoner {}, skipped.", tableName, summonerType, summonerID);
                continue;
        }

        GameObjectSummonData data;
        data.entry = fields[3].Get<uint32>();

        if (!GetGameObjectTemplate(data.entry))
        {
            LOG_ERROR("sql.sql", "Table `{}` has GameObject in group [Summoner ID: {}, Summoner Type: {}, Group ID: {}] with non existing GameObject entry {}, skipped.",
                tableName, summonerID, summonerType, group, data.entry);
            continue;
        }
        const auto position = fields[4].GetArray<float, 3>();
        const float orientation = fields[5].Get<float>();

        data.pos.Relocate(position[0], position[1], position[2], orientation);

        const auto rotation = fields[6].GetArray<float, 4>();

        data.rot = G3D::Quat(rotation[0], rotation[1], rotation[2], rotation[3]);
        data.respawnTime = fields[7].Get<uint32>();

        TempSummonGroupKey key(summonerID, summonerType, group);
        _goSummonDataStore[key].push_back(data);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} GameObject Summons in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadCreatures()
{
    const uint32 oldMSTime = getMSTime();

    const QueryResult result = WorldDatabase.Query(
        "SELECT world_creature.guid, id1, id2, id3, map, equipment, position, orientation, spawn_time_secs, wander_distance, waypoint, "
        "health, mana, movement_type, spawn_mask, phase_mask, event, pool, npc_flag, unit_flags, dynamic_flags, script_name "
        "FROM world_creature "
        "LEFT OUTER JOIN world_game_event_creature ON world_creature.guid = world_game_event_creature.guid "
        "LEFT OUTER JOIN world_pool_creature ON world_creature.guid = world_pool_creature.guid");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creatures. DB table `creature` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    if (sWorld->getBoolConfig(CONFIG_CALCULATE_CREATURE_ZONE_AREA_DATA))
        LOG_INFO("server.loading", "Calculating zone and area fields. This may take a moment...");

    // Build single time for check SpawnMask
    std::map<uint32, uint32> spawnMasks;
    for (uint32 i = 0; i < sMapStore.GetNumRows(); ++i)
        if (sMapStore.LookupEntry(i))
            for (int k = 0; k < MAX_DIFFICULTY; ++k)
                if (GetMapDifficultyData(i, static_cast<Difficulty>(k)))
                    spawnMasks[i] |= 1 << k;

    _creatureDataStore.rehash(result->GetRowCount());
    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        ObjectGuid::LowType spawnID = fields[0].Get<uint32>();
        uint32 id1 = fields[1].Get<uint32>();
        uint32 id2 = fields[2].Get<uint32>();
        uint32 id3 = fields[3].Get<uint32>();

        const CreatureTemplate* cInfo = GetCreatureTemplate(id1);
        if (!cInfo)
        {
            LOG_ERROR("sql.sql", "Table `world_creature` has creature (SpawnId: {}) with non existing creature entry {} in id1 field, skipped.", spawnID, id1);
            continue;
        }
        const CreatureTemplate* cInfo2 = GetCreatureTemplate(id2);
        if (!cInfo2 && id2)
        {
            LOG_ERROR("sql.sql", "Table `world_creature` has creature (SpawnId: {}) with non existing creature entry {} in id2 field, skipped.", spawnID, id2);
            continue;
        }
        const CreatureTemplate* cInfo3 = GetCreatureTemplate(id3);
        if (!cInfo3 && id3)
        {
            LOG_ERROR("sql.sql", "Table `world_creature` has creature (SpawnId: {}) with non existing creature entry {} in id3 field, skipped.", spawnID, id3);
            continue;
        }
        if (!id2 && id3)
        {
            LOG_ERROR("sql.sql", "Table `world_creature` has creature (SpawnId: {}) with creature entry {} in id3 field but no entry in id2 field, skipped.", spawnID, id3);
            continue;
        }
        CreatureData& data = _creatureDataStore[spawnID];
        data.id1 = id1;
        data.id2 = id2;
        data.id3 = id3;
        data.mapID = fields[4].Get<uint16>();
        data.equipmentID = fields[5].Get<int8>();
        const auto position = fields[6].GetArray<float, 3>();
        data.posX = position[0];
        data.posY = position[1];
        data.posZ = position[2];
        data.orientation = fields[7].Get<float>();
        data.spawnTimeSecs = fields[8].Get<uint32>();
        data.wanderDistance = fields[9].Get<float>();
        data.currentWaypoint = fields[10].Get<uint32>();
        data.curHealth = fields[11].Get<uint32>();
        data.curMana = fields[12].Get<uint32>();
        data.movementType = fields[13].Get<uint8>();
        data.spawnMask = fields[14].Get<uint8>();
        data.phaseMask = fields[15].Get<uint32>();
        const int16 gameEvent = fields[16].Get<int16>();
        const uint32 poolID = fields[17].Get<uint32>();
        data.npcFlag = fields[18].Get<uint32>();
        data.unitFlags = fields[19].Get<uint32>();
        data.dynamicFlags = fields[20].Get<uint32>();
        data.ScriptID = GetScriptID(fields[21].Get<std::string>());
        data.spawnGroupID = 0;

        if (!data.ScriptID)
            data.ScriptID = cInfo->ScriptID;

        const MapEntry* mapEntry = sMapStore.LookupEntry(data.mapID);
        if (!mapEntry)
        {
            LOG_ERROR("sql.sql", "Table `world_creature` have creature (SpawnId: {}) that spawned at not existed map (Id: {}), skipped.", spawnID, data.mapID);
            continue;
        }

        // 7 days means no respawn, so set it to 14 days, because manual id reset may be late
        if (mapEntry->IsRaid() && data.spawnTimeSecs >= 7 * DAY && data.spawnTimeSecs < 14 * DAY)
            data.spawnTimeSecs = 14 * DAY;

        // Skip spawnMask check for transport maps
        if (!_transportMaps.contains(data.mapID) && data.spawnMask & ~spawnMasks[data.mapID])
            LOG_ERROR("sql.sql", "Table `world_creature` have creature (SpawnId: {}) that have wrong spawn mask {} including not supported difficulty modes for map (Id: {}).",
                spawnID, data.spawnMask, data.mapID);

        bool ok = true;
        for (uint32 diff = 0; diff < MAX_DIFFICULTY - 1 && ok; ++diff)
        {
            if (_difficultyEntries[diff].contains(data.id1) || _difficultyEntries[diff].contains(data.id2) || _difficultyEntries[diff].contains(data.id3))
            {
                LOG_ERROR("sql.sql", "Table `world_creature` have creature (SpawnId: {}) that listed as difficulty {} template (Entries: {}, {}, {}) in `world_creature_template`, skipped.",
                    spawnID, diff + 1, data.id1, data.id2, data.id3);
                ok = false;
            }
        }
        if (!ok)
            continue;

        // -1 random, 0 no equipment,
        if (data.equipmentID != 0)
        {
            if (!GetEquipmentInfo(data.id1, data.equipmentID) || (data.id2 && !GetEquipmentInfo(data.id2, data.equipmentID))  || (data.id3 && !GetEquipmentInfo(data.id3, data.equipmentID)))
            {
                LOG_ERROR("sql.sql", "Table `world_creature` have creature (Entries: {}, {}, {}) one or more with equipment_id {} not found in table `world_creature_equip_template`, set to no equipment.",
                    data.id1, data.id2, data.id3, data.equipmentID);
                data.equipmentID = 0;
            }
        }
        if (cInfo->HasFlagsExtra(CREATURE_FLAG_EXTRA_INSTANCE_BIND) ||
            (data.id2 && cInfo2->HasFlagsExtra(CREATURE_FLAG_EXTRA_INSTANCE_BIND)) ||
            (data.id3 && cInfo3->HasFlagsExtra(CREATURE_FLAG_EXTRA_INSTANCE_BIND)))
        {
            if (!mapEntry->IsDungeon())
                LOG_ERROR("sql.sql", "Table `world_creature` have creature (SpawnId: {} Entries: {}, {}, {}) with a `world_creature_template`.`flags_extra` "
                                     "in one or more entries including CREATURE_FLAG_EXTRA_INSTANCE_BIND but creature are not in instance.", spawnID, data.id1, data.id2, data.id3);
        }
        if (data.movementType >= MAX_DB_MOTION_TYPE)
        {
            LOG_ERROR("sql.sql", "Table `world_creature` has creature (SpawnId: {} Entries: {}, {}, {}) with wrong movement generator type ({}), "
                                 "ignored and set to IDLE.", spawnID, data.id1, data.id2, data.id3, data.movementType);
            data.movementType = IDLE_MOTION_TYPE;
        }
        if (data.wanderDistance < 0.0f)
        {
            LOG_ERROR("sql.sql", "Table `world_creature` have creature (SpawnId: {} Entries: {}, {}, {}) with `wander_distance`< 0, set to 0.", spawnID, data.id1, data.id2, data.id3);
            data.wanderDistance = 0.0f;
        }
        else if (data.movementType == RANDOM_MOTION_TYPE)
        {
            if (data.wanderDistance == 0.0f)
            {
                LOG_ERROR("sql.sql", "Table `world_creature` have creature (SpawnId: {} Entries: {}, {}, {}) with `movement_type`=1 (random movement) "
                                     "but with `wander_distance`=0, replace by idle movement type (0).", spawnID, data.id1, data.id2, data.id3);
                data.movementType = IDLE_MOTION_TYPE;
            }
        }
        else if (data.movementType == IDLE_MOTION_TYPE)
        {
            if (data.wanderDistance != 0.0f)
            {
                LOG_ERROR("sql.sql", "Table `world_creature` have creature (SpawnId: {} Entries: {}, {}, {}) with "
                                     "`movement_type`=0 (idle) have `wander_distance`<>0, set to 0.", spawnID, data.id1, data.id2, data.id3);
                data.wanderDistance = 0.0f;
            }
        }

        if (data.phaseMask == 0)
        {
            LOG_ERROR("sql.sql", "Table `world_creature` have creature (SpawnId: {} Entries: {}, {}, {}) with "
                                 "`phase_mask`=0 (not visible for anyone), set to 1.", spawnID, data.id1, data.id2, data.id3);
            data.phaseMask = 1;
        }

        if (sWorld->getBoolConfig(CONFIG_CALCULATE_CREATURE_ZONE_AREA_DATA))
        {
            const uint32 zoneID = sMapMgr->GetZoneId(data.phaseMask, data.mapID, data.posX, data.posY, data.posZ);
            const uint32 areaID = sMapMgr->GetAreaId(data.phaseMask, data.mapID, data.posX, data.posY, data.posZ);

            WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_UPD_CREATURE_ZONE_AREA_DATA);

            stmt->SetData(0, zoneID);
            stmt->SetData(1, areaID);
            stmt->SetData(2, spawnID);

            WorldDatabase.Execute(stmt);
        }

        // Add to grid if not managed by the game event or pool system
        if (gameEvent == 0 && poolID == 0)
            AddCreatureToGrid(spawnID, &data);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Creatures in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

// Loads a single creature spawn from DB into the cache.
// Creature::LoadCreatureFromDB() reads from cache (GetCreatureData()), not from DB directly,
// so this must be called first for spawns not loaded at startup.
const CreatureData* ObjectMgr::LoadCreatureDataFromDB(ObjectGuid::LowType spawnID)
{
    if (const CreatureData* data = GetCreatureData(spawnID))
        return data;

    const QueryResult result = WorldDatabase.Query(
        "SELECT id1, id2, id3, map, equipment, position, orientation, spawn_time_secs, wander_distance, "
        "waypoint, health, mana, movement_type, spawn_mask, phase_mask, npc_flag, unit_flags, dynamic_flags, script_name "
        "FROM world_creature WHERE guid = {}", spawnID);
    const auto tableName = "world_creature";

    if (!result)
        return nullptr;

    const Field* fields = result->Fetch();
    uint32 id1 = fields[0].Get<uint32>();
    uint32 id2 = fields[1].Get<uint32>();
    uint32 id3 = fields[2].Get<uint32>();

    const CreatureTemplate* cInfo = GetCreatureTemplate(id1);
    if (!cInfo)
    {
        LOG_ERROR("sql.sql", "Table `{}` has creature (SpawnId: {}) with non-existing creature entry {} in id1 field, skipped.", tableName, spawnID, id1);
        return nullptr;
    }

    if (id2 && !GetCreatureTemplate(id2))
    {
        LOG_ERROR("sql.sql", "Table `{}` has creature (SpawnId: {}) with non-existing creature entry {} in id2 field, skipped.", tableName, spawnID, id2);
        return nullptr;
    }

    if (id3 && !GetCreatureTemplate(id3))
    {
        LOG_ERROR("sql.sql", "Table `{}` has creature (SpawnId: {}) with non-existing creature entry {} in id3 field, skipped.", tableName, spawnID, id3);
        return nullptr;
    }

    if (!id2 && id3)
    {
        LOG_ERROR("sql.sql", "Table `{}` has creature (SpawnId: {}) with creature entry {} in id3 field but no entry in id2 field, skipped.", tableName, spawnID, id3);
        return nullptr;
    }

    CreatureData& creatureData    = _creatureDataStore[spawnID];
    creatureData.id1              = id1;
    creatureData.id2              = id2;
    creatureData.id3              = id3;
    creatureData.mapID            = fields[3].Get<uint16>();
    creatureData.equipmentID      = fields[4].Get<int8>();
    const auto position = fields[5].GetArray<float, 3>();
    creatureData.posX             = position[0];
    creatureData.posY             = position[1];
    creatureData.posZ             = position[2];
    creatureData.orientation      = fields[6].Get<float>();
    creatureData.spawnTimeSecs    = fields[7].Get<uint32>();
    creatureData.wanderDistance   = fields[8].Get<float>();
    creatureData.currentWaypoint  = fields[9].Get<uint32>();
    creatureData.curHealth        = fields[10].Get<uint32>();
    creatureData.curMana          = fields[11].Get<uint32>();
    creatureData.movementType     = fields[12].Get<uint8>();
    creatureData.spawnMask        = fields[13].Get<uint8>();
    creatureData.phaseMask        = fields[14].Get<uint32>();
    creatureData.npcFlag          = fields[15].Get<uint32>();
    creatureData.unitFlags        = fields[16].Get<uint32>();
    creatureData.dynamicFlags     = fields[17].Get<uint32>();
    creatureData.ScriptID         = GetScriptID(fields[18].Get<std::string>());
    creatureData.spawnGroupID     = 0;

    if (!creatureData.ScriptID)
        creatureData.ScriptID = cInfo->ScriptID;

    const MapEntry* mapEntry = sMapStore.LookupEntry(creatureData.mapID);
    if (!mapEntry)
    {
        LOG_ERROR("sql.sql", "Table `{}` has creature (SpawnId: {}) that spawned at non-existing map (Id: {}), skipped.", tableName, spawnID, creatureData.mapID);
        _creatureDataStore.erase(spawnID);
        return nullptr;
    }

    if (mapEntry->IsRaid() && creatureData.spawnTimeSecs >= 7 * DAY && creatureData.spawnTimeSecs < 14 * DAY)
        creatureData.spawnTimeSecs = 14 * DAY;

    bool ok = true;
    for (uint32 diff = 0; diff < MAX_DIFFICULTY - 1 && ok; ++diff)
    {
        if (_difficultyEntries[diff].contains(id1) || _difficultyEntries[diff].contains(id2) || _difficultyEntries[diff].contains(id3))
        {
            LOG_ERROR("sql.sql", "Table `{}` has creature (SpawnId: {}) that is listed as difficulty {} template (Entries: {}, {}, {}) in `world_creature_template`, skipped.",
                tableName, spawnID, diff + 1, id1, id2, id3);
            ok = false;
        }
    }

    if (!ok)
    {
        _creatureDataStore.erase(spawnID);
        return nullptr;
    }

    if (creatureData.equipmentID != 0)
    {
        if (!GetEquipmentInfo(id1, creatureData.equipmentID) ||
            (id2 && !GetEquipmentInfo(id2, creatureData.equipmentID)) ||
            (id3 && !GetEquipmentInfo(id3, creatureData.equipmentID)))
        {
            LOG_ERROR("sql.sql", "Table `{}` has creature (Entries: {}, {}, {}) with equipment {} not found in table `world_creature_equip_template`, set to no equipment.",
                tableName, id1, id2, id3, creatureData.equipmentID);
            creatureData.equipmentID = 0;
        }
    }

    if (creatureData.movementType >= MAX_DB_MOTION_TYPE)
    {
        LOG_ERROR("sql.sql", "Table `{}` has creature (SpawnId: {} Entries: {}, {}, {}) with wrong movement generator type ({}), set to IDLE.",
            tableName, spawnID, id1, id2, id3, creatureData.movementType);
        creatureData.movementType = IDLE_MOTION_TYPE;
    }

    if (creatureData.wanderDistance < 0.0f)
    {
        LOG_ERROR("sql.sql", "Table `{}` has creature (SpawnId: {} Entries: {}, {}, {}) with `wander_distance`< 0, set to 0.", tableName, spawnID, id1, id2, id3);
        creatureData.wanderDistance = 0.0f;
    }
    else if (creatureData.movementType == RANDOM_MOTION_TYPE)
    {
        if (creatureData.wanderDistance == 0.0f)
        {
            LOG_ERROR("sql.sql", "Table `{}` has creature (SpawnId: {} Entries: {}, {}, {}) with `movement_type`=1 (random movement) but with `wander_distance`=0, replace by idle movement type (0).",
                tableName, spawnID, id1, id2, id3);
            creatureData.movementType = IDLE_MOTION_TYPE;
        }
    }
    else if (creatureData.movementType == IDLE_MOTION_TYPE)
    {
        if (creatureData.wanderDistance != 0.0f)
        {
            LOG_ERROR("sql.sql", "Table `{}` has creature (SpawnId: {} Entries: {}, {}, {}) with `movement_type`=0 (idle) have `wander_distance`<>0, set to 0.", tableName, spawnID, id1, id2, id3);
            creatureData.wanderDistance = 0.0f;
        }
    }

    if (creatureData.phaseMask == 0)
    {
        LOG_ERROR("sql.sql", "Table `{}` has creature (SpawnId: {} Entries: {}, {}, {}) with `phase_mask`=0 (not visible for anyone), set to 1.", tableName, spawnID, id1, id2, id3);
        creatureData.phaseMask = 1;
    }

    return &creatureData;
}

void ObjectMgr::LoadCreatureSparring()
{
    const uint32 oldMSTime = getMSTime();

    const QueryResult result = WorldDatabase.Query("SELECT guid, sparring_pct FROM world_creature_sparring");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 sparring data. DB table `world_creature_sparring` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        ObjectGuid::LowType spawnID = fields[0].Get<uint32>();
        float sparringHealthPct = fields[1].Get<float>();

        if (!GetCreatureData(spawnID))
        {
            LOG_ERROR("sql.sql", "Entry {} has a record in `world_creature_sparring` but doesn't exist in `world_creature` table", spawnID);
            continue;
        }
        _creatureSparringStore[spawnID].push_back(sparringHealthPct);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} sparring data in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::AddCreatureToGrid(const ObjectGuid::LowType guid, const CreatureData* data)
{
    uint8 mask = data->spawnMask;
    for (uint8 i = 0; mask != 0; i++, mask >>= 1)
    {
        if (mask & 1)
        {
            GridCoord gridCoord = Acore::ComputeGridCoord(data->posX, data->posY);
            CellObjectGuids& cellGUIDs = _mapObjectGuidsStore[MAKE_PAIR32(data->mapID, i)][gridCoord.GetId()];
            cellGUIDs.creatures.insert(guid);
        }
    }
}

void ObjectMgr::RemoveCreatureFromGrid(const ObjectGuid::LowType guid, const CreatureData* data)
{
    uint8 mask = data->spawnMask;
    for (uint8 i = 0; mask != 0; i++, mask >>= 1)
    {
        if (mask & 1)
        {
            GridCoord gridCoord = Acore::ComputeGridCoord(data->posX, data->posY);
            CellObjectGuids& cellGUIDs = _mapObjectGuidsStore[MAKE_PAIR32(data->mapID, i)][gridCoord.GetId()];
            cellGUIDs.creatures.erase(guid);
        }
    }
}

ObjectGuid::LowType ObjectMgr::AddGOData(uint32 entry, const uint32 mapID,
    const float x, const float y, const float z, const float o, const uint32 spawnTimeDelay,
    const float rotation0, const float rotation1, const float rotation2, const float rotation3)
{
    const GameObjectTemplate* goInfo = GetGameObjectTemplate(entry);
    if (!goInfo)
        return 0;

    Map* map = sMapMgr->CreateBaseMap(mapID);
    if (!map)
        return 0;

    ObjectGuid::LowType spawnId = GenerateGameObjectSpawnID();

    GameObjectData& data = NewGOData(spawnId);
    data.id             = entry;
    data.mapID          = mapID;
    data.posX           = x;
    data.posY           = y;
    data.posZ           = z;
    data.orientation    = o;
    data.rotation.x     = rotation0;
    data.rotation.y     = rotation1;
    data.rotation.z     = rotation2;
    data.rotation.w     = rotation3;
    data.spawnTimeSecs  = static_cast<int32>(spawnTimeDelay);
    data.animProgress   = 100;
    data.spawnMask      = 1;
    data.goState       = GO_STATE_READY;
    data.phaseMask      = PHASEMASK_NORMAL;
    data.artKit         = goInfo->Type == GAME_OBJECT_TYPE_CAPTURE_POINT ? 21 : 0;
    data.dbData = false;
    data.spawnGroupID   = 0;

    AddGameObjectToGrid(spawnId, &data);

    // Spawn if necessary (loaded grids only)
    // We use spawn coords to spawn
    if (!map->Instanceable() && map->IsGridLoaded(x, y))
    {
        GameObject* go = IsGameObjectStaticTransport(data.id) ? new StaticTransport() : new GameObject();
        if (!go->LoadGameObjectFromDB(spawnId, map))
        {
            LOG_ERROR("sql.sql", "AddGOData: cannot add GameObject entry {} to map", entry);
            delete go;
            return 0;
        }
    }

    LOG_DEBUG("maps", "AddGOData: spawnId {} entry {} map {} x {} y {} z {} o {}", spawnId, entry, mapID, x, y, z, o);

    return spawnId;
}

ObjectGuid::LowType ObjectMgr::AddCreData(uint32 entry, const uint32 mapID, const float x, const float y, const float z, const float o, const uint32 spawnTimeDelay)
{
    const CreatureTemplate* cInfo = GetCreatureTemplate(entry);
    if (!cInfo)
        return 0;

    // Only used for extracting creature base stats
    const uint32 level = cInfo->LevelMin == cInfo->MaxLevel ? cInfo->LevelMin : urand(cInfo->LevelMin, cInfo->MaxLevel);

    const CreatureBaseStats* stats = GetCreatureBaseStats(level, cInfo->UnitClass);
    Map* map = sMapMgr->CreateBaseMap(mapID);
    if (!map)
        return 0;

    const ObjectGuid::LowType spawnID = GenerateCreatureSpawnID();
    CreatureData& data = NewOrExistCreatureData(spawnID);
    data.spawnMask = spawnID;
    data.id1 = entry;
    data.id2 = 0;
    data.id3 = 0;
    data.mapID = mapID;
    data.displayid = 0;
    data.equipmentID = 0;
    data.posX = x;
    data.posY = y;
    data.posZ = z;
    data.orientation = o;
    data.spawnTimeSecs = spawnTimeDelay;
    data.wanderDistance = 0;
    data.currentWaypoint = 0;
    data.curHealth = stats->GenerateHealth(cInfo);
    data.curMana = stats->GenerateMana(cInfo);
    data.movementType = cInfo->MovementType;
    data.spawnMask = 1;
    data.phaseMask = PHASEMASK_NORMAL;
    data.dbData = false;
    data.npcFlag = cInfo->FlagNPC;
    data.unitFlags = cInfo->UnitFlags;
    data.dynamicFlags = cInfo->DynamicFlags;

    AddCreatureToGrid(spawnID, &data);

    // Spawn if necessary (loaded grids only)
    if (!map->Instanceable() && map->IsGridLoaded(x, y))
    {
        if (const auto creature = new Creature(); !creature->LoadCreatureFromDB(spawnID, map, true, true))
        {
            LOG_ERROR("sql.sql", "AddCreature: Cannot add creature entry {} to map", entry);
            delete creature;
            return 0;
        }
    }

    return spawnID;
}

void ObjectMgr::LoadGameObjects()
{
    const uint32 oldMSTime = getMSTime();

    const QueryResult result = WorldDatabase.Query(
        "SELECT go.guid, go.id, map, position, orientation, rotation, spawn_time, anim_progress, state, spawn_mask, phase_mask, event, pool, script_name "
        "FROM world_game_object go"
        "LEFT OUTER JOIN world_game_event_game_object ON go.guid = world_game_event_game_object.guid "
        "LEFT OUTER JOIN world_pool_game_object ON go.guid = world_pool_game_object.guid");
    const auto tableName = "world_game_object";

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 GameObjects. DB table `{}` is empty.", tableName);
        LOG_INFO("server.loading", " ");
        return;
    }

    if (sWorld->getBoolConfig(CONFIG_CALCULATE_GAMEOBJECT_ZONE_AREA_DATA))
        LOG_INFO("server.loading", "Calculating zone and area fields. This may take a moment...");

    // Build single time for check spawn_mask
    std::map<uint32, uint32> spawnMasks;
    for (uint32 i = 0; i < sMapStore.GetNumRows(); ++i)
        if (sMapStore.LookupEntry(i))
            for (int k = 0; k < MAX_DIFFICULTY; ++k)
                if (GetMapDifficultyData(i, static_cast<Difficulty>(k)))
                    spawnMasks[i] |= 1 << k;

    _gameObjectDataStore.rehash(result->GetRowCount());
    do
    {
        const Field* fields = result->Fetch();

        ObjectGuid::LowType guid = fields[0].Get<uint32>();
        uint32 entry = fields[1].Get<uint32>();

        const GameObjectTemplate* gInfo = GetGameObjectTemplate(entry);
        if (!gInfo)
        {
            LOG_ERROR("sql.sql", "Table `{}` has GameObject (GUID: {}) with non existing GameObject entry {}, skipped.", tableName, guid, entry);
            continue;
        }

        if (!gInfo->DisplayID)
        {
            switch (gInfo->Type)
            {
                case GAME_OBJECT_TYPE_TRAP:
                case GAME_OBJECT_TYPE_SPELL_FOCUS:
                    break;
                default:
                    LOG_ERROR("sql.sql", "GameObject (GUID: {} Entry {} GoType: {}) doesn't have a displayID ({}), not loaded.", guid, entry, gInfo->Type, gInfo->DisplayID);
                    break;
            }
        }

        if (gInfo->DisplayID && !sGameObjectDisplayInfoStore.LookupEntry(gInfo->DisplayID))
        {
            LOG_ERROR("sql.sql", "GameObject (GUID: {} Entry {} GoType: {}) has an invalid displayID ({}), not loaded.", guid, entry, gInfo->Type, gInfo->DisplayID);
            continue;
        }

        GameObjectData& data = _gameObjectDataStore[guid];

        data.id = entry;
        data.mapID = fields[2].Get<uint16>();
        const auto position = fields[3].GetArray<float, 3>();
        data.posX = position[0];
        data.posY = position[1];
        data.posZ = position[2];
        data.orientation = fields[4].Get<float>();
        const auto rotation = fields[5].GetArray<float, 4>();
        data.rotation.x = rotation[0];
        data.rotation.y = rotation[1];
        data.rotation.z = rotation[2];
        data.rotation.w = rotation[3];
        data.spawnTimeSecs = fields[6].Get<int32>();
        data.ScriptID = GetScriptID(fields[13].Get<std::string>());
        data.spawnGroupID = 0;
        if (!data.ScriptID)
            data.ScriptID = gInfo->ScriptID;

        if (!sMapStore.LookupEntry(data.mapID))
        {
            LOG_ERROR("sql.sql", "Table `{}` has GameObject (GUID: {} Entry: {}) spawned on a non-existed map (Id: {}), skip", tableName, guid, data.id, data.mapID);
            continue;
        }

        if (data.spawnTimeSecs == 0 && gInfo->IsDespawnAtAction())
        {
            LOG_ERROR("sql.sql", "Table `{}` has GameObject (GUID: {} Entry: {}) with `spawn_time` (0) value, but the GameObject is marked as despawn-able at action.", tableName, guid, data.id);
        }

        data.animProgress = fields[7].Get<uint8>();
        data.artKit = 0;

        uint32 goState = fields[8].Get<uint8>();
        if (goState >= MAX_GO_STATE)
        {
            LOG_ERROR("sql.sql", "Table `{}` has GameObject (GUID: {} Entry: {}) with invalid `state` ({}) value, skip", tableName, guid, data.id, goState);
            continue;
        }
        data.goState = static_cast<GOState>(goState);

        data.spawnMask = fields[9].Get<uint8>();

        if (!_transportMaps.contains(data.mapID) && data.spawnMask & ~spawnMasks[data.mapID])
            LOG_ERROR("sql.sql", "Table `{}` has GameObject (GUID: {} Entry: {}) that has wrong spawn mask {} including not supported difficulty modes for map (Id: {}), skip", tableName, guid, data.id, data.spawnMask, data.mapID);

        data.phaseMask = fields[10].Get<uint32>();
        const int16 gameEvent = fields[11].Get<int16>();
        const uint32 poolID = fields[12].Get<uint32>();

        if (data.rotation.x < -1.0f || data.rotation.x > 1.0f)
        {
            LOG_ERROR("sql.sql", "Table `{}` has GameObject (GUID: {} Entry: {}) with invalid rotationX ({}) value, skip", tableName, guid, data.id, data.rotation.x);
            continue;
        }

        if (data.rotation.y < -1.0f || data.rotation.y > 1.0f)
        {
            LOG_ERROR("sql.sql", "Table `{}` has GameObject (GUID: {} Entry: {}) with invalid rotationY ({}) value, skip", tableName, guid, data.id, data.rotation.y);
            continue;
        }

        if (data.rotation.z < -1.0f || data.rotation.z > 1.0f)
        {
            LOG_ERROR("sql.sql", "Table `{}` has GameObject (GUID: {} Entry: {}) with invalid rotationZ ({}) value, skip", tableName, guid, data.id, data.rotation.z);
            continue;
        }

        if (data.rotation.w < -1.0f || data.rotation.w > 1.0f)
        {
            LOG_ERROR("sql.sql", "Table `{}` has GameObject (GUID: {} Entry: {}) with invalid rotationW ({}) value, skip", tableName, guid, data.id, data.rotation.w);
            continue;
        }

        if (fabs(data.rotation.x * data.rotation.x + data.rotation.y * data.rotation.y + data.rotation.z * data.rotation.z + data.rotation.w * data.rotation.w - 1.0f) >= 1e-5f)
        {
            LOG_ERROR("sql.sql", "Table `{}` has GameObject (GUID: {} Entry: {}) with invalid rotation quaternion (non-unit), defaulting to orientation on Z axis only", tableName, guid, data.id);
            data.rotation = G3D::Quat(G3D::Matrix3::fromEulerAnglesZYX(data.orientation, 0.0f, 0.0f));
        }

        if (!MapMgr::IsValidMapCoord(data.mapID, data.posX, data.posY, data.posZ, data.orientation))
        {
            LOG_ERROR("sql.sql", "Table `{}` has GameObject (GUID: {} Entry: {}) with invalid coordinates, skip", tableName, guid, data.id);
            continue;
        }

        if (data.phaseMask == 0)
        {
            LOG_ERROR("sql.sql", "Table `{}` has GameObject (GUID: {} Entry: {}) with `phaseMask`=0 (not visible for anyone), set to 1.", tableName, guid, data.id);
            data.phaseMask = 1;
        }

        if (sWorld->getBoolConfig(CONFIG_CALCULATE_GAMEOBJECT_ZONE_AREA_DATA))
        {
            const uint32 zoneID = sMapMgr->GetZoneId(data.phaseMask, data.mapID, data.posX, data.posY, data.posZ);
            const uint32 areaID = sMapMgr->GetAreaId(data.phaseMask, data.mapID, data.posX, data.posY, data.posZ);

            WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_UPD_GAMEOBJECT_ZONE_AREA_DATA);

            stmt->SetData(0, zoneID);
            stmt->SetData(1, areaID);
            stmt->SetData(2, guid);

            WorldDatabase.Execute(stmt);
        }

        // If not this is to be managed by GameEvent System or Pool system
        if (gameEvent == 0 && poolID == 0)
            AddGameObjectToGrid(guid, &data);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} GameObjects in {} ms", _gameObjectDataStore.size(), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

// Loads a single GameObject spawn from DB into the cache.
// GameObject::LoadGameObjectFromDB() reads from cache (GetGameObjectData()), not from DB directly,
// so this must be called first for spawns not loaded at startup.
const GameObjectData* ObjectMgr::LoadGameObjectDataFromDB(ObjectGuid::LowType spawnID)
{
    if (const GameObjectData* data = GetGameObjectData(spawnID))
        return data;

    const QueryResult result = WorldDatabase.Query("SELECT id, map, position, orientation, rotation, spawn_time, anim_progress, state, spawn_mask, phase_mask, script_name "
                                                   "FROM world_game_object WHERE guid=$1", spawnID);
    const auto tableName = "world_game_object";

    if (!result)
        return nullptr;

    const Field* fields = result->Fetch();
    uint32 entry = fields[0].Get<uint32>();

    const GameObjectTemplate* gInfo = GetGameObjectTemplate(entry);
    if (!gInfo)
    {
        LOG_ERROR("sql.sql", "Table `{}` has GameObject (GUID: {}) with non-existing GameObject entry {}, skipped.", tableName, spawnID, entry);
        return nullptr;
    }

    if (gInfo->DisplayID && !sGameObjectDisplayInfoStore.LookupEntry(gInfo->DisplayID))
    {
        LOG_ERROR("sql.sql", "Table `{}` has GameObject (GUID: {} Entry {} GoType: {}) with an invalid displayId ({}), not loaded.", tableName, spawnID, entry, gInfo->Type, gInfo->DisplayID);
        return nullptr;
    }

    GameObjectData& goData = _gameObjectDataStore[spawnID];
    goData.id = entry;
    goData.mapID = fields[1].Get<uint16>();
    const auto position = fields[2].GetArray<float, 3>();
    goData.posX = position[0];
    goData.posY = position[1];
    goData.posZ = position[2];
    goData.orientation = fields[3].Get<float>();
    const auto rotation = fields[4].GetArray<float, 3>();
    goData.rotation.x = rotation[0];
    goData.rotation.y = rotation[1];
    goData.rotation.z = rotation[2];
    goData.rotation.w = rotation[3];
    goData.spawnTimeSecs = fields[5].Get<int32>();
    goData.animProgress = fields[6].Get<uint8>();
    goData.artKit = 0;
    goData.ScriptID = GetScriptID(fields[10].Get<std::string>());

    if (!goData.ScriptID)
        goData.ScriptID = gInfo->ScriptID;

    if (!sMapStore.LookupEntry(goData.mapID))
    {
        LOG_ERROR("sql.sql", "Table `{}` has GameObject (GUID: {} Entry: {}) spawned on a non-existing map (Id: {}), skipped.", tableName, spawnID, entry, goData.mapID);
        _gameObjectDataStore.erase(spawnID);
        return nullptr;
    }

    if (goData.spawnTimeSecs == 0 && gInfo->IsDespawnAtAction())
    {
        LOG_ERROR("sql.sql", "Table `{}` has GameObject (GUID: {} Entry: {}) with `spawn_time` (0) value, but the GameObject is marked as despawn-able at action.", tableName, spawnID, entry);
    }

    uint32 goState = fields[7].Get<uint8>();
    if (goState >= MAX_GO_STATE)
    {
        LOG_ERROR("sql.sql", "Table `{}` has GameObject (GUID: {} Entry: {}) with invalid `state` ({}) value, skipped.", tableName, spawnID, entry, goState);
        _gameObjectDataStore.erase(spawnID);
        return nullptr;
    }
    goData.goState = static_cast<GOState>(goState);
    goData.spawnMask = fields[8].Get<uint8>();
    goData.phaseMask = fields[9].Get<uint32>();

    if (goData.rotation.x < -1.0f || goData.rotation.x > 1.0f)
    {
        LOG_ERROR("sql.sql", "Table `{}` has GameObject (GUID: {} Entry: {}) with invalid rotationX ({}) value, skipped.", tableName, spawnID, entry, goData.rotation.x);
        _gameObjectDataStore.erase(spawnID);
        return nullptr;
    }

    if (goData.rotation.y < -1.0f || goData.rotation.y > 1.0f)
    {
        LOG_ERROR("sql.sql", "Table `{}` has GameObject (GUID: {} Entry: {}) with invalid rotationY ({}) value, skipped.", tableName, spawnID, entry, goData.rotation.y);
        _gameObjectDataStore.erase(spawnID);
        return nullptr;
    }

    if (goData.rotation.z < -1.0f || goData.rotation.z > 1.0f)
    {
        LOG_ERROR("sql.sql", "Table `{}` has GameObject (GUID: {} Entry: {}) with invalid rotationZ ({}) value, skipped.", tableName, spawnID, entry, goData.rotation.z);
        _gameObjectDataStore.erase(spawnID);
        return nullptr;
    }

    if (goData.rotation.w < -1.0f || goData.rotation.w > 1.0f)
    {
        LOG_ERROR("sql.sql", "Table `{}` has GameObject (GUID: {} Entry: {}) with invalid rotationW ({}) value, skipped.", tableName, spawnID, entry, goData.rotation.w);
        _gameObjectDataStore.erase(spawnID);
        return nullptr;
    }

    if (fabs(goData.rotation.x * goData.rotation.x + goData.rotation.y * goData.rotation.y + goData.rotation.z * goData.rotation.z + goData.rotation.w * goData.rotation.w - 1.0f) >= 1e-5f)
    {
        LOG_ERROR("sql.sql", "Table `{}` has GameObject (GUID: {} Entry: {}) with invalid rotation quaternion (non-unit), defaulting to orientation on Z axis only", tableName, spawnID, entry);
        goData.rotation = G3D::Quat(G3D::Matrix3::fromEulerAnglesZYX(goData.orientation, 0.0f, 0.0f));
    }

    if (!MapMgr::IsValidMapCoord(goData.mapID, goData.posX, goData.posY, goData.posZ, goData.orientation))
    {
        LOG_ERROR("sql.sql", "Table `{}` has GameObject (GUID: {} Entry: {}) with invalid coordinates, skipped.", tableName, spawnID, entry);
        _gameObjectDataStore.erase(spawnID);
        return nullptr;
    }

    if (goData.phaseMask == 0)
    {
        LOG_ERROR("sql.sql", "Table `{}` has GameObject (GUID: {} Entry: {}) with `phase_mask`=0 (not visible for anyone), set to 1.", tableName, spawnID, entry);
        goData.phaseMask = 1;
    }

    return &goData;
}

void ObjectMgr::AddGameObjectToGrid(const ObjectGuid::LowType guid, const GameObjectData* data)
{
    uint8 mask = data->spawnMask;
    for (uint8 i = 0; mask != 0; i++, mask >>= 1)
    {
        if (mask & 1)
        {
            GridCoord gridCoord = Acore::ComputeGridCoord(data->posX, data->posY);
            CellObjectGuids& cell_guids = _mapObjectGuidsStore[MAKE_PAIR32(data->mapID, i)][gridCoord.GetId()];
            cell_guids.gameObjects.insert(guid);
        }
    }
}

void ObjectMgr::RemoveGameObjectFromGrid(const ObjectGuid::LowType guid, const GameObjectData* data)
{
    uint8 mask = data->spawnMask;
    for (uint8 i = 0; mask != 0; i++, mask >>= 1)
    {
        if (mask & 1)
        {
            GridCoord gridCoord = Acore::ComputeGridCoord(data->posX, data->posY);
            CellObjectGuids& cell_guids = _mapObjectGuidsStore[MAKE_PAIR32(data->mapID, i)][gridCoord.GetId()];
            cell_guids.gameObjects.erase(guid);
        }
    }
}

constexpr ServerConfigs qualityToBuyValueConfig[MAX_ITEM_QUALITY] =
{
    RATE_BUYVALUE_ITEM_POOR,                                    // ITEM_QUALITY_POOR
    RATE_BUYVALUE_ITEM_NORMAL,                                  // ITEM_QUALITY_NORMAL
    RATE_BUYVALUE_ITEM_UNCOMMON,                                // ITEM_QUALITY_UNCOMMON
    RATE_BUYVALUE_ITEM_RARE,                                    // ITEM_QUALITY_RARE
    RATE_BUYVALUE_ITEM_EPIC,                                    // ITEM_QUALITY_EPIC
    RATE_BUYVALUE_ITEM_LEGENDARY,                               // ITEM_QUALITY_LEGENDARY
    RATE_BUYVALUE_ITEM_ARTIFACT,                                // ITEM_QUALITY_ARTIFACT
    RATE_BUYVALUE_ITEM_HEIRLOOM,                                // ITEM_QUALITY_HEIRLOOM
};

constexpr ServerConfigs qualityToSellValueConfig[MAX_ITEM_QUALITY] =
{
    RATE_SELLVALUE_ITEM_POOR,                                   // ITEM_QUALITY_POOR
    RATE_SELLVALUE_ITEM_NORMAL,                                 // ITEM_QUALITY_NORMAL
    RATE_SELLVALUE_ITEM_UNCOMMON,                               // ITEM_QUALITY_UNCOMMON
    RATE_SELLVALUE_ITEM_RARE,                                   // ITEM_QUALITY_RARE
    RATE_SELLVALUE_ITEM_EPIC,                                   // ITEM_QUALITY_EPIC
    RATE_SELLVALUE_ITEM_LEGENDARY,                              // ITEM_QUALITY_LEGENDARY
    RATE_SELLVALUE_ITEM_ARTIFACT,                               // ITEM_QUALITY_ARTIFACT
    RATE_SELLVALUE_ITEM_HEIRLOOM,                               // ITEM_QUALITY_HEIRLOOM
};

void ObjectMgr::LoadItemTemplates()
{
    const uint32 oldMSTime = getMSTime();
    const QueryResult result = WorldDatabase.Query(
        "SELECT id, class, subclass, sound_override_subclass, name, display, quality, flags, flags_extra, buy_count, buy_price, sell_price, "
        "inventory_type, allowable_class, allowable_race, item_level, required_level, required_skill, required_skill_rank, required_spell, "
        "required_honor_rank, required_city_rank, required_reputation_faction, required_reputation_rank, max_count, stackable, container_slots, "
        "stat_type, stat_value, scaling_stat_distribution, scaling_stat_value, damage_min1, damage_max1, damage_type1, damage_min2, damage_max2, damage_type2, "
        "armor, holy_res, fire_res, nature_res, frost_res, shadow_res, arcane_res, delay, ammo_type, ranged_mod_range, "
        "spell, spell_trigger, spell_charges, spell_ppm_rate, spell_cooldown, spell_category, spell_category_cooldown, "
        "bonding, description, page_text, language, page_material, start_quest, lock, material, sheath, "
        "random_property, random_suffix, block, item_set, durability, area, map, bag_family, totem_category, "
        "socket_color, socket_content, socket_bonus, gem_properties, disenchant, disenchant_skill, "
        "armor_damage_modifier, duration, item_limit_category, holiday, food_type, min_money_loot, max_money_loot, flags_custom, script_name "
        "FROM world_item_template");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 item templates. DB table `item_template` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    _itemTemplateStore.reserve(result->GetRowCount());
    uint32 count = 0;
    bool enforceDBCAttributes = sWorld->getBoolConfig(CONFIG_DBC_ENFORCE_ITEM_ATTRIBUTES);

    do
    {
        const Field* fields = result->Fetch();

        uint32 entry = fields[0].Get<uint32>();

        ItemTemplate& itemTemplate = _itemTemplateStore[entry];

        itemTemplate.ItemId                    = entry;
        itemTemplate.Class                     = fields[1].Get<uint32>();
        itemTemplate.SubClass                  = fields[2].Get<uint32>();
        itemTemplate.SoundOverrideSubclass     = fields[3].Get<int32>();
        itemTemplate.Name1                     = fields[4].Get<std::string>();
        itemTemplate.DisplayInfoID             = fields[5].Get<uint32>();
        itemTemplate.Quality                   = fields[6].Get<uint32>();
        itemTemplate.Flags                     = static_cast<ItemFlags>(fields[7].Get<uint32>());
        itemTemplate.Flags2                    = static_cast<ItemFlags2>(fields[8].Get<uint32>());
        itemTemplate.BuyCount                  = fields[9].Get<uint32>();
        itemTemplate.BuyPrice                  = fields[10].Get<int32>();
        itemTemplate.SellPrice                 = fields[11].Get<uint32>();
        itemTemplate.InventoryType             = fields[12].Get<uint32>();
        itemTemplate.AllowableClass            = fields[13].Get<int32>();
        itemTemplate.AllowableRace             = fields[14].Get<int32>();
        itemTemplate.ItemLevel                 = fields[15].Get<uint32>();
        itemTemplate.RequiredLevel             = fields[16].Get<uint32>();
        itemTemplate.RequiredSkill             = fields[17].Get<uint32>();
        itemTemplate.RequiredSkillRank         = fields[18].Get<uint32>();
        itemTemplate.RequiredSpell             = fields[19].Get<uint32>();
        itemTemplate.RequiredHonorRank         = fields[20].Get<uint32>();
        itemTemplate.RequiredCityRank          = fields[21].Get<uint32>();
        itemTemplate.RequiredReputationFaction = fields[22].Get<uint32>();
        itemTemplate.RequiredReputationRank    = fields[23].Get<uint32>();
        itemTemplate.MaxCount                  = fields[24].Get<int32>();
        itemTemplate.Stackable                 = fields[25].Get<int32>();
        itemTemplate.ContainerSlots            = fields[26].Get<uint32>();

        const auto statTypes = fields[27].GetArray<uint32, MAX_ITEM_PROTO_STATS>();
        const auto statValues = fields[28].GetArray<int32, MAX_ITEM_PROTO_STATS>();

        uint8 statsCount = 0;
        while (statsCount < MAX_ITEM_PROTO_STATS)
        {
            uint32 statType = statTypes[statsCount];
            if (statType == 0)
                break;
            itemTemplate.ItemStat[statsCount].ItemStatType = statType;
            itemTemplate.ItemStat[statsCount].ItemStatValue = statValues[statsCount];
            statsCount++;
        }
        itemTemplate.StatsCount = statsCount;

        itemTemplate.ScalingStatDistribution = fields[29].Get<uint32>();
        itemTemplate.ScalingStatValue        = fields[30].Get<int32>();

        for (uint8 i = 0; i < MAX_ITEM_PROTO_DAMAGES; ++i)
        {
            itemTemplate.Damage[i].DamageMin  = fields[31 + i * 3].Get<float>();
            itemTemplate.Damage[i].DamageMax  = fields[32 + i * 3].Get<float>();
            itemTemplate.Damage[i].DamageType = fields[33 + i * 3].Get<uint32>();
        }

        itemTemplate.Armor          = fields[37].Get<uint32>();
        itemTemplate.HolyRes        = fields[38].Get<int32>();
        itemTemplate.FireRes        = fields[39].Get<int32>();
        itemTemplate.NatureRes      = fields[40].Get<int32>();
        itemTemplate.FrostRes       = fields[41].Get<int32>();
        itemTemplate.ShadowRes      = fields[42].Get<int32>();
        itemTemplate.ArcaneRes      = fields[43].Get<int32>();
        itemTemplate.Delay          = fields[44].Get<uint32>();
        itemTemplate.AmmoType       = fields[45].Get<uint32>();
        itemTemplate.RangedModRange = fields[46].Get<float>();

        const auto spellIDs               = fields[47].GetArray<int32, MAX_ITEM_PROTO_SPELLS>();
        const auto spellTriggers      = fields[48].GetArray<uint32, MAX_ITEM_PROTO_SPELLS>();
        const auto spellCharges           = fields[49].GetArray<int32, MAX_ITEM_PROTO_SPELLS>();
        const auto spellPPMRates         = fields[50].GetArray<float, MAX_ITEM_PROTO_SPELLS>();
        const auto spellCooldowns         = fields[51].GetArray<int32, MAX_ITEM_PROTO_SPELLS>();
        const auto spellCategories    = fields[52].GetArray<uint32, MAX_ITEM_PROTO_SPELLS>();
        const auto spellCategoryCooldowns = fields[53].GetArray<int32, MAX_ITEM_PROTO_SPELLS>();

        for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
        {
            itemTemplate.Spells[i].SpellId               = spellIDs[i];
            itemTemplate.Spells[i].SpellTrigger          = spellTriggers[i];
            itemTemplate.Spells[i].SpellCharges          = spellCharges[i];
            itemTemplate.Spells[i].SpellPPMRate          = spellPPMRates[i];
            itemTemplate.Spells[i].SpellCooldown         = spellCooldowns[i];
            itemTemplate.Spells[i].SpellCategory         = spellCategories[i];
            itemTemplate.Spells[i].SpellCategoryCooldown = spellCategoryCooldowns[i];
        }

        itemTemplate.Bonding        = fields[54].Get<uint32>();
        itemTemplate.Description    = fields[55].Get<std::string>();
        itemTemplate.PageText       = fields[56].Get<uint32>();
        itemTemplate.LanguageID     = fields[57].Get<uint32>();
        itemTemplate.PageMaterial   = fields[58].Get<uint32>();
        itemTemplate.StartQuest     = fields[59].Get<uint32>();
        itemTemplate.LockID         = fields[60].Get<uint32>();
        itemTemplate.Material       = fields[61].Get<int32>();
        itemTemplate.Sheath         = fields[62].Get<uint32>();
        itemTemplate.RandomProperty = fields[63].Get<int32>();
        itemTemplate.RandomSuffix   = fields[64].Get<int32>();
        itemTemplate.Block          = fields[65].Get<uint32>();
        itemTemplate.ItemSet        = fields[66].Get<uint32>();
        itemTemplate.MaxDurability  = fields[67].Get<uint32>();
        itemTemplate.Area           = fields[68].Get<uint32>();
        itemTemplate.Map            = fields[69].Get<uint32>();
        itemTemplate.BagFamily      = fields[70].Get<uint32>();
        itemTemplate.TotemCategory  = fields[71].Get<uint32>();

        const auto socketColors = fields[72].GetArray<uint32, MAX_ITEM_PROTO_SOCKETS>();
        const auto socketContents = fields[73].GetArray<uint32, MAX_ITEM_PROTO_SOCKETS>();
        for (uint8 i = 0; i < MAX_ITEM_PROTO_SOCKETS; ++i)
        {
            itemTemplate.Socket[i].Color   = socketColors[i];
            itemTemplate.Socket[i].Content = socketContents[i];
        }

        itemTemplate.socketBonus             = fields[74].Get<uint32>();
        itemTemplate.GemProperties           = fields[75].Get<uint32>();
        itemTemplate.DisenchantID            = fields[76].Get<uint32>();
        itemTemplate.RequiredDisenchantSkill = fields[77].Get<uint32>();
        itemTemplate.ArmorDamageModifier     = fields[78].Get<float>();
        itemTemplate.Duration                = fields[79].Get<uint32>();
        itemTemplate.ItemLimitCategory       = fields[80].Get<uint32>();
        itemTemplate.HolidayId               = fields[81].Get<uint32>();
        itemTemplate.FoodType                = fields[82].Get<uint32>();
        itemTemplate.MinMoneyLoot            = fields[83].Get<uint32>();
        itemTemplate.MaxMoneyLoot            = fields[84].Get<uint32>();
        itemTemplate.FlagsCu                 = static_cast<ItemFlagsCustom>(fields[85].Get<uint32>());
        itemTemplate.ScriptId                = GetScriptID(fields[86].Get<std::string>());

        const ItemEntry* dbcItem = sItemStore.LookupEntry(entry);
        if (!dbcItem)
        {
            LOG_DEBUG("sql.sql", "Item (Entry: {}) does not exist in item.dbc! (not correct id?).", entry);
            continue;
        }

        if (enforceDBCAttributes)
        {
            if (itemTemplate.Class != dbcItem->ClassID)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong Class value ({}), must be ({}).", entry, itemTemplate.Class, dbcItem->ClassID);
                itemTemplate.Class = dbcItem->ClassID;
            }
            if (itemTemplate.SubClass != dbcItem->SubclassID)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong Subclass value ({}) for class {}, must be ({}).", entry, itemTemplate.SubClass, itemTemplate.Class, dbcItem->SubclassID);
                itemTemplate.SubClass = dbcItem->SubclassID;
            }
            if (itemTemplate.SoundOverrideSubclass != dbcItem->SoundOverrideSubclassID)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) does not have a correct SoundOverrideSubclass ({}), must be {}.", entry, itemTemplate.SoundOverrideSubclass, dbcItem->SoundOverrideSubclassID);
                itemTemplate.SoundOverrideSubclass = dbcItem->SoundOverrideSubclassID;
            }
            if (itemTemplate.Material != dbcItem->Material)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) does not have a correct material ({}), must be {}.", entry, itemTemplate.Material, dbcItem->Material);
                itemTemplate.Material = dbcItem->Material;
            }
            if (itemTemplate.InventoryType != dbcItem->InventoryType)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong InventoryType value ({}), must be {}.", entry, itemTemplate.InventoryType, dbcItem->InventoryType);
                itemTemplate.InventoryType = dbcItem->InventoryType;
            }
            if (itemTemplate.DisplayInfoID != dbcItem->DisplayInfoID)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) does not have a correct display id ({}), must be {}.", entry, itemTemplate.DisplayInfoID, dbcItem->DisplayInfoID);
                itemTemplate.DisplayInfoID = dbcItem->DisplayInfoID;
            }
            if (itemTemplate.Sheath != dbcItem->SheatheType)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong Sheath ({}), must be {}.", entry, itemTemplate.Sheath, dbcItem->SheatheType);
                itemTemplate.Sheath = dbcItem->SheatheType;
            }
        }

        if (itemTemplate.Quality >= MAX_ITEM_QUALITY)
        {
            LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong Quality value ({})", entry, itemTemplate.Quality);
            itemTemplate.Quality = ITEM_QUALITY_NORMAL;
        }

        if (itemTemplate.HasFlag2(ITEM_FLAG2_FACTION_HORDE))
        {
            if (const FactionEntry* faction = sFactionStore.LookupEntry(HORDE))
                if ((itemTemplate.AllowableRace & faction->BaseRepRaceMask[0]) == 0)
                    LOG_ERROR("sql.sql", "Item (Entry: {}) has value ({}) in `allowable_race` races, not compatible with ITEM_FLAG2_FACTION_HORDE ({}) in `flags` field, "
                                         "item cannot be equipped or used by these races.", entry, itemTemplate.AllowableRace, ITEM_FLAG2_FACTION_HORDE);

            if (itemTemplate.HasFlag2(ITEM_FLAG2_FACTION_ALLIANCE))
                LOG_ERROR("sql.sql", "Item (Entry: {}) has value ({}) in `flags_extra` flags (ITEM_FLAG2_FACTION_ALLIANCE) and ITEM_FLAG2_FACTION_HORDE ({}) in `flags` field, "
                                     "this is a wrong combination.", entry, ITEM_FLAG2_FACTION_ALLIANCE, ITEM_FLAG2_FACTION_HORDE);
        }
        else if (itemTemplate.HasFlag2(ITEM_FLAG2_FACTION_ALLIANCE))
        {
            if (const FactionEntry* faction = sFactionStore.LookupEntry(ALLIANCE))
                if ((itemTemplate.AllowableRace & faction->BaseRepRaceMask[0]) == 0)
                    LOG_ERROR("sql.sql", "Item (Entry: {}) has value ({}) in `allowable_race` races, not compatible with ITEM_FLAG2_FACTION_ALLIANCE ({}) in `flags` field, "
                                         "item cannot be equipped or used by these races.", entry, itemTemplate.AllowableRace, ITEM_FLAG2_FACTION_ALLIANCE);
        }

        if (itemTemplate.BuyCount <= 0)
        {
            LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong BuyCount value ({}), set to default(1).", entry, itemTemplate.BuyCount);
            itemTemplate.BuyCount = 1;
        }

        if (itemTemplate.RequiredSkill >= MAX_SKILL_TYPE)
        {
            LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong RequiredSkill value ({})", entry, itemTemplate.RequiredSkill);
            itemTemplate.RequiredSkill = 0;
        }

        {
            // Can be used in equip slot, as page read use in inventory, or spell casting at use
            bool req = itemTemplate.InventoryType != INVTYPE_NON_EQUIP || itemTemplate.PageText;
            if (!req)
                for (auto & spell : itemTemplate.Spells)
                {
                    if (spell.SpellId)
                    {
                        req = true;
                        break;
                    }
                }

            if (req)
            {
                if (!(itemTemplate.AllowableClass & CLASS_MASK_ALL_PLAYABLE))
                    LOG_ERROR("sql.sql", "Item (Entry: {}) does not have any playable classes ({}) in `allowable_class` and can't be equipped or used.", entry, itemTemplate.AllowableClass);

                if (!(itemTemplate.AllowableRace & sRaceMgr->GetPlayableRaceMask()))
                    LOG_ERROR("sql.sql", "Item (Entry: {}) does not have any playable races ({}) in `allowable_race` and can't be equipped or used.", entry, itemTemplate.AllowableRace);
            }
        }

        if (itemTemplate.RequiredSpell && !sSpellMgr->GetSpellInfo(itemTemplate.RequiredSpell))
        {
            LOG_ERROR("sql.sql", "Item (Entry: {}) has a wrong (non-existing) spell in RequiredSpell ({})", entry, itemTemplate.RequiredSpell);
            itemTemplate.RequiredSpell = 0;
        }

        if (itemTemplate.RequiredReputationRank >= MAX_REPUTATION_RANK)
            LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong reputation rank in RequiredReputationRank ({}), item can't be used.", entry, itemTemplate.RequiredReputationRank);

        if (itemTemplate.RequiredReputationFaction)
        {
            if (!sFactionStore.LookupEntry(itemTemplate.RequiredReputationFaction))
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong (not existing) faction in RequiredReputationFaction ({})", entry, itemTemplate.RequiredReputationFaction);
                itemTemplate.RequiredReputationFaction = 0;
            }

            if (itemTemplate.RequiredReputationRank == MIN_REPUTATION_RANK)
                LOG_ERROR("sql.sql", "Item (Entry: {}) has min. reputation rank in RequiredReputationRank (0) but RequiredReputationFaction > 0, faction setting is useless.", entry);
        }

        if (itemTemplate.MaxCount < -1)
        {
            LOG_ERROR("sql.sql", "Item (Entry: {}) has too large negative in MaxCount ({}), replace by value (-1) no storing limits.", entry, itemTemplate.MaxCount);
            itemTemplate.MaxCount = -1;
        }

        if (itemTemplate.Stackable == 0)
        {
            LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong value in stackable ({}), replace by default 1.", entry, itemTemplate.Stackable);
            itemTemplate.Stackable = 1;
        }
        else if (itemTemplate.Stackable < -1)
        {
            LOG_ERROR("sql.sql", "Item (Entry: {}) has too large negative in stackable ({}), replace by value (-1) no stacking limits.", entry, itemTemplate.Stackable);
            itemTemplate.Stackable = -1;
        }

        if (itemTemplate.ContainerSlots > MAX_BAG_SIZE)
        {
            LOG_ERROR("sql.sql", "Item (Entry: {}) has too large value in ContainerSlots ({}), replace by hardcoded limit ({}).", entry, itemTemplate.ContainerSlots, MAX_BAG_SIZE);
            itemTemplate.ContainerSlots = MAX_BAG_SIZE;
        }

        for (uint32 j = 0; j < itemTemplate.StatsCount; ++j)
        {
            if (itemTemplate.ItemStat[j].ItemStatValue && itemTemplate.ItemStat[j].ItemStatType >= MAX_ITEM_MOD)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong (non-existing?) stat_type[{}] ({})", entry, j, itemTemplate.ItemStat[j].ItemStatType);
                itemTemplate.ItemStat[j].ItemStatType = 0;
            }

            switch (itemTemplate.ItemStat[j].ItemStatType)
            {
                case ITEM_MOD_SPELL_HEALING_DONE:
                case ITEM_MOD_SPELL_DAMAGE_DONE:
                    // Skip warning for specific items: 13113 (FeatherMoon Headdress - Blizzard oversight), 34967 (test item)
                    if (entry != 13113 && entry != 34967)
                        LOG_WARN("sql.sql", "Item (Entry: {}) has deprecated stat_type[{}] ({})", entry, j, itemTemplate.ItemStat[j].ItemStatType);

                    break;
                default:
                    break;
            }
        }

        for (uint8 j = 0; j < MAX_ITEM_PROTO_DAMAGES; ++j)
        {
            if (itemTemplate.Damage[j].DamageType >= MAX_SPELL_SCHOOL)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong damage_type[{}] ({})", entry, j, itemTemplate.Damage[j].DamageType);
                itemTemplate.Damage[j].DamageType = 0;
            }
        }

        // Special format
        if (itemTemplate.Spells[0].SpellId == 483 || itemTemplate.Spells[0].SpellId == 55884)
        {
            // spell[0]
            if (itemTemplate.Spells[0].SpellTrigger != ITEM_SPELLTRIGGER_ON_USE)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong item spell trigger value in spell_trigger[{}] ({}) for special learning format", entry, 0, itemTemplate.Spells[0].SpellTrigger);
                itemTemplate.Spells[0].SpellId = 0;
                itemTemplate.Spells[0].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
                itemTemplate.Spells[1].SpellId = 0;
                itemTemplate.Spells[1].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
            }

            // spell[1] have learning spell
            if (itemTemplate.Spells[1].SpellTrigger != ITEM_SPELLTRIGGER_LEARN_SPELL_ID)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong item spell trigger value in spell_trigger[{}] ({}) for special learning format.", entry, 1, itemTemplate.Spells[1].SpellTrigger);
                itemTemplate.Spells[0].SpellId = 0;
                itemTemplate.Spells[1].SpellId = 0;
                itemTemplate.Spells[1].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
            }
            else if (!itemTemplate.Spells[1].SpellId)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) does not have an expected spell in spell[{}] in special learning format.", entry, 1);
                itemTemplate.Spells[0].SpellId = 0;
                itemTemplate.Spells[1].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
            }
            else if (itemTemplate.Spells[1].SpellId != -1)
            {
                if (!sSpellMgr->GetSpellInfo(itemTemplate.Spells[1].SpellId) && !sDisableMgr->IsDisabledFor(DISABLE_TYPE_SPELL, itemTemplate.Spells[1].SpellId, nullptr))
                {
                    LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong (not existing) spell in spell[{}] ({})", entry, 1, itemTemplate.Spells[1].SpellId);
                    itemTemplate.Spells[0].SpellId = 0;
                    itemTemplate.Spells[1].SpellId = 0;
                    itemTemplate.Spells[1].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
                }
                // Allowed only in special format
                else if (itemTemplate.Spells[1].SpellId == 483 || itemTemplate.Spells[1].SpellId == 55884)
                {
                    LOG_ERROR("sql.sql", "Item (Entry: {}) has broken spell in spell[{}] ({})", entry, 1, itemTemplate.Spells[1].SpellId);
                    itemTemplate.Spells[0].SpellId = 0;
                    itemTemplate.Spells[1].SpellId = 0;
                    itemTemplate.Spells[1].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
                }
            }

            // spell[2], spell[3], spell[4] are empty
            for (uint8 j = 2; j < MAX_ITEM_PROTO_SPELLS; ++j)
            {
                if (itemTemplate.Spells[j].SpellTrigger != ITEM_SPELLTRIGGER_ON_USE)
                {
                    LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong item spell trigger value in spell_trigger[{}] ({})", entry, j, itemTemplate.Spells[j].SpellTrigger);
                    itemTemplate.Spells[j].SpellId = 0;
                    itemTemplate.Spells[j].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
                }
                else if (itemTemplate.Spells[j].SpellId != 0)
                {
                    LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong spell in spell[{}] ({}) for learning special format", entry, j, itemTemplate.Spells[j].SpellId);
                    itemTemplate.Spells[j].SpellId = 0;
                }
            }
        }
        // Normal spell list
        else
        {
            for (uint8 j = 0; j < MAX_ITEM_PROTO_SPELLS; ++j)
            {
                if (itemTemplate.Spells[j].SpellTrigger >= MAX_ITEM_SPELLTRIGGER || itemTemplate.Spells[j].SpellTrigger == ITEM_SPELLTRIGGER_LEARN_SPELL_ID)
                {
                    LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong item spell trigger value in spell_trigger[{}] ({})", entry, j, itemTemplate.Spells[j].SpellTrigger);
                    itemTemplate.Spells[j].SpellId = 0;
                    itemTemplate.Spells[j].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
                }

                if (itemTemplate.Spells[j].SpellId && itemTemplate.Spells[j].SpellId != -1)
                {
                    if (!sSpellMgr->GetSpellInfo(itemTemplate.Spells[j].SpellId) && !sDisableMgr->IsDisabledFor(DISABLE_TYPE_SPELL, itemTemplate.Spells[j].SpellId, nullptr))
                    {
                        LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong (not existing) spell in spell[{}] ({})", entry, j, itemTemplate.Spells[j].SpellId);
                        itemTemplate.Spells[j].SpellId = 0;
                    }
                    // Allowed only in special format
                    else if ((itemTemplate.Spells[j].SpellId == 483) || (itemTemplate.Spells[j].SpellId == 55884))
                    {
                        LOG_ERROR("sql.sql", "Item (Entry: {}) has broken spell in spell[{}] ({})", entry, j, itemTemplate.Spells[j].SpellId);
                        itemTemplate.Spells[j].SpellId = 0;
                    }
                }
            }
        }

        if (itemTemplate.Bonding >= MAX_BIND_TYPE)
            LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong Bonding value ({})", entry, itemTemplate.Bonding);

        if (itemTemplate.PageText && !GetPageText(itemTemplate.PageText))
            LOG_ERROR("sql.sql", "Item (Entry: {}) has non existing first page (Id:{})", entry, itemTemplate.PageText);

        if (itemTemplate.LockID && !sLockStore.LookupEntry(itemTemplate.LockID))
            LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong LockID ({})", entry, itemTemplate.LockID);

        if (itemTemplate.RandomProperty)
        {
            // To be implemented later
            if (itemTemplate.RandomProperty == -1)
                itemTemplate.RandomProperty = 0;

            else if (!sItemRandomPropertiesStore.LookupEntry(GetItemEnchantMod(itemTemplate.RandomProperty)))
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has unknown (wrong or not listed in `world_item_enchantment_template`) RandomProperty ({})", entry, itemTemplate.RandomProperty);
                itemTemplate.RandomProperty = 0;
            }
        }

        if (itemTemplate.RandomSuffix && !sItemRandomSuffixStore.LookupEntry(GetItemEnchantMod(itemTemplate.RandomSuffix)))
        {
            LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong RandomSuffix ({})", entry, itemTemplate.RandomSuffix);
            itemTemplate.RandomSuffix = 0;
        }

        if (itemTemplate.ItemSet && !sItemSetStore.LookupEntry(itemTemplate.ItemSet))
        {
            LOG_ERROR("sql.sql", "Item (Entry: {}) have wrong ItemSet ({})", entry, itemTemplate.ItemSet);
            itemTemplate.ItemSet = 0;
        }

        if (itemTemplate.Area && !sAreaTableStore.LookupEntry(itemTemplate.Area))
            LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong Area ({})", entry, itemTemplate.Area);

        if (itemTemplate.Map && !sMapStore.LookupEntry(itemTemplate.Map))
            LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong Map ({})", entry, itemTemplate.Map);

        if (itemTemplate.BagFamily)
        {
            // Check bits
            for (uint32 j = 0; j < sizeof(itemTemplate.BagFamily) * 8; ++j)
            {
                uint32 mask = 1 << j;
                if ((itemTemplate.BagFamily & mask) == 0)
                    continue;
                if (!sItemBagFamilyStore.LookupEntry(j + 1))
                {
                    LOG_ERROR("sql.sql", "Item (Entry: {}) has bag family bit set not listed in ItemBagFamily.dbc, remove bit", entry);
                    itemTemplate.BagFamily &= ~mask;
                    continue;
                }

                if (BAG_FAMILY_MASK_CURRENCY_TOKENS & mask)
                {
                    if (!sCurrencyTypesStore.LookupEntry(itemTemplate.ItemId))
                    {
                        LOG_ERROR("sql.sql", "Item (Entry: {}) has currency bag family bit set in BagFamily but not listed in CurrencyTypes.dbc, remove bit", entry);
                        itemTemplate.BagFamily &= ~mask;
                    }
                }
            }
        }

        if (itemTemplate.TotemCategory && !sTotemCategoryStore.LookupEntry(itemTemplate.TotemCategory))
            LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong TotemCategory ({})", entry, itemTemplate.TotemCategory);

        for (uint8 j = 0; j < MAX_ITEM_PROTO_SOCKETS; ++j)
        {
            if (itemTemplate.Socket[j].Color && (itemTemplate.Socket[j].Color & SOCKET_COLOR_ALL) != itemTemplate.Socket[j].Color)
            {
                LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong socketColor_{} ({})", entry, j + 1, itemTemplate.Socket[j].Color);
                itemTemplate.Socket[j].Color = 0;
            }
        }

        if (itemTemplate.GemProperties && !sGemPropertiesStore.LookupEntry(itemTemplate.GemProperties))
            LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong GemProperties ({})", entry, itemTemplate.GemProperties);

        if (itemTemplate.FoodType >= MAX_PET_DIET)
        {
            LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong FoodType value ({})", entry, itemTemplate.FoodType);
            itemTemplate.FoodType = 0;
        }

        if (itemTemplate.ItemLimitCategory && !sItemLimitCategoryStore.LookupEntry(itemTemplate.ItemLimitCategory))
        {
            LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong LimitCategory value ({})", entry, itemTemplate.ItemLimitCategory);
            itemTemplate.ItemLimitCategory = 0;
        }

        if (itemTemplate.HolidayId && !sHolidaysStore.LookupEntry(itemTemplate.HolidayId))
        {
            LOG_ERROR("sql.sql", "Item (Entry: {}) has wrong HolidayId value ({})", entry, itemTemplate.HolidayId);
            itemTemplate.HolidayId = 0;
        }

        if (itemTemplate.HasFlagCu(ITEM_FLAGS_CU_DURATION_REAL_TIME) && !itemTemplate.Duration)
        {
            LOG_ERROR("sql.sql", "Item (Entry {}) has flag ITEM_FLAGS_CU_DURATION_REAL_TIME but it does not have duration limit", entry);
            itemTemplate.FlagsCu = static_cast<ItemFlagsCustom>(static_cast<uint32>(itemTemplate.FlagsCu) & ~ITEM_FLAGS_CU_DURATION_REAL_TIME);
        }

        // Set after checks to ensure valid item quality
        itemTemplate.BuyPrice = static_cast<int32>(
            static_cast<float>(itemTemplate.BuyPrice) * sWorld->getRate(qualityToBuyValueConfig[itemTemplate.Quality]));
        itemTemplate.SellPrice *= static_cast<int32>(
            static_cast<float>(itemTemplate.SellPrice) * sWorld->getRate(qualityToSellValueConfig[itemTemplate.Quality]));

        // Fill categories map
        for (auto & spell : itemTemplate.Spells)
            if (spell.SpellId && spell.SpellCategory && spell.SpellCategoryCooldown)
            {
                if (auto ct = sSpellsByCategoryStore.find(spell.SpellCategory); ct != sSpellsByCategoryStore.end())
                    ct->second.emplace(true, spell.SpellId);
                else
                    sSpellsByCategoryStore[spell.SpellCategory].emplace(true, spell.SpellId);
            }

        ++count;
    } while (result->NextRow());

    {
        uint32 max = 0;
        for (const auto &entry: _itemTemplateStore | std::views::keys)
            if (entry > max)
                max = entry;
        if (max)
        {
            _itemTemplateStoreFast.clear();
            _itemTemplateStoreFast.resize(max + 1, nullptr);
            for (auto &[entry, itemTemplate] : _itemTemplateStore)
                _itemTemplateStoreFast[entry] = &itemTemplate;
        }
    }

    // Check if item templates for DBC referenced character start outfit are present
    std::set<uint32> notFoundOutfit;
    for (auto entry : sCharStartOutfitStore)
    {
        for (int j : entry->ItemID)
        {
            if (j <= 0)
                continue;

            if (const uint32 itemID = j; !GetItemTemplate(itemID))
                notFoundOutfit.insert(itemID);
        }
    }

    for (unsigned int entry : notFoundOutfit)
        LOG_ERROR("sql.sql", "Item (Entry: {}) does not exist in `world_item_template` but is referenced in `CharStartOutfit.dbc`", entry);

    LOG_INFO("server.loading", ">> Loaded {} Item Templates in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

const ItemTemplate* ObjectMgr::GetItemTemplate(const uint32 entry) const {
    return entry < _itemTemplateStoreFast.size() ? _itemTemplateStoreFast[entry] : nullptr;
}

void ObjectMgr::LoadItemSetNames()
{
    const uint32 oldMSTime = getMSTime();

    _itemSetNameStore.clear();

    std::set<uint32> itemSetItems;

    // Fill item set member ids
    for (uint32 entryId = 0; entryId < sItemSetStore.GetNumRows(); ++entryId)
    {
        const ItemSetEntry* setEntry = sItemSetStore.LookupEntry(entryId);
        if (!setEntry)
            continue;

        for (unsigned int i : setEntry->ItemID)
            if (i)
                itemSetItems.insert(i);
    }

    const QueryResult result = WorldDatabase.Query("SELECT id, name, inventory_type FROM world_item_set_name");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 item set names. DB table `world_item_set_name` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    _itemSetNameStore.rehash(result->GetRowCount());
    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();

        uint32 entry = fields[0].Get<uint32>();
        if (!itemSetItems.contains(entry))
        {
            LOG_ERROR("sql.sql", "Item set name (Entry: {}) not found in ItemSet.dbc, data useless.", entry);
            continue;
        }

        auto& [name, inventoryType] = _itemSetNameStore[entry];
        name = fields[1].Get<std::string>();

        uint32 invType = fields[2].Get<uint32>();
        if (invType >= MAX_INVTYPE)
        {
            LOG_ERROR("sql.sql", "Item set name (Entry: {}) has wrong InventoryType value ({})", entry, invType);
            invType = INVTYPE_NON_EQUIP;
        }

        inventoryType = invType;
        itemSetItems.erase(entry);
        ++count;
    } while (result->NextRow());

    if (!itemSetItems.empty())
    {
        for (unsigned int entry : itemSetItems)
        {
            // Add data from world_item_template if available
            if (const ItemTemplate* pProto = GetItemTemplate(entry))
            {
                LOG_ERROR("sql.sql", "Item set part (Entry: {}) does not have entry in `world_item_set_name`, adding data from `item_template`.", entry);
                auto& [name, inventoryType] = _itemSetNameStore[entry];
                name = pProto->Name1;
                inventoryType = pProto->InventoryType;
                ++count;
            }
            else
                LOG_ERROR("sql.sql", "Item set part (Entry: {}) does not have entry in `world_item_set_name`, set will not display properly.", entry);
        }
    }

    LOG_INFO("server.loading", ">> Loaded {} Item Set Names in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadVehicleTemplateAccessories()
{
    const uint32 oldMSTime = getMSTime();

    _vehicleTemplateAccessoryStore.clear();

    uint32 count = 0;

    const QueryResult result = WorldDatabase.Query("SELECT entry, accessory, seat, minion, summon_type, summon_timer FROM world_vehicle_template_accessory");
    const auto tableName = "world_vehicle_template_accessory";

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 vehicle template accessories. DB table `{}` is empty.", tableName);
        LOG_INFO("server.loading", " ");
        return;
    }

    do
    {
        const Field* fields = result->Fetch();

        const uint32 uiEntry = fields[0].Get<uint32>();
        const uint32 uiAccessory = fields[1].Get<uint32>();
        const int8 uiSeat = fields[2].Get<int8>();
        const bool bMinion = fields[3].Get<bool>();
        const uint8 uiSummonType = fields[4].Get<uint8>();
        const uint32 uiSummonTimer = fields[5].Get<uint32>();

        if (!GetCreatureTemplate(uiEntry))
        {
            LOG_ERROR("sql.sql", "Table `{}`: creature template entry {} does not exist.", tableName, uiEntry);
            continue;
        }

        if (!GetCreatureTemplate(uiAccessory))
        {
            LOG_ERROR("sql.sql", "Table `{}`: Accessory {} does not exist.", tableName, uiAccessory);
            continue;
        }

        if (!_spellClickInfoStore.contains(uiEntry))
        {
            LOG_ERROR("sql.sql", "Table `{}`: creature template entry {} has no data in `world_npc_spell_click_spell`", tableName, uiEntry);
            continue;
        }

        _vehicleTemplateAccessoryStore[uiEntry].emplace_back(uiAccessory, uiSeat, bMinion, uiSummonType, uiSummonTimer);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Vehicle Template Accessories in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadVehicleAccessories()
{
    const uint32 oldMSTime = getMSTime();

    _vehicleAccessoryStore.clear();  // Needed for reload case

    uint32 count = 0;
    const QueryResult result = WorldDatabase.Query("SELECT guid, accessory, seat, minion, summon_type, summon_timer FROM world_vehicle_accessory");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 Vehicle Accessories in {} ms", GetMSTimeDiffToNow(oldMSTime));
        return;
    }

    do
    {
        const Field* fields = result->Fetch();

        const uint32 uiGUID = fields[0].Get<uint32>();
        const uint32 uiAccessory = fields[1].Get<uint32>();
        const int8 uiSeat = fields[2].Get<int8>();
        const bool bMinion = fields[3].Get<bool>();
        const uint8 uiSummonType = fields[4].Get<uint8>();
        const uint32 uiSummonTimer = fields[5].Get<uint32>();

        if (!GetCreatureTemplate(uiAccessory))
        {
            LOG_ERROR("sql.sql", "Table `world_vehicle_accessory`: Accessory {} does not exist.", uiAccessory);
            continue;
        }

        _vehicleAccessoryStore[uiGUID].emplace_back(uiAccessory, uiSeat, bMinion, uiSummonType, uiSummonTimer);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Vehicle Accessories in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadVehicleSeatAddon()
{
    const uint32 oldMSTime = getMSTime();

    _vehicleSeatAddonStore.clear();

    uint32 count = 0;

    const QueryResult result = WorldDatabase.Query("SELECT seat, seat_orientation, exit_param_x, exit_param_y, exit_param_z, exit_param_o, exit_param_value FROM world_vehicle_seat_addon");

    if (!result)
    {
        LOG_ERROR("server.loading", ">> Loaded 0 vehicle seat addons. DB table `vehicle_seat_addon` is empty.");
        return;
    }

    do
    {
        const Field* fields = result->Fetch();

        const uint32 seatID = fields[0].Get<uint32>();
        float orientation = fields[1].Get<float>();
        const float exitX = fields[2].Get<float>();
        const float exitY = fields[3].Get<float>();
        const float exitZ = fields[4].Get<float>();
        const float exitO = fields[5].Get<float>();
        const uint8 exitParam = fields[6].Get<uint8>();

        if (!sVehicleSeatStore.LookupEntry(seatID))
        {
            LOG_ERROR("sql.sql", "Table `world_vehicle_seat_addon`: SeatID: {} does not exist in VehicleSeat.dbc. Skipping entry.", seatID);
            continue;
        }

        // Sanitizing values
        if (orientation > static_cast<float>(M_PI * 2))
        {
            LOG_ERROR("sql.sql", "Table `world_vehicle_seat_addon`: SeatID: {} is using invalid angle offset value ({}). Set Value to 0.", seatID, orientation);
            orientation = 0.0f;
        }

        if (exitParam >= AsUnderlyingType(VehicleExitParameters::VehicleExitParamMax))
        {
            LOG_ERROR("sql.sql", "Table `world_vehicle_seat_addon`: SeatID: {} is using invalid exit parameter value ({}). Setting to 0 (none).", seatID, exitParam);
            continue;
        }

        _vehicleSeatAddonStore[seatID] = VehicleSeatAddon(orientation, exitX, exitY, exitZ, exitO, exitParam);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Vehicle Seat Addon entries in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadPetLevelInfo()
{
    const uint32 oldMSTime = getMSTime();

    const QueryResult result = WorldDatabase.Query("SELECT creature, level, health, mana, strength, agility, stamina, intellect, spirit, armor, min_damage, max_damage FROM world_pet_level_stats");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 level pet stats definitions. DB table `world_pet_level_stats` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();

        uint32 creatureID = fields[0].Get<uint32>();
        if (!GetCreatureTemplate(creatureID))
        {
            LOG_ERROR("sql.sql", "Wrong creature id {} in `world_pet_level_stats` table, ignoring.", creatureID);
            continue;
        }

        uint32 current_level = fields[1].Get<uint8>();
        if (current_level > sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        {
            if (current_level > STRONG_MAX_LEVEL)  // Hardcoded level maximum
                LOG_ERROR("sql.sql", "Wrong (> {}) level {} in `world_pet_level_stats` table, ignoring.", STRONG_MAX_LEVEL, current_level);
            else
            {
                LOG_DEBUG("sql.sql", "Unused (> MaxPlayerLevel in worldserver.conf) level {} in `world_pet_level_stats` table, ignoring.", current_level);
                ++count; // Make result loading percent "expected" correct in case disabled detail mode for example.
            }
            continue;
        }
        if (current_level < 1)
        {
            LOG_ERROR("sql.sql", "Wrong (<1) level {} in `world_pet_level_stats` table, ignoring.", current_level);
            continue;
        }

        PetLevelInfo*& pInfoMapEntry = _petInfoStore[creatureID];

        if (!pInfoMapEntry)
            pInfoMapEntry = new PetLevelInfo[sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL)];

        // Data for level 1 stored in [0] array element, ...
        PetLevelInfo* pLevelInfo = &pInfoMapEntry[current_level - 1];

        pLevelInfo->health = fields[2].Get<uint32>();
        pLevelInfo->mana = fields[3].Get<uint32>();
        for (uint8 i = 0; i < MAX_STATS; i++)
            pLevelInfo->stats[i] = fields[i + 4].Get<uint32>();
        pLevelInfo->armor = fields[9].Get<uint32>();
        pLevelInfo->min_dmg = fields[10].Get<uint32>();
        pLevelInfo->max_dmg = fields[11].Get<uint32>();

        ++count;
    } while (result->NextRow());

    // Fill gaps and check integrity
    for (auto itr = _petInfoStore.begin(); itr != _petInfoStore.end(); ++itr)
    {
        PetLevelInfo* pInfo = itr->second;

        // Fatal error if no level 1 data
        if (!pInfo || pInfo[0].health == 0)
        {
            LOG_ERROR("sql.sql", "Creature {} does not have pet stats data for Level 1!", itr->first);
            exit(1);
        }

        // Fill level gaps
        for (uint32 level = 1; level < sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL); ++level)
        {
            if (pInfo[level].health == 0)
            {
                LOG_ERROR("sql.sql", "Creature {} has no data for Level {} pet stats data, using data of Level {}.", itr->first, level + 1, level);
                pInfo[level] = pInfo[level - 1];
            }
        }
    }

    LOG_INFO("server.loading", ">> Loaded {} Level Pet Stats Definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

const PetLevelInfo* ObjectMgr::GetPetLevelInfo(const uint32 creatureID, uint8 level) const
{
    if (level > sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        level = sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);

    const auto itr = _petInfoStore.find(creatureID);
    if (itr == _petInfoStore.end())
        return nullptr;
    return &itr->second[level - 1];  // Data for level 1 stored in [0] array element, ...
}

void ObjectMgr::PlayerCreateInfoAddItemHelper(const uint32 race_, const uint32 class_, uint32 itemID, int32 count) const {
    if (!_playerInfo[race_][class_])
        return;

    if (count > 0)
        _playerInfo[race_][class_]->item.emplace_back(itemID, count);
    else
    {
        if (count < -1)
            LOG_ERROR("sql.sql", "Invalid count {} specified on item {} be removed from original player create info (use -1)!", count, itemID);

        for (uint32 gender = 0; gender < GENDER_NONE; ++gender)
        {
            if (const CharStartOutfitEntry* entry = GetCharStartOutfitEntry(race_, class_, gender))
            {
                bool found = false;
                for (uint8 x = 0; x < MAX_OUTFIT_ITEMS; ++x)
                {
                    if (entry->ItemID[x] > 0 && static_cast<uint32>(entry->ItemID[x]) == itemID)
                    {
                        found = true;
                        const_cast<CharStartOutfitEntry*>(entry)->ItemID[x] = 0;
                        break;
                    }
                }

                if (!found)
                    LOG_ERROR("sql.sql", "Item {} specified to be removed from original create info not found in dbc!", itemID);
            }
        }
    }
}

void ObjectMgr::LoadPlayerCreateInfo()
{
    const uint32 oldMSTime = getMSTime();

    if (_playerInfo.empty() || _playerInfo.size() != sRaceMgr->GetMaxRaces())
    {
        _playerInfo.clear();
        _playerInfo.resize(sRaceMgr->GetMaxRaces());
        for (auto& classVec : _playerInfo)
            classVec.resize(MAX_CLASSES, nullptr);
    }

    const QueryResult result = WorldDatabase.Query("SELECT race, class, map, zone, position, orientation FROM world_player_create_info");
    const auto tableName = "world_player_create_info";

    if (!result)
    {
        LOG_INFO("server.loading", " ");
        LOG_WARN("server.loading", ">> Loaded 0 player create definitions. DB table `{}` is empty.", tableName);
        exit(1);
    }
    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();

        uint32 currentRace = fields[0].Get<uint8>();
        uint32 currentClass = fields[1].Get<uint8>();

        if (currentRace >= sRaceMgr->GetMaxRaces())
        {
            LOG_ERROR("sql.sql", "Wrong race {} in `{}` table, ignoring.", currentRace, tableName);
            continue;
        }

        const ChrRacesEntry* rEntry = sChrRacesStore.LookupEntry(currentRace);
        if (!rEntry)
        {
            LOG_ERROR("sql.sql", "Wrong race {} in `{}` table, ignoring.", currentRace, tableName);
            continue;
        }

        if (currentClass >= MAX_CLASSES)
        {
            LOG_ERROR("sql.sql", "Wrong class {} in `{}` table, ignoring.", currentClass, tableName);
            continue;
        }

        if (!sChrClassesStore.LookupEntry(currentClass))
        {
            LOG_ERROR("sql.sql", "Wrong class {} in `{}` table, ignoring.", currentClass, tableName);
            continue;
        }

        const uint32 mapID = fields[2].Get<uint16>();
        const uint32 areaID = fields[3].Get<uint32>();  // zone

        const auto position = fields[4].GetArray<float, 3>();
        const float positionX = position[0];
        const float positionY = position[1];
        const float positionZ = position[2];
        const float orientation = fields[5].Get<float>();

        // Accept DB data only for valid position (and non instance-able)
        if (!MapMgr::IsValidMapCoord(mapID, positionX, positionY, positionZ, orientation))
        {
            LOG_ERROR("sql.sql", "Wrong home position for class {} race {} pair in `{}` table, ignoring.", currentClass, currentRace, tableName);
            continue;
        }

        if (sMapStore.LookupEntry(mapID)->InstanceAble())
        {
            LOG_ERROR("sql.sql", "Home position in instance-able map for class {} race {} pair in `{}` table, ignoring.", currentClass, currentRace, tableName);
            continue;
        }

        const auto info = new PlayerInfo();
        info->mapId = mapID;
        info->areaId = areaID;
        info->positionX = positionX;
        info->positionY = positionY;
        info->positionZ = positionZ;
        info->orientation = orientation;
        info->displayId_m = rEntry->ModelMale;
        info->displayId_f = rEntry->ModelFemale;
        _playerInfo[currentRace][currentClass] = info;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Player Create Definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadPlayerCreateInfoItems() const {
    const uint32 oldMSTime = getMSTime();

    LOG_INFO("server.loading", "Loading Player Create Items Data...");

    const QueryResult result = WorldDatabase.Query("SELECT race, class, item, amount FROM world_player_create_info_item");
    const auto tableName = "world_player_create_info_item";

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 Custom Player Create Items. DB Table `{}` Is Empty.", tableName);
        return;
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();

        uint32 currentRace = fields[0].Get<uint8>();
        if (currentRace >= sRaceMgr->GetMaxRaces())
        {
            LOG_ERROR("sql.sql", "Wrong race {} in `{}` table, ignoring.", currentRace, tableName);
            continue;
        }

        uint32 currentClass = fields[1].Get<uint8>();
        if (currentClass >= MAX_CLASSES)
        {
            LOG_ERROR("sql.sql", "Wrong class {} in `{}` table, ignoring.", currentClass, tableName);
            continue;
        }

        uint32 itemID = fields[2].Get<uint32>();
        if (!GetItemTemplate(itemID))
        {
            LOG_ERROR("sql.sql", "Item id {} (race {} class {}) in `{}` table but not listed in `item_template`, ignoring.", itemID, currentRace, currentClass, tableName);
            continue;
        }

        const int32 amount = fields[3].Get<int32>();
        if (!amount)
        {
            LOG_ERROR("sql.sql", "Item id {} (class {} race {}) have amount == 0 in `{}` table, ignoring.", itemID, currentRace, currentClass, tableName);
            continue;
        }

        if (!currentRace || !currentClass)
        {
            const uint32 minRace = currentRace ? currentRace : 1;
            const uint32 maxRace = currentRace ? currentRace + 1 : sRaceMgr->GetMaxRaces();
            const uint32 minClass = currentClass ? currentClass : 1;
            const uint32 maxClass = currentClass ? currentClass + 1 : MAX_CLASSES;
            for (uint32 r = minRace; r < maxRace; ++r)
                for (uint32 c = minClass; c < maxClass; ++c)
                    PlayerCreateInfoAddItemHelper(r, c, itemID, amount);
        }
        else
            PlayerCreateInfoAddItemHelper(currentRace, currentClass, itemID, amount);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Custom Player Create Items in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadPlayerCreateInfoSkills() const {
    const uint32 oldMSTime = getMSTime();

    LOG_INFO("server.loading", "Loading Player Create Skill Data...");

    const QueryResult result = WorldDatabase.Query("SELECT race_mask, class_mask, skill, rank FROM world_player_create_info_skill");
    const auto tableName = "world_player_create_info_skill";

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 Player Create Skills. DB Table `{}` Is Empty.", tableName);
        return;
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();
        uint32 raceMask = fields[0].Get<uint32>();
        uint32 classMask = fields[1].Get<uint32>();
        PlayerCreateInfoSkill skill{};
        skill.SkillId = fields[2].Get<uint16>();
        skill.Rank = fields[3].Get<uint16>();

        if (skill.Rank >= MAX_SKILL_STEP)
        {
            LOG_ERROR("sql.sql", "Skill rank value {} set for skill {} raceMask {} classMask {} is too high, max allowed value is {}", skill.Rank, skill.SkillId, raceMask, classMask, MAX_SKILL_STEP);
            continue;
        }

        if (raceMask != 0 && !(raceMask & sRaceMgr->GetPlayableRaceMask()))
        {
            LOG_ERROR("sql.sql", "Wrong race mask {} in `{}` table, ignoring.", raceMask, tableName);
            continue;
        }

        if (classMask != 0 && !(classMask & CLASS_MASK_ALL_PLAYABLE))
        {
            LOG_ERROR("sql.sql", "Wrong class mask {} in `{}` table, ignoring.", classMask, tableName);
            continue;
        }

        if (!sSkillLineStore.LookupEntry(skill.SkillId))
        {
            LOG_ERROR("sql.sql", "Wrong skill id {} in `{}` table, ignoring.", skill.SkillId, tableName);
            continue;
        }

        for (uint32 raceIndex = RACE_HUMAN; raceIndex < sRaceMgr->GetMaxRaces(); ++raceIndex)
        {
            if (raceMask == 0 || ((1 << (raceIndex - 1)) & raceMask))
            {
                for (uint32 classIndex = CLASS_WARRIOR; classIndex < MAX_CLASSES; ++classIndex)
                {
                    if (classMask == 0 || ((1 << (classIndex - 1)) & classMask))
                    {
                        if (!GetSkillRaceClassInfo(skill.SkillId, raceIndex, classIndex))
                            continue;

                        if (PlayerInfo* info = _playerInfo[raceIndex][classIndex])
                        {
                            info->skills.push_back(skill);
                            ++count;
                        }
                    }
                }
            }
        }
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Player Create Skills in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadPlayerCreateInfoSpells() const {
    const uint32 oldMSTime = getMSTime();

    LOG_INFO("server.loading", "Loading Player Create Spell Data...");

    const QueryResult result = WorldDatabase.Query("SELECT race_mask, class_mask, spell FROM world_player_create_info_spell_custom");
    const auto tableName = "world_player_create_info_spell_custom";

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 player create spells. DB table `{}` is empty.", tableName);
        return;
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();
        uint32 raceMask = fields[0].Get<uint32>();
        if (raceMask != 0 && !(raceMask & sRaceMgr->GetPlayableRaceMask()))
        {
            LOG_ERROR("sql.sql", "Wrong race mask {} in `{}` table, ignoring.", raceMask, tableName);
            continue;
        }

        uint32 classMask = fields[1].Get<uint32>();
        if (classMask != 0 && !(classMask & CLASS_MASK_ALL_PLAYABLE))
        {
            LOG_ERROR("sql.sql", "Wrong class mask {} in `{}` table, ignoring.", classMask, tableName);
            continue;
        }

        uint32 spellID = fields[2].Get<uint32>();
        for (uint32 raceIndex = RACE_HUMAN; raceIndex < sRaceMgr->GetMaxRaces(); ++raceIndex)
        {
            if (raceMask == 0 || ((1 << (raceIndex - 1)) & raceMask))
            {
                for (uint32 classIndex = CLASS_WARRIOR; classIndex < MAX_CLASSES; ++classIndex)
                {
                    if (classMask == 0 || ((1 << (classIndex - 1)) & classMask))
                    {
                        if (PlayerInfo* info = _playerInfo[raceIndex][classIndex])
                        {
                            info->customSpells.push_back(spellID);
                            ++count;
                        }
                    }
                }
            }
        }
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Custom Player Create Spells in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadPlayerCreateInfoCastSpell() const {
    const uint32 oldMSTime = getMSTime();

    LOG_INFO("server.loading", "Loading Player Create Cast Spell Data...");

    const QueryResult result = WorldDatabase.Query("SELECT race_mask, class_mask, spell FROM world_player_create_info_cast_spell");
    const auto tableName = "world_player_create_info_cast_spell";

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 Player Create Cast Spells. DB Table `{}` Is Empty.", tableName);
        return;
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();
        uint32 raceMask = fields[0].Get<uint32>();

        if (raceMask != 0 && !(raceMask & sRaceMgr->GetPlayableRaceMask()))
        {
            LOG_ERROR("sql.sql", "Wrong race mask {} in `{}` table, ignoring.", raceMask, tableName);
            continue;
        }

        uint32 classMask = fields[1].Get<uint32>();
        if (classMask != 0 && !(classMask & CLASS_MASK_ALL_PLAYABLE))
        {
            LOG_ERROR("sql.sql", "Wrong class mask {} in `{}` table, ignoring.", classMask, tableName);
            continue;
        }

        uint32 spellID = fields[2].Get<uint32>();
        for (uint32 raceIndex = RACE_HUMAN; raceIndex < sRaceMgr->GetMaxRaces(); ++raceIndex)
        {
            if (raceMask == 0 || ((1 << (raceIndex - 1)) & raceMask))
            {
                for (uint32 classIndex = CLASS_WARRIOR; classIndex < MAX_CLASSES; ++classIndex)
                {
                    if (classMask == 0 || ((1 << (classIndex - 1)) & classMask))
                    {
                        if (PlayerInfo* info = _playerInfo[raceIndex][classIndex])
                        {
                            info->castSpells.push_back(spellID);
                            ++count;
                        }
                    }
                }
            }
        }
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Player Create Cast Spells in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadPlayerCreateInfoActions() const {
    const uint32 oldMSTime = getMSTime();

    LOG_INFO("server.loading", "Loading Player Create Action Data...");

    const QueryResult result = WorldDatabase.Query("SELECT race, class, button, action, type FROM world_player_create_info_action");
    const auto tableName = "world_player_create_info_action";

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 Player Create Actions. DB Table `{}` Is Empty.", tableName);
        return;
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();

        uint32 currentRace = fields[0].Get<uint8>();
        if (currentRace >= sRaceMgr->GetMaxRaces())
        {
            LOG_ERROR("sql.sql", "Wrong race {} in `{}` table, ignoring.", currentRace, tableName);
            continue;
        }

        uint32 currentClass = fields[1].Get<uint8>();
        if (currentClass >= MAX_CLASSES)
        {
            LOG_ERROR("sql.sql", "Wrong class {} in `{}` table, ignoring.", currentClass, tableName);
            continue;
        }

        if (PlayerInfo* info = _playerInfo[currentRace][currentClass])
            info->action.emplace_back(fields[2].Get<uint16>(), fields[3].Get<uint32>(), fields[4].Get<uint16>());

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Player Create Actions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadPlayerCreateInfoStats()
{
    struct RaceStats
    {
        int16 StatModifier[MAX_STATS];
    };

    const uint32 oldMSTime = getMSTime();

    LOG_INFO("server.loading", "Loading Player Create Level Stats Data...");

    std::vector<RaceStats> raceStatModifiers;

    raceStatModifiers.resize(sRaceMgr->GetMaxRaces());

    const QueryResult raceStatsResult  = WorldDatabase.Query("SELECT race, strength, agility, stamina, intellect, spirit FROM world_player_race_stats");

    if (!raceStatsResult)
    {
        LOG_WARN("server.loading", ">> Loaded 0 race stats definitions. DB table `world_player_race_stats` is empty.");
        LOG_INFO("server.loading", " ");
        exit(1);
    }

    do
    {
        const Field* fields = raceStatsResult->Fetch();

        uint32 current_race = fields[0].Get<uint8>();
        if (current_race >= sRaceMgr->GetMaxRaces())
        {
            LOG_ERROR("sql.sql", "Wrong race {} in `world_player_race_stats` table, ignoring.", current_race);
            continue;
        }

        for (uint32 i = 0; i < MAX_STATS; ++i)
            raceStatModifiers[current_race].StatModifier[i] = fields[i + 1].Get<int16>();

    } while (raceStatsResult->NextRow());

    const QueryResult result = WorldDatabase.Query("SELECT class, level, strength, agility, stamina, intellect, spirit, base_hp, base_mana FROM world_player_class_stats");

    if (!result)
    {
        LOG_ERROR("server.loading", ">> Loaded 0 level stats definitions. DB table `world_player_class_stats` is empty.");
        exit(1);
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();

        uint32 current_class = fields[0].Get<uint8>();
        if (current_class >= MAX_CLASSES)
        {
            LOG_ERROR("sql.sql", "Wrong class {} in `world_player_class_stats` table, ignoring.", current_class);
            continue;
        }

        uint32 current_level = fields[1].Get<uint8>();
        if (current_level > sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        {
            if (current_level > STRONG_MAX_LEVEL) // hardcoded level maximum
                LOG_ERROR("sql.sql", "Wrong (> {}) level {} in `world_player_class_stats` table, ignoring.", STRONG_MAX_LEVEL, current_level);
            else
                LOG_DEBUG("sql.sql", "Unused (> MaxPlayerLevel in worldserver.conf) level {} in `world_player_class_stats` table, ignoring.", current_level);

            continue;
        }

        std::array<uint16, MAX_STATS> classStats{};
        for (int i = 0; i < MAX_STATS; ++i)
            classStats[i] = fields[i + 2].Get<uint16>();

        for (std::size_t race = 0; race < raceStatModifiers.size(); ++race)
        {
            if (PlayerInfo* info = _playerInfo[race][current_class])
            {
                if (!info->levelInfo)
                    info->levelInfo = new PlayerLevelInfo[sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL)];

                PlayerLevelInfo& levelInfo = info->levelInfo[current_level - 1];
                for (int i = 0; i < MAX_STATS; ++i)
                    levelInfo.stats[i] = classStats[i] + raceStatModifiers[race].StatModifier[i];
            }
        }

        PlayerClassInfo* info = _playerClassInfo[current_class];
        if (!info)
        {
            info = new PlayerClassInfo();
            info->levelInfo = new PlayerClassLevelInfo[sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL)];
            _playerClassInfo[current_class] = info;
        }

        PlayerClassLevelInfo& levelInfo = info->levelInfo[current_level - 1];

        levelInfo.baseHealth = fields[7].Get<uint32>();
        levelInfo.baseMana = fields[8].Get<uint32>();

        ++count;
    } while (result->NextRow());

    // Fill gaps and check integrity
    for (int race = RACE_HUMAN; race < sRaceMgr->GetMaxRaces(); ++race)
    {
        // skip non existed races
        if (!sChrRacesStore.LookupEntry(race))
            continue;

        for (int class_ = 0; class_ < MAX_CLASSES; ++class_)
        {
            // skip non existed classes
            if (!sChrClassesStore.LookupEntry(class_))
                continue;


            const PlayerInfo* info = _playerInfo[race][class_];
            if (!info)
                continue;

            // Fatal error if no initial stats data
            if (!info->levelInfo || (info->levelInfo[StartPlayerLevel - 1].stats[0] == 0 && class_ != CLASS_DEATH_KNIGHT) ||
                (info->levelInfo[StartHeroicPlayerLevel - 1].stats[0] == 0 && class_ == CLASS_DEATH_KNIGHT))
            {
                LOG_ERROR("sql.sql", "Race {} class {} initial level does not have stats data!", race, class_);
                exit(1);
            }

            const PlayerClassInfo* pClassInfo = _playerClassInfo[class_];

            // Fatal error if no initial health/mana data
            if (!pClassInfo->levelInfo || (pClassInfo->levelInfo[StartPlayerLevel - 1].baseHealth == 0 && class_ != CLASS_DEATH_KNIGHT) ||
                (pClassInfo->levelInfo[StartHeroicPlayerLevel - 1].baseHealth == 0 && class_ == CLASS_DEATH_KNIGHT))
            {
                LOG_ERROR("sql.sql", "Class {} initial level does not have health/mana data!", class_);
                exit(1);
            }

            // Fill level gaps for stats
            for (uint32 level = 1; level < sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL); ++level)
            {
                if ((info->levelInfo[level].stats[0] == 0 && class_ != CLASS_DEATH_KNIGHT) ||
                    (level >= StartHeroicPlayerLevel && info->levelInfo[level].stats[0] == 0 && class_ == CLASS_DEATH_KNIGHT))
                {
                    LOG_ERROR("sql.sql", "Race {} class {} level {} does not have stats data. Using stats data of level {}.", race, class_, level + 1, level);
                    info->levelInfo[level] = info->levelInfo[level - 1];
                }
            }

            // Fill level gaps for health/mana
            for (uint32 level = 1; level < sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL); ++level)
            {
                if ((pClassInfo->levelInfo[level].baseHealth == 0 && class_ != CLASS_DEATH_KNIGHT) ||
                    (level >= StartHeroicPlayerLevel && pClassInfo->levelInfo[level].baseHealth == 0 && class_ == CLASS_DEATH_KNIGHT))
                {
                    LOG_ERROR("sql.sql", "Class {} level {} does not have health/mana data. Using stats data of level {}.", class_, level + 1, level);
                    pClassInfo->levelInfo[level] = pClassInfo->levelInfo[level - 1];
                }
            }
        }
    }

    LOG_INFO("server.loading", ">> Loaded {} Level Stats Definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadPlayerCreateInfoXP()
{
    const uint32 oldMSTime = getMSTime();

    LOG_INFO("server.loading", "Loading Player Create XP Data...");

    _playerXPPerLevel.resize(sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL));
    for (uint32 level = 0; level < sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL); ++level)
        _playerXPPerLevel[level] = 0;

    const QueryResult result = WorldDatabase.Query("SELECT level, experience FROM world_player_xp_for_level");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 xp for level definitions. DB table `world_player_xp_for_level` is empty.");
        LOG_INFO("server.loading", " ");
        exit(1);
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        const uint32 currentLevel = fields[0].Get<uint8>();
        const uint32 currentXP = fields[1].Get<uint32>();

        if (currentLevel >= sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        {
            if (currentLevel > STRONG_MAX_LEVEL) // Hardcoded level maximum
                LOG_ERROR("sql.sql", "Wrong (> {}) level {} in `world_player_xp_for_level` table, ignoring.", STRONG_MAX_LEVEL, currentLevel);
            else
            {
                LOG_DEBUG("sql.sql", "Unused (> MaxPlayerLevel in worldserver.conf) level {} in `world_player_xp_for_level` table, ignoring.", currentLevel);
                ++count;  // Make result loading percent "expected" correct in case disabled detail mode for example.
            }
            continue;
        }
        _playerXPPerLevel[currentLevel] = currentXP;
        ++count;
    } while (result->NextRow());

    // Fill level gaps
    for (uint32 level = 1; level < sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL); ++level)
    {
        if (_playerXPPerLevel[level] == 0)
        {
            LOG_ERROR("sql.sql", "Level {} does not have XP for level data. Using data of level [{}] + 100.", level + 1, level);
            _playerXPPerLevel[level] = _playerXPPerLevel[level - 1] + 100;
        }
    }

    LOG_INFO("server.loading", ">> Loaded {} XP For Level Definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadPlayerInfo()
{
    LoadPlayerCreateInfo();
    LoadPlayerCreateInfoItems();
    LoadPlayerCreateInfoSkills();
    LoadPlayerCreateInfoSpells();
    LoadPlayerCreateInfoCastSpell();
    LoadPlayerCreateInfoActions();
    LoadPlayerCreateInfoStats();
    LoadPlayerCreateInfoXP();
}

void ObjectMgr::GetPlayerClassLevelInfo(const uint32 class_, uint8 level, PlayerClassLevelInfo* info) const
{
    if (level < 1 || class_ >= MAX_CLASSES)
        return;

    if (level > sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        level = sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);

    const PlayerClassInfo* pInfo = _playerClassInfo[class_];
    *info = pInfo->levelInfo[level - 1];
}

void ObjectMgr::GetPlayerLevelInfo(const uint32 race, const uint32 class_, const uint8 level, PlayerLevelInfo* info) const
{
    if (level < 1 || race >= sRaceMgr->GetMaxRaces() || class_ >= MAX_CLASSES)
        return;

    const PlayerInfo* pInfo = _playerInfo[race][class_];
    if (!pInfo)
        return;

    if (level <= sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        *info = pInfo->levelInfo[level - 1];
    else
        BuildPlayerLevelInfo(race, class_, level, info);
}

void ObjectMgr::BuildPlayerLevelInfo(const uint8 race, const uint8 class_, const uint8 level, PlayerLevelInfo* plInfo) const
{
    // Base data (last known level)
    *plInfo = _playerInfo[race][class_]->levelInfo[sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL) - 1];

    // If conversion from uint32 to uint8 causes unexpected behavior, change lvl to uint32
    for (uint8 lvl = sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL) - 1; lvl < level; ++lvl)
    {
        switch (class_)
        {
        case CLASS_WARRIOR:
            plInfo->stats[STAT_STRENGTH]  += lvl > 23 ? 2 : lvl > 1  ? 1 : 0;
            plInfo->stats[STAT_STAMINA]   += lvl > 23 ? 2 : lvl > 1  ? 1 : 0;
            plInfo->stats[STAT_AGILITY]   += lvl > 36 ? 1 : lvl > 6 && lvl % 2 ? 1 : 0;
            plInfo->stats[STAT_INTELLECT] += lvl > 9 && !(lvl % 2) ? 1 : 0;
            plInfo->stats[STAT_SPIRIT]    += lvl > 9 && !(lvl % 2) ? 1 : 0;
            break;
        case CLASS_PALADIN:
            plInfo->stats[STAT_STRENGTH]  += lvl > 3  ? 1 : 0;
            plInfo->stats[STAT_STAMINA]   += lvl > 33 ? 2 : lvl > 1 ? 1 : 0;
            plInfo->stats[STAT_AGILITY]   += lvl > 38 ? 1 : lvl > 7 && !(lvl % 2) ? 1 : 0;
            plInfo->stats[STAT_INTELLECT] += lvl > 6 && lvl % 2 ? 1 : 0;
            plInfo->stats[STAT_SPIRIT]    += lvl > 7 ? 1 : 0;
            break;
        case CLASS_HUNTER:
            plInfo->stats[STAT_STRENGTH]  += lvl > 4  ? 1 : 0;
            plInfo->stats[STAT_STAMINA]   += lvl > 4  ? 1 : 0;
            plInfo->stats[STAT_AGILITY]   += lvl > 33 ? 2 : lvl > 1 ? 1 : 0;
            plInfo->stats[STAT_INTELLECT] += lvl > 8 && lvl % 2 ? 1 : 0;
            plInfo->stats[STAT_SPIRIT]    += lvl > 38 ? 1 : lvl > 9 && !(lvl % 2) ? 1 : 0;
            break;
        case CLASS_ROGUE:
            plInfo->stats[STAT_STRENGTH]  += lvl > 5  ? 1 : 0;
            plInfo->stats[STAT_STAMINA]   += lvl > 4  ? 1 : 0;
            plInfo->stats[STAT_AGILITY]   += lvl > 16 ? 2 : lvl > 1 ? 1 : 0;
            plInfo->stats[STAT_INTELLECT] += lvl > 8 && !(lvl % 2) ? 1 : 0;
            plInfo->stats[STAT_SPIRIT]    += lvl > 38 ? 1 : lvl > 9 && !(lvl % 2) ? 1 : 0;
            break;
        case CLASS_PRIEST:
            plInfo->stats[STAT_STRENGTH]  += lvl > 9 && !(lvl % 2) ? 1 : 0;
            plInfo->stats[STAT_STAMINA]   += lvl > 5  ? 1 : 0;
            plInfo->stats[STAT_AGILITY]   += lvl > 38 ? 1 : lvl > 8 && lvl % 2 ? 1 : 0;
            plInfo->stats[STAT_INTELLECT] += lvl > 22 ? 2 : lvl > 1 ? 1 : 0;
            plInfo->stats[STAT_SPIRIT]    += lvl > 3  ? 1 : 0;
            break;
        case CLASS_SHAMAN:
            plInfo->stats[STAT_STRENGTH]  += lvl > 34 ? 1 : lvl > 6 && lvl % 2 ? 1 : 0;
            plInfo->stats[STAT_STAMINA]   += lvl > 4 ? 1 : 0;
            plInfo->stats[STAT_AGILITY]   += lvl > 7 && !(lvl % 2) ? 1 : 0;
            plInfo->stats[STAT_INTELLECT] += lvl > 5 ? 1 : 0;
            plInfo->stats[STAT_SPIRIT]    += lvl > 4 ? 1 : 0;
            break;
        case CLASS_MAGE:
            plInfo->stats[STAT_STRENGTH]  += lvl > 9 && !(lvl % 2) ? 1 : 0;
            plInfo->stats[STAT_STAMINA]   += lvl > 5  ? 1 : 0;
            plInfo->stats[STAT_AGILITY]   += lvl > 9 && !(lvl % 2) ? 1 : 0;
            plInfo->stats[STAT_INTELLECT] += lvl > 24 ? 2 : lvl > 1 ? 1 : 0;
            plInfo->stats[STAT_SPIRIT]    += lvl > 33 ? 2 : lvl > 2 ? 1 : 0;
            break;
        case CLASS_WARLOCK:
            plInfo->stats[STAT_STRENGTH]  += lvl > 9 && !(lvl % 2) ? 1 : 0;
            plInfo->stats[STAT_STAMINA]   += lvl > 38 ? 2 : lvl > 3 ? 1 : 0;
            plInfo->stats[STAT_AGILITY]   += lvl > 9 && !(lvl % 2) ? 1 : 0;
            plInfo->stats[STAT_INTELLECT] += lvl > 33 ? 2 : lvl > 2 ? 1 : 0;
            plInfo->stats[STAT_SPIRIT]    += lvl > 38 ? 2 : lvl > 3 ? 1 : 0;
            break;
        case CLASS_DRUID:
            plInfo->stats[STAT_STRENGTH]  += lvl > 38 ? 2 : lvl > 6 && lvl % 2 ? 1 : 0;
            plInfo->stats[STAT_STAMINA]   += lvl > 32 ? 2 : lvl > 4 ? 1 : 0;
            plInfo->stats[STAT_AGILITY]   += lvl > 38 ? 2 : lvl > 8 && lvl % 2 ? 1 : 0;
            plInfo->stats[STAT_INTELLECT] += lvl > 38 ? 3 : lvl > 4 ? 1 : 0;
            plInfo->stats[STAT_SPIRIT]    += lvl > 38 ? 3 : lvl > 5 ? 1 : 0;
        default:
            break;
        }
    }
}

void ObjectMgr::LoadQuestDetails()
{
    const QueryResult result = WorldDatabase.Query("SELECT id, emote, emote_delay FROM world_quest_details");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 quest details. DB table `world_quest_details` is empty.");
        return;
    }
    do
    {
        const Field* fields = result->Fetch();
        const uint32 questID = fields[0].Get<uint32>();

        if (auto itr = _questTemplates.find(questID); itr != _questTemplates.end())
        {
            const auto emotes = fields[1].GetArray<uint16, QUEST_EMOTE_COUNT>();
            const auto emoteDelays = fields[2].GetArray<uint32, QUEST_EMOTE_COUNT>();

            Quest* qInfo = itr->second;
            for (int i = 0; i < QUEST_EMOTE_COUNT; ++i)
            {
                qInfo->DetailsEmote[i] = emotes[i];
                qInfo->DetailsEmoteDelay[i] = emoteDelays[i];
            }
        }
        else
            LOG_ERROR("sql.sql", "Table `world_quest_details` has data for quest {} but such quest does not exist", questID);
    } while (result->NextRow());
}

void ObjectMgr::LoadQuestRequestItems()
{
    const QueryResult result = WorldDatabase.Query("SELECT id, emote_on_complete, emote_on_incomplete, completion_text FROM world_quest_request_item");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 quest request items. DB table `world_quest_request_item` is empty.");
        return;
    }
    do
    {
        const Field* fields = result->Fetch();
        const uint32 questID = fields[0].Get<uint32>();

        if (auto itr = _questTemplates.find(questID); itr != _questTemplates.end())
        {
            Quest* qInfo = itr->second;
            qInfo->EmoteOnComplete = fields[1].Get<uint16>();
            qInfo->EmoteOnIncomplete = fields[2].Get<uint16>();
            qInfo->RequestItemsText = fields[3].Get<std::string>();
        }
        else
            LOG_ERROR("sql.sql", "Table `world_quest_request_item` has data for quest {} but such quest does not exist", questID);
    } while (result->NextRow());
}

void ObjectMgr::LoadQuestOfferRewards()
{
    const QueryResult result = WorldDatabase.Query("SELECT id, emote, emote_delay, text FROM world_quest_offer_reward");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 quest reward emotes. DB table `world_quest_offer_reward` is empty.");
        return;
    }
    do
    {
        const Field* fields = result->Fetch();
        const uint32 questID = fields[0].Get<uint32>();

        if (auto itr = _questTemplates.find(questID); itr != _questTemplates.end())
        {
            Quest* qInfo = itr->second;
            const auto emotes = fields[1].GetArray<uint16, QUEST_EMOTE_COUNT>();
            const auto emoteDelays = fields[2].GetArray<uint32, QUEST_EMOTE_COUNT>();
            for (int i = 0; i < QUEST_EMOTE_COUNT; ++i)
            {
                qInfo->OfferRewardEmote[i] = emotes[i];
                qInfo->OfferRewardEmoteDelay[i] = emoteDelays[i];
            }
            qInfo->OfferRewardText = fields[3].Get<std::string>();
        }
        else
            LOG_ERROR("sql.sql", "Table `world_quest_offer_reward` has data for quest {} but such quest does not exist", questID);
    } while (result->NextRow());
}

void ObjectMgr::LoadQuestTemplateAddons()
{
    const QueryResult result = WorldDatabase.Query(
        "SELECT id, max_level, allowable_classes, source_spell, prev_quest, next_quest, exclusive_group, quest_breadcrumb, "
        "reward_mail_template, reward_mail_delay, required_skill, required_skill_points, "
        "required_min_rep_faction, required_max_rep_faction, required_min_rep_value, required_max_rep_value, "
        "provided_item_count, reward_mail_sender, special_flags "
        "FROM world_quest_template_addon LEFT JOIN world_quest_mail_sender ON id=quest");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 quest template addons. DB table `world_quest_template_addon` is empty.");
        return;
    }
    do
    {
        const Field* fields = result->Fetch();
        const uint32 questID= fields[0].Get<uint32>();

        if (auto itr = _questTemplates.find(questID); itr != _questTemplates.end())
            itr->second->LoadQuestTemplateAddon(fields);
        else
            LOG_ERROR("sql.sql", "Table `world_quest_template_addon` has data for quest {} but such quest does not exist", questID);
    } while (result->NextRow());
}

void ObjectMgr::LoadQuests()
{
    const uint32 oldMSTime = getMSTime();

    // For reload case
    for (const auto qTemplate: _questTemplates | std::views::values)
        delete qTemplate;
    _questTemplates.clear();

    mExclusiveQuestGroups.clear();

    const QueryResult result = WorldDatabase.Query(
        "SELECT id, quest_type, quest_level, min_level, quest_sort, quest_info, suggested_players, time_allowed, allowable_races, "
        "title, objectives, details, area_description, completed_text, start_item, flags, poi_continent, poi_x, poi_y, poi_priority, "
        "item_drop, item_drop_quantity, required_faction1, required_faction2, required_faction_value1, required_faction_value2, "
        "required_npc_or_go, required_npc_or_go_count, objective_text, required_item, required_item_count, required_player_kills, "
        "reward_item, reward_amount, reward_choice_item, reward_choice_item_quantity, reward_faction, reward_faction_value, reward_faction_override, "
        "reward_money, reward_money_difficulty, reward_arena_points, reward_honor, reward_kill_honor, reward_next_quest, "
        "reward_spell, reward_display_spell, reward_talents, reward_title, reward_xp_difficulty "
        "FROM world_quest_template");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 quests definitions. DB table `world_quest_template` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    // Create multimap previous quest for each existed quest.
    // Some quests can have many previous maps set by NextQuestId in previous quest.
    // For example, set of race quests can lead to single not race specific quest.
    do
    {
        const Field* fields = result->Fetch();
        const auto newQuest = new Quest(fields);
        _questTemplates[newQuest->GetQuestId()] = newQuest;
    } while (result->NextRow());

    {
        uint32 max = 0;
        for (const auto &entry: _questTemplates | std::views::keys)
            if (entry > max)
                max = entry;
        if (max)
        {
            _questTemplatesFast.clear();
            _questTemplatesFast.resize(max + 1, nullptr);
            for (auto &[entry, qTemplate] : _questTemplates)
                _questTemplatesFast[entry] = qTemplate;
        }
    }

    for (const auto &qTemplate: _questTemplates | std::views::values)
        qTemplate->InitializeQueryData();

    std::map<uint32, uint32> usedMailTemplates;

    LoadQuestDetails();
    LoadQuestRequestItems();
    LoadQuestOfferRewards();
    LoadQuestTemplateAddons();

    // Post-processing
    for (auto iter = _questTemplates.begin(); iter != _questTemplates.end(); ++iter)
    {
        // Skip post-loading checks for disabled quests
        if (sDisableMgr->IsDisabledFor(DISABLE_TYPE_QUEST, iter->first, nullptr))
            continue;

        Quest* qInfo = iter->second;

        // Additional quest integrity checks (GO, creature_template and item_template must be loaded already)

        if (qInfo->GetQuestMethod() >= 3)
            LOG_ERROR("sql.sql", "Quest {} has Method = {}, expected values are 0, 1 or 2.", qInfo->GetQuestId(), qInfo->GetQuestMethod());

        if (qInfo->SpecialFlags & ~QUEST_SPECIAL_FLAGS_DB_ALLOWED)
        {
            LOG_ERROR("sql.sql", "Quest {} has SpecialFlags = {} > max allowed value. Correct SpecialFlags to value <= {}",
                qInfo->GetQuestId(), qInfo->SpecialFlags, QUEST_SPECIAL_FLAGS_DB_ALLOWED);
            qInfo->SpecialFlags &= QUEST_SPECIAL_FLAGS_DB_ALLOWED;
        }

        if (qInfo->Flags & QUEST_FLAGS_DAILY && qInfo->Flags & QUEST_FLAGS_WEEKLY)
        {
            LOG_ERROR("sql.sql", "Weekly Quest {} is marked as daily quest in Flags, removed daily flag.", qInfo->GetQuestId());
            qInfo->Flags &= ~QUEST_FLAGS_DAILY;
        }

        if (qInfo->Flags & QUEST_FLAGS_DAILY)
        {
            if (!(qInfo->SpecialFlags & QUEST_SPECIAL_FLAGS_REPEATABLE))
            {
                LOG_ERROR("sql.sql", "Daily Quest {} not marked as repeatable in SpecialFlags, added.", qInfo->GetQuestId());
                qInfo->SpecialFlags |= QUEST_SPECIAL_FLAGS_REPEATABLE;
            }
        }

        if (qInfo->Flags & QUEST_FLAGS_WEEKLY)
        {
            if (!(qInfo->SpecialFlags & QUEST_SPECIAL_FLAGS_REPEATABLE))
            {
                LOG_ERROR("sql.sql", "Weekly Quest {} not marked as repeatable in SpecialFlags, added.", qInfo->GetQuestId());
                qInfo->SpecialFlags |= QUEST_SPECIAL_FLAGS_REPEATABLE;
            }
        }

        if (qInfo->SpecialFlags & QUEST_SPECIAL_FLAGS_MONTHLY)
        {
            if (!(qInfo->SpecialFlags & QUEST_SPECIAL_FLAGS_REPEATABLE))
            {
                LOG_ERROR("sql.sql", "Monthly quest {} not marked as repeatable in SpecialFlags, added.", qInfo->GetQuestId());
                qInfo->SpecialFlags |= QUEST_SPECIAL_FLAGS_REPEATABLE;
            }
        }

        if (qInfo->Flags & QUEST_FLAGS_TRACKING)
        {
            // At auto-reward can be rewarded only RewardChoiceItemId[0]
            for (int j = 1; j < QUEST_REWARD_CHOICES_COUNT; ++j )
            {
                if (uint32 id = qInfo->RewardChoiceItemId[j])
                {
                    LOG_ERROR("sql.sql", "Quest {} has RewardChoiceItemId[{}] = {} but item from RewardChoiceItemId[{}] can't be rewarded with quest flag QUEST_FLAGS_TRACKING.",
                        qInfo->GetQuestId(), j, id, j);
                    // No changes, quest ignore this data
                }
            }
        }

        // Client quest log visual (area case)
        if (qInfo->ZoneOrSort > 0)
        {
            if (!sAreaTableStore.LookupEntry(qInfo->ZoneOrSort))
            {
                LOG_ERROR("sql.sql", "Quest {} has ZoneOrSort = {} (zone case) but zone with this id does not exist.", qInfo->GetQuestId(), qInfo->ZoneOrSort);
                // No changes, quest not dependent from this value but can have problems at client
            }
        }

        // Client quest log visual (sort case)
        if (qInfo->ZoneOrSort < 0)
        {
            if (!sQuestSortStore.LookupEntry(-qInfo->ZoneOrSort))
            {
                LOG_ERROR("sql.sql", "Quest {} has ZoneOrSort = {} (sort case) but quest sort with this id does not exist.", qInfo->GetQuestId(), qInfo->ZoneOrSort);
                // No changes, quest not dependent from this value but can have problems at client (note some may be 0, we must allow this so no check)
            }

            // Check for proper RequiredSkillId value (skill case)
            if (uint32 skill_id = SkillByQuestSort(-qInfo->ZoneOrSort))
            {
                if (qInfo->RequiredSkillId != skill_id)
                {
                    LOG_ERROR("sql.sql", "Quest {} has ZoneOrSort = {} but RequiredSkillId does not have a corresponding value ({}).", qInfo->GetQuestId(), qInfo->ZoneOrSort, skill_id);
                    // Override, and force proper value here?
                }
            }
        }

        // RequiredClasses, can be 0/CLASS_MASK_ALL_PLAYABLE to allow any class
        if (qInfo->RequiredClasses && !(qInfo->RequiredClasses & CLASS_MASK_ALL_PLAYABLE))
        {
            LOG_ERROR("sql.sql", "Quest {} does not contain any playable classes in RequiredClasses ({}), value set to 0 (all classes).", qInfo->GetQuestId(), qInfo->RequiredClasses);
            qInfo->RequiredClasses = 0;
        }

        // AllowableRaces, can be 0/PlayableRaceMask to allow any race
        if (qInfo->AllowableRaces && !(qInfo->AllowableRaces & sRaceMgr->GetPlayableRaceMask()))
        {
            LOG_ERROR("sql.sql", "Quest {} does not contain any playable races in AllowableRaces ({}), value set to 0 (all races).", qInfo->GetQuestId(), qInfo->AllowableRaces);
            qInfo->AllowableRaces = 0;
        }

        // RequiredSkillId, can be 0
        if (qInfo->RequiredSkillId && !sSkillLineStore.LookupEntry(qInfo->RequiredSkillId))
        {
            LOG_ERROR("sql.sql", "Quest {} has RequiredSkillId = {} but this skill does not exist", qInfo->GetQuestId(), qInfo->RequiredSkillId);
        }

        if (qInfo->RequiredSkillPoints && qInfo->RequiredSkillPoints > sWorld->GetConfigMaxSkillValue())
        {
            LOG_ERROR("sql.sql", "Quest {} has RequiredSkillPoints = {} but max possible skill is {}, quest can't be done.",
                qInfo->GetQuestId(), qInfo->RequiredSkillPoints, sWorld->GetConfigMaxSkillValue());
            // No changes, quest can't be done for this requirement
        }
        // else Skill quests can have 0 skill level, this is ok

        if (qInfo->RequiredFactionId2 && !sFactionStore.LookupEntry(qInfo->RequiredFactionId2))
        {
            LOG_ERROR("sql.sql", "Quest {} has RequiredFactionId2 = {} but faction template {} does not exist, quest can't be done.",
                qInfo->GetQuestId(), qInfo->RequiredFactionId2, qInfo->RequiredFactionId2);
            // No changes, quest can't be done for this requirement
        }

        if (qInfo->RequiredFactionId1 && !sFactionStore.LookupEntry(qInfo->RequiredFactionId1))
        {
            LOG_ERROR("sql.sql", "Quest {} has RequiredFactionId1 = {} but faction template {} does not exist, quest can't be done.",
                qInfo->GetQuestId(), qInfo->RequiredFactionId1, qInfo->RequiredFactionId1);
            // No changes, quest can't be done for this requirement
        }

        if (qInfo->RequiredMinRepFaction && !sFactionStore.LookupEntry(qInfo->RequiredMinRepFaction))
        {
            LOG_ERROR("sql.sql", "Quest {} has RequiredMinRepFaction = {} but faction template {} does not exist, quest can't be done.",
                qInfo->GetQuestId(), qInfo->RequiredMinRepFaction, qInfo->RequiredMinRepFaction);
            // No changes, quest can't be done for this requirement
        }

        if (qInfo->RequiredMaxRepFaction && !sFactionStore.LookupEntry(qInfo->RequiredMaxRepFaction))
        {
            LOG_ERROR("sql.sql", "Quest {} has RequiredMaxRepFaction = {} but faction template {} does not exist, quest can't be done.",
                qInfo->GetQuestId(), qInfo->RequiredMaxRepFaction, qInfo->RequiredMaxRepFaction);
            // No changes, quest can't be done for this requirement
        }

        if (qInfo->RequiredMinRepValue && qInfo->RequiredMinRepValue > ReputationMgr::Reputation_Cap)
        {
            LOG_ERROR("sql.sql", "Quest {} has RequiredMinRepValue = {} but max reputation is {}, quest can't be done.",
                qInfo->GetQuestId(), qInfo->RequiredMinRepValue, ReputationMgr::Reputation_Cap);
            // No changes, quest can't be done for this requirement
        }

        if (qInfo->RequiredMinRepValue && qInfo->RequiredMaxRepValue && qInfo->RequiredMaxRepValue <= qInfo->RequiredMinRepValue)
        {
            LOG_ERROR("sql.sql", "Quest {} has RequiredMaxRepValue = {} and RequiredMinRepValue = {}, quest can't be done.",
                qInfo->GetQuestId(), qInfo->RequiredMaxRepValue, qInfo->RequiredMinRepValue);
            // No changes, quest can't be done for this requirement
        }

        if (!qInfo->RequiredFactionId1 && qInfo->RequiredFactionValue1 != 0)
        {
            LOG_ERROR("sql.sql", "Quest {} has RequiredFactionValue1 = {} but RequiredFactionId1 is 0, value has no effect", qInfo->GetQuestId(), qInfo->RequiredFactionValue1);
            // Warning
        }

        if (!qInfo->RequiredFactionId2 && qInfo->RequiredFactionValue2 != 0)
        {
            LOG_ERROR("sql.sql", "Quest {} has RequiredFactionValue2 = {} but RequiredFactionId2 is 0, value has no effect", qInfo->GetQuestId(), qInfo->RequiredFactionValue2);
            // Warning
        }

        if (!qInfo->RequiredMinRepFaction && qInfo->RequiredMinRepValue != 0)
        {
            LOG_ERROR("sql.sql", "Quest {} has RequiredMinRepValue = {} but RequiredMinRepFaction is 0, value has no effect", qInfo->GetQuestId(), qInfo->RequiredMinRepValue);
            // Warning
        }

        if (!qInfo->RequiredMaxRepFaction && qInfo->RequiredMaxRepValue != 0)
        {
            LOG_ERROR("sql.sql", "Quest {} has RequiredMaxRepValue = {} but RequiredMaxRepFaction is 0, value has no effect", qInfo->GetQuestId(), qInfo->RequiredMaxRepValue);
            // Warning
        }

        if (qInfo->RewardTitleId && !sCharTitlesStore.LookupEntry(qInfo->RewardTitleId))
        {
            LOG_ERROR("sql.sql", "Quest {} has RewardTitleId = {} but CharTitle Id {} does not exist, quest can't be rewarded with title.",
                qInfo->GetQuestId(), qInfo->GetCharTitleId(), qInfo->GetCharTitleId());
            qInfo->RewardTitleId = 0;
            // Quest can't reward this title
        }

        if (qInfo->StartItem)
        {
            if (!GetItemTemplate(qInfo->StartItem))
            {
                LOG_ERROR("sql.sql", "Quest {} has StartItem = {} but item with entry {} does not exist, quest can't be done.", qInfo->GetQuestId(), qInfo->StartItem, qInfo->StartItem);
                qInfo->StartItem = 0;  // Quest can't be done for this requirement
            }
            else if (qInfo->StartItemCount == 0)
            {
                LOG_ERROR("sql.sql", "Quest {} has StartItem = {} but StartItemCount = 0, set to 1 but need fix in DB.", qInfo->GetQuestId(), qInfo->StartItem);
                qInfo->StartItemCount = 1;  // Update to 1 for allow quest work for backward compatibility with DB
            }
        }
        else if (qInfo->StartItemCount > 0)
        {
            LOG_ERROR("sql.sql", "Quest {} has StartItem = 0 but StartItemCount = {}, useless value.", qInfo->GetQuestId(), qInfo->StartItemCount);
            qInfo->StartItemCount = 0;  // No quest work changes in fact
        }

        if (qInfo->SourceSpellID)
        {
            const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(qInfo->SourceSpellID);
            if (!spellInfo)
            {
                LOG_ERROR("sql.sql", "Quest {} has SourceSpellID = {} but spell {} doesn't exist, quest can't be done.", qInfo->GetQuestId(), qInfo->SourceSpellID, qInfo->SourceSpellID);
                qInfo->SourceSpellID = 0;  // Quest can't be done for this requirement
            }
            else if (!SpellMgr::ComputeIsSpellValid(spellInfo))
            {
                LOG_ERROR("sql.sql", "Quest {} has SourceSpellID = {} but spell {} is broken, quest can't be done.", qInfo->GetQuestId(), qInfo->SourceSpellID, qInfo->SourceSpellID);
                qInfo->SourceSpellID = 0;  // Quest can't be done for this requirement
            }
        }

        for (uint8 j = 0; j < QUEST_ITEM_OBJECTIVES_COUNT; ++j)
        {
            if (uint32 id = qInfo->RequiredItemId[j])
            {
                if (qInfo->RequiredItemCount[j] == 0)
                {
                    LOG_ERROR("sql.sql", "Quest {} has RequiredItemId[{}] = {} but RequiredItemCount[{}] = 0, quest can't be done.", qInfo->GetQuestId(), j, id, j);
                    // No changes, quest can't be done for this requirement
                }

                qInfo->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);

                if (!GetItemTemplate(id))
                {
                    LOG_ERROR("sql.sql", "Quest {} has RequiredItemId[{}] = {} but item with entry {} does not exist, quest can't be done.", qInfo->GetQuestId(), j, id, id);
                    qInfo->RequiredItemCount[j] = 0;  // Prevent incorrect work of quest
                }
            }
            else if (qInfo->RequiredItemCount[j] > 0)
            {
                LOG_ERROR("sql.sql", "Quest {} has RequiredItemId[{}] = 0 but RequiredItemCount[{}] = {}, quest can't be done.", qInfo->GetQuestId(), j, j, qInfo->RequiredItemCount[j]);
                qInfo->RequiredItemCount[j] = 0;  // Prevent incorrect work of quest
            }
        }

        for (uint8 j = 0; j < QUEST_SOURCE_ITEM_IDS_COUNT; ++j)
        {
            if (uint32 id = qInfo->ItemDrop[j])
            {
                if (!GetItemTemplate(id))
                {
                    LOG_ERROR("sql.sql", "Quest {} has ItemDrop[{}] = {} but item with entry {} does not exist, quest can't be done.", qInfo->GetQuestId(), j, id, id);
                    // No changes, quest can't be done for this requirement
                }
            }
            else
            {
                if (qInfo->ItemDropQuantity[j] > 0)
                {
                    LOG_ERROR("sql.sql", "Quest {} has ItemDrop[{}] = 0 but ItemDropQuantity[{}] = {}.", qInfo->GetQuestId(), j, j, qInfo->ItemDropQuantity[j]);
                    // No changes, quest ignore this data
                }
            }
        }

        for (uint8 j = 0; j < QUEST_OBJECTIVES_COUNT; ++j)
        {
            int32 id = qInfo->RequiredNpcOrGo[j];
            if (id < 0 && !GetGameObjectTemplate(-id))
            {
                LOG_ERROR("sql.sql", "Quest {} has RequiredNpcOrGo[{}] = {} but GameObject {} does not exist, quest can't be done.", qInfo->GetQuestId(), j, id, static_cast<uint32>(-id));
                qInfo->RequiredNpcOrGo[j] = 0;  // Quest can't be done for this requirement
            }

            if (id > 0 && !GetCreatureTemplate(id))
            {
                LOG_ERROR("sql.sql", "Quest {} has RequiredNpcOrGo[{}] = {} but creature with entry {} does not exist, quest can't be done.", qInfo->GetQuestId(), j, id, static_cast<uint32>(id));
                qInfo->RequiredNpcOrGo[j] = 0; // Quest can't be done for this requirement
            }

            if (id)
            {
                // In fact SpeakTo and Kill are quite same: either you can speak to mob:SpeakTo or you can't:Kill/Cast
                qInfo->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST | QUEST_SPECIAL_FLAGS_SPEAKTO);

                if (!qInfo->RequiredNpcOrGoCount[j])
                {
                    LOG_ERROR("sql.sql", "Quest {} has RequiredNpcOrGo[{}] = {} but RequiredNpcOrGoCount[{}] = 0, quest can't be done.", qInfo->GetQuestId(), j, id, j);
                    // No changes, quest can be incorrectly done, but we already report this
                }
            }
            else if (qInfo->RequiredNpcOrGoCount[j] > 0)
            {
                LOG_ERROR("sql.sql", "Quest {} has RequiredNpcOrGo[{}] = 0 but RequiredNpcOrGoCount[{}] = {}.", qInfo->GetQuestId(), j, j, qInfo->RequiredNpcOrGoCount[j]);
                // No changes, quest ignore this data
            }
        }

        for (uint8 j = 0; j < QUEST_REWARD_CHOICES_COUNT; ++j)
        {
            if (uint32 id = qInfo->RewardChoiceItemId[j])
            {
                if (!GetItemTemplate(id))
                {
                    LOG_ERROR("sql.sql", "Quest {} has RewardChoiceItemId[{}] = {} but item with entry {} does not exist, quest will not reward this item.", qInfo->GetQuestId(), j, id, id);
                    qInfo->RewardChoiceItemId[j] = 0;  // No changes, quest will not reward this
                }

                if (!qInfo->RewardChoiceItemCount[j])
                {
                    LOG_ERROR("sql.sql", "Quest {} has RewardChoiceItemId[{}] = {} but RewardChoiceItemCount[{}] = 0, quest can't be done.", qInfo->GetQuestId(), j, id, j);
                    // No changes, quest can't be done
                }
            }
            else if (qInfo->RewardChoiceItemCount[j] > 0)
            {
                LOG_ERROR("sql.sql", "Quest {} has RewardChoiceItemId[{}] = 0 but RewardChoiceItemCount[{}] = {}.", qInfo->GetQuestId(), j, j, qInfo->RewardChoiceItemCount[j]);
                // No changes, quest ignore this data
            }
        }

        for (uint8 j = 0; j < QUEST_REWARDS_COUNT; ++j)
        {
            if (!qInfo->RewardItemId[0] && qInfo->RewardItemId[j])
                LOG_ERROR("sql.sql", "Quest {} has no RewardItemId[0] but has RewardItem[{}]. Reward item will not be loaded.", qInfo->GetQuestId(), j);
            if (!qInfo->RewardItemId[1] && j > 1 && qInfo->RewardItemId[j])
                LOG_ERROR("sql.sql", "Quest {} has no RewardItemId[1] but has RewardItem[{}]. Reward item will not be loaded.", qInfo->GetQuestId(), j);
            if (!qInfo->RewardItemId[2] && j > 2 && qInfo->RewardItemId[j])
                LOG_ERROR("sql.sql", "Quest {} has no RewardItemId[2] but has RewardItem[{}]. Reward item will not be loaded.", qInfo->GetQuestId(), j);
        }

        for (uint8 j = 0; j < QUEST_REWARDS_COUNT; ++j)
        {
            if (uint32 id = qInfo->RewardItemId[j])
            {
                if (!GetItemTemplate(id))
                {
                    LOG_ERROR("sql.sql", "Quest {} has RewardItemId[{}] = {} but item with entry {} does not exist, quest will not reward this item.", qInfo->GetQuestId(), j, id, id);
                    qInfo->RewardItemId[j] = 0;  // No changes, quest will not reward this item
                }

                if (!qInfo->RewardItemIdCount[j])
                {
                    LOG_ERROR("sql.sql", "Quest {} has RewardItemId[{}] = {} but RewardItemIdCount[{}] = 0, quest will not reward this item.", qInfo->GetQuestId(), j, id, j);
                    // No changes
                }
            }
            else if (qInfo->RewardItemIdCount[j] > 0)
            {
                LOG_ERROR("sql.sql", "Quest {} has RewardItemId[{}] = 0 but RewardItemIdCount[{}] = {}.", qInfo->GetQuestId(), j, j, qInfo->RewardItemIdCount[j]);
                // No changes, quest ignore this data
            }
        }

        for (uint8 j = 0; j < QUEST_REPUTATIONS_COUNT; ++j)
        {
            if (qInfo->RewardFactionId[j])
            {
                if (std::abs(qInfo->RewardFactionValueId[j]) > 9)
                {
                    LOG_ERROR("sql.sql", "Quest {} has RewardFactionValueId[{}] = {}. That is outside the range of valid values (-9 to 9).", qInfo->GetQuestId(), j, qInfo->RewardFactionValueId[j]);
                }
                if (!sFactionStore.LookupEntry(qInfo->RewardFactionId[j]))
                {
                    LOG_ERROR("sql.sql", "Quest {} has RewardFactionId[{}] = {} but raw faction (faction.dbc) {} does not exist, quest will not reward reputation for this faction.",
                        qInfo->GetQuestId(), j, qInfo->RewardFactionId[j], qInfo->RewardFactionId[j]);
                    qInfo->RewardFactionId[j] = 0;  // Quest will not reward this
                }
            }

            else if (qInfo->RewardFactionValueIdOverride[j] != 0)
            {
                LOG_ERROR("sql.sql", "Quest {} has RewardFactionId[{}] = 0 but RewardFactionValueIdOverride[{}] = {}.", qInfo->GetQuestId(), j, j, qInfo->RewardFactionValueIdOverride[j]);
                // No changes, quest ignore this data
            }
        }

        if (qInfo->RewardDisplaySpell)
        {
            if (const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(qInfo->RewardDisplaySpell); !spellInfo)
            {
                LOG_ERROR("sql.sql", "Quest {} has RewardDisplaySpell = {} but spell {} does not exist, spell removed as display reward.",
                    qInfo->GetQuestId(), qInfo->RewardDisplaySpell, qInfo->RewardDisplaySpell);
                qInfo->RewardDisplaySpell = 0;  // No spell reward will display for this quest
            }

            else if (!SpellMgr::ComputeIsSpellValid(spellInfo))
            {
                LOG_ERROR("sql.sql", "Quest {} has RewardDisplaySpell = {} but spell {} is broken, quest will not have a spell reward.",
                    qInfo->GetQuestId(), qInfo->RewardDisplaySpell, qInfo->RewardDisplaySpell);
                qInfo->RewardDisplaySpell = 0;  // No spell reward will display for this quest
            }

            else if (GetTalentSpellCost(qInfo->RewardDisplaySpell))
            {
                LOG_ERROR("sql.sql", "Quest {} has RewardDisplaySpell = {} but spell {} is talent, quest will not have a spell reward.",
                    qInfo->GetQuestId(), qInfo->RewardDisplaySpell, qInfo->RewardDisplaySpell);
                qInfo->RewardDisplaySpell = 0;  // No spell reward will display for this quest
            }
        }

        if (qInfo->RewardSpell > 0)
        {
            if (const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(qInfo->RewardSpell); !spellInfo)
            {
                LOG_ERROR("sql.sql", "Quest {} has RewardSpell = {} but spell {} does not exist, quest will not have a spell reward.", qInfo->GetQuestId(), qInfo->RewardSpell, qInfo->RewardSpell);
                qInfo->RewardSpell = 0;  // No spell will be cast on player
            }

            else if (!SpellMgr::ComputeIsSpellValid(spellInfo))
            {
                LOG_ERROR("sql.sql", "Quest {} has RewardSpell = {} but spell {} is broken, quest will not have a spell reward.", qInfo->GetQuestId(), qInfo->RewardSpell, qInfo->RewardSpell);
                qInfo->RewardSpell = 0;  // No spell will be cast on player
            }

            else if (GetTalentSpellCost(qInfo->RewardSpell))
            {
                LOG_ERROR("sql.sql", "Quest {} has RewardDisplaySpell = {} but spell {} is talent, quest will not have a spell reward.", qInfo->GetQuestId(), qInfo->RewardSpell, qInfo->RewardSpell);
                qInfo->RewardSpell = 0;  // No spell will be cast on player
            }
        }

        if (qInfo->RewardMailTemplateId)
        {
            if (!sMailTemplateStore.LookupEntry(qInfo->RewardMailTemplateId))
            {
                LOG_ERROR("sql.sql", "Quest {} has RewardMailTemplateId = {} but mail template  {} does not exist, quest will not have a mail reward.",
                    qInfo->GetQuestId(), qInfo->RewardMailTemplateId, qInfo->RewardMailTemplateId);
                qInfo->RewardMailTemplateId = 0;  // No mail will send to player
                qInfo->RewardMailDelay = 0;  // No mail will send to player
                qInfo->RewardMailSenderEntry = 0;
            }
            else if (usedMailTemplates.contains(qInfo->RewardMailTemplateId))
            {
                const auto used_mt_itr = usedMailTemplates.find(qInfo->RewardMailTemplateId);
                LOG_ERROR("sql.sql", "Quest {} has RewardMailTemplateId = {} but mail template  {} already used for quest {}, quest will not have a mail reward.",
                    qInfo->GetQuestId(), qInfo->RewardMailTemplateId, qInfo->RewardMailTemplateId, used_mt_itr->second);
                qInfo->RewardMailTemplateId = 0;  // No mail will send to player
                qInfo->RewardMailDelay = 0;  // No mail will send to player
                qInfo->RewardMailSenderEntry = 0;
            }
            else
                usedMailTemplates[qInfo->RewardMailTemplateId] = qInfo->GetQuestId();
        }

        if (qInfo->RewardNextQuest)
        {
            if (auto qNextItr = _questTemplates.find(qInfo->RewardNextQuest); qNextItr == _questTemplates.end())
            {
                LOG_ERROR("sql.sql", "Quest {} has RewardNextQuest = {} but quest {} does not exist, quest chain will not work.", qInfo->GetQuestId(), qInfo->RewardNextQuest, qInfo->RewardNextQuest);
                qInfo->RewardNextQuest = 0;
            }
            else
                qNextItr->second->prevChainQuests.push_back(qInfo->GetQuestId());
        }

        // Fill additional data stores
        if (qInfo->PrevQuestId)
        {
            if (!_questTemplates.contains(std::abs(qInfo->GetPrevQuestId())))
                LOG_ERROR("sql.sql", "Quest {} has PrevQuestId {}, but no such quest", qInfo->GetQuestId(), qInfo->GetPrevQuestId());
            else
                qInfo->prevQuests.push_back(qInfo->PrevQuestId);
        }

        if (qInfo->NextQuestId)
        {
            if (auto qNextItr = _questTemplates.find(qInfo->GetNextQuestId()); qNextItr == _questTemplates.end())
                LOG_ERROR("sql.sql", "Quest {} has NextQuestId {}, but no such quest", qInfo->GetQuestId(), qInfo->GetNextQuestId());
            else
                qNextItr->second->prevQuests.push_back(static_cast<int32>(qInfo->GetQuestId()));
        }

        if (qInfo->ExclusiveGroup)
            mExclusiveQuestGroups.insert(std::pair(qInfo->ExclusiveGroup, qInfo->GetQuestId()));

        if (uint32 breadcrumbForQuestId = qInfo->GetBreadcrumbForQuestId())
        {
            if (!_questTemplates.contains(breadcrumbForQuestId))
                LOG_ERROR("sql.sql", "Quest {} has BreadcrumbForQuestId {}, but no such quest exists", qInfo->GetQuestId(), breadcrumbForQuestId);
            else
                _breadcrumbsForQuest[breadcrumbForQuestId].push_back(qInfo->GetQuestId());

            if (qInfo->GetNextQuestId())
                LOG_ERROR("sql.sql", "Quest {} is a breadcrumb quest but also has NextQuestID {} set", qInfo->GetQuestId(), qInfo->GetNextQuestId());
            if (qInfo->GetExclusiveGroup())
                LOG_ERROR("sql.sql", "Quest {} is a breadcrumb quest but also has ExclusiveGroup {} set", qInfo->GetQuestId(), qInfo->GetExclusiveGroup());
        }

        if (qInfo->TimeAllowed)
            qInfo->SetSpecialFlag(QUEST_SPECIAL_FLAGS_TIMED);
        if (qInfo->RequiredPlayerKills)
            qInfo->SetSpecialFlag(QUEST_SPECIAL_FLAGS_PLAYER_KILL);
    }

    for (const auto& [questId, quest] : _questTemplates)
    {
        if (sDisableMgr->IsDisabledFor(DISABLE_TYPE_QUEST, questId, nullptr))
            continue;

        uint32 breadcrumbForQuestId = quest->GetBreadcrumbForQuestId();
        if (!breadcrumbForQuestId)
            continue;

        std::set<uint32> visitedQuests;
        visitedQuests.insert(questId);

        while (breadcrumbForQuestId)
        {
            if (!visitedQuests.insert(breadcrumbForQuestId).second)
            {
                LOG_ERROR("sql.sql", "Breadcrumb quests {} and {} form a loop", questId, breadcrumbForQuestId);
                break;
            }

            const Quest* targetQuest = GetQuestTemplate(breadcrumbForQuestId);
            if (!targetQuest)
                break;

            breadcrumbForQuestId = targetQuest->GetBreadcrumbForQuestId();
        }
    }

    // Check QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT for spell with SPELL_EFFECT_QUEST_COMPLETE
    for (uint32 i = 0; i < sSpellMgr->GetSpellInfoStoreSize(); ++i)
    {
        const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(i);
        if (!spellInfo)
            continue;

        for (uint8 j = 0; j < MAX_SPELL_EFFECTS; ++j)
        {
            if (spellInfo->Effects[j].Effect != SPELL_EFFECT_QUEST_COMPLETE)
                continue;

            uint32 quest_id = spellInfo->Effects[j].MiscValue;

            const Quest* quest = GetQuestTemplate(quest_id);

            // Some quest referenced in spells not exist (outdated spells)
            if (!quest)
                continue;

            if (!quest->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT))
            {
                LOG_ERROR("sql.sql", "Spell (id: {}) have SPELL_EFFECT_QUEST_COMPLETE for quest {}, but quest not have SpecialFlag QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT. "
                                     "Quest flags must be fixed, quest modified to enable objective.", spellInfo->ID, quest_id);
                // This will prevent quest completing without objective
            }
        }
    }

    LOG_INFO("server.loading", ">> Loaded {} Quests Definitions in {} ms", _questTemplates.size(), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadScripts(ScriptsType type)
{
    const uint32 oldMSTime = getMSTime();

    ScriptMapMap* scripts = GetScriptsMapByType(type);
    if (!scripts)
        return;

    std::string tableName;
    if (tableName.empty())
        return;

    if (sScriptMgr->IsScriptScheduled())  // Function cannot be called when scripts are in use.
        return;



    scripts->clear();  // Need for reload support

    bool isSpellScriptTable = false;

    std::string sql;
    switch (type)
    {
    case SCRIPTS_SPELL:
        sql = "SELECT entry, delay, command, data_i, data_f, effect_index FROM world_spell_script";
        tableName = "world_spell_script";
        isSpellScriptTable = true;
        break;
    case SCRIPTS_EVENT:
        sql = "SELECT entry, delay, command, data_i, data_f FROM world_event_script";
        tableName = "world_event_script";
        break;
    case SCRIPTS_WAYPOINT:
        sql = "SELECT entry, delay, command, data_i, data_f FROM world_waypoint_script";
        tableName = "world_waypoint_script";
        break;
    default:
        return;
    }
    LOG_INFO("server.loading", "Loading {}...", tableName);

    const QueryResult result = WorldDatabase.Query(sql);
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 script definitions. DB table `{}` is empty!", tableName);
        return;
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();
        ScriptInfo tmp{};
        tmp.type = type;
        tmp.id = fields[0].Get<uint32>();
        if (isSpellScriptTable)
            tmp.id |= fields[5].Get<uint8>() << 24;
        tmp.delay = fields[1].Get<uint32>();
        tmp.command = static_cast<ScriptCommands>(fields[2].Get<uint32>());
        const auto dataI = fields[3].GetArray<int32, 3>();
        tmp.Raw.nData[0] = static_cast<uint32>(dataI[0]);
        tmp.Raw.nData[1] = static_cast<uint32>(dataI[1]);
        tmp.Raw.nData[2] = dataI[2];
        const auto dataF = fields[4].GetArray<float, 4>();
        tmp.Raw.fData[0] = dataF[0];
        tmp.Raw.fData[1] = dataF[1];
        tmp.Raw.fData[2] = dataF[2];
        tmp.Raw.fData[3] = dataF[3];

        // Generic command args check
        switch (tmp.command)
        {
            case SCRIPT_COMMAND_TALK:
                {
                    if (tmp.Talk.ChatType > CHAT_TYPE_WHISPER && tmp.Talk.ChatType != CHAT_MSG_RAID_BOSS_WHISPER)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has invalid talk type (data_i[0] = {}) in SCRIPT_COMMAND_TALK for script id {}", tableName, tmp.Talk.ChatType, tmp.id);
                        continue;
                    }
                    if (!GetBroadcastText(static_cast<uint32>(tmp.Talk.TextID)))
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has invalid talk text id (data_i[2] = {}) in SCRIPT_COMMAND_TALK for script id {}", tableName, tmp.Talk.TextID, tmp.id);
                        continue;
                    }
                    break;
                }

            case SCRIPT_COMMAND_EMOTE:
                {
                    if (!sEmotesStore.LookupEntry(tmp.Emote.EmoteID))
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has invalid emote id (data_i[0] = {}) in SCRIPT_COMMAND_EMOTE for script id {}", tableName, tmp.Emote.EmoteID, tmp.id);
                        continue;
                    }
                    break;
                }

            case SCRIPT_COMMAND_TELEPORT_TO:
                {
                    if (!sMapStore.LookupEntry(tmp.TeleportTo.MapID))
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has invalid map (Id: {}) in SCRIPT_COMMAND_TELEPORT_TO for script id {}", tableName, tmp.TeleportTo.MapID, tmp.id);
                        continue;
                    }

                    if (!Acore::IsValidMapCoord(tmp.TeleportTo.DestX, tmp.TeleportTo.DestY, tmp.TeleportTo.DestZ, tmp.TeleportTo.Orientation))
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has invalid coordinates (X: {} Y: {} Z: {} O: {}) in SCRIPT_COMMAND_TELEPORT_TO for script id {}",
                            tableName, tmp.TeleportTo.DestX, tmp.TeleportTo.DestY, tmp.TeleportTo.DestZ, tmp.TeleportTo.Orientation, tmp.id);
                        continue;
                    }
                    break;
                }

            case SCRIPT_COMMAND_QUEST_EXPLORED:
                {
                    const Quest* quest = GetQuestTemplate(tmp.QuestExplored.QuestID);
                    if (!quest)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has invalid quest (ID: {}) in SCRIPT_COMMAND_QUEST_EXPLORED in `data_i[0]` for script id {}", tableName, tmp.QuestExplored.QuestID, tmp.id);
                        continue;
                    }

                    if (!quest->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT))
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has quest (ID: {}) in SCRIPT_COMMAND_QUEST_EXPLORED in `data_i[0]` for script id {}, "
                                             "but quest not have SpecialFlag QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT in quest flags. "
                                             "Script command or quest flags wrong. Quest modified to require objective.", tableName, tmp.QuestExplored.QuestID, tmp.id);

                        // This will prevent quest completing without objective
                        const_cast<Quest*>(quest)->SetSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT);
                        // continue; - quest objective requirement set and command can be allowed
                    }

                    if (static_cast<float>(tmp.QuestExplored.Distance) > DEFAULT_VISIBILITY_DISTANCE)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has too large distance ({}) for exploring objective complete in `data_i[1]` "
                                             "in SCRIPT_COMMAND_QUEST_EXPLORED in `data_i[0]` for script id {}", tableName, tmp.QuestExplored.Distance, tmp.id);
                        continue;
                    }

                    if (tmp.QuestExplored.Distance && static_cast<float>(tmp.QuestExplored.Distance) > DEFAULT_VISIBILITY_DISTANCE)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has too large distance ({}) for exploring objective complete in `data_i[1]` "
                                             "in SCRIPT_COMMAND_QUEST_EXPLORED in `data_i[0]` for script id {}, "
                                             "max distance is {} or 0 for disable distance check", tableName, tmp.QuestExplored.Distance, tmp.id, DEFAULT_VISIBILITY_DISTANCE);
                        continue;
                    }

                    if (tmp.QuestExplored.Distance && static_cast<float>(tmp.QuestExplored.Distance) < INTERACTION_DISTANCE)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has too small distance ({}) for exploring objective complete in `data_i[1]` "
                                             "in SCRIPT_COMMAND_QUEST_EXPLORED in `data_i[0]` for script id {}, "
                                             "min distance is {} or 0 for disable distance check", tableName, tmp.QuestExplored.Distance, tmp.id, INTERACTION_DISTANCE);
                        continue;
                    }

                    break;
                }

            case SCRIPT_COMMAND_KILL_CREDIT:
                {
                    if (!GetCreatureTemplate(tmp.KillCredit.CreatureEntry))
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has invalid Creature (Entry: {}) in SCRIPT_COMMAND_KILL_CREDIT for script id {}",
                            tableName, tmp.KillCredit.CreatureEntry, tmp.id);
                        continue;
                    }
                    break;
                }

            case SCRIPT_COMMAND_RESPAWN_GAME_OBJECT:
                {
                    const GameObjectData* data = GetGameObjectData(tmp.RespawnGameObject.GOGuid);
                    if (!data)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has invalid GameObject (GUID: {}) in SCRIPT_COMMAND_RESPAWN_GAME_OBJECT for script id {}",
                            tableName, tmp.RespawnGameObject.GOGuid, tmp.id);
                        continue;
                    }

                    const GameObjectTemplate* info = GetGameObjectTemplate(data->id);
                    if (!info)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has GameObject with invalid entry (GUID: {} Entry: {}) in SCRIPT_COMMAND_RESPAWN_GAME_OBJECT for script id {}",
                            tableName, tmp.RespawnGameObject.GOGuid, data->id, tmp.id);
                        continue;
                    }

                    if (info->Type == GAME_OBJECT_TYPE_FISHING_NODE ||
                        info->Type == GAME_OBJECT_TYPE_FISHING_HOLE ||
                        info->Type == GAME_OBJECT_TYPE_DOOR        ||
                        info->Type == GAME_OBJECT_TYPE_BUTTON      ||
                        info->Type == GAME_OBJECT_TYPE_TRAP)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` have GameObject type ({}) unsupported by command SCRIPT_COMMAND_RESPAWN_GAME_OBJECT for script id {}", tableName, info->Entry, tmp.id);
                        continue;
                    }
                    break;
                }

            case SCRIPT_COMMAND_TEMP_SUMMON_CREATURE:
                {
                    if (!Acore::IsValidMapCoord(tmp.TempSummonCreature.PosX, tmp.TempSummonCreature.PosY, tmp.TempSummonCreature.PosZ, tmp.TempSummonCreature.Orientation))
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has invalid coordinates (X: {} Y: {} Z: {} O: {}) in SCRIPT_COMMAND_TEMP_SUMMON_CREATURE for script id {}",
                            tableName, tmp.TempSummonCreature.PosX, tmp.TempSummonCreature.PosY, tmp.TempSummonCreature.PosZ, tmp.TempSummonCreature.Orientation, tmp.id);
                        continue;
                    }

                    if (!GetCreatureTemplate(tmp.TempSummonCreature.CreatureEntry))
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has invalid creature (Entry: {}) in SCRIPT_COMMAND_TEMP_SUMMON_CREATURE for script id {}",
                            tableName, tmp.TempSummonCreature.CreatureEntry, tmp.id);
                        continue;
                    }
                    break;
                }

            case SCRIPT_COMMAND_OPEN_DOOR:
            case SCRIPT_COMMAND_CLOSE_DOOR:
                {
                    const GameObjectData* data = GetGameObjectData(tmp.ToggleDoor.GOGuid);
                    if (!data)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has invalid GameObject (GUID: {}) in {} for script id {}",
                            tableName, tmp.ToggleDoor.GOGuid, GetScriptCommandName(tmp.command), tmp.id);
                        continue;
                    }

                    const GameObjectTemplate* info = GetGameObjectTemplate(data->id);
                    if (!info)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has GameObject with invalid entry (GUID: {} Entry: {}) in {} for script id {}",
                            tableName, tmp.ToggleDoor.GOGuid, data->id, GetScriptCommandName(tmp.command), tmp.id);
                        continue;
                    }

                    if (info->Type != GAME_OBJECT_TYPE_DOOR)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has GameObject type ({}) non supported by command {} for script id {}",
                            tableName, info->Entry, GetScriptCommandName(tmp.command), tmp.id);
                        continue;
                    }

                    break;
                }

            case SCRIPT_COMMAND_REMOVE_AURA:
                {
                    if (!sSpellMgr->GetSpellInfo(tmp.RemoveAura.SpellID))
                    {
                        LOG_ERROR("sql.sql", "Table `{}` using non-existent spell (id: {}) in SCRIPT_COMMAND_REMOVE_AURA for script id {}",
                            tableName, tmp.RemoveAura.SpellID, tmp.id);
                        continue;
                    }
                    if (tmp.RemoveAura.Flags & ~0x1)  // 1 bits (0, 1)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` using unknown flags in data_i[1] ({}) in SCRIPT_COMMAND_REMOVE_AURA for script id {}",
                            tableName, tmp.RemoveAura.Flags, tmp.id);
                        continue;
                    }
                    break;
                }

            case SCRIPT_COMMAND_CAST_SPELL:
                {
                    if (!sSpellMgr->GetSpellInfo(tmp.CastSpell.SpellID))
                    {
                        LOG_ERROR("sql.sql", "Table `{}` using non-existent spell (id: {}) in SCRIPT_COMMAND_CAST_SPELL for script id {}", tableName, tmp.CastSpell.SpellID, tmp.id);
                        continue;
                    }
                    if (tmp.CastSpell.Flags > 4)  // Targeting type
                    {
                        LOG_ERROR("sql.sql", "Table `{}` using unknown target in data_i[1] ({}) in SCRIPT_COMMAND_CAST_SPELL for script id {}", tableName, tmp.CastSpell.Flags, tmp.id);
                        continue;
                    }
                    if (tmp.CastSpell.Flags != 4 && tmp.CastSpell.CreatureEntry & ~0x1)  // 1 bit (0, 1)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` using unknown flags in data_i[2] ({}) in SCRIPT_COMMAND_CAST_SPELL for script id {}", tableName, tmp.CastSpell.CreatureEntry, tmp.id);
                        continue;
                    }
                    if (tmp.CastSpell.Flags == 4 && !GetCreatureTemplate(tmp.CastSpell.CreatureEntry))
                    {
                        LOG_ERROR("sql.sql", "Table `{}` using invalid creature entry in data_i[2] ({}) in SCRIPT_COMMAND_CAST_SPELL for script id {}", tableName, tmp.CastSpell.CreatureEntry, tmp.id);
                        continue;
                    }
                    break;
                }

            case SCRIPT_COMMAND_CREATE_ITEM:
                {
                    if (!GetItemTemplate(tmp.CreateItem.ItemEntry))
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has nonexistent item (entry: {}) in SCRIPT_COMMAND_CREATE_ITEM for script id {}", tableName, tmp.CreateItem.ItemEntry, tmp.id);
                        continue;
                    }
                    if (!tmp.CreateItem.Amount)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` SCRIPT_COMMAND_CREATE_ITEM but amount is {} for script id {}", tableName, tmp.CreateItem.Amount, tmp.id);
                        continue;
                    }
                    break;
                }
            default:
                break;
        }

        if (!scripts->contains(tmp.id))
        {
            ScriptMap emptyMap;
            (*scripts)[tmp.id] = emptyMap;
        }
        (*scripts)[tmp.id].insert(std::pair(tmp.delay, tmp));

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} script definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadSpellScripts()
{
    LoadScripts(SCRIPTS_SPELL);

    for (const auto &key: sSpellScripts | std::views::keys)
    {
        uint32 spellID = static_cast<uint32>(key) & 0x00FFFFFF;

        const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(spellID);
        if (!spellInfo)
        {
            LOG_ERROR("sql.sql", "Table `world_spell_script` has not existing spell (Id: {}) as script id", spellID);
            continue;
        }

        const auto i = static_cast<SpellEffIndex>((static_cast<uint32>(key) >> 24) & 0x000000FF);
        if (static_cast<uint32>(i) >= MAX_SPELL_EFFECTS)
        {
            LOG_ERROR("sql.sql", "Table `world_spell_script` has too high effect index {} for spell (Id: {}) as script id", static_cast<uint32>(i), spellID);
        }

        // Check for correct spell_effect
        if (!spellInfo->Effects[i].Effect || (spellInfo->Effects[i].Effect != SPELL_EFFECT_SCRIPT_EFFECT && spellInfo->Effects[i].Effect != SPELL_EFFECT_DUMMY))
            LOG_ERROR("sql.sql", "Table `world_spell_script` - spell {} effect {} is not SPELL_EFFECT_SCRIPT_EFFECT or SPELL_EFFECT_DUMMY", spellID, static_cast<uint32>(i));
    }
}

void ObjectMgr::LoadEventScripts()
{
    LoadScripts(SCRIPTS_EVENT);

    std::set<uint32> evt_scripts;
    // Load all possible script entries from GameObjects
    const GameObjectTemplateContainer* goTemplateContainer = GetGameObjectTemplates();
    for (const auto &goTemplate: *goTemplateContainer | std::views::values)
        if (uint32 eventId = goTemplate.GetEventScriptId())
            evt_scripts.insert(eventId);

    // Load all possible script entries from spells
    for (uint32 i = 1; i < sSpellMgr->GetSpellInfoStoreSize(); ++i)
        if (const SpellInfo* spell = sSpellMgr->GetSpellInfo(i))
            for (uint8 j = 0; j < MAX_SPELL_EFFECTS; ++j)
                if (spell->Effects[j].Effect == SPELL_EFFECT_SEND_EVENT)
                    if (spell->Effects[j].MiscValue)
                        evt_scripts.insert(spell->Effects[j].MiscValue);

    for (auto & pathIdx : sTaxiPathNodesByPath)
    {
        for (const auto node : pathIdx)
        {
            if (node->ArrivalEventID)
                evt_scripts.insert(node->ArrivalEventID);

            if (node->DepartureEventID)
                evt_scripts.insert(node->DepartureEventID);
        }
    }

    // Then check if all scripts are in above list of possible script entries
    for (auto itr = sEventScripts.begin(); itr != sEventScripts.end(); ++itr)
    {
        if (auto itr2 = evt_scripts.find(itr->first); itr2 == evt_scripts.end())
            LOG_ERROR("sql.sql", "Table `world_event_script` has script (Id: {}) not referring to any `world_game_object_template` "
                                 "type 10 data2 field, type 3 data6 field, type 13 data 2 field or any spell effect {}", itr->first, SPELL_EFFECT_SEND_EVENT);
    }
}

void ObjectMgr::LoadWaypointScripts()
{
    LoadScripts(SCRIPTS_WAYPOINT);

    std::set<uint32> actionSet;

    for (const auto &entry: sWaypointScripts | std::views::keys)
        actionSet.insert(entry);

    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_WAYPOINT_DATA_ACTION);

    if (const QueryResult result = WorldDatabase.Query(stmt))
    {
        do
        {
            const Field* fields = result->Fetch();
            uint32 action = fields[0].Get<uint32>();
            actionSet.erase(action);
        } while (result->NextRow());
    }

    for (unsigned int itr : actionSet)
        LOG_ERROR("sql.sql", "There is no waypoint which links to the waypoint script {}", itr);
}

void ObjectMgr::LoadSpellScriptNames()
{
    const uint32 oldMSTime = getMSTime();

    _spellScriptsStore.clear();  // Need for reload case

    const QueryResult result = WorldDatabase.Query("SELECT spell, script_name FROM world_spell_script_name");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 spell script names. DB table `world_spell_script_name` is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();

        int32 spellID = fields[0].Get<int32>();
        auto scriptName = fields[1].Get<std::string>();

        bool allRanks = false;
        if (spellID <= 0)
        {
            allRanks = true;
            spellID = -spellID;
        }

        const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(spellID);
        if (!spellInfo)
        {
            LOG_ERROR("sql.sql", "ScriptName: `{}` spell (spell:{}) does not exist in `Spell.dbc`.", scriptName, fields[0].Get<int32>());
            continue;
        }

        if (allRanks)
        {
            if (sSpellMgr->GetFirstSpellInChain(spellID) != static_cast<uint32>(spellID))
            {
                LOG_ERROR("sql.sql", "ScriptName: `{}` spell (spell:{}) is not first rank of spell.", scriptName, fields[0].Get<int32>());
                continue;
            }
            while (spellInfo)
            {
                _spellScriptsStore.insert(SpellScriptsContainer::value_type(spellInfo->ID, GetScriptID(scriptName)));
                spellInfo = spellInfo->GetNextRankSpell();
            }
        }
        else
            _spellScriptsStore.insert(SpellScriptsContainer::value_type(spellInfo->ID, GetScriptID(scriptName)));
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} spell script names in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::ValidateSpellScripts()
{
    const uint32 oldMSTime = getMSTime();

    if (_spellScriptsStore.empty())
    {
        LOG_INFO("server.loading", ">> Validated 0 scripts.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    for (auto itr = _spellScriptsStore.begin(); itr != _spellScriptsStore.end();)
    {
        const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(itr->first);
        std::vector<std::pair<SpellScriptLoader*, SpellScriptsContainer::iterator> > SpellScriptLoaders;
        sScriptMgr->CreateSpellScriptLoaders(itr->first, SpellScriptLoaders);
        itr = _spellScriptsStore.upper_bound(itr->first);

        for (auto sItr = SpellScriptLoaders.begin(); sItr != SpellScriptLoaders.end(); ++sItr)
        {
            SpellScript* spellScript = sItr->first->GetSpellScript();
            AuraScript* auraScript = sItr->first->GetAuraScript();
            bool valid = true;
            if (!spellScript && !auraScript)
            {
                LOG_ERROR("sql.sql", "Functions GetSpellScript() and GetAuraScript() of script `{}` do not return objects - script skipped", GetScriptName(sItr->second->second));
                valid = false;
            }
            if (spellScript)
            {
                spellScript->_Init(&sItr->first->GetName(), spellInfo->ID);
                spellScript->_Register();
                if (!spellScript->_Validate(spellInfo))
                    valid = false;
                delete spellScript;
            }
            if (auraScript)
            {
                auraScript->_Init(&sItr->first->GetName(), spellInfo->ID);
                auraScript->_Register();
                if (!auraScript->_Validate(spellInfo))
                    valid = false;
                delete auraScript;
            }
            if (!valid)
                _spellScriptsStore.erase(sItr->second);
        }
        ++count;
    }

    LOG_INFO("server.loading", ">> Validated {} scripts in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::InitializeSpellInfoPrecomputedData()
{
    const uint32 limit = sSpellStore.GetNumRows();
    for(uint32 i = 0; i <= limit; ++i)
        if (const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(i))
        {
            const_cast<SpellInfo*>(spellInfo)->SetStackableWithRanks(spellInfo->ComputeIsStackableWithRanks());
            const_cast<SpellInfo*>(spellInfo)->SetCritCapable(spellInfo->ComputeIsCritCapable());
            const_cast<SpellInfo*>(spellInfo)->SetSpellValid(SpellMgr::ComputeIsSpellValid(spellInfo, false));
        }
}

void ObjectMgr::LoadPageTexts()
{
    const uint32 oldMSTime = getMSTime();

    const QueryResult result = WorldDatabase.Query("SELECT id, text, next_page FROM world_page_text");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 page texts. DB table `world_page_text` is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        auto& [Text, NextPage] = _pageTextStore[fields[0].Get<uint32>()];
        Text = fields[1].Get<std::string>();
        NextPage = fields[2].Get<uint32>();

        ++count;
    } while (result->NextRow());

    for (auto itr = _pageTextStore.begin(); itr != _pageTextStore.end(); ++itr)
    {
        if (itr->second.NextPage)
        {
            if (auto itr2 = _pageTextStore.find(itr->second.NextPage); itr2 == _pageTextStore.end())
                LOG_ERROR("sql.sql", "Page text (Id: {}) has not existing next page (ID: {})", itr->first, itr->second.NextPage);
        }
    }

    LOG_INFO("server.loading", ">> Loaded {} Page Texts in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

const PageText* ObjectMgr::GetPageText(const uint32 pageEntry)
{
    if (const auto itr = _pageTextStore.find(pageEntry); itr != _pageTextStore.end())
        return &itr->second;
    return nullptr;
}

void ObjectMgr::LoadInstanceTemplate()
{
    const uint32 oldMSTime = getMSTime();

    const QueryResult result = WorldDatabase.Query("SELECT map, parent, script, allow_mount FROM world_instance_template");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 instance templates. DB table `world_instance_template` is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        const uint16 mapID = fields[0].Get<uint16>();

        if (!MapMgr::IsValidMAP(mapID, true))
        {
            LOG_ERROR("sql.sql", "ObjectMgr::LoadInstanceTemplate: bad mapID {} for template!", mapID);
            continue;
        }

        InstanceTemplate instanceTemplate;

        instanceTemplate.Parent = fields[1].Get<uint32>();
        instanceTemplate.ScriptID = GetScriptID(fields[2].Get<std::string>());
        instanceTemplate.AllowMount = fields[3].Get<bool>();

        _instanceTemplateStore[mapID] = instanceTemplate;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Instance Templates in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

const InstanceTemplate* ObjectMgr::GetInstanceTemplate(const uint32 mapID)
{
    if (const auto itr = _instanceTemplateStore.find(static_cast<uint16>(mapID)); itr != _instanceTemplateStore.end())
        return &itr->second;
    return nullptr;
}

void ObjectMgr::LoadInstanceEncounters()
{
    const uint32 oldMSTime = getMSTime();

    const QueryResult result = WorldDatabase.Query("SELECT id, credit_type, credit_entry, last_encounter_dungeon FROM world_instance_encounter");
    const auto tableName = "world_instance_encounter";
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 instance encounters, table `{}` is empty!", tableName);
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    std::map<uint32, const DungeonEncounterEntry*> dungeonLastBosses;
    do
    {
        const Field* fields = result->Fetch();
        uint32 entry = fields[0].Get<uint32>();
        uint8 creditType = fields[1].Get<uint8>();
        uint32 creditEntry = fields[2].Get<uint32>();
        uint32 lastEncounterDungeon = fields[3].Get<uint16>();
        const DungeonEncounterEntry* dungeonEncounter = sDungeonEncounterStore.LookupEntry(entry);
        if (!dungeonEncounter)
        {
            LOG_ERROR("sql.sql", "Table `{}` has an invalid encounter id {}, skipped!", tableName, entry);
            continue;
        }

        if (lastEncounterDungeon && !sLFGMgr->GetLFGDungeonEntry(lastEncounterDungeon))
        {
            LOG_ERROR("sql.sql", "Table `{}` has an encounter {} ({}) marked as final for invalid dungeon id {}, skipped!",
                tableName, entry, dungeonEncounter->EncounterName, lastEncounterDungeon);
            continue;
        }

        auto itr = dungeonLastBosses.find(lastEncounterDungeon);
        if (lastEncounterDungeon)
        {
            if (itr != dungeonLastBosses.end())
            {
                LOG_ERROR("sql.sql", "Table `{}` specified encounter {} ({}) as last encounter but {} ({}) is already marked as one, skipped!",
                    tableName, entry, dungeonEncounter->EncounterName, itr->second->ID, itr->second->EncounterName);
                continue;
            }

            dungeonLastBosses[lastEncounterDungeon] = dungeonEncounter;
        }

        switch (creditType)
        {
            case ENCOUNTER_CREDIT_KILL_CREATURE:
                {
                    const CreatureTemplate* creatureInfo = GetCreatureTemplate(creditEntry);
                    if (!creatureInfo)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has an invalid creature (entry {}) linked to the encounter {} ({}), skipped!", tableName, creditEntry, entry, dungeonEncounter->EncounterName);
                        continue;
                    }
                    const_cast<CreatureTemplate*>(creatureInfo)->FlagsExtra |= CREATURE_FLAG_EXTRA_DUNGEON_BOSS;
                    break;
                }
            case ENCOUNTER_CREDIT_CAST_SPELL:
                {
                    const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(creditEntry);
                    if (!spellInfo)
                    {
                        LOG_ERROR("sql.sql", "Table `{}` has an invalid spell (entry {}) linked to the encounter {} ({}), skipped!", tableName, creditEntry, entry, dungeonEncounter->EncounterName);
                        continue;
                    }
                    const_cast<SpellInfo*>(spellInfo)->AttributesCu |= SPELL_ATTR0_CU_ENCOUNTER_REWARD;
                    break;
                }
            default:
                LOG_ERROR("sql.sql", "Table `{}` has an invalid credit type ({}) for encounter {} ({}), skipped!", tableName, creditType, entry, dungeonEncounter->EncounterName);
                continue;
        }

        DungeonEncounterList& encounters = _dungeonEncounterStore[MAKE_PAIR32(dungeonEncounter->MapID, dungeonEncounter->Difficulty)];
        encounters.push_back(new DungeonEncounter(dungeonEncounter, static_cast<EncounterCreditType>(creditType), creditEntry, lastEncounterDungeon));
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Instance Encounters in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

const GossipText* ObjectMgr::GetGossipText(const uint32 textID) const
{
    if (const auto itr = _gossipTextStore.find(textID); itr != _gossipTextStore.end())
        return &itr->second;
    return nullptr;
}

void ObjectMgr::LoadGossipText()
{
    const uint32 oldMSTime = getMSTime();

    const QueryResult result = WorldDatabase.Query("SELECT id, text, broadcast_text, language, probability, emote FROM world_npc_text");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 npc texts, table is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    _gossipTextStore.rehash(result->GetRowCount());

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();

        uint32 id = fields[0].Get<uint32>();
        if (!id)
        {
            LOG_ERROR("sql.sql", "Table `world_npc_text` has record wit reserved id 0, ignore.");
            continue;
        }

        auto& [Options] = _gossipTextStore[id];
        auto textValues = fields[1].GetArray<std::string, MAX_GOSSIP_TEXT_OPTIONS, 2>();
        auto broadCastValues = fields[2].GetArray<uint32, MAX_GOSSIP_TEXT_OPTIONS>();
        auto languageValues = fields[3].GetArray<uint8, MAX_GOSSIP_TEXT_OPTIONS>();
        auto probabilityValues = fields[4].GetArray<float, MAX_GOSSIP_TEXT_OPTIONS>();
        auto emoteValues = fields[5].GetArray<uint16, MAX_GOSSIP_TEXT_OPTIONS, MAX_GOSSIP_TEXT_EMOTES * 2>();

        for (uint8 i = 0; i < MAX_GOSSIP_TEXT_OPTIONS; ++i)
        {
            Options[i].Text0          = textValues[i][0];
            Options[i].Text1          = textValues[i][1];
            Options[i].BroadcastTextID = broadCastValues[i];
            Options[i].Language        = languageValues[i];
            Options[i].Probability     = probabilityValues[i];

            for (uint8 j = 0; j < MAX_GOSSIP_TEXT_EMOTES; ++j)
            {
                Options[i].Emotes[j]._Delay = emoteValues[i][j * 2];
                Options[i].Emotes[j]._Emote = emoteValues[i][j * 2 + 1];
            }
        }

        for (uint8 i = 0; i < MAX_GOSSIP_TEXT_OPTIONS; i++)
        {
            if (Options[i].BroadcastTextID && !GetBroadcastText(Options[i].BroadcastTextID))
            {
                LOG_ERROR("sql.sql", "GossipText (Id: {}) in table `world_npc_text` has non-existing or incompatible BroadcastTextID{} {}.", id, i, Options[i].BroadcastTextID);
                Options[i].BroadcastTextID = 0;
            }
        }

        count++;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Npc Texts in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::ReturnOrDeleteOldMails(const bool serverUp)
{
    const uint32 oldMSTime = getMSTime();

    const time_t curTime = GameTime::GetGameTime().count();

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_EXPIRED_MAIL);
    stmt->SetData(0, static_cast<uint32>(curTime));
    const QueryResult result = CharacterDatabase.Query(stmt);
    if (!result)
        return;

    std::map<uint32 /*messageId*/, MailItemInfoVec> itemsCache;
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_EXPIRED_MAIL_ITEMS);
    stmt->SetData(0, static_cast<uint32>(curTime));
    if (const QueryResult items = CharacterDatabase.Query(stmt))
    {
        do
        {
            MailItemInfo item{};
            const Field* fields = items->Fetch();
            item.itemGUID = fields[0].Get<uint32>();
            item.itemTemplate = fields[1].Get<uint32>();
            uint32 mailId = fields[2].Get<uint32>();
            itemsCache[mailId].push_back(item);
        } while (items->NextRow());
    }

    uint32 deletedCount = 0;
    uint32 returnedCount = 0;
    do
    {
        const Field* fields = result->Fetch();
        const auto m = new Mail;
        m->messageID        = fields[0].Get<uint32>();
        m->messageType      = fields[1].Get<uint8>();
        m->sender           = fields[2].Get<uint32>();
        m->receiver         = fields[3].Get<uint32>();
        const bool hasItems = fields[4].Get<bool>();
        m->expire_time      = static_cast<time_t>(fields[5].Get<uint32>());
        m->deliver_time     = static_cast<time_t>(0);
        m->stationery       = fields[6].Get<uint8>();
        m->checked          = fields[7].Get<uint8>();
        m->mailTemplateId   = fields[8].Get<int16>();

        const Player* player = nullptr;
        if (serverUp)
            player = ObjectAccessor::FindPlayerByLowGUID(m->receiver);

        if (player) // Don't modify mails of a logged in player
        {
            delete m;
            continue;
        }

        // Delete or return mail
        if (hasItems)
        {
            // Read items from cache
            m->items.swap(itemsCache[m->messageID]);

            // If it is mail from non-player, or if it's already return mail, it shouldn't be returned, but deleted
            if (!m->IsSentByPlayer() || m->IsSentByGM() || (m->IsCODPayment() || m->IsReturnedMail()))
            {
                for (const auto& [itemGUID, itemTemplate] : m->items)
                {
                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE);
                    stmt->SetData(0, itemGUID);
                    CharacterDatabase.Execute(stmt);
                }

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_MAIL_ITEM_BY_ID);
                stmt->SetData(0, m->messageID);
                CharacterDatabase.Execute(stmt);
            }
            else
            {
                // Mail will be returned
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_MAIL_RETURNED);
                stmt->SetData(0, m->receiver);
                stmt->SetData(1, m->sender);
                stmt->SetData(2, static_cast<uint32>(curTime + 30 * DAY));
                stmt->SetData(3, static_cast<uint32>(curTime));
                stmt->SetData (4, static_cast<uint8>(MAIL_CHECK_MASK_RETURNED));
                stmt->SetData(5, m->messageID);
                CharacterDatabase.Execute(stmt);
                for (const auto& [itemGUID, itemTemplate] : m->items)
                {
                    // Update receiver in mail items for its proper delivery, and in instance_item for avoid lost item at sender delete
                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_MAIL_ITEM_RECEIVER);
                    stmt->SetData(0, m->sender);
                    stmt->SetData(1, itemGUID);
                    CharacterDatabase.Execute(stmt);

                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ITEM_OWNER);
                    stmt->SetData(0, m->sender);
                    stmt->SetData(1, itemGUID);
                    CharacterDatabase.Execute(stmt);
                }

                // Update global data
                sCharacterCache->IncreaseCharacterMailCount(ObjectGuid(HighGuid::Player, m->sender));
                sCharacterCache->DecreaseCharacterMailCount(ObjectGuid(HighGuid::Player, m->receiver));

                delete m;
                ++returnedCount;
                continue;
            }
        }

        sCharacterCache->DecreaseCharacterMailCount(ObjectGuid(HighGuid::Player, m->receiver));

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_MAIL_BY_ID);
        stmt->SetData(0, m->messageID);
        CharacterDatabase.Execute(stmt);
        delete m;
        ++deletedCount;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Processed {} expired mails: {} deleted and {} returned in {} ms", deletedCount + returnedCount, deletedCount, returnedCount, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadQuestAreaTriggers()
{
    const uint32 oldMSTime = getMSTime();

    _questAreaTriggerStore.clear();

    const QueryResult result = WorldDatabase.Query("SELECT id, quest FROM world_area_trigger_involved_relation");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 quest trigger points. DB table `world_area_trigger_involved_relation` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        ++count;

        const Field* fields = result->Fetch();

        uint32 triggerID = fields[0].Get<uint32>();
        uint32 questID   = fields[1].Get<uint32>();

        if (!GetAreaTrigger(triggerID))
        {
            LOG_ERROR("sql.sql", "Area trigger (ID:{}) does not exist in `AreaTrigger.dbc`.", triggerID);
            continue;
        }

        const Quest* quest = GetQuestTemplate(questID);

        if (!quest)
        {
            LOG_ERROR("sql.sql", "Table `world_area_trigger_involved_relation` has record (id: {}) for not existing quest {}", triggerID, questID);
            continue;
        }

        if (!quest->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT))
        {
            LOG_ERROR("sql.sql", "Table `world_area_trigger_involved_relation` has record (id: {}) for not quest {}, "
                                 "but quest not have SpecialFlag QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT. "
                                 "Trigger or quest flags must be fixed, quest modified to require objective.", triggerID, questID);

            // This will prevent quest completing without objective
            const_cast<Quest*>(quest)->SetSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT);
            // continue; - quest modified to required objective and trigger can be allowed.
        }

        _questAreaTriggerStore[triggerID] = questID;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Quest Trigger Points in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

const QuestGreeting* ObjectMgr::GetQuestGreeting(const TypeID type, uint32 id) const
{
    uint8 typeIndex;
    if (type == TYPEID_UNIT)
        typeIndex = 0;
    else if (type == TYPEID_GAMEOBJECT)
        typeIndex = 1;
    else
        return nullptr;

    const std::pair<uint32, uint8> pairKey = std::make_pair(id, typeIndex);
    const auto itr = _questGreetingStore.find(pairKey);
    if (itr == _questGreetingStore.end())
        return nullptr;
    return &itr->second;
}

void ObjectMgr::LoadQuestGreetings()
{
    const uint32 oldMSTime = getMSTime();

    _questGreetingStore.clear(); // For reload case

    const QueryResult result = WorldDatabase.Query("SELECT id, type, emote_type, emote_delay, greeting FROM world_quest_greeting");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 quest greetings. DB table `quest_greeting` is empty.");
        return;
    }

    do
    {
        const Field* fields = result->Fetch();

        uint32 id = fields[0].Get<uint32>();
        uint8 type = fields[1].Get<uint8>();
        switch (type)
        {
            case 0: // Creature
                if (!GetCreatureTemplate(id))
                {
                    LOG_ERROR("sql.sql", "Table `world_quest_greeting`: Creature template entry {} does not exist.", id);
                    continue;
                }
                break;
            case 1: // GameObject
                if (!GetGameObjectTemplate(id))
                {
                    LOG_ERROR("sql.sql", "Table `world_quest_greeting`: GameObject template entry {} does not exist.", id);
                    continue;
                }
                break;
            default:
                LOG_ERROR("sql.sql", "Table `world_quest_greeting` has unknown type {} for id {}, skipped.", type, id);
                continue;
        }

        std::pair<uint32, uint8> pairKey = std::make_pair(id, type);
        auto& [EmoteType, EmoteDelay, Greeting] = _questGreetingStore[pairKey];

        EmoteType = fields[2].Get<uint16>();
        EmoteDelay = fields[3].Get<uint32>();
        Greeting = fields[4].Get<std::string>();
    }
    while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} quest_greeting in {} ms", _questGreetingStore.size(), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadTavernAreaTriggers()
{
    const uint32 oldMSTime = getMSTime();

    _tavernAreaTriggerStore.clear();  // Need for reload case

    const QueryResult result = WorldDatabase.Query("SELECT id, faction FROM world_area_trigger_tavern");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 tavern triggers. DB table `world_area_trigger_tavern` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        ++count;
        const Field* fields = result->Fetch();

        uint32 triggerID = fields[0].Get<uint32>();

        if (!GetAreaTrigger(triggerID))
        {
            LOG_ERROR("sql.sql", "Area trigger (ID:{}) does not exist in `AreaTrigger.dbc`.", triggerID);
            continue;
        }

        uint32 faction = fields[1].Get<uint32>();

        _tavernAreaTriggerStore.emplace(triggerID, faction);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Tavern Triggers in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadAreaTriggerScripts()
{
    const uint32 oldMSTime = getMSTime();

    _areaTriggerScriptStore.clear();  // Need for reload case
    const QueryResult result = WorldDatabase.Query("SELECT id, script_name FROM world_area_trigger_script");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 AreaTrigger Scripts. DB Table `world_area_trigger_script` Is Empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        ++count;

        const Field* fields = result->Fetch();

        uint32 triggerID = fields[0].Get<uint32>();
        if (!GetAreaTrigger(triggerID))
        {
            LOG_ERROR("sql.sql", "Area trigger (ID:{}) does not exist in `AreaTrigger.dbc`.", triggerID);
            continue;
        }
        _areaTriggerScriptStore[triggerID] = GetScriptID(fields[1].Get<std::string>());
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} AreaTrigger Scripts in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

uint32 ObjectMgr::GetNearestTaxiNode(const WorldLocation& loc, const uint32 teamID)
{
    return GetNearestTaxiNode(loc.GetPositionX(), loc.GetPositionY(), loc.GetPositionZ(), loc.GetMapId(), teamID);
}

uint32 ObjectMgr::GetNearestTaxiNode(const float x, const float y, const float z, const uint32 mapID, const uint32 teamID)
{
    bool found = false;
    float dist = 10000;
    uint32 id = 0;

    for (const TaxiNodesEntry* node : sTaxiNodesStore)
    {
        if (!node || node->MapID != mapID || (!node->MountCreatureID[teamID == TEAM_ALLIANCE ? 1 : 0] && node->MountCreatureID[0] != 32981)) // DK flight
            continue;

        const auto field = static_cast<uint8>((node->ID - 1) / 32);
        const uint32 subMask = 1 << ((node->ID - 1) % 32);

        // Skip not taxi network nodes
        if (field >= TaxiMaskSize || (sTaxiNodesMask[field] & subMask) == 0)
            continue;

        const float dist2 = (node->X - x) * (node->X - x) + (node->Y - y) * (node->Y - y) + (node->Z - z) * (node->Z - z);
        if (found)
        {
            if (dist2 < dist)
            {
                dist = dist2;
                id = node->ID;
            }
        }
        else
        {
            found = true;
            dist = dist2;
            id = node->ID;
        }
    }

    return id;
}

void ObjectMgr::GetTaxiPath(const uint32 source, const uint32 destination, uint32& path, uint32& cost)
{
    const auto src_i = sTaxiPathSetBySource.find(source);
    if (src_i == sTaxiPathSetBySource.end())
    {
        path = 0;
        cost = 0;
        return;
    }

    TaxiPathSetForSource& pathSet = src_i->second;

    const auto dest_i = pathSet.find(destination);
    if (dest_i == pathSet.end())
    {
        path = 0;
        cost = 0;
        return;
    }

    cost = dest_i->second->Price;
    path = dest_i->second->ID;
}

uint32 ObjectMgr::GetTaxiMountDisplayID(const uint32 id, const TeamID teamID, const bool allowedAltTeam /* = false */) const {
    CreatureModel mountModel;
    const CreatureTemplate* mountInfo = nullptr;

    // Select mount creature id
    if (const TaxiNodesEntry* node = sTaxiNodesStore.LookupEntry(id))
    {
        uint32 mount_entry = node->MountCreatureID[teamID == TEAM_ALLIANCE ? 1 : 0];

        // Fix for Alliance not being able to use Acherus taxi.
        // Only one mount type for both sides.
        if (mount_entry == 0 && allowedAltTeam)
        {
            // Simply reverse the selection. At least one team in theory should have a valid mount ID to choose.
            mount_entry = node->MountCreatureID[teamID];
        }

        mountInfo = GetCreatureTemplate(mount_entry);
        if (mountInfo)
        {
            const CreatureModel* model = mountInfo->GetRandomValidModel();
            if (!model)
            {
                LOG_ERROR("sql.sql", "No DisplayID found for the taxi mount with the entry {}! Can't load it!", mount_entry);
                return 0;
            }
            mountModel = *model;
        }
    }

    // mountInfo is not actually used but the mount_id was updated
    GetCreatureModelRandomGender(&mountModel, mountInfo);

    return mountModel.CreatureDisplayID;
}

void ObjectMgr::LoadAreaTriggers()
{
    const uint32 oldMSTime = getMSTime();

    _areaTriggerStore.clear();

    const QueryResult result = WorldDatabase.Query("SELECT id, map, x, y, z, radius, length, width, height, orientation FROM world_area_trigger");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 area trigger definitions. DB table `world_area_trigger` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();

        ++count;

        AreaTrigger at;
        at.entry = fields[0].Get<uint32>();
        at.map = fields[1].Get<uint32>();
        at.x = fields[2].Get<float>();
        at.y = fields[3].Get<float>();
        at.z = fields[4].Get<float>();
        at.radius = fields[5].Get<float>();
        at.length = fields[6].Get<float>();
        at.width = fields[7].Get<float>();
        at.height = fields[8].Get<float>();
        at.orientation = fields[9].Get<float>();

        if (!sMapStore.LookupEntry(at.map))
        {
            LOG_ERROR("sql.sql", "Area trigger (ID:{}) map (ID: {}) does not exist in `Map.dbc`.", at.entry, at.map);
            continue;
        }

        _areaTriggerStore[at.entry] = at;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Area Trigger Definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadAreaTriggerTeleports()
{
    const uint32 oldMSTime = getMSTime();

    _areaTriggerTeleportStore.clear();  // Need for reload case

    const QueryResult result = WorldDatabase.Query("SELECT id, map, position, orientation FROM world_area_trigger_teleport");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 area trigger teleport definitions. DB table `world_area_trigger_teleport` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        ++count;
        const Field* fields = result->Fetch();

        uint32 triggerID = fields[0].Get<uint32>();

        AreaTriggerTeleport at{};

        at.targetMapID = fields[1].Get<uint16>();
        const auto position = fields[2].GetArray<float, 3>();
        at.targetX = position[0];
        at.targetY = position[1];
        at.targetZ = position[2];
        at.targetOrientation = fields[3].Get<float>();

        if (!GetAreaTrigger(triggerID))
        {
            LOG_ERROR("sql.sql", "Area trigger (ID:{}) does not exist in `AreaTrigger.dbc`.", triggerID);
            continue;
        }
        if (!sMapStore.LookupEntry(at.targetMapID))
        {
            LOG_ERROR("sql.sql", "Area trigger (ID:{}) target map (ID: {}) does not exist in `Map.dbc`.", triggerID, at.targetMapID);
            continue;
        }
        if (at.targetX == 0 && at.targetY == 0 && at.targetZ == 0)
        {
            LOG_ERROR("sql.sql", "Area trigger (ID:{}) target coordinates not provided.", triggerID);
            continue;
        }

        _areaTriggerTeleportStore[triggerID] = at;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Area Trigger Teleport Definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadAccessRequirements()
{
    const uint32 oldMSTime = getMSTime();

    if (!_accessRequirementStore.empty())
    {
        for (auto &reqMap: _accessRequirementStore | std::views::values)
        {
            for (const auto &req: reqMap | std::views::values)
            {
                for (const auto & quest : req->quests)
                    delete quest;
                for (const auto & achievement : req->achievements)
                    delete achievement;
                for (const auto & item : req->items)
                    delete item;
                delete req;
            }
        }
        _accessRequirementStore.clear();  // Need for reload case
    }

    const QueryResult access_template_result = WorldDatabase.Query("SELECT id, map, difficulty, min_level, max_level, min_avg_item_level FROM world_dungeon_access_template");
    if (!access_template_result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 access requirement definitions. DB table `world_dungeon_access_template` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    uint32 countProgressionRequirements = 0;

    do
    {
        const Field* fields = access_template_result->Fetch();

        // Get the common variables for the access requirements
        uint8 dungeonAccessID = fields[0].Get<uint8>();
        uint32 mapID = fields[1].Get<uint32>();
        uint8 difficulty = fields[2].Get<uint8>();

        // Set up the access requirements
        const auto ar = new DungeonProgressionRequirements();
        ar->levelMin = fields[3].Get<uint8>();
        ar->levelMax = fields[4].Get<uint8>();
        ar->reqItemLevel = fields[5].Get<uint16>();

        if (const QueryResult progressionReqResults = WorldDatabase.Query("SELECT requirement_type, requirement, requirement_note, faction, priority, leader_only "
                                                                          "FROM world_dungeon_access_requirement WHERE dungeon_access=$1", dungeonAccessID))
        {
            do
            {
                const Field* progressionRequirementRow = progressionReqResults->Fetch();

                const uint8 reqType = progressionRequirementRow[0].Get<uint8>();
                const uint32 reqID = progressionRequirementRow[1].Get<uint32>();
                const std::string reqNote = progressionRequirementRow[2].Get<std::string>();
                const uint8 reqFaction = progressionRequirementRow[3].Get<uint8>();
                const uint8 reqPriority = progressionRequirementRow[4].IsNull() ? UINT8_MAX : progressionRequirementRow[4].Get<uint8>();
                const bool reqLeaderOnly = progressionRequirementRow[5].Get<bool>();

                auto progressionRequirement = new ProgressionRequirement();
                progressionRequirement->id = reqID;
                progressionRequirement->note = reqNote;
                progressionRequirement->faction = static_cast<TeamID>(reqFaction);
                progressionRequirement->priority = reqPriority;
                progressionRequirement->checkLeaderOnly = reqLeaderOnly;

                std::vector<ProgressionRequirement*>* currentRequirementsList = nullptr;

                switch (reqType)
                {
                case 0:
                {
                    // Achievement
                    if (!sAchievementStore.LookupEntry(progressionRequirement->id))
                    {
                        LOG_ERROR("sql.sql", "Required achievement {} for faction {} does not exist for map {} difficulty {}, remove or fix this achievement requirement.",
                            progressionRequirement->id, reqFaction, mapID, difficulty);
                        break;
                    }

                    currentRequirementsList = &ar->achievements;
                    break;
                }
                case 1:
                {
                    // Quest
                    if (!GetQuestTemplate(progressionRequirement->id))
                    {
                        LOG_ERROR("sql.sql", "Required quest {} for faction {} does not exist for map {} difficulty {}, remove or fix this quest requirement.",
                            progressionRequirement->id, reqFaction, mapID, difficulty);
                        break;
                    }

                    currentRequirementsList = &ar->quests;
                    break;
                }
                case 2:
                {
                    // Item
                    if (!GetItemTemplate(progressionRequirement->id))
                    {
                        LOG_ERROR("sql.sql", "Required item {} for faction {} does not exist for map {} difficulty {}, remove or fix this item requirement.",
                            progressionRequirement->id, reqFaction, mapID, difficulty);
                        break;
                    }

                    currentRequirementsList = &ar->items;
                    break;
                }
                default:
                    LOG_ERROR("sql.sql", "requirementType of {} is not valid for map {} difficulty {}. Please use 0 for achievements, 1 for quest, 2 for items or remove this entry from the db.",
                        reqType, mapID, difficulty);
                    break;
                }

                // Check if array is valid and delete the progression requirement
                if (!currentRequirementsList)
                {
                    delete progressionRequirement;
                    continue;
                }

                //Insert into the array
                if (currentRequirementsList->size() > reqPriority)
                    currentRequirementsList->insert(currentRequirementsList->begin() + reqPriority, progressionRequirement);
                else
                    currentRequirementsList->push_back(progressionRequirement);

            } while (progressionReqResults->NextRow());
        }

        // Sort all arrays for priority
        auto sortFunction = [](const ProgressionRequirement* const a, const ProgressionRequirement* const b) { return a->priority > b->priority; };
        std::ranges::sort(ar->achievements, sortFunction);
        std::ranges::sort(ar->quests, sortFunction);
        std::ranges::sort(ar->items, sortFunction);

        countProgressionRequirements += ar->achievements.size();
        countProgressionRequirements += ar->quests.size();
        countProgressionRequirements += ar->items.size();
        count++;

        _accessRequirementStore[mapID][difficulty] = ar;
    } while (access_template_result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Rows From dungeon_access_template And {} Rows From dungeon_access_requirements in {} ms", count, countProgressionRequirements, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

/// Searches for the AreaTrigger which teleports players out of the given map with instance_template.parent field support
const AreaTriggerTeleport* ObjectMgr::GetGoBackTrigger(const uint32 mapID) const
{
    bool useParentDbValue = false;
    uint32 parentId = 0;
    const MapEntry* mapEntry = sMapStore.LookupEntry(mapID);
    if (!mapEntry || mapEntry->EntranceMap < 0)
        return nullptr;

    if (mapEntry->IsDungeon())
    {
        const InstanceTemplate* iTemplate = sObjectMgr->GetInstanceTemplate(mapID);

        if (!iTemplate)
            return nullptr;

        parentId = iTemplate->Parent;
        useParentDbValue = true;
    }

    const auto entranceMap = static_cast<uint32>(mapEntry->EntranceMap);
    for (const auto &[entry, tele] : _areaTriggerTeleportStore)
        if ((!useParentDbValue && tele.targetMapID == entranceMap) || (useParentDbValue && tele.targetMapID == parentId))
            if (const AreaTrigger* atEntry = GetAreaTrigger(entry); atEntry && atEntry->map == mapID)
                return &tele;
    return nullptr;
}

/// Searches for the AreaTrigger which teleports players to the given map
const AreaTriggerTeleport* ObjectMgr::GetMapEntranceTrigger(const uint32 mapID) const
{
    for (const auto &tele: _areaTriggerTeleportStore | std::views::values)
    {
        if (tele.targetMapID == mapID)  // ID is used to determine correct Scarlet Monastery instance
            return &tele;  // No need to check if exists in sAreaTriggerStore, already done at loading
    }
    return nullptr;
}

void ObjectMgr::SetHighestGUIDs()
{
    QueryResult result = CharacterDatabase.Query("SELECT MAX(guid) FROM characters");
    if (result)
        GetGuidSequenceGenerator<HighGuid::Player>().Set((*result)[0].Get<uint32>() + 1);

    result = CharacterDatabase.Query("SELECT MAX(guid) FROM item_instance");
    if (result)
        GetGuidSequenceGenerator<HighGuid::Item>().Set((*result)[0].Get<uint32>() + 1);

    // Cleanup other tables from not existed guids ( >= _hiItemGuid)
    CharacterDatabase.Execute("DELETE FROM character_inventory WHERE item >= '{}'", GetGuidSequenceGenerator<HighGuid::Item>().GetNextAfterMaxUsed());     // One-time query
    CharacterDatabase.Execute("DELETE FROM mail_items WHERE item_guid >= '{}'", GetGuidSequenceGenerator<HighGuid::Item>().GetNextAfterMaxUsed());         // One-time query
    CharacterDatabase.Execute("DELETE FROM auctionhouse WHERE itemguid >= '{}'", GetGuidSequenceGenerator<HighGuid::Item>().GetNextAfterMaxUsed());        // One-time query
    CharacterDatabase.Execute("DELETE FROM guild_bank_item WHERE item_guid >= '{}'", GetGuidSequenceGenerator<HighGuid::Item>().GetNextAfterMaxUsed());    // One-time query

    result = WorldDatabase.Query("SELECT MAX(guid) FROM world_transport");
    if (result)
        GetGuidSequenceGenerator<HighGuid::Mo_Transport>().Set((*result)[0].Get<uint32>() + 1);

    result = CharacterDatabase.Query("SELECT MAX(id) FROM auctionhouse");
    if (result)
        _auctionID = (*result)[0].Get<uint32>() + 1;

    result = CharacterDatabase.Query("SELECT MAX(id) FROM mail");
    if (result)
        _mailID = (*result)[0].Get<uint32>() + 1;

    result = CharacterDatabase.Query("SELECT MAX(arenateamid) FROM arena_team");
    if (result)
        sArenaTeamMgr->SetNextArenaTeamId((*result)[0].Get<uint32>() + 1);

    result = CharacterDatabase.Query("SELECT MAX(fight_id) FROM log_arena_fights");
    if (result)
        sArenaTeamMgr->SetLastArenaLogId((*result)[0].Get<uint32>());

    result = CharacterDatabase.Query("SELECT MAX(setguid) FROM character_equipmentsets");
    if (result)
        _equipmentSetGUID = (*result)[0].Get<uint64>() + 1;

    result = CharacterDatabase.Query("SELECT MAX(guildId) FROM guild");
    if (result)
        sGuildMgr->SetNextGuildId((*result)[0].Get<uint32>() + 1);

    result = WorldDatabase.Query("SELECT MAX(guid) FROM world_creature");
    if (result)
        _creatureSpawnID = (*result)[0].Get<uint32>() + 1;

    result = WorldDatabase.Query("SELECT MAX(guid) FROM world_game_object");
    if (result)
        _gameObjectSpawnID = (*result)[0].Get<uint32>() + 1;
}

uint32 ObjectMgr::GenerateAuctionID()
{
    if (_auctionID >= 0xFFFFFFFE)
    {
        LOG_ERROR("server.worldserver", "Auctions ids overflow!! Can't continue, shutting down server. ");
        World::StopNow(ERROR_EXIT_CODE);
    }
    return _auctionID++;
}

uint64 ObjectMgr::GenerateEquipmentSetGUID()
{
    if (_equipmentSetGUID >= static_cast<uint64>(0xFFFFFFFFFFFFFFFELL))
    {
        LOG_ERROR("server.worldserver", "EquipmentSet guid overflow!! Can't continue, shutting down server. ");
        World::StopNow(ERROR_EXIT_CODE);
    }
    return _equipmentSetGUID++;
}

uint32 ObjectMgr::GenerateMailID()
{
    if (_mailID >= 0xFFFFFFFE)
    {
        LOG_ERROR("server.worldserver", "Mail ids overflow!! Can't continue, shutting down server. ");
        World::StopNow(ERROR_EXIT_CODE);
    }
    std::lock_guard guard(_mailIdMutex);
    return _mailID++;
}

uint32 ObjectMgr::GenerateCreatureSpawnID()
{
    if (_creatureSpawnID >= static_cast<uint32>(0xFFFFFF))
    {
        LOG_ERROR("server.worldserver", "Creature spawn id overflow!! Can't continue, shutting down server. Search on forum for TCE00007 for more info.");
        World::StopNow(ERROR_EXIT_CODE);
    }
    return _creatureSpawnID++;
}

uint32 ObjectMgr::GenerateGameObjectSpawnID()
{
    if (_gameObjectSpawnID >= static_cast<uint32>(0xFFFFFF))
    {
        LOG_ERROR("server.worldserver", "GameObject spawn id overflow!! Can't continue, shutting down server. Search on forum for TCE00007 for more info. ");
        World::StopNow(ERROR_EXIT_CODE);
    }
    return _gameObjectSpawnID++;
}

inline void CheckGOLockID(const GameObjectTemplate* goInfo, const uint32 dataN, uint32 N)
{
    if (sLockStore.LookupEntry(dataN))
        return;
    LOG_ERROR("sql.sql", "GameObject (Entry: {} GoType: {}) have data{}={} but lock (Id: {}) not found.", goInfo->Entry, goInfo->Type, N, goInfo->Door.lockId, goInfo->Door.lockId);
}

inline void CheckGOLinkedTrapID(const GameObjectTemplate* goInfo, uint32 dataN, uint32 N)
{
    if (const GameObjectTemplate* trapInfo = sObjectMgr->GetGameObjectTemplate(dataN))
    {
        if (trapInfo->Type != GAME_OBJECT_TYPE_TRAP)
            LOG_ERROR("sql.sql", "GameObject (Entry: {} GoType: {}) have data{}={} but GO (Entry {}) have not GAME_OBJECT_TYPE_TRAP ({}) type.",
                goInfo->Entry, goInfo->Type, N, dataN, dataN, GAME_OBJECT_TYPE_TRAP);
    }
}

inline void CheckGOSpellID(const GameObjectTemplate* goInfo, uint32 dataN, uint32 N)
{
    if (sSpellMgr->GetSpellInfo(dataN))
        return;

    LOG_ERROR("sql.sql", "GameObject (Entry: {} GoType: {}) have data{}={} but Spell (Entry {}) not exist.", goInfo->Entry, goInfo->Type, N, dataN, dataN);
}

inline void CheckAndFixGOChairHeightID(const GameObjectTemplate* goInfo, const uint32& dataN, uint32 N)
{
    if (dataN <= (UNIT_STAND_STATE_SIT_HIGH_CHAIR - UNIT_STAND_STATE_SIT_LOW_CHAIR))
        return;

    LOG_ERROR("sql.sql", "GameObject (Entry: {} GoType: {}) have data{}={} but correct chair height in range 0..{}.",
        goInfo->Entry, goInfo->Type, N, dataN, UNIT_STAND_STATE_SIT_HIGH_CHAIR - UNIT_STAND_STATE_SIT_LOW_CHAIR);

    // Prevent client and server unexpected work
    const_cast<uint32&>(dataN) = 0;
}

inline void CheckGONoDamageImmuneID(GameObjectTemplate* goTemplate, uint32 dataN, uint32 N)
{
    // 0/1 correct values
    if (dataN <= 1)
        return;
    LOG_ERROR("sql.sql", "GameObject (Entry: {} GoType: {}) have data{}={} but expected boolean (0/1) noDamageImmune field value.", goTemplate->Entry, goTemplate->Type, N, dataN);
}

inline void CheckGOConsumable(const GameObjectTemplate* goInfo, uint32 dataN, uint32 N)
{
    // 0/1 correct values
    if (dataN <= 1)
        return;
    LOG_ERROR("sql.sql", "GameObject (Entry: {} GoType: {}) have data{}={} but expected boolean (0/1) consumable field value.", goInfo->Entry, goInfo->Type, N, dataN);
}

void ObjectMgr::LoadGameObjectTemplate()
{
    const uint32 oldMSTime = getMSTime();

    const QueryResult result = WorldDatabase.Query("SELECT id, type, display, name, icon_name, cast_bar_caption, alert_text, size, data, ai_name, script_name FROM world_game_object_template");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 GameObject definitions. DB table `world_game_object_template` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    _gameObjectTemplateStore.rehash(result->GetRowCount());
    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        uint32 entry = fields[0].Get<uint32>();

        GameObjectTemplate& got = _gameObjectTemplateStore[entry];

        got.Entry = entry;
        got.Type = fields[1].Get<uint32>();
        got.DisplayID = fields[2].Get<uint32>();
        got.name = fields[3].Get<std::string>();
        got.IconName = fields[4].Get<std::string>();
        got.CastBarCaption = fields[5].Get<std::string>();
        got.AlertText = fields[6].Get<std::string>();
        got.Size = fields[7].Get<float>();

        // data[0] and data[5] can be -1
        const auto goData = fields[8].GetArray<int32, MAX_GAME_OBJECT_DATA>();
        for (uint8 i = 0; i < MAX_GAME_OBJECT_DATA; ++i)
            got.Raw.data[i] = goData[i];

        got.AIName = fields[9].Get<std::string>();
        got.ScriptID = GetScriptID(fields[10].Get<std::string>());
        got.IsForQuests = false;

        if (!got.AIName.empty() && !sGameObjectAIRegistry->HasItem(got.AIName))
            LOG_ERROR("sql.sql", "GameObject (Entry: {}) has non-registered `AIName` '{}' set, removing", got.Entry, got.AIName);

        switch (got.Type)
        {
        case GAME_OBJECT_TYPE_DOOR:
            {
                if (got.Door.lockId)
                    CheckGOLockID(&got, got.Door.lockId, 1);
                CheckGONoDamageImmuneID(&got, got.Door.noDamageImmune, 3);
                break;
            }
        case GAME_OBJECT_TYPE_BUTTON:
            {
                if (got.Button.lockId)
                    CheckGOLockID(&got, got.Button.lockId, 1);
                CheckGONoDamageImmuneID(&got, got.Button.noDamageImmune, 4);
                break;
            }
        case GAME_OBJECT_TYPE_QUEST_GIVER:
            {
                if (got.QuestGiver.lockId)
                    CheckGOLockID(&got, got.QuestGiver.lockId, 0);
                CheckGONoDamageImmuneID(&got, got.QuestGiver.noDamageImmune, 5);
                break;
            }
        case GAME_OBJECT_TYPE_CHEST:
            {
                if (got.Chest.lockId)
                    CheckGOLockID(&got, got.Chest.lockId, 0);
                CheckGOConsumable(&got, got.Chest.consumable, 3);

                if (got.Chest.linkedTrapId)
                    CheckGOLinkedTrapID(&got, got.Chest.linkedTrapId, 7);
                break;
            }
        case GAME_OBJECT_TYPE_TRAP:
            {
                if (got.Trap.lockId)
                    CheckGOLockID(&got, got.Trap.lockId, 0);
                break;
            }
        case GAME_OBJECT_TYPE_CHAIR:
            CheckAndFixGOChairHeightID(&got, got.Chair.height, 1);
            break;
        case GAME_OBJECT_TYPE_SPELL_FOCUS:
            {
                if (got.SpellFocus.focusId && !sSpellFocusObjectStore.LookupEntry(got.SpellFocus.focusId))
                    LOG_ERROR("sql.sql", "GameObject (Entry: {} GoType: {}) have data0={} but SpellFocus (Id: {}) not exist.", entry, got.Type, got.SpellFocus.focusId, got.SpellFocus.focusId);

                if (got.SpellFocus.linkedTrapId)
                    CheckGOLinkedTrapID(&got, got.SpellFocus.linkedTrapId, 2);
                break;
            }
        case GAME_OBJECT_TYPE_GOOBER:
            {
                if (got.Goober.lockId)
                    CheckGOLockID(&got, got.Goober.lockId, 0);
                CheckGOConsumable(&got, got.Goober.consumable, 3);

                if (got.Goober.pageId && !GetPageText(got.Goober.pageId))
                    LOG_ERROR("sql.sql", "GameObject (Entry: {} GoType: {}) have data7={} but PageText (Entry {}) not exist.", entry, got.Type, got.Goober.pageId, got.Goober.pageId);
                CheckGONoDamageImmuneID(&got, got.Goober.noDamageImmune, 11);

                if (got.Goober.linkedTrapId)
                    CheckGOLinkedTrapID(&got, got.Goober.linkedTrapId, 12);
                break;
            }
        case GAME_OBJECT_TYPE_AREA_DAMAGE:
            {
                if (got.AreaDamage.lockId)
                    CheckGOLockID(&got, got.AreaDamage.lockId, 0);
                break;
            }
        case GAME_OBJECT_TYPE_CAMERA:
            {
                if (got.Camera.lockId)
                    CheckGOLockID(&got, got.Camera.lockId, 0);
                break;
            }
        case GAME_OBJECT_TYPE_MO_TRANSPORT:
            {
                if (got.MOTransport.taxiPathId)
                {
                    if (got.MOTransport.taxiPathId >= sTaxiPathNodesByPath.size() || sTaxiPathNodesByPath[got.MOTransport.taxiPathId].empty())
                        LOG_ERROR("sql.sql", "GameObject (Entry: {} GoType: {}) have data0={} but TaxiPath (Id: {}) not exist.",
                            entry, got.Type, got.MOTransport.taxiPathId, got.MOTransport.taxiPathId);
                }
                if (uint32 transportMap = got.MOTransport.mapID)
                    _transportMaps.insert(transportMap);
                break;
            }
        case GAME_OBJECT_TYPE_SUMMONING_RITUAL:
            break;
        case GAME_OBJECT_TYPE_SPELLCASTER:
            {
                // Always must have spell
                CheckGOSpellID(&got, got.SpellCaster.spellId, 0);
                break;
            }
        case GAME_OBJECT_TYPE_FLAGSTAND:
            {
                if (got.Flagstand.lockId)
                    CheckGOLockID(&got, got.Flagstand.lockId, 0);
                CheckGONoDamageImmuneID(&got, got.Flagstand.noDamageImmune, 5);
                break;
            }
        case GAME_OBJECT_TYPE_FISHING_HOLE:
            {
                if (got.FishingHole.lockId)
                    CheckGOLockID(&got, got.FishingHole.lockId, 4);
                break;
            }
        case GAME_OBJECT_TYPE_FLAG_DROP:
            {
                if (got.FlagDrop.lockId)
                    CheckGOLockID(&got, got.FlagDrop.lockId, 0);
                CheckGONoDamageImmuneID(&got, got.FlagDrop.noDamageImmune, 3);
                break;
            }
        case GAME_OBJECT_TYPE_BARBER_CHAIR:
            CheckAndFixGOChairHeightID(&got, got.BarberChair.chairHeight, 0);
            break;
        default:
            break;
        }

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Game Object Templates in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadGameObjectTemplateAddons()
{
    const uint32 oldMSTime = getMSTime();

    const QueryResult result = WorldDatabase.Query("SELECT id, faction, flags, min_gold, max_gold, artkit FROM world_game_object_template_addon");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 GameObject template addon definitions. DB table `world_game_object_template_addon` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        uint32 entry = fields[0].Get<uint32>();

        const GameObjectTemplate* got = GetGameObjectTemplate(entry);
        if (!got)
        {
            LOG_ERROR("sql.sql", "GameObject template (Entry: {}) does not exist but has a record in `world_game_object_template_addon`", entry);
            continue;
        }

        GameObjectTemplateAddon& gameObjectAddon = _gameObjectTemplateAddonStore[entry];
        gameObjectAddon.faction = fields[1].Get<uint32>();
        gameObjectAddon.flags   = fields[2].Get<uint32>();
        gameObjectAddon.minGold = fields[3].Get<uint32>();
        gameObjectAddon.maxGold = fields[4].Get<uint32>();

        const auto artKits = fields[5].GetArray<uint32, 4>();
        for (uint32 i = 0; i < 4; i++)
        {
            uint32 artKitID = artKits[i];
            if (!artKitID)
                continue;
            if (!sGameObjectArtKitStore.LookupEntry(artKitID))
            {
                LOG_ERROR("sql.sql", "GameObject (Entry: {}) has invalid `artkit[{}]` {} defined, set to zero instead.", entry, i, artKitID);
                continue;
            }
            gameObjectAddon.artKits[i] = artKitID;
        }

        if (gameObjectAddon.faction && !sFactionTemplateStore.LookupEntry(gameObjectAddon.faction))
            LOG_ERROR("sql.sql",
                "GameObject (Entry: {}) has invalid faction ({}) defined in `world_game_object_template_addon`.",
                entry, gameObjectAddon.faction);

        if (gameObjectAddon.maxGold > 0)
        {
            switch (got->Type)
            {
                case GAME_OBJECT_TYPE_CHEST:
                case GAME_OBJECT_TYPE_FISHING_HOLE:
                    break;
                default:
                    LOG_ERROR("sql.sql",
                        "GameObject (Entry {} GoType: {}) can't be looted but has max_gold set in `world_game_object_template_addon`.",
                        entry, got->Type);
                    break;
            }
        }

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Game Object Template Addons in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadExplorationBaseXP()
{
    const uint32 oldMSTime = getMSTime();

    const QueryResult result = WorldDatabase.Query("SELECT level, base_xp FROM world_exploration_base_xp");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 BaseXP definitions. DB table `world_exploration_base_xp` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();
        const uint8 level = fields[0].Get<uint8>();
        _baseXPTable[level] = fields[1].Get<uint32>();
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} BaseXP Definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

uint32 ObjectMgr::GetBaseXP(const uint8 level)
{
    return _baseXPTable[level] ? _baseXPTable[level] : 0;
}

uint32 ObjectMgr::GetXPForLevel(const uint8 level) const
{
    if (level < _playerXPPerLevel.size())
        return _playerXPPerLevel[level];
    return 0;
}

void ObjectMgr::LoadPetNames()
{
    const uint32 oldMSTime = getMSTime();
    const QueryResult result = WorldDatabase.Query("SELECT id, part1, part2 FROM world_pet_name_generation");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 pet name parts. DB table `world_pet_name_generation` is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();
        const uint32 entry = fields[0].Get<uint32>();
        const auto part1 = fields[1].GetVector<std::string>();
        for (const auto& word : part1)
            _petHalfName0[entry].push_back(word);
        count += part1.size();

        const auto part2 = fields[2].GetVector<std::string>();
        for (const auto& word : part1)
            _petHalfName1[entry].push_back(word);
        count += part2.size();
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Pet Name Parts in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadPetNumber()
{
    const uint32 oldMSTime = getMSTime();

    if (const QueryResult result = CharacterDatabase.Query("SELECT MAX(id) FROM character_pet"))
    {
        const Field* fields = result->Fetch();
        _hiPetNumber = fields[0].Get<uint32>() + 1;
    }

    LOG_INFO("server.loading", ">> Loaded The Max Pet Number: {} in {} ms", _hiPetNumber - 1, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

std::string ObjectMgr::GeneratePetName(const uint32 entry)
{
    std::vector<std::string>& list0 = _petHalfName0[entry];
    std::vector<std::string>& list1 = _petHalfName1[entry];

    if (list0.empty() || list1.empty())
    {
        const CreatureTemplate* cinfo = GetCreatureTemplate(entry);
        std::string petname = GetPetName(cinfo->Family);
        if (petname.empty())
            return cinfo->Name;
        return petname;
    }

    return *(list0.begin() + urand(0, list0.size() - 1)) + *(list1.begin() + urand(0, list1.size() - 1));
}

uint32 ObjectMgr::GeneratePetNumber()
{
    std::lock_guard guard(_hiPetNumberMutex);
    return ++_hiPetNumber;
}

void ObjectMgr::LoadReputationRewardRate()
{
    const uint32 oldMSTime = getMSTime();

    _repRewardRateStore.clear();

    const QueryResult result = WorldDatabase.Query("SELECT faction, quest_rate, quest_daily_rate, quest_weekly_rate, quest_monthly_rate, "
                                                   "quest_repeatable_rate, creature_rate, spell_rate FROM world_reputation_reward_rate");
    const auto tableName = "world_reputation_reward_rate";
    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded `{}`, table is empty!", tableName);
        return;
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        uint32 factionID = fields[0].Get<uint32>();

        RepRewardRate repRate{};
        repRate.questRate           = fields[1].Get<float>();
        repRate.questDailyRate      = fields[2].Get<float>();
        repRate.questWeeklyRate     = fields[3].Get<float>();
        repRate.questMonthlyRate    = fields[4].Get<float>();
        repRate.questRepeatableRate = fields[5].Get<float>();
        repRate.creatureRate        = fields[6].Get<float>();
        repRate.spellRate           = fields[7].Get<float>();

        if (!sFactionStore.LookupEntry(factionID))
        {
            LOG_ERROR("sql.sql", "Faction (faction.dbc) {} does not exist but is used in `{}`", factionID, tableName);
            continue;
        }
        if (repRate.questRate < 0.0f)
        {
            LOG_ERROR("sql.sql", "Table `{}` has quest_rate with invalid rate {}, skipping data for faction {}", tableName, repRate.questRate, factionID);
            continue;
        }
        if (repRate.questDailyRate < 0.0f)
        {
            LOG_ERROR("sql.sql", "Table `{}` has quest_daily_rate with invalid rate {}, skipping data for faction {}", tableName, repRate.questDailyRate, factionID);
            continue;
        }
        if (repRate.questWeeklyRate < 0.0f)
        {
            LOG_ERROR("sql.sql", "Table `{}` has quest_weekly_rate with invalid rate {}, skipping data for faction {}", tableName, repRate.questWeeklyRate, factionID);
            continue;
        }
        if (repRate.questMonthlyRate < 0.0f)
        {
            LOG_ERROR("sql.sql", "Table `{}` has quest_monthly_rate with invalid rate {}, skipping data for faction {}", tableName, repRate.questMonthlyRate, factionID);
            continue;
        }
        if (repRate.questRepeatableRate < 0.0f)
        {
            LOG_ERROR("sql.sql", "Table `{}` has quest_repeatable_rate with invalid rate {}, skipping data for faction {}", tableName, repRate.questRepeatableRate, factionID);
            continue;
        }
        if (repRate.creatureRate < 0.0f)
        {
            LOG_ERROR("sql.sql", "Table `{}` has creature_rate with invalid rate {}, skipping data for faction {}", tableName, repRate.creatureRate, factionID);
            continue;
        }
        if (repRate.spellRate < 0.0f)
        {
            LOG_ERROR("sql.sql", "Table `{}` has spell_rate with invalid rate {}, skipping data for faction {}", tableName, repRate.spellRate, factionID);
            continue;
        }

        _repRewardRateStore[factionID] = repRate;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Reputation Reward Rate in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadReputationOnKill()
{
    const uint32 oldMSTime = getMSTime();

    _repOnKillStore.clear();

    uint32 count = 0;

    const auto tableName = "world_creature_on_kill_reputation";
    const QueryResult result = WorldDatabase.Query(
        "SELECT id, rep_faction1, rep_faction2, is_team_award1, rep_max_cap1, rep_value1, "
        "is_team_award2, rep_max_cap2, rep_value2, team_dependent FROM world_creature_on_kill_reputation");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creature award reputation definitions. DB table `{}` is empty.", tableName);
        LOG_INFO("server.loading", " ");
        return;
    }

    do
    {
        const Field* fields = result->Fetch();

        uint32 creatureID = fields[0].Get<uint32>();

        ReputationOnKillEntry repOnKill{};
        repOnKill.RepFaction1 = fields[1].Get<int16>();
        repOnKill.RepFaction2 = fields[2].Get<int16>();
        repOnKill.IsTeamAward1 = fields[3].Get<bool>();
        repOnKill.ReputationMaxCap1 = fields[4].Get<uint8>();
        repOnKill.RepValue1 = fields[5].Get<float>();
        repOnKill.IsTeamAward2 = fields[6].Get<bool>();
        repOnKill.ReputationMaxCap2 = fields[7].Get<uint8>();
        repOnKill.RepValue2 = fields[8].Get<float>();
        repOnKill.TeamDependent = fields[9].Get<bool>();

        if (!GetCreatureTemplate(creatureID))
        {
            LOG_ERROR("sql.sql", "Table `{}` have data for not existed creature entry ({}), skipped", tableName, creatureID);
            continue;
        }
        if (repOnKill.RepFaction1 && !sFactionStore.LookupEntry(repOnKill.RepFaction1))
        {
            LOG_ERROR("sql.sql", "Faction (faction.dbc) {} does not exist but is used in `{}`", repOnKill.RepFaction1, tableName);
            continue;
        }
        if (repOnKill.RepFaction2 && !sFactionStore.LookupEntry(repOnKill.RepFaction2))
        {
            LOG_ERROR("sql.sql", "Faction (faction.dbc) {} does not exist but is used in `{}`", repOnKill.RepFaction2, tableName);
            continue;
        }

        _repOnKillStore[creatureID] = repOnKill;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Creature Award Reputation Definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadReputationSpilloverTemplate()
{
    const uint32 oldMSTime = getMSTime();

    _repSpilloverTemplateStore.clear();

    const auto tableName = "world_reputation_spillover_template";
    const QueryResult result = WorldDatabase.Query("SELECT faction, factions, rate, rank FROM world_reputation_spillover_template");
    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded `{}`, table is empty.", tableName);
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        uint32 factionID = fields[0].Get<uint16>();
        const auto factions = fields[1].GetArray<uint32, MAX_SPILLOVER_FACTIONS>();
        const auto rates = fields[2].GetArray<float, MAX_SPILLOVER_FACTIONS>();
        const auto ranks = fields[3].GetArray<uint32, MAX_SPILLOVER_FACTIONS>();

        RepSpilloverTemplate repTemplate{};
        for (int i = 0; i < MAX_SPILLOVER_FACTIONS; ++i)
        {
            repTemplate.faction[i] = factions[i];
            repTemplate.factionRate[i] = rates[i];
            repTemplate.factionRank[i] = ranks[i];
        }

        const FactionEntry* factionEntry = sFactionStore.LookupEntry(factionID);
        if (!factionEntry)
        {
            LOG_ERROR("sql.sql", "Faction (faction.dbc) {} does not exist but is used in `{}`", factionID, tableName);
            continue;
        }

        if (factionEntry->Team == 0)
        {
            LOG_ERROR("sql.sql", "Faction (faction.dbc) {} in `{}` does not belong to any team, skipping", factionID, tableName);
            continue;
        }

        bool factionsOk = true;
        for (uint32 i = 0; i < MAX_SPILLOVER_FACTIONS; ++i)
        {
            if (!repTemplate.faction[i])
                continue;

            if (!sFactionStore.LookupEntry(repTemplate.faction[i]))
            {
                LOG_ERROR("sql.sql", "Faction (faction.dbc) {} does not exist but is used in `{}`", repTemplate.faction[i], tableName);
                factionsOk = false;
                break;
            }

            if (const FactionEntry* factionSpillover = sFactionStore.LookupEntry(repTemplate.faction[i]); !factionSpillover)
            {
                LOG_ERROR("sql.sql", "Spillover faction (faction.dbc) {} does not exist but is used in `{}` for faction {}, skipping", repTemplate.faction[i], tableName, factionID);
                factionsOk = false;
                break;
            }
            else if (factionSpillover->ReputationListID < 0)
            {
                LOG_ERROR("sql.sql", "Spillover faction (faction.dbc) {} for faction {} in `{}` can not be listed for client, and then useless, skipping", repTemplate.faction[i], factionID, tableName);
                factionsOk = false;
                break;
            }

            if (repTemplate.factionRank[i] >= MAX_REPUTATION_RANK)
            {
                LOG_ERROR("sql.sql", "Rank {} used in `{}` for spillover faction {} is not valid, skipping", repTemplate.factionRank[i], tableName, repTemplate.faction[i]);
                factionsOk = false;
                break;
            }
        }
        if (!factionsOk)
            continue;

        _repSpilloverTemplateStore[factionID] = repTemplate;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Reputation Spillover Template in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadPointsOfInterest()
{
    const uint32 oldMSTime = getMSTime();

    _pointsOfInterestStore.clear();

    const QueryResult result = WorldDatabase.Query("SELECT id, position, icon, flags, importance, name FROM world_poi");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 Points of Interest definitions. DB table `world_poi` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        uint32 point_id = fields[0].Get<uint32>();

        PointOfInterest POI;
        POI.ID = point_id;
        const auto position = fields[1].GetArray<float, 2>();
        POI.PositionX = position[0];
        POI.PositionY = position[1];
        POI.Icon = fields[2].Get<uint32>();
        POI.Flags = fields[3].Get<uint32>();
        POI.Importance = fields[4].Get<uint32>();
        POI.Name = fields[5].Get<std::string>();

        if (!Acore::IsValidMapCoord(POI.PositionX, POI.PositionY))
        {
            LOG_ERROR("sql.sql", "Table `world_poi` (ID: {}) have invalid coordinates (X: {} Y: {}), ignored.", point_id, POI.PositionX, POI.PositionY);
            continue;
        }

        _pointsOfInterestStore[point_id] = POI;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Points of Interest Definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadQuestPOI()
{
    if (!sWorld->getBoolConfig(CONFIG_QUEST_POI_ENABLED))
    {
        LOG_INFO("server.loading", ">> Loaded 0 quest POI definitions. Disabled by config.");
        LOG_INFO("server.loading", " ");
        return;
    }

    const uint32 oldMSTime = getMSTime();

    _questPOIStore.clear();

    const QueryResult result = WorldDatabase.Query("SELECT quest, id, objective_index, map, world_map_area, floor, priority, flags FROM world_quest_poi ORDER BY quest");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 quest POI definitions. DB table `world_quest_poi` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    const QueryResult points = WorldDatabase.Query("SELECT quest, id, x, y FROM world_quest_poi_point ORDER BY quest DESC, index");
    std::map<std::pair<uint32, uint32>, std::vector<QuestPOIPoint>> poiPoints;

    if (points)
    {
        do
        {
            const Field* fields = result->Fetch();
            const uint32 questID = fields[0].Get<uint32>();
            const uint32 id = fields[1].Get<uint32>();
            const int32 x = fields[2].Get<int32>();
            const int32 y = fields[3].Get<int32>();
            poiPoints[std::make_pair(questID, id)].emplace_back(x, y);
        } while (points->NextRow());
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        const uint32 questID = fields[0].Get<uint32>();
        const uint32 id = fields[1].Get<uint32>();
        const int32 objIndex = fields[2].Get<int32>();
        const uint32 mapID = fields[3].Get<uint32>();
        const uint32 areaID = fields[4].Get<uint32>();
        const uint32 floorID = fields[5].Get<uint32>();
        const uint32 priority = fields[6].Get<uint32>();
        const uint32 flags = fields[7].Get<uint32>();

        QuestPOI POI(id, objIndex, mapID, areaID, floorID, priority, flags);
        if (const auto key = std::make_pair(questID, id); poiPoints.contains(key))
        {
            POI.points = poiPoints[key];
            _questPOIStore[questID].emplace_back(POI);
        }
        else
            LOG_ERROR("sql.sql", "Table `world_quest_poi` references unknown quest points for quest {} POI id {}", questID, id);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Quest POI definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadNPCSpellClickSpells()
{
    const uint32 oldMSTime = getMSTime();

    _spellClickInfoStore.clear();

    const auto tableName = "world_npc_spell_click_spell";
    const QueryResult result = WorldDatabase.Query("SELECT npc, spell, cast_flags, user_type FROM world_npc_spell_click_spell");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 spell click spells. DB table `{}` is empty.", tableName);
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        uint32 npcEntry = fields[0].Get<uint32>();
        if (!GetCreatureTemplate(npcEntry))
        {
            LOG_ERROR("sql.sql", "Table `{}` references unknown creature template {}. Skipping entry.", tableName, npcEntry);
            continue;
        }

        uint32 spellID = fields[1].Get<uint32>();
        if (!sSpellMgr->GetSpellInfo(spellID))
        {
            LOG_ERROR("sql.sql", "Table `{}` references unknown spellID {}. Skipping entry.", tableName, spellID);
            continue;
        }

        const uint8 castFlags = fields[2].Get<uint8>();

        uint32 userType = fields[3].Get<uint32>();
        if (userType >= SPELL_CLICK_USER_MAX)
            LOG_ERROR("sql.sql", "Table `{}` references unknown user type {}. Skipping entry.", tableName, userType);

        SpellClickInfo info{};
        info.spellID = spellID;
        info.castFlags = castFlags;
        info.userType = static_cast<SpellClickUserTypes>(userType);
        _spellClickInfoStore.insert(SpellClickInfoContainer::value_type(npcEntry, info));

        ++count;
    } while (result->NextRow());

    // All SpellClick data loaded, now we check if there are creatures with UNIT_NPC_FLAG_SPELL_CLICK but with no data.
    // NOTE: It *CAN* be the other way around: no SpellClick flag but with SpellClick data, in case of creature-only vehicle accessories
    const CreatureTemplateContainer* ctc = GetCreatureTemplates();
    for (auto itr = ctc->begin(); itr != ctc->end(); ++itr)
    {
        if ((itr->second.FlagNPC & UNIT_NPC_FLAG_SPELL_CLICK) && !_spellClickInfoStore.contains(itr->second.Entry))
        {
            LOG_ERROR("sql.sql", "Table `{}`: creature template {} has UNIT_NPC_FLAG_SPELL_CLICK but no data in SpellClick table! Removing flag", tableName, itr->second.Entry);
            const_cast<CreatureTemplate*>(&itr->second)->FlagNPC &= ~UNIT_NPC_FLAG_SPELL_CLICK;
        }
    }

    LOG_INFO("server.loading", ">> Loaded {} SpellClick Definitions in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::DeleteCreatureData(const ObjectGuid::LowType guid)
{
    if (const CreatureData* data = GetCreatureData(guid))
        RemoveCreatureFromGrid(guid, data);
    _creatureDataStore.erase(guid);
}

void ObjectMgr::DeleteGOData(const ObjectGuid::LowType guid)
{
    if (const GameObjectData* data = GetGameObjectData(guid))
        RemoveGameObjectFromGrid(guid, data);
    _gameObjectDataStore.erase(guid);
}

const SpawnData* ObjectMgr::GetSpawnData(const SpawnObjectType type, const ObjectGuid::LowType spawnID) const
{
    switch (type)
    {
        case SPAWN_TYPE_CREATURE:
            return GetCreatureData(spawnID);
        case SPAWN_TYPE_GAMEOBJECT:
            return GetGameObjectData(spawnID);
        default:
            return nullptr;
    }
}

void ObjectMgr::LoadQuestRelationsHelper(QuestRelations& map, const std::string& table, const bool starter, const bool go) const {
    const uint32 oldMSTime = getMSTime();

    map.clear();

    const QueryResult result = WorldDatabase.Query("SELECT qr.id, quest, pool FROM {} qr LEFT JOIN world_pool_quest pq ON qr.quest = pq.id", table);
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 quest relations from `{}`, table is empty.", table);
        LOG_INFO("server.loading", " ");
        return;
    }

    PooledQuestRelation* poolRelationMap = go ? &sPoolMgr->mQuestGORelation : &sPoolMgr->mQuestCreatureRelation;
    if (starter)
        poolRelationMap->clear();

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();
        const uint32 id = fields[0].Get<uint32>();
        const uint32 quest = fields[1].Get<uint32>();
        const uint32 poolID = fields[2].Get<uint32>();

        if (!_questTemplates.contains(quest))
        {
            LOG_ERROR("sql.sql", "Table `{}`: Quest {} listed for entry {} does not exist.", table, quest, id);
            continue;
        }

        if (!poolID || !starter)
            map.insert(QuestRelations::value_type(id, quest));
        else if (starter)
            poolRelationMap->insert(PooledQuestRelation::value_type(quest, id));

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Quest Relations From {} in {} ms", count, table, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadGameObjectQuestStarters()
{
    LoadQuestRelationsHelper(_goQuestRelations, "world_game_object_quest_starter", true, true);

    for (auto itr = _goQuestRelations.begin(); itr != _goQuestRelations.end(); ++itr)
    {
        if (const GameObjectTemplate* goInfo = GetGameObjectTemplate(itr->first); !goInfo)
            LOG_ERROR("sql.sql", "Table `world_game_object_quest_starter` have data for not existed GameObject entry ({}) and existed quest {}", itr->first, itr->second);
        else if (goInfo->Type != GAME_OBJECT_TYPE_QUEST_GIVER)
            LOG_ERROR("sql.sql", "Table `world_game_object_quest_starter` have data GameObject entry ({}) for quest {}, but GO is not GAME_OBJECT_TYPE_QUEST_GIVER", itr->first, itr->second);
    }
}

void ObjectMgr::LoadGameObjectQuestEnders()
{
    LoadQuestRelationsHelper(_goQuestInvolvedRelations, "world_game_object_quest_ender", false, true);

    for (auto itr = _goQuestInvolvedRelations.begin(); itr != _goQuestInvolvedRelations.end(); ++itr)
    {
        if (const GameObjectTemplate* goInfo = GetGameObjectTemplate(itr->first); !goInfo)
            LOG_ERROR("sql.sql", "Table `world_game_object_quest_ender` have data for not existed GameObject entry ({}) and existed quest {}", itr->first, itr->second);
        else if (goInfo->Type != GAME_OBJECT_TYPE_QUEST_GIVER)
            LOG_ERROR("sql.sql", "Table `world_game_object_quest_ender` have data GameObject entry ({}) for quest {}, but GO is not GAME_OBJECT_TYPE_QUEST_GIVER", itr->first, itr->second);
    }
}

void ObjectMgr::LoadCreatureQuestStarters()
{
    LoadQuestRelationsHelper(_creatureQuestRelations, "world_creature_quest_starter", true, false);

    for (auto itr = _creatureQuestRelations.begin(); itr != _creatureQuestRelations.end(); ++itr)
    {
        if (const CreatureTemplate* cInfo = GetCreatureTemplate(itr->first); !cInfo)
            LOG_ERROR("sql.sql", "Table `world_creature_quest_starter` have data for not existed creature entry ({}) and existed quest {}", itr->first, itr->second);
        else if (!(cInfo->FlagNPC & UNIT_NPC_FLAG_QUEST_GIVER))
            LOG_ERROR("sql.sql", "Table `world_creature_quest_starter` has Creature entry ({}) for quest {}, but FlagNPC does not include UNIT_NPC_FLAG_QUEST_GIVER", itr->first, itr->second);
    }
}

void ObjectMgr::LoadCreatureQuestEnders()
{
    LoadQuestRelationsHelper(_creatureQuestInvolvedRelations, "world_creature_quest_ender", false, false);

    for (auto itr = _creatureQuestInvolvedRelations.begin(); itr != _creatureQuestInvolvedRelations.end(); ++itr)
    {
        if (const CreatureTemplate* cInfo = GetCreatureTemplate(itr->first); !cInfo)
            LOG_ERROR("sql.sql", "Table `world_creature_quest_ender` have data for not existed creature entry ({}) and existed quest {}", itr->first, itr->second);
        else if (!(cInfo->FlagNPC & UNIT_NPC_FLAG_QUEST_GIVER))
            LOG_ERROR("sql.sql", "Table `world_creature_quest_ender` has Creature entry ({}) for quest {}, but FlagNPC does not include UNIT_NPC_FLAG_QUEST_GIVER", itr->first, itr->second);
    }
}

void ObjectMgr::LoadReservedPlayerNamesDB()
{
    const uint32 oldMSTime = getMSTime();

    _reservedNamesStore.clear();

    const QueryResult result = CharacterDatabase.Query("SELECT name FROM reserved_name");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 reserved names. DB table `reserved_name` is empty!");
        return;
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();
        auto name = fields[0].Get<std::string>();

        std::wstring wstr;
        if (!Utf8toWStr(name, wstr))
        {
            LOG_ERROR("sql.sql", "Table `reserved_name` have invalid name: {}", name);
            continue;
        }

        wstrToLower(wstr);

        _reservedNamesStore.insert(wstr);
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} reserved names from DB in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadReservedPlayerNamesDBC()
{
    if (!sWorld->getBoolConfig(CONFIG_STRICT_NAMES_RESERVED))
    {
        LOG_WARN("server.loading", ">> Loaded 0 reserved names from DBC. Config option disabled.");
        return;
    }

    const uint32 oldMSTime = getMSTime();

    uint32 count = 0;
    for (const NamesReservedEntry* reservedStore : sNamesReservedStore)
    {
        std::wstring wstr;

        Utf8toWStr(reservedStore->Pattern, wstr);

        // DBC does not have clean entries, remove the junk.
        boost::algorithm::replace_all(wstr, "\\<", "");
        boost::algorithm::replace_all(wstr, "\\>", "");

        _reservedNamesStore.insert(wstr);
        count++;
    }

    LOG_INFO("server.loading", ">> Loaded {} reserved names from DBC in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

bool ObjectMgr::IsReservedName(const std::string_view name) const
{
    std::wstring wstr;
    if (!Utf8toWStr (name, wstr))
        return false;
    wstrToLower(wstr);
    return _reservedNamesStore.contains(wstr);
}

void ObjectMgr::AddReservedPlayerName(const std::string& name)
{
    if (IsReservedName(name))
        return;
    std::wstring wstr;
    if (!Utf8toWStr(name, wstr))
    {
        LOG_ERROR("server", "Could not add invalid name to reserved player names: {}", name);
        return;
    }
    wstrToLower(wstr);

    _reservedNamesStore.insert(wstr);

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_RESERVED_PLAYER_NAME);
    stmt->SetData(0, name);
    CharacterDatabase.Execute(stmt);
}

void ObjectMgr::LoadProfanityNamesFromDB()
{
    const uint32 oldMSTime = getMSTime();

    _profanityNamesStore.clear();                                // need for reload case

    const QueryResult result = CharacterDatabase.Query("SELECT name FROM profanity_name");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 profanity names. DB table `profanity_name` is empty!");
        return;
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();
        auto name = fields[0].Get<std::string>();

        std::wstring wstr;
        if (!Utf8toWStr(name, wstr))
        {
            LOG_ERROR("sql.sql", "Table `profanity_name` have invalid name: {}", name);
            continue;
        }

        wstrToLower(wstr);

        _profanityNamesStore.insert(wstr);
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} profanity names from DB in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadProfanityNamesFromDBC()
{
    if (!sWorld->getBoolConfig(CONFIG_STRICT_NAMES_PROFANITY))
    {
        LOG_WARN("server.loading", ">> Loaded 0 profanity names from DBC. Config option disabled.");
        return;
    }

    const uint32 oldMSTime = getMSTime();

    uint32 count = 0;

    for (const NamesProfanityEntry* profanityStore : sNamesProfanityStore)
    {
        std::wstring wstr;

        Utf8toWStr(profanityStore->Pattern, wstr);

        // DBC does not have clean entries, remove the junk.
        boost::algorithm::replace_all(wstr, "\\<", "");
        boost::algorithm::replace_all(wstr, "\\>", "");

        _profanityNamesStore.insert(wstr);
        count++;
    }

    LOG_INFO("server.loading", ">> Loaded {} profanity names from DBC in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

bool ObjectMgr::IsProfanityName(const std::string_view name) const
{
    std::wstring wstr;
    if (!Utf8toWStr(name, wstr))
        return false;
    wstrToLower(wstr);
    return _profanityNamesStore.contains(wstr);
}

void ObjectMgr::AddProfanityPlayerName(const std::string& name)
{
    if (IsProfanityName(name))
        return;
    std::wstring wstr;
    if (!Utf8toWStr(name, wstr))
    {
        LOG_ERROR("server", "Could not add invalid name to profanity player names: {}", name);
        return;
    }
    wstrToLower(wstr);

    _profanityNamesStore.insert(wstr);

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_PROFANITY_PLAYER_NAME);
    stmt->SetData(0, name);
    CharacterDatabase.Execute(stmt);
}

enum LanguageType
{
    LT_BASIC_LATIN    = 0x0000,
    LT_EXTENDED_LATIN = 0x0001,
    LT_CYRILLIC       = 0x0002,
    LT_EAST_ASIA      = 0x0004,
    LT_ANY            = 0xFFFF
};

static LanguageType GetRealmLanguageType(const bool create)
{
    switch (realm.Timezone)
    {
        case REALM_ZONE_UNKNOWN:  // Any language
        case REALM_ZONE_DEVELOPMENT:
        case REALM_ZONE_TEST_SERVER:
        case REALM_ZONE_QA_SERVER:
            return LT_ANY;
        case REALM_ZONE_UNITED_STATES:  // Extended-Latin
        case REALM_ZONE_OCEANIC:
        case REALM_ZONE_LATIN_AMERICA:
        case REALM_ZONE_ENGLISH:
        case REALM_ZONE_GERMAN:
        case REALM_ZONE_FRENCH:
        case REALM_ZONE_SPANISH:
            return LT_EXTENDED_LATIN;
        case REALM_ZONE_KOREA:  // East-Asian
        case REALM_ZONE_TAIWAN:
        case REALM_ZONE_CHINA:
            return LT_EAST_ASIA;
        case REALM_ZONE_RUSSIAN:  // Cyrillic
            return LT_CYRILLIC;
        default:
            return create ? LT_BASIC_LATIN : LT_ANY;  // Basic-Latin at create, any at login
    }
}

bool isValidString(const std::wstring& wstr, const uint32 strictMask, const bool numericOrSpace, const bool create = false)
{
    if (strictMask == 0)                                       // any language, ignore realm
    {
        if (isExtendedLatinString(wstr, numericOrSpace))
            return true;
        if (isCyrillicString(wstr, numericOrSpace))
            return true;
        if (isEastAsianString(wstr, numericOrSpace))
            return true;
        return false;
    }

    if (strictMask & 0x2)  // Realm zone specific
    {
        LanguageType lt = GetRealmLanguageType(create);
        if (lt & LT_EXTENDED_LATIN && isExtendedLatinString(wstr, numericOrSpace))
            return true;
        if (lt & LT_CYRILLIC && isCyrillicString(wstr, numericOrSpace))
            return true;
        if (lt & LT_EAST_ASIA && isEastAsianString(wstr, numericOrSpace))
            return true;
    }

    if (strictMask & 0x1)  // Basic Latin
    {
        if (isBasicLatinString(wstr, numericOrSpace))
            return true;
    }

    return false;
}

uint8 ObjectMgr::CheckPlayerName(const std::string_view name, const bool create)
{
    std::wstring wName;

    // Check for invalid characters
    if (!Utf8toWStr(name, wName))
        return CHAR_NAME_INVALID_CHARACTER;

    // Check for too long name
    if (wName.size() > MAX_PLAYER_NAME)
        return CHAR_NAME_TOO_LONG;

    // Check for too short name
    if (wName.size() < sWorld->getIntConfig(CONFIG_MIN_PLAYER_NAME))
        return CHAR_NAME_TOO_SHORT;

    // Check for mixed languages
    if (!isValidString(wName, sWorld->getIntConfig(CONFIG_STRICT_PLAYER_NAMES), false, create))
        return CHAR_NAME_MIXED_LANGUAGES;

    // Check for three consecutive letters
    wstrToLower(wName);
    for (std::size_t i = 2; i < wName.size(); ++i)
        if (wName[i] == wName[i - 1] && wName[i] == wName[i - 2])
            return CHAR_NAME_THREE_CONSECUTIVE;

    // Check Reserved Name
    if (sObjectMgr->IsReservedName(name))
        return CHAR_NAME_RESERVED;

    // Check Profanity Name
    if (sObjectMgr->IsProfanityName(name))
        return CHAR_NAME_PROFANE;

    return CHAR_NAME_SUCCESS;
}

bool ObjectMgr::IsValidCharterName(const std::string_view name)
{
    std::wstring wName;
    if (!Utf8toWStr(name, wName))
        return false;

    if (wName.size() > MAX_CHARTER_NAME)
        return false;

    if (wName.size() < sWorld->getIntConfig(CONFIG_MIN_CHARTER_NAME))
        return false;

    // Check Reserved Name
    if (sObjectMgr->IsReservedName(name))
        return false;

    // Check Profanity Name
    if (sObjectMgr->IsProfanityName(name))
        return false;

    return isValidString(wName, sWorld->getIntConfig(CONFIG_STRICT_CHARTER_NAMES), true);
}

bool ObjectMgr::IsValidChannelName(const std::string& name)
{
    std::wstring wName;
    if (!Utf8toWStr(name, wName))
        return false;

    if (wName.size() > MAX_CHANNEL_NAME)
        return false;

    return isValidString(wName, sWorld->getIntConfig(CONFIG_STRICT_CHANNEL_NAMES), true);
}

PetNameInvalidReason ObjectMgr::CheckPetName(const std::string_view name)
{
    std::wstring wName;
    if (!Utf8toWStr(name, wName))
        return PET_NAME_INVALID;

    if (wName.size() > MAX_PET_NAME)
        return PET_NAME_TOO_LONG;

    if (wName.size() < sWorld->getIntConfig(CONFIG_MIN_PET_NAME))
        return PET_NAME_TOO_SHORT;

    if (!isValidString(wName, sWorld->getIntConfig(CONFIG_STRICT_PET_NAMES), false))
        return PET_NAME_MIXED_LANGUAGES;

    // Check Reserved Name
    if (sObjectMgr->IsReservedName(name))
        return PET_NAME_RESERVED;

    // Check Profanity Name
    if (sObjectMgr->IsProfanityName(name))
        return PET_NAME_PROFANE;

    return PET_NAME_SUCCESS;
}

void ObjectMgr::LoadGameObjectForQuests() const {
    const uint32 oldMSTime = getMSTime();

    if (GetGameObjectTemplates()->empty())
    {
        LOG_WARN("server.loading", ">> Loaded 0 GameObjects for quests");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    // Collect GO entries for GO that must activated
    const auto goTemplateContainer = const_cast<GameObjectTemplateContainer*>(GetGameObjectTemplates());
    for (auto &goTemplate: *goTemplateContainer | std::views::values)
    {
        goTemplate.IsForQuests = false;
        switch (goTemplate.Type)
        {
        case GAME_OBJECT_TYPE_QUEST_GIVER:
            goTemplate.IsForQuests = true;
            ++count;
            break;
        case GAME_OBJECT_TYPE_CHEST:
            {
                // Find quest loot for GO
                if (goTemplate.Chest.questId || LootTemplates_Gameobject.HaveQuestLootFor(goTemplate.GetLootId()))
                {
                    goTemplate.IsForQuests = true;
                    ++count;
                }
                break;
            }
        case GAME_OBJECT_TYPE_GENERIC:
            {
                if (goTemplate.Generic.questID > 0)  // Quests objects
                {
                    goTemplate.IsForQuests = true;
                    ++count;
                }
                break;
            }
        case GAME_OBJECT_TYPE_SPELL_FOCUS:
            {
                if (goTemplate.SpellFocus.questID > 0)  // Quests objects
                {
                    goTemplate.IsForQuests = true;
                    ++count;
                }
                break;
            }
        case GAME_OBJECT_TYPE_GOOBER:
            {
                if (goTemplate.Goober.questId > 0)  // Quests objects
                {
                    goTemplate.IsForQuests = true;
                    ++count;
                }
                break;
            }
        default:
            break;
        }
    }

    LOG_INFO("server.loading", ">> Loaded {} GameObjects for quests in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

bool ObjectMgr::LoadAcoreStrings()
{
    const uint32 oldMSTime = getMSTime();

    _acoreStringStore.clear();  // For reload case
    const QueryResult result = WorldDatabase.Query("SELECT id, content FROM world_localized_string");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 acore strings. DB table `world_localized_string` is empty.");
        LOG_INFO("server.loading", " ");
        return false;
    }

    do
    {
        const Field* fields = result->Fetch();

        const uint32 entry = fields[0].Get<uint32>();
        _acoreStringStore[entry] = fields[1].Get<std::string>();
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Acore Strings in {} ms", static_cast<uint32>(_acoreStringStore.size()), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");

    return true;
}

void ObjectMgr::LoadFishingBaseSkillLevel()
{
    const uint32 oldMSTime = getMSTime();

    _fishingBaseForAreaStore.clear();

    const QueryResult result = WorldDatabase.Query("SELECT id, skill FROM world_skill_fishing_base_level");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 areas for fishing base skill level. DB table `world_skill_fishing_base_level` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();
        const uint32 entry = fields[0].Get<uint32>();
        const int32 skill = fields[1].Get<int16>();

        if (!sAreaTableStore.LookupEntry(entry))
        {
            LOG_ERROR("sql.sql", "AreaID {} defined in `world_skill_fishing_base_level` does not exist", entry);
            continue;
        }

        _fishingBaseForAreaStore[entry] = skill;
        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} areas for fishing base skill level in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::ChangeFishingBaseSkillLevel(uint32 entry, int32 skill)
{
    if (!sAreaTableStore.LookupEntry(entry))
    {
        LOG_ERROR("sql.sql", "AreaId {} defined in `world_skill_fishing_base_level` does not exist", entry);
        return;
    }

    _fishingBaseForAreaStore[entry] = skill;

    LOG_INFO("server.loading", ">> Fishing base skill level of area {} changed to {}", entry, skill);
    LOG_INFO("server.loading", " ");
}

bool ObjectMgr::CheckDeclinedNames(const std::wstring& wOwnName, const DeclinedName& names)
{
    // Get main part of the name
    const std::wstring mainPart = GetMainPartOfName(wOwnName, 0);

    // Prepare flags
    bool x = true;
    bool y = true;

    // Check declined names
    for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
    {
        std::wstring wName;
        if (!Utf8toWStr(names.name[i], wName))
            return false;

        if (mainPart != GetMainPartOfName(wName, i + 1))
            x = false;

        if (wOwnName != wName)
            y = false;
    }
    return x || y;
}

uint32 ObjectMgr::GetAreaTriggerScriptId(const uint32 triggerID)
{
    if (const auto i = _areaTriggerScriptStore.find(triggerID); i != _areaTriggerScriptStore.end())
        return i->second;
    return 0;
}

SpellScriptsBounds ObjectMgr::GetSpellScriptsBounds(const uint32 spellID)
{
    return {_spellScriptsStore.lower_bound(spellID), _spellScriptsStore.upper_bound(spellID)};
}

// This allows calculating base reputations to offline players, just by race and class
int32 ObjectMgr::GetBaseReputationOf(const FactionEntry* factionEntry, const uint8 race, const uint8 playerClass)
{
    if (!factionEntry)
        return 0;

    const uint32 raceMask = 1 << (race - 1);
    const uint32 classMask = 1 << (playerClass - 1);

    for (int i = 0; i < 4; i++)
    {
        if ((!factionEntry->BaseRepClassMask[i] || factionEntry->BaseRepClassMask[i] & classMask) && (!factionEntry->BaseRepRaceMask[i] || factionEntry->BaseRepRaceMask[i] & raceMask))
            return factionEntry->BaseRepValue[i];
    }
    return 0;
}

SkillRangeType GetSkillRangeType(const SkillRaceClassInfoEntry* rcEntry)
{
    const SkillLineEntry* skill = sSkillLineStore.LookupEntry(rcEntry->SkillID);
    if (!skill)
        return SKILL_RANGE_NONE;

    if (sSkillTiersStore.LookupEntry(rcEntry->SkillTierID))
        return SKILL_RANGE_RANK;

    if (rcEntry->SkillID == SKILL_RUNEFORGING)
        return SKILL_RANGE_MONO;

    switch (skill->CategoryID)
    {
        case SKILL_CATEGORY_ARMOR:
            return SKILL_RANGE_MONO;
        case SKILL_CATEGORY_LANGUAGES:
            return SKILL_RANGE_LANGUAGE;
        default:
            break;
    }

    return SKILL_RANGE_LEVEL;
}

void ObjectMgr::LoadGameTele()
{
    const uint32 oldMSTime = getMSTime();

    _gameTeleStore.clear();

    const QueryResult result = WorldDatabase.Query("SELECT id, position, orientation, map, name FROM world_game_teleport");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 GameTeleports. DB table `world_game_teleport` is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();

        uint32 id = fields[0].Get<uint32>();

        GameTele gt;

        const auto position = fields[1].GetArray<float, 3>();
        gt.PositionX = position[0];
        gt.PositionY = position[1];
        gt.PositionZ = position[2];
        gt.Orientation = fields[2].Get<float>();
        gt.MapID = fields[3].Get<uint16>();
        gt.Name = fields[4].Get<std::string>();

        if (!MapMgr::IsValidMapCoord(gt.MapID, gt.PositionX, gt.PositionY, gt.PositionZ, gt.Orientation))
        {
            LOG_ERROR("sql.sql", "Wrong position for id {} (name: {}) in `world_game_teleport` table, ignoring.", id, gt.Name);
            continue;
        }
        if (!Utf8toWStr(gt.Name, gt.WNameLow))
        {
            LOG_ERROR("sql.sql", "Wrong UTF8 name for id {} in `world_game_teleport` table, ignoring.", id);
            continue;
        }

        wstrToLower(gt.WNameLow);

        _gameTeleStore[id] = gt;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} GameTeleports in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

const GameTele* ObjectMgr::GetGameTele(const std::string_view name, const bool exactSearch) const
{
    // Explicit name case
    std::wstring wName;
    if (!Utf8toWStr(name, wName))
        return nullptr;

    // Converting string that we try to find to lower case
    wstrToLower(wName);

    // Alternative first GameTele what contains wNameLow as substring in case no GameTele location found
    const GameTele* alt = nullptr;
    for (const auto &tele: _gameTeleStore | std::views::values)
    {
        if (tele.WNameLow == wName)
            return &tele;
        if (!exactSearch && !alt && tele.WNameLow.find(wName) != std::wstring::npos)
            alt = &tele;
    }

    return alt;
}

bool ObjectMgr::AddGameTele(GameTele& tele)
{
    // Find max id
    uint32 newID = 0;
    for (const auto &id: _gameTeleStore | std::views::keys)
        if (id > newID)
            newID = id;

    // Use next
    ++newID;

    if (!Utf8toWStr(tele.Name, tele.WNameLow))
        return false;

    wstrToLower(tele.WNameLow);

    _gameTeleStore[newID] = tele;

    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_INS_GAME_TELE);
    stmt->SetData(0, newID);
    stmt->SetData(1, tele.PositionX);
    stmt->SetData(2, tele.PositionY);
    stmt->SetData(3, tele.PositionZ);
    stmt->SetData(4, tele.Orientation);
    stmt->SetData(5, tele.MapID);
    stmt->SetData(6, tele.Name);

    WorldDatabase.Execute(stmt);
    return true;
}

bool ObjectMgr::DeleteGameTele(const std::string_view name)
{
    // Explicit name case
    std::wstring wName;
    if (!Utf8toWStr(name, wName))
        return false;

    // Converting string that we try to find to lower case
    wstrToLower(wName);

    for (auto itr = _gameTeleStore.begin(); itr != _gameTeleStore.end(); ++itr)
    {
        if (itr->second.WNameLow == wName)
        {
            WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_GAME_TELE);
            stmt->SetData(0, itr->second.Name);
            WorldDatabase.Execute(stmt);

            _gameTeleStore.erase(itr);
            return true;
        }
    }

    return false;
}

void ObjectMgr::LoadMailLevelRewards()
{
    const uint32 oldMSTime = getMSTime();

    _mailLevelRewardStore.clear();

    const QueryResult result = WorldDatabase.Query("SELECT level, race_mask, mail_template, sender_entry FROM world_mail_level_reward");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 level dependent mail rewards. DB table `world_mail_level_reward` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();

        uint8 level           = fields[0].Get<uint8>();
        uint32 raceMask       = fields[1].Get<uint32>();
        uint32 mailTemplateID = fields[2].Get<uint32>();
        uint32 senderEntry    = fields[3].Get<uint32>();

        if (level > MAX_LEVEL)
        {
            LOG_ERROR("sql.sql", "Table `world_mail_level_reward` have data for level {} that more supported by client ({}), ignoring.", level, MAX_LEVEL);
            continue;
        }

        if (!(raceMask & sRaceMgr->GetPlayableRaceMask()))
        {
            LOG_ERROR("sql.sql", "Table `world_mail_level_reward` have raceMask ({}) for level {} that not include any player races, ignoring.", raceMask, level);
            continue;
        }

        if (!sMailTemplateStore.LookupEntry(mailTemplateID))
        {
            LOG_ERROR("sql.sql", "Table `world_mail_level_reward` have invalid mailTemplateID ({}) for level {} that invalid not include any player races, ignoring.", mailTemplateID, level);
            continue;
        }

        if (!GetCreatureTemplate(senderEntry))
        {
            LOG_ERROR("sql.sql", "Table `world_mail_level_reward` have not existed sender creature entry ({}) for level {} that invalid not include any player races, ignoring.", senderEntry, level);
            continue;
        }

        _mailLevelRewardStore[level].emplace_back(raceMask, mailTemplateID, senderEntry);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Level Dependent Mail Rewards in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadTrainers()
{
    const uint32 oldMSTime = getMSTime();

    // For reload case
    _trainers.clear();
    _classTrainers.clear();

    std::unordered_map<uint32, std::vector<Trainer::Spell>> spellsByTrainer;
    const auto table = "world_trainer_spell";
    if (const QueryResult trainerSpellsResult = WorldDatabase.Query("SELECT trainer, spell, money_cost, req_skill_line, req_skill_rank, req_ability, req_level FROM world_trainer_spell"))
    {
        do
        {
            const Field* fields = trainerSpellsResult->Fetch();

            Trainer::Spell spell;
            uint32 trainerID = fields[0].Get<uint32>();
            spell.SpellID = fields[1].Get<uint32>();
            spell.MoneyCost = fields[2].Get<uint32>();
            spell.ReqSkillLine = fields[3].Get<uint32>();
            spell.ReqSkillRank = fields[4].Get<uint32>();
            const auto abilities = fields[5].GetArray<uint32, 3>();
            spell.ReqAbility[0] = abilities[0];
            spell.ReqAbility[1] = abilities[1];
            spell.ReqAbility[2] = abilities[2];
            spell.ReqLevel = fields[6].Get<uint8>();

            if (!sSpellMgr->GetSpellInfo(spell.SpellID))
            {
                LOG_ERROR("sql.sql", "Table `{}` references non-existing spell (SpellID: {}) for TrainerID {}, ignoring", table, spell.SpellID, trainerID);
                continue;
            }
            if (GetTalentSpellCost(spell.SpellID))
            {
                LOG_ERROR("sql.sql", "Table `{}` references non-existing spell (SpellID: {}) which is a talent, for TrainerID {}, ignoring", table, spell.SpellID, trainerID);
                continue;
            }
            if (spell.ReqSkillLine && !sSkillLineStore.LookupEntry(spell.ReqSkillLine))
            {
                LOG_ERROR("sql.sql", "Table `{}` references non-existing skill (ReqSkillLine: {}) for TrainerID {} and SpellID {}, ignoring", table, spell.ReqSkillLine, spell.SpellID, trainerID);
                continue;
            }

            bool allReqValid = true;
            for (std::size_t i = 0; i < spell.ReqAbility.size(); ++i)
            {
                if (uint32 requiredSpell = spell.ReqAbility[i]; requiredSpell && !sSpellMgr->GetSpellInfo(requiredSpell))
                {
                    LOG_ERROR("sql.sql", "Table `{}` references non-existing spell (ReqAbility[{}] : {}) for TrainerID {} and SpellID {}, ignoring", table, i, requiredSpell, trainerID, spell.SpellID);
                    allReqValid = false;
                }
            }

            if (!allReqValid)
                continue;

            spellsByTrainer[trainerID].push_back(spell);
        } while (trainerSpellsResult->NextRow());
    }

    if (const QueryResult trainersResult = WorldDatabase.Query("SELECT id, type, requirement, greeting FROM world_trainer"))
    {
        do
        {
            const Field* fields = trainersResult->Fetch();

            uint32 trainerID = fields[0].Get<uint32>();
            auto trainerType = static_cast<Trainer::Type>(fields[1].Get<uint8>());
            uint32 requirement = fields[2].Get<uint32>();
            auto greeting = fields[3].Get<std::string>();
            std::vector<Trainer::Spell> spells;
            if (auto spellsItr = spellsByTrainer.find(trainerID); spellsItr != spellsByTrainer.end())
            {
                spells = std::move(spellsItr->second);
                spellsByTrainer.erase(spellsItr);
            }

            auto [it, isNew] = _trainers.emplace(std::piecewise_construct, std::forward_as_tuple(trainerID),
                std::forward_as_tuple(trainerID, trainerType, requirement, std::move(greeting), std::move(spells)));
            ASSERT(isNew);
            if (trainerType == Trainer::Type::Class)
            {
                if (!requirement || requirement >= MAX_CLASSES)
                    LOG_ERROR("sql.sql", "Table `world_trainer` has invalid class requirement for trainer {}, ignoring", trainerID);
                else
                    _classTrainers[static_cast<uint8>(requirement)].push_back(&it->second);
            }
        } while (trainersResult->NextRow());
    }

    for (const auto& [trainerID, trainerSpells] : spellsByTrainer)
    {
        for (const Trainer::Spell& unusedSpell : trainerSpells)
            LOG_ERROR("sql.sql", "Table `{}` references non-existing trainer (TrainerID: {}) for SpellID {}, ignoring", table, trainerID, unusedSpell.SpellID);
    }

    LOG_INFO("server.loading", ">> Loaded {} Trainers in {} ms", _trainers.size(), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadCreatureDefaultTrainers()
{
    const uint32 oldMSTime = getMSTime();

    _creatureDefaultTrainers.clear();

    if (const QueryResult result = WorldDatabase.Query("SELECT id, trainer FROM world_creature_default_trainer"))
    {
        do
        {
            const Field* fields = result->Fetch();
            const uint32 creatureID = fields[0].Get<uint32>();
            const uint32 trainerID = fields[1].Get<uint32>();

            if (!GetCreatureTemplate(creatureID))
            {
                LOG_ERROR("sql.sql", "Table `world_creature_default_trainer` references non-existing creature template (CreatureID: %u), ignoring", creatureID);
                continue;
            }

            _creatureDefaultTrainers[creatureID] = trainerID;

        } while (result->NextRow());
    }

    LOG_INFO("server.loading", ">> Loaded {} default trainers in {} ms", _creatureDefaultTrainers.size(), GetMSTimeDiffToNow(oldMSTime));
}

Trainer::Trainer* ObjectMgr::GetTrainer(const uint32 creatureId)
{
    if (auto itr = _creatureDefaultTrainers.find(creatureId); itr != _creatureDefaultTrainers.end())
        return Acore::Containers::MapGetValuePtr(_trainers, itr->second);
    return nullptr;
}

int ObjectMgr::LoadReferenceVendor(const uint32 vendor, const int32 item, std::set<uint32>* skip_vendors)
{
    // Find all items from the reference vendor
    WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_NPC_VENDOR_REF);
    stmt->SetData(0, static_cast<uint32>(item));

    const QueryResult result = WorldDatabase.Query(stmt);
    if (!result)
        return 0;

    int count = 0;
    do
    {
        const Field* fields = result->Fetch();
        const int32 itemID = fields[0].Get<int32>();

        // If item is a negative, it's a reference
        if (itemID < 0)
            count += LoadReferenceVendor(vendor, -itemID, skip_vendors);
        else
        {
            const uint32 maxCount = fields[1].Get<uint32>();
            const uint32 incrTime = fields[2].Get<uint32>();
            const uint32 extendedCost = fields[3].Get<uint32>();

            if (!IsVendorItemValid(vendor, itemID, maxCount, incrTime, extendedCost, nullptr, skip_vendors))
                continue;

            VendorItemData& vList = _cacheVendorItemStore[vendor];
            vList.AddItem(itemID, maxCount, incrTime, extendedCost);
            ++count;
        }
    } while (result->NextRow());

    return count;
}

void ObjectMgr::LoadVendors()
{
    const uint32 oldMSTime = getMSTime();

    // For reload case
    for (auto &itemData: _cacheVendorItemStore | std::views::values)
        itemData.Clear();
    _cacheVendorItemStore.clear();

    std::set<uint32> skip_vendors;

    const QueryResult result = WorldDatabase.Query("SELECT entry, item, max_count, incr_time, extended_cost FROM world_npc_vendor ORDER BY entry, slot ASC, item, extended_cost");
    if (!result)
    {
        LOG_INFO("server.loading", " ");
        LOG_WARN("server.loading", ">> Loaded 0 Vendors. DB table `world_npc_vendor` is empty!");
        return;
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();

        const uint32 entry = fields[0].Get<uint32>();
        const int32 itemID = fields[1].Get<int32>();

        // If item is a negative, it's a reference
        if (itemID < 0)
            count += LoadReferenceVendor(entry, -itemID, &skip_vendors);
        else
        {
            const uint32 maxCount = fields[2].Get<uint32>();
            const uint32 incrTime = fields[3].Get<uint32>();
            const uint32 extendedCost = fields[4].Get<uint32>();

            if (!IsVendorItemValid(entry, itemID, maxCount, incrTime, extendedCost, nullptr, &skip_vendors))
                continue;

            VendorItemData& vList = _cacheVendorItemStore[entry];
            vList.AddItem(itemID, maxCount, incrTime, extendedCost);
            ++count;
        }
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Vendors in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadGossipMenu()
{
    const uint32 oldMSTime = getMSTime();

    _gossipMenusStore.clear();

    const QueryResult result = WorldDatabase.Query("SELECT menu, text FROM world_gossip_menu");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 gossip_menu entries. DB table `world_gossip_menu` is empty!");
        LOG_INFO("server.loading", " ");
        return;
    }

    do
    {
        const Field* fields = result->Fetch();

        GossipMenus gMenu;

        gMenu.MenuID = fields[0].Get<uint32>();
        gMenu.TextID = fields[1].Get<uint32>();

        if (!GetGossipText(gMenu.TextID))
        {
            LOG_ERROR("sql.sql", "Table `world_gossip_menu` entry {} are using non-existing TextID {}", gMenu.MenuID, gMenu.TextID);
            continue;
        }

        _gossipMenusStore.insert(GossipMenusContainer::value_type(gMenu.MenuID, gMenu));
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} `world_gossip_menu` entries in {} ms", static_cast<uint32>(_gossipMenusStore.size()), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadGossipMenuItems()
{
    const uint32 oldMSTime = getMSTime();

    _gossipMenuItemsStore.clear();

    const auto table = "world_gossip_menu_option";
    const QueryResult result = WorldDatabase.Query(
        "SELECT menu, id, icon, text, broadcast, type, npc_flag, action_menu, action_poi, box_coded, box_money, box_text, box_broadcast "
        "FROM world_gossip_menu_option ORDER BY menu, id");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 `{}` IDs. DB table `gossip_menu_option` is empty!", table);
        LOG_INFO("server.loading", " ");
        return;
    }

    do
    {
        const Field* fields = result->Fetch();

        GossipMenuItems gMenuItem;
        gMenuItem.MenuID                = fields[0].Get<uint32>();
        gMenuItem.OptionID              = fields[1].Get<uint16>();
        gMenuItem.OptionIcon            = fields[2].Get<uint32>();
        gMenuItem.OptionText            = fields[3].Get<std::string>();
        gMenuItem.OptionBroadcastTextID = fields[4].Get<uint32>();
        gMenuItem.OptionType            = fields[5].Get<uint8>();
        gMenuItem.OptionNpcFlag         = fields[6].Get<uint32>();
        gMenuItem.ActionMenuID          = fields[7].Get<uint32>();
        gMenuItem.ActionPoiID           = fields[8].Get<uint32>();
        gMenuItem.BoxCoded              = fields[9].Get<bool>();
        gMenuItem.BoxMoney              = fields[10].Get<uint32>();
        gMenuItem.BoxText               = fields[11].Get<std::string>();
        gMenuItem.BoxBroadcastTextID    = fields[12].Get<uint32>();

        if (gMenuItem.OptionIcon >= GOSSIP_ICON_MAX)
        {
            LOG_ERROR("sql.sql", "Table `{}` for menu {}, id {} has unknown icon id {}. Replacing with GOSSIP_ICON_CHAT",
                table, gMenuItem.MenuID, gMenuItem.OptionID, gMenuItem.OptionIcon);
            gMenuItem.OptionIcon = GOSSIP_ICON_CHAT;
        }

        if (gMenuItem.OptionBroadcastTextID && !GetBroadcastText(gMenuItem.OptionBroadcastTextID))
        {
            LOG_ERROR("sql.sql", "Table `{}` for menu {}, id {} has non-existing or incompatible OptionBroadcastTextID {}, ignoring.",
                table, gMenuItem.MenuID, gMenuItem.OptionID, gMenuItem.OptionBroadcastTextID);
            gMenuItem.OptionBroadcastTextID = 0;
        }

        if (gMenuItem.OptionType >= GOSSIP_OPTION_MAX)
            LOG_ERROR("sql.sql", "Table `{}` for menu {}, id {} has unknown option id {}. Option will not be used",
                table, gMenuItem.MenuID, gMenuItem.OptionID, gMenuItem.OptionType);

        if (gMenuItem.ActionPoiID && !GetPointOfInterest(gMenuItem.ActionPoiID))
        {
            LOG_ERROR("sql.sql", "Table `{}` for menu {}, id {} use non-existing ActionPoiID {}, ignoring",
                table, gMenuItem.MenuID, gMenuItem.OptionID, gMenuItem.ActionPoiID);
            gMenuItem.ActionPoiID = 0;
        }

        if (gMenuItem.BoxBroadcastTextID && !GetBroadcastText(gMenuItem.BoxBroadcastTextID))
        {
            LOG_ERROR("sql.sql", "Table `{}` for menu {}, id {} has non-existing or incompatible BoxBroadcastTextID {}, ignoring.",
                table, gMenuItem.MenuID, gMenuItem.OptionID, gMenuItem.BoxBroadcastTextID);
            gMenuItem.BoxBroadcastTextID = 0;
        }

        _gossipMenuItemsStore.insert(GossipMenuItemsContainer::value_type(gMenuItem.MenuID, gMenuItem));
    } while (result->NextRow());

    // Warn if any trainer creature templates reference a GossipMenuId that has no world_gossip_menu_option entries.
    // This will cause the gossip menu to fall back to MenuID 0 at runtime which will display: "I wish to unlearn my talents."
    std::set<uint32> checkedMenuIds;
    for (const auto &tmpl: _creatureTemplateStore | std::views::values)
    {
        uint32 menuID = tmpl.GossipMenuId;
        if (!menuID)
            continue;

        if (!(tmpl.FlagNPC & UNIT_NPC_FLAG_TRAINER))
            continue;

        if (checkedMenuIds.contains(menuID))
            continue;

        checkedMenuIds.insert(menuID);

        if (auto [first, second] = _gossipMenuItemsStore.equal_range(menuID); first == second)
            LOG_WARN("server.loading", "Trainer creature template references GossipMenuID {} has no `{}` entries. This will fallback to MenuID 0.", menuID, table);
    }

    LOG_INFO("server.loading", ">> Loaded {} `{}` entries in {} ms", static_cast<uint32>(_gossipMenuItemsStore.size()), table, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::AddVendorItem(const uint32 entry, const uint32 item, const uint32 maxCount, const uint32 incrTime, const uint32 extendedCost, const bool persist /*= true*/)
{
    VendorItemData& vList = _cacheVendorItemStore[entry];
    vList.AddItem(item, maxCount, incrTime, extendedCost);

    if (persist)
    {
        WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_INS_NPC_VENDOR);
        stmt->SetData(0, entry);
        stmt->SetData(1, item);
        stmt->SetData(2, maxCount);
        stmt->SetData(3, incrTime);
        stmt->SetData(4, extendedCost);
        WorldDatabase.Execute(stmt);
    }
}

bool ObjectMgr::RemoveVendorItem(const uint32 entry, const uint32 item, const bool persist /*= true*/)
{
    const auto iter = _cacheVendorItemStore.find(entry);
    if (iter == _cacheVendorItemStore.end())
        return false;

    if (!iter->second.RemoveItem(item))
        return false;

    if (persist)
    {
        WorldDatabasePreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_NPC_VENDOR);
        stmt->SetData(0, entry);
        stmt->SetData(1, item);
        WorldDatabase.Execute(stmt);
    }

    return true;
}

bool ObjectMgr::IsVendorItemValid(const uint32 vendorEntry, const uint32 itemID, const uint32 maxCount, const uint32 incrTime,
    const uint32 extendedCost, const Player* player, std::set<uint32>* /*skip_vendors*/, uint32 /*npcFlag*/) const
{
    if (!sObjectMgr->GetItemTemplate(itemID))
    {
        if (player)
            ChatHandler(player->GetSession()).PSendSysMessage(LANG_ITEM_NOT_FOUND, itemID);
        else
            LOG_ERROR("sql.sql", "Table `world_(game_event_)npc_vendor` for Vendor (Entry: {}) have in item list non-existed item ({}), ignore", vendorEntry, itemID);
        return false;
    }

    if (extendedCost && !sItemExtendedCostStore.LookupEntry(extendedCost))
    {
        if (player)
            ChatHandler(player->GetSession()).PSendSysMessage(LANG_EXTENDED_COST_NOT_EXIST, extendedCost);
        else
            LOG_ERROR("sql.sql", "Table `world_(game_event_)npc_vendor` have Item (Entry: {}) with wrong ExtendedCost ({}) for vendor ({}), ignore", itemID, extendedCost, vendorEntry);
        return false;
    }

    if (maxCount > 0 && incrTime == 0)
    {
        if (player)
            ChatHandler(player->GetSession()).PSendSysMessage("MaxCount != 0 ({}) but IncrTime == 0", maxCount);
        else
            LOG_ERROR("sql.sql", "Table `world_(game_event_)npc_vendor` has `max_count` ({}) for item {} of vendor (Entry: {}) but `incr_time`=0, ignore", maxCount, itemID, vendorEntry);
        return false;
    }
    if (maxCount == 0 && incrTime > 0)
    {
        if (player)
            ChatHandler(player->GetSession()).PSendSysMessage("MaxCount == 0 but IncrTime<>= 0");
        else
            LOG_ERROR("sql.sql", "Table `world_(game_event_)npc_vendor` has `max_count`=0 for item {} of vendor (Entry: {}) but `incr_time`<>0, ignore", itemID, vendorEntry);
        return false;
    }

    const VendorItemData* vItems = GetNpcVendorItemList(vendorEntry);
    if (!vItems)
        return true;  // Later checks for non-empty lists

    if (vItems->FindItemCostPair(itemID, extendedCost))
    {
        if (player)
            ChatHandler(player->GetSession()).PSendSysMessage(LANG_ITEM_ALREADY_IN_LIST, itemID, extendedCost);
        else
            LOG_ERROR("sql.sql", "Table `world_npc_vendor` has duplicate items {} (with extended cost {}) for vendor (Entry: {}), ignoring", itemID, extendedCost, vendorEntry);
        return false;
    }

    return true;
}

void ObjectMgr::LoadScriptNames()
{
    const uint32 oldMSTime = getMSTime();

    // We insert an empty placeholder here so we can use the script id 0 as dummy for "no script found".
    _scriptNamesStore.emplace_back("");

    const QueryResult result = WorldDatabase.Query(
        "SELECT DISTINCT script_name FROM world_achievement_criteria_data WHERE script_name <> '' AND type = 11 "
        "UNION SELECT DISTINCT script_name FROM world_battleground_template WHERE script_name <> '' "
        "UNION SELECT DISTINCT script_name FROM world_creature WHERE script_name <> '' "
        "UNION SELECT DISTINCT script_name FROM world_creature_template WHERE script_name <> '' "
        "UNION SELECT DISTINCT script_name FROM world_game_object WHERE script_name <> '' "
        "UNION SELECT DISTINCT script_name FROM world_game_object_template WHERE script_name <> '' "
        "UNION SELECT DISTINCT script_name FROM world_item_template WHERE script_name <> '' "
        "UNION SELECT DISTINCT script_name FROM world_area_trigger_script WHERE script_name <> '' "
        "UNION SELECT DISTINCT script_name FROM world_spell_script_name WHERE script_name <> '' "
        "UNION SELECT DISTINCT script_name FROM world_transport WHERE script_name <> '' "
        "UNION SELECT DISTINCT script_name FROM world_game_weather WHERE script_name <> '' "
        "UNION SELECT DISTINCT script_name FROM world_condition WHERE script_name <> '' "
        "UNION SELECT DISTINCT script_name FROM world_outdoor_pvp_template WHERE script_name <> '' "
        "UNION SELECT DISTINCT script FROM world_instance_template WHERE script <> ''");

    if (!result)
    {
        LOG_INFO("server.loading", " ");
        LOG_ERROR("sql.sql", ">> Loaded empty set of Script Names!");
        return;
    }

    _scriptNamesStore.reserve(result->GetRowCount() + 1);

    do
    {
        _scriptNamesStore.push_back((*result)[0].Get<std::string>());
    } while (result->NextRow());

    std::ranges::sort(_scriptNamesStore);
    LOG_INFO("server.loading", ">> Loaded {} ScriptNames in {} ms", _scriptNamesStore.size(), GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

const std::string& ObjectMgr::GetScriptName(const uint32 id) const
{
    static std::string const empty;
    return id < _scriptNamesStore.size() ? _scriptNamesStore[id] : empty;
}

uint32 ObjectMgr::GetScriptID(const std::string& name)
{
    // Use binary search to find the script name in the sorted vector.
    // Assume "" is the first element.
    if (name.empty())
        return 0;

    const auto itr = std::ranges::lower_bound(_scriptNamesStore, name);
    if (itr == _scriptNamesStore.end() || *itr != name)
        return 0;

    return static_cast<uint32>(itr - _scriptNamesStore.begin());
}

void ObjectMgr::LoadBroadcastTexts()
{
    const uint32 oldMSTime = getMSTime();

    _broadcastTextStore.clear(); // For reload case

    const QueryResult result = WorldDatabase.Query("SELECT id, language, text_male, text_female FROM world_broadcast_text");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 broadcast texts. DB table `world_broadcast_text` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    _broadcastTextStore.rehash(result->GetRowCount());

    do
    {
        const Field* fields = result->Fetch();

        BroadcastText bct;
        bct.Id = fields[0].Get<uint32>();
        bct.LanguageID = fields[1].Get<uint32>();
        bct.MaleText = fields[2].Get<std::string>();
        bct.FemaleText = fields[3].Get<std::string>();

        if (!GetLanguageDescByID(bct.LanguageID))
        {
            LOG_DEBUG("misc", "BroadcastText (ID: {}) in table `world_broadcast_text` using Language {} but Language does not exist.", bct.Id, bct.LanguageID);
            bct.LanguageID = LANG_UNIVERSAL;
        }

        _broadcastTextStore[bct.Id] = bct;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Broadcast Texts in {} ms", _broadcastTextStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

const CreatureBaseStats* ObjectMgr::GetCreatureBaseStats(const uint8 level, const uint8 unitClass)
{
    if (const auto it = _creatureBaseStatsStore.find(MAKE_PAIR16(level, unitClass)); it != _creatureBaseStatsStore.end())
        return &it->second;

    struct DefaultCreatureBaseStats : CreatureBaseStats
    {
        DefaultCreatureBaseStats() : CreatureBaseStats()
        {
            BaseArmor = 1;
            for (uint8 j = 0; j < MAX_EXPANSIONS; ++j)
            {
                BaseHealth[j] = 1;
                BaseDamage[j] = 0.0f;
            }
            BaseMana = 0;
            AttackPower = 0;
            RangedAttackPower = 0;
            Strength = 0;
            Agility = 0;
            Stamina = 0;
            Intellect = 0;
            Spirit = 0;
        }
    };
    static const DefaultCreatureBaseStats defStats;
    return &defStats;
}

void ObjectMgr::LoadCreatureClassLevelStats()
{
    const uint32 oldMSTime = getMSTime();

    const QueryResult result = WorldDatabase.Query(
        "SELECT level, class, health, mana, armor, attack_power, ranged_attack_power, damage, strength, agility, stamina, intellect, spirit "
        "FROM world_creature_class_level_stats");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creature base stats. DB table `world_creature_class_level_stats` is empty.");
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        uint8 Level = fields[0].Get<uint8>();
        uint8 Class = fields[1].Get<uint8>();

        if (!Class || ((1 << (Class - 1)) & CLASS_MASK_ALL_CREATURES) == 0)
            LOG_ERROR("sql.sql", "Creature base stats for level {} has invalid class {}", Level, Class);

        CreatureBaseStats stats{};
        const auto baseHealth = fields[2].GetArray<uint32, MAX_EXPANSIONS>();
        const auto baseDamage = fields[7].GetArray<float, MAX_EXPANSIONS>();

        for (uint8 i = 0; i < MAX_EXPANSIONS; ++i)
        {
            stats.BaseHealth[i] = baseHealth[i];

            if (stats.BaseHealth[i] == 0)
            {
                LOG_ERROR("sql.sql", "Creature base stats for class {}, level {} has invalid zero base HP[{}] - set to 1", Class, Level, i);
                stats.BaseHealth[i] = 1;
            }

            // If no data is available, get them from lower expansions
            if (stats.BaseHealth[i] <= 1)
            {
                for (uint8 j = i; j > 0;)
                {
                    --j;
                    if (stats.BaseHealth[j] > 1)
                    {
                        stats.BaseHealth[i] = stats.BaseHealth[j];
                        break;
                    }
                }
            }

            stats.BaseDamage[i] = baseDamage[i];
            if (stats.BaseDamage[i] < 0.0f)
            {
                LOG_ERROR("sql.sql", "Creature base stats for class {}, level {} has invalid negative base damage[{}] - set to 0.0", Class, Level, i);
                stats.BaseDamage[i] = 0.0f;
            }
        }

        stats.BaseMana = fields[3].Get<uint32>();
        stats.BaseArmor = static_cast<float>(fields[4].Get<uint32>());

        stats.AttackPower = fields[5].Get<uint32>();
        stats.RangedAttackPower = fields[6].Get<uint32>();

        stats.Strength = fields[8].Get<uint32>();
        stats.Agility = fields[9].Get<uint32>();
        stats.Stamina = fields[10].Get<uint32>();
        stats.Intellect = fields[11].Get<uint32>();
        stats.Spirit = fields[12].Get<uint32>();

        if (!stats.Strength || !stats.Agility || !stats.Stamina || !stats.Intellect || !stats.Spirit)
        {
            // Once these attributes are implemented, this should probably be uncommented.
            // LOG_WARN("server.loading", "Creature base attributes for class {}, level {} are missing!", Class, Level);
        }

        _creatureBaseStatsStore[MAKE_PAIR16(Level, Class)] = stats;

        ++count;
    } while (result->NextRow());

    const CreatureTemplateContainer* ctc = GetCreatureTemplates();
    for (auto itr = ctc->begin(); itr != ctc->end(); ++itr)
    {
        for (uint16 lvl = itr->second.LevelMin; lvl <= itr->second.MaxLevel; ++lvl)
        {
            if (!_creatureBaseStatsStore.contains(MAKE_PAIR16(lvl, itr->second.UnitClass)))
                LOG_ERROR("sql.sql", "Missing base stats for creature class {} level {}", itr->second.UnitClass, lvl);
        }
    }

    LOG_INFO("server.loading", ">> Loaded {} Creature Base Stats in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadFactionChangeAchievements()
{
    const uint32 oldMSTime = getMSTime();

    const auto table = "world_player_faction_change_achievement";
    const QueryResult result = WorldDatabase.Query("SELECT entry_a, entry_h FROM world_player_faction_change_achievement");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 faction change achievement pairs. DB table `{}` is empty.", table);
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        uint32 alliance = fields[0].Get<uint32>();
        uint32 horde = fields[1].Get<uint32>();

        if (!sAchievementStore.LookupEntry(alliance))
            LOG_ERROR("sql.sql", "Achievement {} (alliance_id) referenced in `{}` does not exist, pair skipped!", alliance, table);
        else if (!sAchievementStore.LookupEntry(horde))
            LOG_ERROR("sql.sql", "Achievement {} (horde_id) referenced in `{}` does not exist, pair skipped!", horde, table);
        else
            FactionChangeAchievements[alliance] = horde;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} faction change achievement pairs in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadFactionChangeItems()
{
    const uint32 oldMSTime = getMSTime();

    const auto table = "world_player_faction_change_item";
    const QueryResult result = WorldDatabase.Query("SELECT entry_a, entry_h FROM world_player_faction_change_item");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 faction change item pairs. DB table `{}` is empty.", table);
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();

        uint32 alliance = fields[0].Get<uint32>();
        uint32 horde = fields[1].Get<uint32>();

        if (!GetItemTemplate(alliance))
            LOG_ERROR("sql.sql", "Item {} (alliance_id) referenced in `{}` does not exist, pair skipped!", alliance, table);
        else if (!GetItemTemplate(horde))
            LOG_ERROR("sql.sql", "Item {} (horde_id) referenced in `{}` does not exist, pair skipped!", horde, table);
        else
            FactionChangeItems[alliance] = horde;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} faction change item pairs in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadFactionChangeQuests()
{
    const uint32 oldMSTime = getMSTime();

    const auto table = "world_player_faction_change_quest";
    const QueryResult result = WorldDatabase.Query("SELECT entry_a, entry_h FROM world_player_faction_change_quest");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 faction change quest pairs. DB table `{}` is empty.", table);
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();

        uint32 alliance = fields[0].Get<uint32>();
        uint32 horde = fields[1].Get<uint32>();

        if (!GetQuestTemplate(alliance))
            LOG_ERROR("sql.sql", "Quest {} (alliance_id) referenced in `{}` does not exist, pair skipped!", alliance, table);
        else if (!GetQuestTemplate(horde))
            LOG_ERROR("sql.sql", "Quest {} (horde_id) referenced in `{}` does not exist, pair skipped!", horde, table);
        else
            FactionChangeQuests[alliance] = horde;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} faction change quest pairs in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadFactionChangeReputations()
{
    const uint32 oldMSTime = getMSTime();

    const auto table = "world_player_faction_change_reputation";
    const QueryResult result = WorldDatabase.Query("SELECT entry_a, entry_h FROM world_player_faction_change_reputation");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 faction change reputation pairs. DB table `{}` is empty.", table);
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();

        uint32 alliance = fields[0].Get<uint32>();
        uint32 horde = fields[1].Get<uint32>();

        if (!sFactionStore.LookupEntry(alliance))
            LOG_ERROR("sql.sql", "Reputation {} (alliance_id) referenced in `{}` does not exist, pair skipped!", alliance, table);
        else if (!sFactionStore.LookupEntry(horde))
            LOG_ERROR("sql.sql", "Reputation {} (horde_id) referenced in `` does not exist, pair skipped!", horde, table);
        else
            FactionChangeReputation[alliance] = horde;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} faction change reputation pairs in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadFactionChangeSpells()
{
    const uint32 oldMSTime = getMSTime();

    const auto table = "world_player_faction_change_spell";
    const QueryResult result = WorldDatabase.Query("SELECT entry_a, entry_h FROM world_player_faction_change_spell");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 faction change spell pairs. DB table `{}` is empty.", table);
        LOG_INFO("server.loading", " ");
        return;
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();

        uint32 alliance = fields[0].Get<uint32>();
        uint32 horde = fields[1].Get<uint32>();

        if (!sSpellMgr->GetSpellInfo(alliance))
            LOG_ERROR("sql.sql", "Spell {} (alliance_id) referenced in `{}` does not exist, pair skipped!", alliance, table);
        else if (!sSpellMgr->GetSpellInfo(horde))
            LOG_ERROR("sql.sql", "Spell {} (horde_id) referenced in `{}` does not exist, pair skipped!", horde, table);
        else
            FactionChangeSpells[alliance] = horde;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} faction change spell pairs in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

void ObjectMgr::LoadFactionChangeTitles()
{
    const uint32 oldMSTime = getMSTime();

    const QueryResult result = WorldDatabase.Query("SELECT entry_a, entry_h FROM world_player_faction_change_title");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 faction change title pairs. DB table `world_player_faction_change_title` is empty.");
        return;
    }

    uint32 count = 0;

    do
    {
        const Field* fields = result->Fetch();

        uint32 alliance = fields[0].Get<uint32>();
        uint32 horde = fields[1].Get<uint32>();

        if (!sCharTitlesStore.LookupEntry(alliance))
            LOG_ERROR("sql.sql", "Title {} (alliance_id) referenced in `world_player_faction_change_title` does not exist, pair skipped!", alliance);
        else if (!sCharTitlesStore.LookupEntry(horde))
            LOG_ERROR("sql.sql", "Title {} (horde_id) referenced in `world_player_faction_change_title` does not exist, pair skipped!", horde);
        else
            FactionChangeTitles[alliance] = horde;

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} faction change title pairs in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

const GameObjectTemplate* ObjectMgr::GetGameObjectTemplate(const uint32 entry)
{
    if (const auto itr = _gameObjectTemplateStore.find(entry); itr != _gameObjectTemplateStore.end())
        return &itr->second;
    return nullptr;
}

bool ObjectMgr::IsGameObjectStaticTransport(const uint32 entry)
{
    const GameObjectTemplate* goInfo = GetGameObjectTemplate(entry);
    return goInfo && goInfo->Type == GAME_OBJECT_TYPE_TRANSPORT;
}

const GameObjectTemplateAddon* ObjectMgr::GetGameObjectTemplateAddon(const uint32 entry) const
{
    const auto itr = _gameObjectTemplateAddonStore.find(entry);
    if (itr != _gameObjectTemplateAddonStore.end())
        return &itr->second;
    return nullptr;
}

const CreatureTemplate* ObjectMgr::GetCreatureTemplate(const uint32 entry) const {
    return entry < _creatureTemplateStoreFast.size() ? _creatureTemplateStoreFast[entry] : nullptr;
}

const VehicleAccessoryList* ObjectMgr::GetVehicleAccessoryList(const Vehicle* veh) const
{
    if (const Creature* cre = veh->GetBase()->ToCreature())
    {
        // Give preference to GUID-based accessories
        const auto itr = _vehicleAccessoryStore.find(cre->GetSpawnId());
        if (itr != _vehicleAccessoryStore.end())
            return &itr->second;
    }

    // Otherwise return entry-based
    const auto itr = _vehicleTemplateAccessoryStore.find(veh->GetCreatureEntry());
    if (itr != _vehicleTemplateAccessoryStore.end())
        return &itr->second;
    return nullptr;
}

const PlayerInfo* ObjectMgr::GetPlayerInfo(const uint32 race, const uint32 class_) const
{
    if (race >= sRaceMgr->GetMaxRaces())
        return nullptr;
    if (class_ >= MAX_CLASSES)
        return nullptr;
    const PlayerInfo* info = _playerInfo[race][class_];
    if (!info)
        return nullptr;
    return info;
}

void ObjectMgr::LoadGameObjectQuestItems()
{
    const uint32 oldMSTime = getMSTime();

    const QueryResult result = WorldDatabase.Query("SELECT entry, item, id FROM world_game_object_quest_item ORDER BY id ASC");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 GameObject quest items. DB table `world_game_object_quest_item` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        uint32 entry = fields[0].Get<uint32>();
        uint32 item = fields[1].Get<uint32>();
        uint32 idx = fields[2].Get<uint32>();

        if (!GetGameObjectTemplate(entry))
        {
            LOG_ERROR("sql.sql", "Table `world_game_object_quest_item` has data for nonexistent GameObject (entry: {}, idx: {}), skipped", entry, idx);
            continue;
        }

        if (!sItemStore.LookupEntry(item))
        {
            LOG_ERROR("sql.sql", "Table `world_game_object_quest_item` has nonexistent item (ID: {}) in GameObject (entry: {}, idx: {}), skipped", item, entry, idx);
            continue;
        }

        _gameObjectQuestItemStore[entry].push_back(item);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} GameObject Quest Items in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadCreatureQuestItems()
{
    const uint32 oldMSTime = getMSTime();

    const QueryResult result = WorldDatabase.Query("SELECT creature, item, id FROM world_creature_quest_item ORDER BY id ASC");

    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 creature quest items. DB table `world_creature_quest_item` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();

        uint32 entry = fields[0].Get<uint32>();
        uint32 item = fields[1].Get<uint32>();
        uint32 idx = fields[2].Get<uint32>();

        if (!GetCreatureTemplate(entry))
        {
            LOG_ERROR("sql.sql", "Table `world_creature_quest_item` has data for nonexistent creature (entry: {}, idx: {}), skipped", entry, idx);
            continue;
        }
        if (!sItemStore.LookupEntry(item))
        {
            LOG_ERROR("sql.sql", "Table `world_creature_quest_item` has nonexistent item (ID: {}) in creature (entry: {}, idx: {}), skipped", item, entry, idx);
            continue;
        };

        _creatureQuestItemStore[entry].push_back(item);

        ++count;
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Creature Quest Items in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadQuestMoneyRewards()
{
    const uint32 oldMSTime = getMSTime();

    _questMoneyRewards.clear();

    const QueryResult result = WorldDatabase.Query("SELECT level, money FROM world_quest_money_reward ORDER BY level");
    if (!result)
    {
        LOG_WARN("server.loading", ">> Loaded 0 quest money rewards. DB table `world_quest_money_reward` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        const Field* fields = result->Fetch();
        uint32 Level = fields[0].Get<uint32>();
        const auto money = fields[1].GetArray<uint32, MAX_QUEST_MONEY_REWARDS>();

        QuestMoneyRewardArray& questMoneyReward = _questMoneyRewards[Level];
        for (uint8 i = 0; i < MAX_QUEST_MONEY_REWARDS; ++i)
        {
            questMoneyReward[i] = money[i];
            ++count;
        }
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} Quest Money Rewards in {} ms", count, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

uint32 ObjectMgr::GetQuestMoneyReward(const uint8 level, const uint32 questMoneyDifficulty) const
{
    if (questMoneyDifficulty < MAX_QUEST_MONEY_REWARDS)
        if (const auto& itr = _questMoneyRewards.find(level); itr != _questMoneyRewards.end())
            return itr->second.at(questMoneyDifficulty);
    return 0;
}
