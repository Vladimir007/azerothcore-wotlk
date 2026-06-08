/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/** \file
    \ingroup world
*/

#include "World.h"
#include "AccountMgr.h"
#include "AchievementMgr.h"
#include "AddonMgr.h"
#include "ArenaTeamMgr.h"
#include "ArenaSeasonMgr.h"
#include "AuctionHouseMgr.h"
#include "BattlefieldMgr.h"
#include "BattlegroundMgr.h"
#include "CalendarMgr.h"
#include "Channel.h"
#include "ChannelMgr.h"
#include "CharacterDatabaseCleaner.h"
#include "Chat.h"
#include "ChatPackets.h"
#include "Common.h"
#include "ConditionMgr.h"
#include "Config.h"
#include "CreatureAIRegistry.h"
#include "CreatureGroups.h"
#include "CreatureTextMgr.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "DisableMgr.h"
#include "DynamicVisibility.h"
#include "GameEventMgr.h"
#include "GameGraveyard.h"
#include "GameTime.h"
#include "GridNotifiersImpl.h"
#include "GroupMgr.h"
#include "GuildMgr.h"
#include "InstanceSaveMgr.h"
#include "ItemEnchantmentMgr.h"
#include "LFGMgr.h"
#include "Log.h"
#include "LootItemStorage.h"
#include "LootMgr.h"
#include "M2Stores.h"
#include "MapMgr.h"
#include "MotdMgr.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "OutdoorPvPMgr.h"
#include "PetitionMgr.h"
#include "Player.h"
#include "PoolMgr.h"
#include "RaceMgr.h"
#include "Realm.h"
#include "ScriptMgr.h"
#include "ServerMailMgr.h"
#include "SkillDiscovery.h"
#include "SkillExtraItems.h"
#include "SmartAI.h"
#include "SpellMgr.h"
#include "TaskScheduler.h"
#include "TicketMgr.h"
#include "Transport.h"
#include "TransportMgr.h"
#include "UpdateTime.h"
#include "Util.h"
#include "VMapFactory.h"
#include "VMapMgr.h"
#include "WaypointMovementGenerator.h"
#include "WeatherMgr.h"
#include "WhoListCacheMgr.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "WorldSessionMgr.h"
#include "WorldState.h"
#include "WorldStateDefines.h"
#include <boost/asio/ip/address.hpp>
#include <cmath>

std::atomic_long World::_stopEvent = false;
uint8 World::_exitCode = SHUTDOWN_EXIT_CODE;
uint32 World::m_worldLoopCounter = 0;

float World::_maxVisibleDistanceOnContinents = DEFAULT_VISIBILITY_DISTANCE;
float World::_maxVisibleDistanceInInstances  = DEFAULT_VISIBILITY_INSTANCE;
float World::_maxVisibleDistanceInBGArenas   = DEFAULT_VISIBILITY_BGARENAS;

Realm realm;

/// World constructor
World::World()
{
    _allowMovement = true;
    _shutdownMask = 0;
    _shutdownTimer = 0;
    _nextDailyQuestReset = 0s;
    _nextWeeklyQuestReset = 0s;
    _nextMonthlyQuestReset = 0s;
    _nextRandomBGReset = 0s;
    _nextCalendarOldEventsDeletionTime = 0s;
    _nextGuildReset = 0s;
    _defaultDbcLocale = LOCALE_enUS;
    _mail_expire_check_timer = 0s;
    _isClosed = false;
    _cleaningFlags = 0;
}

/// World destructor
World::~World()
{
    VMAP::VMapFactory::clear();
}

std::unique_ptr<IWorld>& getWorldInstance()
{
    static std::unique_ptr<IWorld> instance = std::make_unique<World>();
    return instance;
}

bool World::IsClosed() const
{
    return _isClosed;
}

void World::SetClosed(bool val)
{
    _isClosed = val;

    // Invert the value, for simplicity for scripters.
    sScriptMgr->OnOpenStateChange(!val);
}

/// Initialize config values
void World::LoadConfigSettings()
{
    sScriptMgr->OnBeforeConfigLoad();

    // load update time related configs
    sWorldUpdateTime.LoadFromConfig();

    ///- Read the player limit and the Message of the day from the config file
    sWorldSessionMgr->SetPlayerAmountLimit(sConfigMgr->GetOption<int32>("PlayerLimit", 1000));

    _worldConfig.Initialize();

    for (uint8 i = 0; i < MAX_MOVE_TYPE; ++i)
        playerBaseMoveSpeed[i] = baseMoveSpeed[i];

    //visibility on continents
    _maxVisibleDistanceOnContinents = sConfigMgr->GetOption<float>("Visibility.Distance.Continents", DEFAULT_VISIBILITY_DISTANCE);
    if (_maxVisibleDistanceOnContinents < 45 * getRate(RATE_CREATURE_AGGRO))
    {
        LOG_ERROR("server.loading", "Visibility.Distance.Continents can't be less max aggro radius {}", 45 * getRate(RATE_CREATURE_AGGRO));
        _maxVisibleDistanceOnContinents = 45 * getRate(RATE_CREATURE_AGGRO);
    }
    else if (_maxVisibleDistanceOnContinents > MAX_VISIBILITY_DISTANCE)
    {
        LOG_ERROR("server.loading", "Visibility.Distance.Continents can't be greater {}", MAX_VISIBILITY_DISTANCE);
        _maxVisibleDistanceOnContinents = MAX_VISIBILITY_DISTANCE;
    }

    //visibility in instances
    _maxVisibleDistanceInInstances = sConfigMgr->GetOption<float>("Visibility.Distance.Instances", DEFAULT_VISIBILITY_INSTANCE);
    if (_maxVisibleDistanceInInstances < 45 * getRate(RATE_CREATURE_AGGRO))
    {
        LOG_ERROR("server.loading", "Visibility.Distance.Instances can't be less max aggro radius {}", 45 * getRate(RATE_CREATURE_AGGRO));
        _maxVisibleDistanceInInstances = 45 * getRate(RATE_CREATURE_AGGRO);
    }
    else if (_maxVisibleDistanceInInstances > MAX_VISIBILITY_DISTANCE)
    {
        LOG_ERROR("server.loading", "Visibility.Distance.Instances can't be greater {}", MAX_VISIBILITY_DISTANCE);
        _maxVisibleDistanceInInstances = MAX_VISIBILITY_DISTANCE;
    }

    //visibility in BG/Arenas
    _maxVisibleDistanceInBGArenas = sConfigMgr->GetOption<float>("Visibility.Distance.BGArenas", DEFAULT_VISIBILITY_BGARENAS);
    if (_maxVisibleDistanceInBGArenas < 45 * getRate(RATE_CREATURE_AGGRO))
    {
        LOG_ERROR("server.loading", "Visibility.Distance.BGArenas can't be less max aggro radius {}", 45 * getRate(RATE_CREATURE_AGGRO));
        _maxVisibleDistanceInBGArenas = 45 * getRate(RATE_CREATURE_AGGRO);
    }
    else if (_maxVisibleDistanceInBGArenas > MAX_VISIBILITY_DISTANCE)
    {
        LOG_ERROR("server.loading", "Visibility.Distance.BGArenas can't be greater {}", MAX_VISIBILITY_DISTANCE);
        _maxVisibleDistanceInBGArenas = MAX_VISIBILITY_DISTANCE;
    }

    LOG_INFO("server.loading", "Will clear `logs` table of entries older than {} seconds every {} minutes.",
        getIntConfig(CONFIG_LOGDB_CLEARTIME), getIntConfig(CONFIG_LOGDB_CLEARINTERVAL));

    ///- Read the "Data" directory from the config file
    std::string dataPath = sConfigMgr->GetOption<std::string>("DataDir", "./");
    if (dataPath.empty() || (dataPath.at(dataPath.length() - 1) != '/' && dataPath.at(dataPath.length() - 1) != '\\'))
        dataPath.push_back('/');

    if (dataPath[0] == '~')
    {
        const char* home = getenv("HOME");
        if (home)
            dataPath.replace(0, 1, home);
    }

    _dataPath = dataPath;
    LOG_INFO("server.loading", "Using DataDir {}", _dataPath);

    // call ScriptMgr if we're reloading the configuration
    sScriptMgr->OnAfterConfigLoad();
}

/// Initialize the World
void World::SetInitialWorldSettings()
{
    ///- Server startup begin
    uint32 startupBegin = getMSTime();

    ///- Initialize the random number generator
    srand((unsigned int)GameTime::GetGameTime().count());

    ///- Initialize detour memory management
    dtAllocSetCustom(dtCustomAlloc, dtCustomFree);

    ///- Initialize VMapMgr function pointers (to untangle game/collision circular deps)
    VMAP::VMapMgr* vmmgr2 = VMAP::VMapFactory::createOrGetVMapMgr();
    vmmgr2->GetLiquidFlagsPtr = &GetLiquidFlags;
    vmmgr2->IsVMAPDisabledForPtr = &DisableMgr::IsVMAPDisabledFor;

    ///- Initialize config settings
    LoadConfigSettings();

    ///- Init highest guids before any table loading to prevent using not initialized guids in some code.
    sObjectMgr->SetHighestGUIDs();

    if (!sConfigMgr->isDryRun())
    {
        ///- Check the existence of the map files for all starting areas.
        if (!MapMgr::ExistMapAndVMap(MAP_EASTERN_KINGDOMS, -6240.32f, 331.033f)
                || !MapMgr::ExistMapAndVMap(MAP_EASTERN_KINGDOMS, -8949.95f, -132.493f)
                || !MapMgr::ExistMapAndVMap(MAP_KALIMDOR, -618.518f, -4251.67f)
                || !MapMgr::ExistMapAndVMap(MAP_EASTERN_KINGDOMS, 1676.35f, 1677.45f)
                || !MapMgr::ExistMapAndVMap(MAP_KALIMDOR, 10311.3f, 832.463f)
                || !MapMgr::ExistMapAndVMap(MAP_KALIMDOR, -2917.58f, -257.98f)
                || !MapMgr::ExistMapAndVMap(MAP_OUTLAND, 10349.6f, -6357.29f)
                || !MapMgr::ExistMapAndVMap(MAP_OUTLAND, -3961.64f, -13931.2f))
        {
            LOG_ERROR("server.loading", "Failed to find map files for starting areas");
            exit(1);
        }
    }

    ///- Initialize pool manager
    sPoolMgr->Initialize();

    ///- Initialize game event manager
    sGameEventMgr->Initialize();

    ///- Loading strings. Getting no records means core load has to be canceled because no error message can be output.
    LOG_INFO("server.loading", " ");
    LOG_INFO("server.loading", "Loading Acore Strings...");
    if (!sObjectMgr->LoadAcoreStrings())
        exit(1);                                            // Error message displayed in function already

    ///- Custom Hook for loading DB items
    sScriptMgr->OnLoadCustomDatabaseTable();

    ///- Load the DBC files
    LOG_INFO("server.loading", "Initialize Data Stores...");
    LoadDBCStores();
    DetectDBCLang();

    // Load cinematic cameras
    LoadM2Cameras(_dataPath);

    LOG_INFO("server.loading", "Loading Player race data...");
    sRaceMgr->LoadRaces();

    LOG_INFO("server.loading", "Loading Game Graveyard...");
    sGraveyard->LoadGraveyardFromDB();

    ///- Initilize static helper structures
    AIRegistry::Initialize();

    LOG_INFO("server.loading", "Loading SpellInfo Store...");
    sSpellMgr->LoadSpellInfoStore();

    LOG_INFO("server.loading", "Loading Spell Cooldown Overrides...");
    sSpellMgr->LoadSpellCooldownOverrides();

    LOG_INFO("server.loading", "Loading SpellInfo Data Corrections...");
    sSpellMgr->LoadSpellInfoCorrections();

    LOG_INFO("server.loading", "Loading Spell Rank Data...");
    sSpellMgr->LoadSpellRanks();

    LOG_INFO("server.loading", "Loading Spell Specific And Aura State...");
    sSpellMgr->LoadSpellSpecificAndAuraState();

    LOG_INFO("server.loading", "Loading SkillLineAbilityMultiMap Data...");
    sSpellMgr->LoadSkillLineAbilityMap();

    LOG_INFO("server.loading", "Loading SpellInfo Custom Attributes...");
    sSpellMgr->LoadSpellInfoCustomAttributes();

    LOG_INFO("server.loading", "Loading Spell Jump Distances...");
    sSpellMgr->LoadSpellJumpDistances();

    LOG_INFO("server.loading", "Loading SpellInfo Immunity infos...");
    sSpellMgr->LoadSpellInfoImmunities();

    LOG_INFO("server.loading", "Loading Player Totem models...");
    sObjectMgr->LoadPlayerTotemModels();

    LOG_INFO("server.loading", "Loading Player Shapeshift models...");
    sObjectMgr->LoadPlayerShapeshiftModels();

    LOG_INFO("server.loading", "Loading GameObject Models...");
    LoadGameObjectModelList(_dataPath);

    LOG_INFO("server.loading", "Loading Script Names...");
    sObjectMgr->LoadScriptNames();

    LOG_INFO("server.loading", "Loading Instance Template...");
    sObjectMgr->LoadInstanceTemplate();

    LOG_INFO("server.loading", "Loading Character Cache...");
    sCharacterCache->LoadCharacterCacheStorage();

    // Must be called before `creature_respawn`/`gameobject_respawn` tables
    LOG_INFO("server.loading", "Loading Instances...");
    sInstanceSaveMgr->LoadInstances();

    LOG_INFO("server.loading", "Loading Broadcast Texts...");
    sObjectMgr->LoadBroadcastTexts();

    LOG_INFO("server.loading", "Loading Localization Strings...");
    uint32 oldMSTime = getMSTime();;

    sObjectMgr->SetDBCLocaleIndex(GetDefaultDbcLocale());        // Get once for all the locale index of DBC language (console/broadcasts)
    LOG_INFO("server.loading", ">> Localization Strings loaded in {} ms", GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");

    LOG_INFO("server.loading", "Loading Page Texts...");
    sObjectMgr->LoadPageTexts();

    LOG_INFO("server.loading", "Loading Game Object Templates...");         // must be after LoadPageTexts
    sObjectMgr->LoadGameObjectTemplate();

    LOG_INFO("server.loading", "Loading Game Object Template Addons...");
    sObjectMgr->LoadGameObjectTemplateAddons();

    LOG_INFO("server.loading", "Loading Transport Templates...");
    sTransportMgr->LoadTransportTemplates();

    LOG_INFO("server.loading", "Loading Spell Required Data...");
    sSpellMgr->LoadSpellRequired();

    LOG_INFO("server.loading", "Loading Spell Group Types...");
    sSpellMgr->LoadSpellGroups();

    LOG_INFO("server.loading", "Loading Spell Learn Skills...");
    sSpellMgr->LoadSpellLearnSkills();                           // must be after LoadSpellRanks

    LOG_INFO("server.loading", "Loading Spell Proc Conditions and Data...");
    sSpellMgr->LoadSpellProcs();

    LOG_INFO("server.loading", "Loading Spell Bonus Data...");
    sSpellMgr->LoadSpellBonuses();

    LOG_INFO("server.loading", "Loading Aggro Spells Definitions...");
    sSpellMgr->LoadSpellThreats();

    LOG_INFO("server.loading", "Loading Mixology Bonuses...");
    sSpellMgr->LoadSpellMixology();

    LOG_INFO("server.loading", "Loading Spell Group Stack Rules...");
    sSpellMgr->LoadSpellGroupStackRules();

    LOG_INFO("server.loading", "Loading NPC Texts...");
    sObjectMgr->LoadGossipText();

    LOG_INFO("server.loading", "Loading Enchant Spells Proc Datas...");
    sSpellMgr->LoadSpellEnchantProcData();

    LOG_INFO("server.loading", "Loading Item Random Enchantments Table...");
    LoadRandomEnchantmentsTable();

    LOG_INFO("server.loading", "Loading Disables");
    sDisableMgr->LoadDisables();                                  // must be before loading quests and items

    LOG_INFO("server.loading", "Loading Items...");                         // must be after LoadRandomEnchantmentsTable and LoadPageTexts
    sObjectMgr->LoadItemTemplates();

    LOG_INFO("server.loading", "Loading Item Set Names...");                // must be after LoadItemPrototypes
    sObjectMgr->LoadItemSetNames();

    LOG_INFO("server.loading", "Loading Creature Model Based Info Data...");
    sObjectMgr->LoadCreatureModelInfo();

    LOG_INFO("server.loading", "Loading Creature Custom IDs Config...");
    sObjectMgr->LoadCreatureCustomIDs();

    LOG_INFO("server.loading", "Loading Creature Templates...");
    sObjectMgr->LoadCreatureTemplates();

    LOG_INFO("server.loading", "Loading Equipment Templates...");           // must be after LoadCreatureTemplates
    sObjectMgr->LoadEquipmentTemplates();

    LOG_INFO("server.loading", "Loading Creature Template Addons...");
    sObjectMgr->LoadCreatureTemplateAddons();

    LOG_INFO("server.loading", "Loading Reputation Reward Rates...");
    sObjectMgr->LoadReputationRewardRate();

    LOG_INFO("server.loading", "Loading Creature Reputation OnKill Data...");
    sObjectMgr->LoadReputationOnKill();

    LOG_INFO("server.loading", "Loading Reputation Spillover Data..." );
    sObjectMgr->LoadReputationSpilloverTemplate();

    LOG_INFO("server.loading", "Loading Points Of Interest Data...");
    sObjectMgr->LoadPointsOfInterest();

    LOG_INFO("server.loading", "Loading Creature Base Stats...");
    sObjectMgr->LoadCreatureClassLevelStats();

    LOG_INFO("server.loading", "Loading Creature Data...");
    sObjectMgr->LoadCreatures();

    LOG_INFO("server.loading", "Loading Creature sparring...");
    sObjectMgr->LoadCreatureSparring();

    LOG_INFO("server.loading", "Loading Temporary Summon Data...");
    sObjectMgr->LoadTempSummons();                               // must be after LoadCreatureTemplates() and LoadGameObjectTemplates()

    LOG_INFO("server.loading", "Loading Gameobject Summon Data...");
    sObjectMgr->LoadGameObjectSummons();                         // must be after LoadCreatureTemplates() and LoadGameObjectTemplates()

    LOG_INFO("server.loading", "Loading Pet Levelup Spells...");
    sSpellMgr->LoadPetLevelupSpellMap();

    LOG_INFO("server.loading", "Loading Pet default Spells additional to Levelup Spells...");
    sSpellMgr->LoadPetDefaultSpells();

    LOG_INFO("server.loading", "Loading Creature Addon Data...");
    sObjectMgr->LoadCreatureAddons();                            // must be after LoadCreatureTemplates() and LoadCreatures()

    LOG_INFO("server.loading", "Loading Creature Movement Overrides...");
    sObjectMgr->LoadCreatureMovementOverrides(); // must be after LoadCreatures()

    LOG_INFO("server.loading", "Loading Gameobject Data...");
    sObjectMgr->LoadGameObjects();

    LOG_INFO("server.loading", "Loading GameObject Addon Data...");
    sObjectMgr->LoadGameObjectAddons();                          // must be after LoadGameObjectTemplate() and LoadGameobjects()

    LOG_INFO("server.loading", "Loading GameObject Quest Items...");
    sObjectMgr->LoadGameObjectQuestItems();

    LOG_INFO("server.loading", "Loading Creature Quest Items...");
    sObjectMgr->LoadCreatureQuestItems();

    LOG_INFO("server.loading", "Loading Creature Linked Respawn...");
    sObjectMgr->LoadLinkedRespawn();                             // must be after LoadCreatures(), LoadGameObjects()

    LOG_INFO("server.loading", "Loading Weather Data...");
    WeatherMgr::LoadWeatherData();

    LOG_INFO("server.loading", "Loading Quests...");
    sObjectMgr->LoadQuests();                                    // must be loaded after DBCs, creature_template, item_template, gameobject tables

    LOG_INFO("server.loading", "Checking Quest Disables");
    sDisableMgr->CheckQuestDisables();                           // must be after loading quests

    LOG_INFO("server.loading", "Loading Quest POI");
    sObjectMgr->LoadQuestPOI();

    LOG_INFO("server.loading", "Loading Quests Starters and Enders...");
    sObjectMgr->LoadQuestStartersAndEnders();                    // must be after quest load

    LOG_INFO("server.loading", "Loading Quest Greetings...");
    sObjectMgr->LoadQuestGreetings();                               // must be loaded after creature_template, gameobject_template tables

    LOG_INFO("server.loading", "Loading Quest Money Rewards...");
    sObjectMgr->LoadQuestMoneyRewards();

    LOG_INFO("server.loading", "Loading Objects Pooling Data...");
    sPoolMgr->LoadFromDB();

    LOG_INFO("server.loading", "Loading Game Event Data...");               // must be after loading pools fully
    sGameEventMgr->LoadHolidayDates();                           // Must be after loading DBC
    sGameEventMgr->LoadFromDB();                                 // Must be after loading holiday dates

    LOG_INFO("server.loading", "Loading UNIT_NPC_FLAG_SPELLCLICK Data..."); // must be after LoadQuests
    sObjectMgr->LoadNPCSpellClickSpells();

    LOG_INFO("server.loading", "Loading Vehicle Template Accessories...");
    sObjectMgr->LoadVehicleTemplateAccessories();                // must be after LoadCreatureTemplates() and LoadNPCSpellClickSpells()

    LOG_INFO("server.loading", "Loading Vehicle Accessories...");
    sObjectMgr->LoadVehicleAccessories();                       // must be after LoadCreatureTemplates() and LoadNPCSpellClickSpells()

    LOG_INFO("server.loading", "Loading Vehicle Seat Addon Data...");
    sObjectMgr->LoadVehicleSeatAddon();                         // must be after loading DBC

    LOG_INFO("server.loading", "Loading SpellArea Data...");                // must be after quest load
    sSpellMgr->LoadSpellAreas();

    LOG_INFO("server.loading", "Loading Area Trigger Definitions");
    sObjectMgr->LoadAreaTriggers();

    LOG_INFO("server.loading", "Loading Area Trigger Teleport Definitions...");
    sObjectMgr->LoadAreaTriggerTeleports();

    LOG_INFO("server.loading", "Loading Access Requirements...");
    sObjectMgr->LoadAccessRequirements();                        // must be after item template load

    LOG_INFO("server.loading", "Loading Quest Area Triggers...");
    sObjectMgr->LoadQuestAreaTriggers();                         // must be after LoadQuests

    LOG_INFO("server.loading", "Loading Tavern Area Triggers...");
    sObjectMgr->LoadTavernAreaTriggers();

    LOG_INFO("server.loading", "Loading AreaTrigger Script Names...");
    sObjectMgr->LoadAreaTriggerScripts();

    LOG_INFO("server.loading", "Loading LFG Entrance Positions..."); // Must be after areatriggers
    sLFGMgr->LoadLFGDungeons();

    LOG_INFO("server.loading", "Loading Dungeon Boss Data...");
    sObjectMgr->LoadInstanceEncounters();

    LOG_INFO("server.loading", "Loading LFG Rewards...");
    sLFGMgr->LoadRewards();

    LOG_INFO("server.loading", "Loading Graveyard-Zone Links...");
    sGraveyard->LoadGraveyardZones();

    LOG_INFO("server.loading", "Loading Spell Pet Auras...");
    sSpellMgr->LoadSpellPetAuras();

    LOG_INFO("server.loading", "Loading Spell Target Coordinates...");
    sSpellMgr->LoadSpellTargetPositions();

    LOG_INFO("server.loading", "Loading Enchant Custom Attributes...");
    sSpellMgr->LoadEnchantCustomAttr();

    LOG_INFO("server.loading", "Loading linked Spells...");
    sSpellMgr->LoadSpellLinked();

    LOG_INFO("server.loading", "Loading Player Create Data...");
    sObjectMgr->LoadPlayerInfo();

    LOG_INFO("server.loading", "Loading Exploration BaseXP Data...");
    sObjectMgr->LoadExplorationBaseXP();

    LOG_INFO("server.loading", "Loading Pet Name Parts...");
    sObjectMgr->LoadPetNames();

    CharacterDatabaseCleaner::CleanDatabase();

    LOG_INFO("server.loading", "Loading The Max Pet Number...");
    sObjectMgr->LoadPetNumber();

    LOG_INFO("server.loading", "Loading Pet Level Stats...");
    sObjectMgr->LoadPetLevelInfo();

    LOG_INFO("server.loading", "Loading Player Level Dependent Mail Rewards...");
    sObjectMgr->LoadMailLevelRewards();

    LOG_INFO("server.loading", "Load Mail Server definitions...");
    sServerMailMgr->LoadMailServerTemplates();

    // Loot tables
    LoadLootTables();

    LOG_INFO("server.loading", "Loading Skill Discovery Table...");
    LoadSkillDiscoveryTable();

    LOG_INFO("server.loading", "Loading Skill Extra Item Table...");
    LoadSkillExtraItemTable();

    LOG_INFO("server.loading", "Loading Skill Perfection Data Table...");
    LoadSkillPerfectItemTable();

    LOG_INFO("server.loading", "Loading Skill Fishing Base Level Requirements...");
    sObjectMgr->LoadFishingBaseSkillLevel();

    LOG_INFO("server.loading", "Loading Achievements...");
    sAchievementMgr->LoadAchievementReferenceList();
    LOG_INFO("server.loading", "Loading Achievement Criteria Lists...");
    sAchievementMgr->LoadAchievementCriteriaList();
    LOG_INFO("server.loading", "Loading Achievement Criteria Data...");
    sAchievementMgr->LoadAchievementCriteriaData();
    LOG_INFO("server.loading", "Loading Achievement Rewards...");
    sAchievementMgr->LoadRewards();
    LOG_INFO("server.loading", "Loading Completed Achievements...");
    sAchievementMgr->LoadCompletedAchievements();

    ///- Load dynamic data tables from the database
    LOG_INFO("server.loading", "Loading Item Auctions...");
    sAuctionMgr->LoadAuctionItems();
    LOG_INFO("server.loading", "Loading Auctions...");
    sAuctionMgr->LoadAuctions();

    sGuildMgr->LoadGuilds();

    LOG_INFO("server.loading", "Loading ArenaTeams...");
    sArenaTeamMgr->LoadArenaTeams();

    LOG_INFO("server.loading", "Loading Groups...");
    sGroupMgr->LoadGroups();

    LOG_INFO("server.loading", "Loading Reserved Names...");
    sObjectMgr->LoadReservedPlayerNamesDB();
    sObjectMgr->LoadReservedPlayerNamesDBC(); // Needs to be after LoadReservedPlayerNamesDB()

    LOG_INFO("server.loading", "Loading Profanity Names...");
    sObjectMgr->LoadProfanityNamesFromDB();
    sObjectMgr->LoadProfanityNamesFromDBC(); // Needs to be after LoadProfanityNamesFromDB()

    LOG_INFO("server.loading", "Loading GameObjects for Quests...");
    sObjectMgr->LoadGameObjectForQuests();

    LOG_INFO("server.loading", "Loading BattleMasters...");
    sBattlegroundMgr->LoadBattleMastersEntry();

    LOG_INFO("server.loading", "Loading GameTeleports...");
    sObjectMgr->LoadGameTele();

    LOG_INFO("server.loading", "Loading Trainers..."); // must be after LoadCreatureTemplates
    sObjectMgr->LoadTrainers();

    LOG_INFO("server.loading", "Loading Creature default trainers...");
    sObjectMgr->LoadCreatureDefaultTrainers();

    LOG_INFO("server.loading", "Loading Gossip Menu...");
    sObjectMgr->LoadGossipMenu();

    LOG_INFO("server.loading", "Loading Gossip Menu Options...");
    sObjectMgr->LoadGossipMenuItems();

    LOG_INFO("server.loading", "Loading Vendors...");
    sObjectMgr->LoadVendors();                                   // must be after load CreatureTemplate and ItemTemplate

    LOG_INFO("server.loading", "Loading Waypoints...");
    sWaypointMgr->Load();

    LOG_INFO("server.loading", "Loading Waypoint Addons...");
    sWaypointMgr->LoadWaypointAddons();

    LOG_INFO("server.loading", "Loading SmartAI Waypoints...");
    sSmartWaypointMgr->LoadFromDB();

    LOG_INFO("server.loading", "Loading Creature Formations...");
    sFormationMgr->LoadCreatureFormations();

    LOG_INFO("server.loading", "Loading WorldStates...");              // must be loaded before battleground, outdoor PvP and conditions
    sWorldState->LoadWorldStates();

    LOG_INFO("server.loading", "Loading Conditions...");
    sConditionMgr->LoadConditions();

    LOG_INFO("server.loading", "Loading Faction Change Achievement Pairs...");
    sObjectMgr->LoadFactionChangeAchievements();

    LOG_INFO("server.loading", "Loading Faction Change Spell Pairs...");
    sObjectMgr->LoadFactionChangeSpells();

    LOG_INFO("server.loading", "Loading Faction Change Item Pairs...");
    sObjectMgr->LoadFactionChangeItems();

    LOG_INFO("server.loading", "Loading Faction Change Reputation Pairs...");
    sObjectMgr->LoadFactionChangeReputations();

    LOG_INFO("server.loading", "Loading Faction Change Title Pairs...");
    sObjectMgr->LoadFactionChangeTitles();

    LOG_INFO("server.loading", "Loading Faction Change Quest Pairs...");
    sObjectMgr->LoadFactionChangeQuests();

    LOG_INFO("server.loading", "Loading GM Tickets...");
    sTicketMgr->LoadTickets();

    LOG_INFO("server.loading", "Loading GM Surveys...");
    sTicketMgr->LoadSurveys();

    LOG_INFO("server.loading", "Loading Client Addons...");
    AddonMgr::LoadFromDB();

    // pussywizard:
    LOG_INFO("server.loading", "Deleting Invalid Mail Items...");
    LOG_INFO("server.loading", " ");
    CharacterDatabase.Execute("DELETE mi FROM mail_items mi LEFT JOIN item_instance ii ON mi.item_guid = ii.guid WHERE ii.guid IS NULL");
    CharacterDatabase.Execute("DELETE mi FROM mail_items mi LEFT JOIN mail m ON mi.mail_id = m.id WHERE m.id IS NULL");
    CharacterDatabase.Execute("UPDATE mail m LEFT JOIN mail_items mi ON m.id = mi.mail_id SET m.has_items=0 WHERE m.has_items<>0 AND mi.mail_id IS NULL");

    ///- Handle outdated emails (delete/return)
    LOG_INFO("server.loading", "Returning Old Mails...");
    LOG_INFO("server.loading", " ");
    sObjectMgr->ReturnOrDeleteOldMails(false);

    ///- Load and initialize scripts
    sObjectMgr->LoadSpellScripts();                              // must be after load Creature/Gameobject(Template/Data)
    sObjectMgr->LoadEventScripts();                              // must be after load Creature/Gameobject(Template/Data)
    sObjectMgr->LoadWaypointScripts();

    LOG_INFO("server.loading", "Loading Spell Script Names...");
    sObjectMgr->LoadSpellScriptNames();

    LOG_INFO("server.loading", "Loading Creature Texts...");
    sCreatureTextMgr->LoadCreatureTexts();

    LOG_INFO("server.loading", "Loading Scripts...");
    sScriptMgr->LoadDatabase();

    LOG_INFO("server.loading", "Validating Spell Scripts...");
    sObjectMgr->ValidateSpellScripts();

    LOG_INFO("server.loading", "Loading SmartAI Scripts...");
    sSmartScriptMgr->LoadSmartAIFromDB();

    LOG_INFO("server.loading", "Loading Calendar Data...");
    sCalendarMgr->LoadFromDB();

    LOG_INFO("server.loading", "Initializing SpellInfo Precomputed Data..."); // must be called after loading items, professions, spells and pretty much anything
    LOG_INFO("server.loading", " ");
    sObjectMgr->InitializeSpellInfoPrecomputedData();

    LOG_INFO("server.loading", "Initialize Commands...");
    Acore::ChatCommands::LoadCommandMap();

    ///- Initialize game time and timers
    LOG_INFO("server.loading", "Initialize Game Time and Timers");
    LOG_INFO("server.loading", " ");

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_INS_UPTIME);
    stmt->SetData(0, static_cast<uint32>(GameTime::GetStartTime().count()));
    LoginDatabase.Execute(stmt);

    _timers[WORLD_UPDATE_UPTIME].SetInterval(getIntConfig(CONFIG_UPTIME_UPDATE)*MINUTE * IN_MILLISECONDS);
    //Update "uptime" table based on configuration entry in minutes.

    _timers[WORLD_UPDATE_CLEANDB].SetInterval(getIntConfig(CONFIG_LOGDB_CLEARINTERVAL)*MINUTE * IN_MILLISECONDS);
    // clean logs table every 14 days by default

    _timers[WORLD_UPDATE_PING_DB].SetInterval(getIntConfig(CONFIG_DB_PING_INTERVAL)*MINUTE * IN_MILLISECONDS);  // PostgreSQL ping time in minutes

    // our speed up
    _timers[WORLD_UPDATE_5_SECS].SetInterval(5 * IN_MILLISECONDS);

    _timers[WORLD_UPDATE_WHO_LIST].SetInterval(5 * IN_MILLISECONDS); // update who list cache every 5 seconds

    _mail_expire_check_timer = GameTime::GetGameTime() + 6h;

    ///- Initialize MapMgr
    LOG_INFO("server.loading", "Starting Map System");
    LOG_INFO("server.loading", " ");
    sMapMgr->Initialize();

    LOG_INFO("server.loading", "Starting Game Event system...");
    LOG_INFO("server.loading", " ");
    uint32 nextGameEvent = sGameEventMgr->StartSystem();
    _timers[WORLD_UPDATE_EVENTS].SetInterval(nextGameEvent);    //depend on next event

    LOG_INFO("server.loading", "Loading WorldState...");
    sWorldState->Load(); // must be called after loading game events

    // Delete all characters which have been deleted X days before
    Player::DeleteOldCharacters();

    // Delete all items which have been deleted X days before
    Player::DeleteOldRecoveryItems();

    // Delete all custom channels which haven't been used for PreserveCustomChannelDuration days.
    Channel::CleanOldChannelsInDB();

    LOG_INFO("server.loading", "Initializing Opcodes...");
    opcodeTable.Initialize();

    LOG_INFO("server.loading", "Loading Arena Season Rewards...");
    sArenaSeasonMgr->LoadRewards();
    LOG_INFO("server.loading", "Loading Active Arena Season...");
    sArenaSeasonMgr->LoadActiveSeason();

    sTicketMgr->Initialize();

    ///- Initialize Battlegrounds
    LOG_INFO("server.loading", "Starting Battleground System");
    sBattlegroundMgr->LoadBattlegroundTemplates();
    sBattlegroundMgr->InitAutomaticArenaPointDistribution();

    ///- Initialize outdoor pvp
    LOG_INFO("server.loading", "Starting Outdoor PvP System");
    sOutdoorPvPMgr->InitOutdoorPvP();

    ///- Initialize Battlefield
    LOG_INFO("server.loading", "Starting Battlefield System");
    sBattlefieldMgr->InitBattlefield();

    LOG_INFO("server.loading", "Loading Transports...");
    sTransportMgr->SpawnContinentTransports();

    LOG_INFO("server.loading", "Calculate Next Daily Quest Reset Time...");
    InitDailyQuestResetTime();

    LOG_INFO("server.loading", "Calculate Next Weekly Quest Reset Time..." );
    InitWeeklyQuestResetTime();

    LOG_INFO("server.loading", "Calculate Next Monthly Quest Reset Time...");
    InitMonthlyQuestResetTime();

    LOG_INFO("server.loading", "Calculate Random Battleground Reset Time..." );
    InitRandomBGResetTime();

    LOG_INFO("server.loading", "Calculate Deletion Of Old Calendar Events Time...");
    InitCalendarOldEventsDeletionTime();

    LOG_INFO("server.loading", "Calculate Guild Cap Reset Time...");
    LOG_INFO("server.loading", " ");
    InitGuildResetTime();

    LOG_INFO("server.loading", "Load Petitions...");
    sPetitionMgr->LoadPetitions();

    LOG_INFO("server.loading", "Load Petition Signs...");
    sPetitionMgr->LoadSignatures();

    LOG_INFO("server.loading", "Load Stored Loot Items...");
    sLootItemStorage->LoadStorageFromDB();

    LOG_INFO("server.loading", "Load Channel Rights...");
    ChannelMgr::LoadChannelRights();

    LOG_INFO("server.loading", "Load Channels...");
    ChannelMgr::LoadChannels();

    sScriptMgr->OnBeforeWorldInitialized();

    uint32 startupDuration = GetMSTimeDiffToNow(startupBegin);

    LOG_INFO("server.loading", " ");
    LOG_INFO("server.loading", "WORLD: World Initialized In {} Minutes {} Seconds", (startupDuration / 60000), ((startupDuration % 60000) / 1000)); // outError for red color in console
    LOG_INFO("server.loading", " ");

    if (sConfigMgr->isDryRun())
    {
        sMapMgr->UnloadAll();
        LOG_INFO("server.loading", "AzerothCore Dry Run Completed, Terminating.");
        exit(0);
    }
}

void World::DetectDBCLang()
{
    uint8 m_lang_confid = sConfigMgr->GetOption<int32>("DBC.Locale", 255);

    if (m_lang_confid >= TOTAL_LOCALES)
    {
        LOG_ERROR("server.loading", "Incorrect DBC.Locale! Must be < {} (set to 0)", TOTAL_LOCALES);
        m_lang_confid = LOCALE_enUS;
    }

    _defaultDbcLocale = static_cast<LocaleConstant>(m_lang_confid);

    LOG_INFO("server.loading", "Using {} Locale As Default.", localeNames[GetDefaultDbcLocale()]);
    LOG_INFO("server.loading", " ");
}

/// Update the World !
void World::Update(uint32 diff)
{
    ///- Update the game time and check for shutdown time
    _UpdateGameTime();
    Seconds currentGameTime = GameTime::GetGameTime();

    sWorldUpdateTime.UpdateWithDiff(diff);

    // Record update if recording set in log and diff is greater then minimum set in log
    sWorldUpdateTime.RecordUpdateTime(GameTime::GetGameTimeMS(), diff, sWorldSessionMgr->GetActiveSessionCount());

    DynamicVisibilityMgr::Update(sWorldSessionMgr->GetActiveSessionCount());

    ///- Update the different timers
    for (int i = 0; i < WORLD_UPDATE_COUNT; ++i)
    {
        if (_timers[i].GetCurrent() >= 0)
            _timers[i].Update(diff);
        else
            _timers[i].SetCurrent(0);
    }

    // pussywizard: our speed up and functionality
    if (_timers[WORLD_UPDATE_5_SECS].Passed())
    {
        _timers[WORLD_UPDATE_5_SECS].Reset();

        // moved here from HandleCharEnumOpcode
        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_EXPIRED_BANS);
        CharacterDatabase.Execute(stmt);
    }

    ///- Update Who List Cache
    if (_timers[WORLD_UPDATE_WHO_LIST].Passed())
    {
        _timers[WORLD_UPDATE_WHO_LIST].Reset();
        sWhoListCacheMgr->Update();
    }

    {
        /// Handle daily quests reset time
        if (currentGameTime > _nextDailyQuestReset)
        {
            ResetDailyQuests();
        }

        /// Handle weekly quests reset time
        if (currentGameTime > _nextWeeklyQuestReset)
        {
            ResetWeeklyQuests();
        }

        /// Handle monthly quests reset time
        if (currentGameTime > _nextMonthlyQuestReset)
        {
            ResetMonthlyQuests();
        }
    }

    if (currentGameTime > _nextRandomBGReset)
    {
        ResetRandomBG();
    }

    if (currentGameTime > _nextCalendarOldEventsDeletionTime)
    {
        CalendarDeleteOldEvents();
    }

    if (currentGameTime > _nextGuildReset)
    {
        ResetGuildCap();
    }

    {
        // pussywizard: handle expired auctions, auctions expired when realm was offline are also handled here (not during loading when many required things aren't loaded yet)
        sAuctionMgr->Update(diff);
    }

    if (currentGameTime > _mail_expire_check_timer)
    {
        sObjectMgr->ReturnOrDeleteOldMails(true);
        _mail_expire_check_timer = currentGameTime + 6h;
    }

    {
        sWorldSessionMgr->UpdateSessions(diff);
    }

    /// <li> Clean logs table
    if (getIntConfig(CONFIG_LOGDB_CLEARTIME) > 0) // if not enabled, ignore the timer
    {
        if (_timers[WORLD_UPDATE_CLEANDB].Passed())
        {
            _timers[WORLD_UPDATE_CLEANDB].Reset();

            LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_DEL_OLD_LOGS);
            stmt->SetData(0, getIntConfig(CONFIG_LOGDB_CLEARTIME));
            stmt->SetData(1, static_cast<uint32>(currentGameTime.count()));
            LoginDatabase.Execute(stmt);
        }
    }

    {
        sLFGMgr->Update(diff, 0); // pussywizard: remove obsolete stuff before finding compatibility during map update
    }

    {
        ///- Update objects when the timer has passed (maps, transport, creatures, ...)
        sMapMgr->Update(diff);
    }

    {
        sBattlegroundMgr->Update(diff);
    }

    {
        sOutdoorPvPMgr->Update(diff);
    }

    {
        sWorldState->Update(diff);
    }

    {
        sBattlefieldMgr->Update(diff);
    }

    {
        sLFGMgr->Update(diff, 2); // pussywizard: handle created proposals
    }

    {
        // execute callbacks from sql queries that were queued recently
        ProcessQueryCallbacks();
    }

    /// <li> Update uptime table
    if (_timers[WORLD_UPDATE_UPTIME].Passed())
    {
        _timers[WORLD_UPDATE_UPTIME].Reset();

        LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_UPTIME_PLAYERS);
        stmt->SetData(0, static_cast<uint32>(GameTime::GetUptime().count()));
        stmt->SetData(1, sWorldSessionMgr->GetMaxPlayerCount());
        stmt->SetData(2, static_cast<uint32>(GameTime::GetStartTime().count()));
        LoginDatabase.Execute(stmt);
    }

    ///- Process Game events when necessary
    if (_timers[WORLD_UPDATE_EVENTS].Passed())
    {
        _timers[WORLD_UPDATE_EVENTS].Reset();                   // to give time for Update() to be processed
        uint32 nextGameEvent = sGameEventMgr->Update();
        _timers[WORLD_UPDATE_EVENTS].SetInterval(nextGameEvent);
        _timers[WORLD_UPDATE_EVENTS].Reset();
    }

    ///- Ping to keep PostgreSQL connections alive
    if (_timers[WORLD_UPDATE_PING_DB].Passed())
    {
        _timers[WORLD_UPDATE_PING_DB].Reset();
        LOG_DEBUG("sql.driver", "Ping PostgreSQL to keep connection alive");
        CharacterDatabase.KeepAlive();
        LoginDatabase.KeepAlive();
        WorldDatabase.KeepAlive();
    }

    {
        // update the instance reset times
        sInstanceSaveMgr->Update();
    }

    {
        sScriptMgr->OnWorldUpdate(diff);
    }
}

// Internally uses setFloatConfig. Retained for backwards compatibility
void World::setRate(ServerConfigs index, float value)
{
    setFloatConfig(index, value);
}

// Internally uses getFloatConfig. Retained for backwards compatibility
float World::getRate(ServerConfigs index) const
{
    return getFloatConfig(index);
}

void World::setBoolConfig(ServerConfigs index, bool value)
{
    _worldConfig.OverwriteConfigValue<bool>(index, value);
}

bool World::getBoolConfig(ServerConfigs index) const
{
    return _worldConfig.GetConfigValue<bool>(index);
}

void World::setFloatConfig(ServerConfigs index, float value)
{
    _worldConfig.OverwriteConfigValue<float>(index, value);
}

float World::getFloatConfig(ServerConfigs index) const
{
    return _worldConfig.GetConfigValue<float>(index);
}

void World::setIntConfig(ServerConfigs index, uint32 value)
{
    _worldConfig.OverwriteConfigValue<uint32>(index, value);
}

uint32 World::getIntConfig(ServerConfigs index) const
{
    return _worldConfig.GetConfigValue<uint32>(index);
}

void World::setStringConfig(ServerConfigs index, std::string const& value)
{
    _worldConfig.OverwriteConfigValue<std::string>(index, value);
}

std::string_view World::getStringConfig(ServerConfigs index) const
{
    return _worldConfig.GetConfigValue(index);
}

void World::ForceGameEventUpdate()
{
    _timers[WORLD_UPDATE_EVENTS].Reset();                   // to give time for Update() to be processed
    uint32 nextGameEvent = sGameEventMgr->Update();
    _timers[WORLD_UPDATE_EVENTS].SetInterval(nextGameEvent);
    _timers[WORLD_UPDATE_EVENTS].Reset();
}

/// Update the game time
void World::_UpdateGameTime()
{
    ///- update the time
    Seconds lastGameTime = GameTime::GetGameTime();
    GameTime::UpdateGameTimers();

    Seconds elapsed = GameTime::GetGameTime() - lastGameTime;

    ///- if there is a shutdown timer
    if (!IsStopped() && _shutdownTimer > 0 && elapsed > 0s)
    {
        ///- ... and it is overdue, stop the world (set m_stopEvent)
        if (_shutdownTimer <= elapsed.count())
        {
            if (!(_shutdownMask & SHUTDOWN_MASK_IDLE) || sWorldSessionMgr->GetActiveAndQueuedSessionCount() == 0)
                _stopEvent = true;                         // exist code already set
            else
                _shutdownTimer = 1;                        // minimum timer value to wait idle state
        }
        ///- ... else decrease it and if necessary display a shutdown countdown to the users
        else
        {
            _shutdownTimer -= elapsed.count();

            ShutdownMsg();
        }
    }
}

/// Shutdown the server
void World::ShutdownServ(uint32 time, uint32 options, uint8 exitcode, std::string const& reason)
{
    // ignore if server shutdown at next tick
    if (IsStopped())
        return;

    _shutdownMask = options;
    _exitCode = exitcode;
    _shutdownReason = reason;

    LOG_DEBUG("server.worldserver", "Server shutdown called with ShutdownMask {}, ExitCode {}, Time {}, Reason {}", ShutdownMask(options), ShutdownExitCode(exitcode), secsToTimeString(time), reason);

    ///- If the shutdown time is 0, set m_stopEvent (except if shutdown is 'idle' with remaining sessions)
    if (time == 0)
    {
        if (!(options & SHUTDOWN_MASK_IDLE) || sWorldSessionMgr->GetActiveAndQueuedSessionCount() == 0)
            _stopEvent = true;                             // exist code already set
        else
            _shutdownTimer = 1;                            //So that the session count is re-evaluated at next world tick
    }
    ///- Else set the shutdown timer and warn users
    else
    {
        _shutdownTimer = time;
        ShutdownMsg(true, nullptr, reason);
    }

    sScriptMgr->OnShutdownInitiate(ShutdownExitCode(exitcode), ShutdownMask(options));
}

/**
 * @brief Displays a shutdown message at specific intervals or immediately if required.
 *
 * Show the time remaining for a server shutdown/restart with a reason appended if one is provided.
 * Messages are displayed at regular intervals such as every
 * 12 hours, 1 hour, 5 minutes, 1 minute, 30 seconds, 10 seconds,
 * and every second in the last 10 seconds.
 *
 * @param show   Forces the message to be displayed immediately.
 * @param player The player who should recieve the message (can be nullptr for global messages).
 * @param reason The reason for the shutdown, appended to the message if provided.
 */
void World::ShutdownMsg(bool show, Player* player, std::string const& reason)
{
    // Do not show a message for idle shutdown
    if (_shutdownMask & SHUTDOWN_MASK_IDLE)
        return;

    bool twelveHours = (_shutdownTimer > 12 * HOUR && (_shutdownTimer % (12 * HOUR)) == 0); // > 12 h ; every 12 h
    bool oneHour = (_shutdownTimer < 12 * HOUR && (_shutdownTimer % HOUR) == 0); // < 12 h ; every 1 h
    bool fiveMin = (_shutdownTimer < 30 * MINUTE && (_shutdownTimer % (5 * MINUTE)) == 0); // < 30 min ; every 5 min
    bool oneMin = (_shutdownTimer < 15 * MINUTE && (_shutdownTimer % MINUTE) == 0); // < 15 min ; every 1 min
    bool thirtySec = (_shutdownTimer < 5 * MINUTE && (_shutdownTimer % 30) == 0); // < 5 min; every 30 sec
    bool tenSec = (_shutdownTimer < 1 * MINUTE && (_shutdownTimer % 10) == 0); // < 1 min; every 10 sec
    bool oneSec = (_shutdownTimer < 10 * SECOND && (_shutdownTimer % 1) == 0); // < 10 sec; every 1 sec

    ///- Display a message every 12 hours, hour, 5 minutes, minute, 30 seconds, 10 seconds and finally seconds
    if (show || twelveHours || oneHour || fiveMin || oneMin || thirtySec || tenSec || oneSec)
    {
        std::string str = secsToTimeString(_shutdownTimer).append(".");
        if (!reason.empty())
            str += " - " + reason;
        // Display the reason every 12 hours, hour, 5 minutes, minute. At 60 seconds and at 10 seconds
        else if (!_shutdownReason.empty() && (twelveHours || oneHour || fiveMin || oneMin || _shutdownTimer == 60 || _shutdownTimer == 10))
            str += " - " + _shutdownReason;

        ServerMessageType msgid = (_shutdownMask & SHUTDOWN_MASK_RESTART) ? SERVER_MSG_RESTART_TIME : SERVER_MSG_SHUTDOWN_TIME;
        sWorldSessionMgr->SendServerMessage(msgid, str, player);
        LOG_WARN("server.worldserver", "Server {} in {}", (_shutdownMask & SHUTDOWN_MASK_RESTART ? "restarting" : "shutdown"), str);
    }
}

/// Cancel a planned server shutdown
void World::ShutdownCancel()
{
    // nothing cancel or too later
    if (!_shutdownTimer || _stopEvent)
        return;

    ServerMessageType msgid = (_shutdownMask & SHUTDOWN_MASK_RESTART) ? SERVER_MSG_RESTART_CANCELLED : SERVER_MSG_SHUTDOWN_CANCELLED;

    _shutdownMask = 0;
    _shutdownTimer = 0;
    _exitCode = SHUTDOWN_EXIT_CODE;                       // to default value
    sWorldSessionMgr->SendServerMessage(msgid);

    LOG_DEBUG("server.worldserver", "Server {} cancelled.", (_shutdownMask & SHUTDOWN_MASK_RESTART ? "restart" : "shuttingdown"));

    sScriptMgr->OnShutdownCancel();
}

void World::UpdateRealmCharCount(uint32 accountId)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_COUNT);
    stmt->SetData(0, accountId);
    _queryProcessor.AddCallback(CharacterDatabase.AsyncQuery(stmt).WithPreparedCallback(std::bind(&World::_UpdateRealmCharCount, this, std::placeholders::_1,accountId)));
}

void World::_UpdateRealmCharCount(QueryResult resultCharCount, uint32 accountId)
{
    uint8 charCount{0};
    if (resultCharCount)
    {
        Field* fields = resultCharCount->Fetch();
        charCount = uint8(fields[1].Get<uint64>());
    }

    LoginDatabaseTransaction trans = LoginDatabase.BeginTransaction();

    LoginDatabasePreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_CHARACTERS_COUNT);
    stmt->SetData(0, charCount);
    stmt->SetData(1, accountId);
    trans->Append(stmt);

    LoginDatabase.CommitTransaction(trans);
}

void World::InitWeeklyQuestResetTime()
{
    Seconds wstime = Seconds(sWorldState->getWorldState(WORLD_STATE_CUSTOM_WEEKLY_QUEST_RESET_TIME));
    _nextWeeklyQuestReset = wstime > 0s ? wstime : Seconds(Acore::Time::GetNextTimeWithDayAndHour(4, 6));

    if (wstime == 0s)
    {
        sWorldState->setWorldState(WORLD_STATE_CUSTOM_WEEKLY_QUEST_RESET_TIME, _nextWeeklyQuestReset.count());
    }
}

void World::InitDailyQuestResetTime()
{
    Seconds wstime = Seconds(sWorldState->getWorldState(WORLD_STATE_CUSTOM_DAILY_QUEST_RESET_TIME));
    _nextDailyQuestReset = wstime > 0s ? wstime : Seconds(Acore::Time::GetNextTimeWithDayAndHour(-1, 6));

    if (wstime == 0s)
    {
        sWorldState->setWorldState(WORLD_STATE_CUSTOM_DAILY_QUEST_RESET_TIME, _nextDailyQuestReset.count());
    }
}

void World::InitMonthlyQuestResetTime()
{
    Seconds wstime = Seconds(sWorldState->getWorldState(WORLD_STATE_CUSTOM_MONTHLY_QUEST_RESET_TIME));
    _nextMonthlyQuestReset = wstime > 0s ? wstime : Seconds(Acore::Time::GetNextTimeWithDayAndHour(-1, 6));

    if (wstime == 0s)
    {
        sWorldState->setWorldState(WORLD_STATE_CUSTOM_MONTHLY_QUEST_RESET_TIME, _nextMonthlyQuestReset.count());
    }
}

void World::InitRandomBGResetTime()
{
    Seconds wstime = Seconds(sWorldState->getWorldState(WORLD_STATE_CUSTOM_BG_DAILY_RESET_TIME));
    _nextRandomBGReset = wstime > 0s ? wstime : Seconds(Acore::Time::GetNextTimeWithDayAndHour(-1, 6));

    if (wstime == 0s)
    {
        sWorldState->setWorldState(WORLD_STATE_CUSTOM_BG_DAILY_RESET_TIME, _nextRandomBGReset.count());
    }
}

void World::InitCalendarOldEventsDeletionTime()
{
    Seconds currentDeletionTime = Seconds(sWorldState->getWorldState(WORLD_STATE_CUSTOM_DAILY_CALENDAR_DELETION_OLD_EVENTS_TIME));
    Seconds nextDeletionTime = currentDeletionTime > 0s ? currentDeletionTime : Seconds(Acore::Time::GetNextTimeWithDayAndHour(-1, getIntConfig(CONFIG_CALENDAR_DELETE_OLD_EVENTS_HOUR)));

    if (currentDeletionTime == 0s)
    {
        sWorldState->setWorldState(WORLD_STATE_CUSTOM_DAILY_CALENDAR_DELETION_OLD_EVENTS_TIME, nextDeletionTime.count());
    }
}

void World::InitGuildResetTime()
{
    Seconds wstime = Seconds(sWorldState->getWorldState(WORLD_STATE_CUSTOM_GUILD_DAILY_RESET_TIME));
    _nextGuildReset = wstime > 0s ? wstime : Seconds(Acore::Time::GetNextTimeWithDayAndHour(-1, 6));

    if (wstime == 0s)
    {
        sWorldState->setWorldState(WORLD_STATE_CUSTOM_GUILD_DAILY_RESET_TIME, _nextGuildReset.count());
    }
}

void World::ResetDailyQuests()
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_QUEST_STATUS_DAILY);
    CharacterDatabase.Execute(stmt);

    WorldSessionMgr::SessionMap const& sessionMap = sWorldSessionMgr->GetAllSessions();
    for (WorldSessionMgr::SessionMap::const_iterator itr = sessionMap.begin(); itr != sessionMap.end(); ++itr)
        if (itr->second->GetPlayer())
            itr->second->GetPlayer()->ResetDailyQuestStatus();

    _nextDailyQuestReset = Seconds(Acore::Time::GetNextTimeWithDayAndHour(-1, 6));
    sWorldState->setWorldState(WORLD_STATE_CUSTOM_DAILY_QUEST_RESET_TIME, _nextDailyQuestReset.count());

    // change available dailies
    sPoolMgr->ChangeDailyQuests();
}

void World::ResetWeeklyQuests()
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_QUEST_STATUS_WEEKLY);
    CharacterDatabase.Execute(stmt);

    WorldSessionMgr::SessionMap const& sessionMap = sWorldSessionMgr->GetAllSessions();
    for (WorldSessionMgr::SessionMap::const_iterator itr = sessionMap.begin(); itr != sessionMap.end(); ++itr)
        if (itr->second->GetPlayer())
            itr->second->GetPlayer()->ResetWeeklyQuestStatus();

    _nextWeeklyQuestReset = Seconds(Acore::Time::GetNextTimeWithDayAndHour(4, 6));
    sWorldState->setWorldState(WORLD_STATE_CUSTOM_WEEKLY_QUEST_RESET_TIME, _nextWeeklyQuestReset.count());

    // change available weeklies
    sPoolMgr->ChangeWeeklyQuests();
}

void World::ResetMonthlyQuests()
{
    LOG_INFO("server.worldserver", "Monthly quests reset for all characters.");

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_QUEST_STATUS_MONTHLY);
    CharacterDatabase.Execute(stmt);

    WorldSessionMgr::SessionMap const& sessionMap = sWorldSessionMgr->GetAllSessions();
    for (WorldSessionMgr::SessionMap::const_iterator itr = sessionMap.begin(); itr != sessionMap.end(); ++itr)
        if (itr->second->GetPlayer())
            itr->second->GetPlayer()->ResetMonthlyQuestStatus();

    _nextMonthlyQuestReset = Seconds(Acore::Time::GetNextTimeWithMonthAndHour(-1, 6));
    sWorldState->setWorldState(WORLD_STATE_CUSTOM_MONTHLY_QUEST_RESET_TIME, _nextMonthlyQuestReset.count());
}

void World::ResetEventSeasonalQuests(uint16 event_id)
{
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_QUEST_STATUS_SEASONAL);
    stmt->SetData(0, event_id);
    CharacterDatabase.Execute(stmt);

    WorldSessionMgr::SessionMap const& sessionMap = sWorldSessionMgr->GetAllSessions();
    for (WorldSessionMgr::SessionMap::const_iterator itr = sessionMap.begin(); itr != sessionMap.end(); ++itr)
        if (itr->second->GetPlayer())
            itr->second->GetPlayer()->ResetSeasonalQuestStatus(event_id);
}

void World::ResetRandomBG()
{
    LOG_DEBUG("server.worldserver", "Random BG status reset for all characters.");

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_BATTLEGROUND_RANDOM);
    CharacterDatabase.Execute(stmt);

    WorldSessionMgr::SessionMap const& sessionMap = sWorldSessionMgr->GetAllSessions();
    for (WorldSessionMgr::SessionMap::const_iterator itr = sessionMap.begin(); itr != sessionMap.end(); ++itr)
        if (itr->second->GetPlayer())
            itr->second->GetPlayer()->SetRandomWinner(false);

    _nextRandomBGReset = Seconds(Acore::Time::GetNextTimeWithDayAndHour(-1, 6));
    sWorldState->setWorldState(WORLD_STATE_CUSTOM_BG_DAILY_RESET_TIME, _nextRandomBGReset.count());
}

void World::CalendarDeleteOldEvents()
{
    LOG_INFO("server.worldserver", "Calendar deletion of old events.");

    _nextCalendarOldEventsDeletionTime = Seconds(Acore::Time::GetNextTimeWithDayAndHour(-1, getIntConfig(CONFIG_CALENDAR_DELETE_OLD_EVENTS_HOUR)));
    sWorldState->setWorldState(WORLD_STATE_CUSTOM_DAILY_CALENDAR_DELETION_OLD_EVENTS_TIME, _nextCalendarOldEventsDeletionTime.count());
    sCalendarMgr->DeleteOldEvents();
}

void World::ResetGuildCap()
{
    LOG_INFO("server.worldserver", "Guild Daily Cap reset.");

    _nextGuildReset = Seconds(Acore::Time::GetNextTimeWithDayAndHour(-1, 6));
    sWorldState->setWorldState(WORLD_STATE_CUSTOM_GUILD_DAILY_RESET_TIME, _nextGuildReset.count());

    sGuildMgr->ResetTimes();
}

void World::UpdateAreaDependentAuras()
{
    WorldSessionMgr::SessionMap const& sessionMap = sWorldSessionMgr->GetAllSessions();
    for (WorldSessionMgr::SessionMap::const_iterator itr = sessionMap.begin(); itr != sessionMap.end(); ++itr)
        if (itr->second && itr->second->GetPlayer() && itr->second->GetPlayer()->IsInWorld())
        {
            itr->second->GetPlayer()->UpdateAreaDependentAuras(itr->second->GetPlayer()->GetAreaId());
            itr->second->GetPlayer()->UpdateZoneDependentAuras(itr->second->GetPlayer()->GetZoneId());
        }
}

void World::ProcessQueryCallbacks()
{
    _queryProcessor.ProcessReadyCallbacks();
}

uint32 World::GetNextWhoListUpdateDelaySecs()
{
    if (_timers[WORLD_UPDATE_5_SECS].Passed())
        return 1;

    uint32 t = _timers[WORLD_UPDATE_5_SECS].GetInterval() - _timers[WORLD_UPDATE_5_SECS].GetCurrent();
    t = std::min(t, (uint32)_timers[WORLD_UPDATE_5_SECS].GetInterval());

    return uint32(std::ceil(t / 1000.0f));
}
