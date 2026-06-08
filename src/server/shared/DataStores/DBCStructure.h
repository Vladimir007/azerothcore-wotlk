#ifndef DBC_STRUCTURE_H
#define DBC_STRUCTURE_H

#include <array>

#include "DatabaseEnv.h"
#include "DBCDefines.h"
#include "Util.h"

struct AchievementEntry
{
    explicit AchievementEntry(const QueryResult& result);

    uint32 ID;
    int32 RequiredFaction;
    int32 MapID;
    std::string Name;
    uint32 CategoryID;
    uint32 Points;
    uint32 Flags;
    uint32 Count;
    uint32 RefAchievement;
};

struct AchievementCategoryEntry
{
    explicit AchievementCategoryEntry(const QueryResult& result);

    uint32 ID;
    int32 ParentCategory; // -1 for main category
};

struct AchievementCriteriaEntry
{
    explicit AchievementCriteriaEntry(const QueryResult& result);

    uint32 ID;
    uint32 ReferredAchievement;
    uint32 RequiredType;
    union
    {
        // ACHIEVEMENT_CRITERIA_TYPE_KILL_CREATURE          = 0
        /// @todo: also used for player deaths..
        struct
        {
            uint32 CreatureID;
            uint32 CreatureCount;
        } KillCreature;

        // ACHIEVEMENT_CRITERIA_TYPE_WIN_BG                 = 1
        struct
        {
            uint32 MapID;
            uint32 WinCount;
        } WinBG;

        // ACHIEVEMENT_CRITERIA_TYPE_REACH_LEVEL            = 5
        struct
        {
            uint32 _unused;
            uint32 Level;
        } ReachLevel;

        // ACHIEVEMENT_CRITERIA_TYPE_REACH_SKILL_LEVEL      = 7
        struct
        {
            uint32 SkillID;
            uint32 SkillLevel;
        } ReachSkillLevel;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT   = 8
        struct
        {
            uint32 LinkedAchievement;
        } CompleteAchievement;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST_COUNT   = 9
        struct
        {
            uint32 _unused;
            uint32 TotalQuestCount;
        } CompleteQuestCount;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST_DAILY = 10
        struct
        {
            uint32 _unused;
            uint32 NumberOfDays;
        } CompleteDailyQuestDaily;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUESTS_IN_ZONE = 11
        struct
        {
            uint32 ZoneID;
            uint32 QuestCount;
        } CompleteQuestsInZone;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_DAILY_QUEST   = 14
        struct
        {
            uint32 _unused;
            uint32 QuestCount;
        } CompleteDailyQuest;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_BATTLEGROUND  = 15
        struct
        {
            uint32 MapID;
        } CompleteBattleground;

        // ACHIEVEMENT_CRITERIA_TYPE_DEATH_AT_MAP           = 16
        struct
        {
            uint32 MapID;
        } DeathAtMap;

        // ACHIEVEMENT_CRITERIA_TYPE_DEATH_IN_DUNGEON       = 18
        struct
        {
            uint32 ManLimit;
        } DeathInDungeon;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_RAID          = 19
        struct
        {
            uint32 GroupSize; // Can be 5, 10 or 25
        } CompleteRaid;

        // ACHIEVEMENT_CRITERIA_TYPE_KILLED_BY_CREATURE     = 20
        struct
        {
            uint32 CreatureEntry;
        } KilledByCreature;

        // ACHIEVEMENT_CRITERIA_TYPE_FALL_WITHOUT_DYING     = 24
        struct
        {
            uint32 _unused;
            uint32 FallHeight;
        } FallWithoutDying;

        // ACHIEVEMENT_CRITERIA_TYPE_DEATHS_FROM            = 26
        struct
        {
            uint32 Type;  // see enum EnvironmentalDamageType
        } DeathFrom;

        // ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST         = 27
        struct
        {
            uint32 QuestID;
            uint32 QuestCount;
        } CompleteQuest;

        // ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET        = 28
        // ACHIEVEMENT_CRITERIA_TYPE_BE_SPELL_TARGET2       = 69
        struct
        {
            uint32 SpellID;
            uint32 SpellCount;
        } BeSpellTarget;

        // ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL             = 29
        // ACHIEVEMENT_CRITERIA_TYPE_CAST_SPELL2            = 110
        struct
        {
            uint32 SpellID;
            uint32 CastCount;
        } CastSpell;

        // ACHIEVEMENT_CRITERIA_TYPE_BG_OBJECTIVE_CAPTURE
        struct
        {
            uint32 ObjectiveID;
            uint32 CompleteCount;
        } BGObjective;

        // ACHIEVEMENT_CRITERIA_TYPE_HONORABLE_KILL_AT_AREA = 31
        struct
        {
            uint32 AreaID;  // Reference to AreaTable.dbc
            uint32 KillCount;
        } HonorableKillAtArea;

        // ACHIEVEMENT_CRITERIA_TYPE_WIN_ARENA              = 32
        struct
        {
            uint32 MapID;  // Reference to Map.dbc
            uint32 Count;  // Number of times that the arena must be won.
        } WinArena;

        // ACHIEVEMENT_CRITERIA_TYPE_PLAY_ARENA             = 33
        struct
        {
            uint32 MapID;  // Reference to Map.dbc
        } PlayArena;

        // ACHIEVEMENT_CRITERIA_TYPE_LEARN_SPELL            = 34
        struct
        {
            uint32 SpellID;
        } LearnSpell;

        // ACHIEVEMENT_CRITERIA_TYPE_OWN_ITEM               = 36
        struct
        {
            uint32 ItemID;
            uint32 ItemCount;
        } OwnItem;

        // ACHIEVEMENT_CRITERIA_TYPE_WIN_RATED_ARENA        = 37
        struct
        {
            uint32 _unused;
            uint32 Count;
        } WinRatedArena;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_TEAM_RATING    = 38
        struct
        {
            uint32 TeamType;  // 2, 3 or 5
        } HighestTeamRating;

        // ACHIEVEMENT_CRITERIA_TYPE_REACH_TEAM_RATING      = 39
        struct
        {
            uint32 TeamType;  // 2, 3 or 5
            uint32 PersonalRating;
        } HighestPersonalRating;

        // ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LEVEL      = 40
        struct
        {
            uint32 SkillID;
            uint32 SkillLevel;  // apprentice=1, journeyman=2, expert=3, artisan=4, master=5, grand master=6
        } LearnSkillLevel;

        // ACHIEVEMENT_CRITERIA_TYPE_USE_ITEM               = 41
        struct
        {
            uint32 ItemID;
            uint32 ItemCount;
        } UseItem;

        // ACHIEVEMENT_CRITERIA_TYPE_LOOT_ITEM              = 42
        struct
        {
            uint32 ItemID;
            uint32 ItemCount;
        } LootItem;

        // ACHIEVEMENT_CRITERIA_TYPE_EXPLORE_AREA           = 43
        struct
        {
            uint32 AreaReference;
        } ExploreArea;

        // ACHIEVEMENT_CRITERIA_TYPE_OWN_RANK               = 44
        struct
        {
            /// @todo: This rank is _NOT_ the index from CharTitles.dbc
            uint32 Rank;
        } OwnRank;

        // ACHIEVEMENT_CRITERIA_TYPE_BUY_BANK_SLOT          = 45
        struct
        {
            uint32 _unused;
            uint32 NumberOfSlots;
        } BuyBankSlot;

        // ACHIEVEMENT_CRITERIA_TYPE_GAIN_REPUTATION        = 46
        struct
        {
            uint32 FactionID;
            uint32 ReputationAmount;  // Total reputation amount, so 42000 = exalted
        } GainReputation;

        // ACHIEVEMENT_CRITERIA_TYPE_GAIN_EXALTED_REPUTATION= 47
        struct
        {
            uint32 _unused;
            uint32 NumberOfExaltedFactions;
        } GainExaltedReputation;

        // ACHIEVEMENT_CRITERIA_TYPE_VISIT_BARBER_SHOP      = 48
        struct
        {
            uint32 _unused;
            uint32 NumberOfVisits;
        } VisitBarber;

        // ACHIEVEMENT_CRITERIA_TYPE_EQUIP_EPIC_ITEM        = 49
        /// @todo: where is the required item level stored?
        struct
        {
            uint32 ItemSlot;
            uint32 Count;
        } EquipEpicItem;

        // ACHIEVEMENT_CRITERIA_TYPE_ROLL_NEED_ON_LOOT      = 50
        struct
        {
            uint32 RollValue;
            uint32 Count;
        } RollNeedOnLoot;

        // ACHIEVEMENT_CRITERIA_TYPE_ROLL_GREED_ON_LOOT      = 51
        struct
        {
            uint32 RollValue;
            uint32 Count;
        } RollGreedOnLoot;

        // ACHIEVEMENT_CRITERIA_TYPE_HK_CLASS               = 52
        struct
        {
            uint32 ClassID;
            uint32 Count;
        } HKClass;

        // ACHIEVEMENT_CRITERIA_TYPE_HK_RACE                = 53
        struct
        {
            uint32 RaceID;
            uint32 Count;
        } HKRace;

        // ACHIEVEMENT_CRITERIA_TYPE_DO_EMOTE               = 54
        /// @todo: where is the information about the target stored?
        struct
        {
            uint32 EmoteID;  // enum TextEmotes
            uint32 Count;  // Count of emotes, always required special target or requirements
        } DoEmote;

        // ACHIEVEMENT_CRITERIA_TYPE_DAMAGE_DONE            = 13
        // ACHIEVEMENT_CRITERIA_TYPE_HEALING_DONE           = 55
        struct
        {
            uint32 _unused;
            uint32 Count;
        } HealingDone;

        // ACHIEVEMENT_CRITERIA_TYPE_GET_KILLING_BLOWS      = 56
        struct
        {
            uint32 _unused;
            uint32 KillCount;
        } GetKillingBlow;

        // ACHIEVEMENT_CRITERIA_TYPE_EQUIP_ITEM             = 57
        struct
        {
            uint32 ItemID;
            uint32 Count;
        } EquipItem;

        // ACHIEVEMENT_CRITERIA_TYPE_MONEY_FROM_QUEST_REWARD= 62
        struct
        {
            uint32 _unused;
            uint32 GoldInCopper;
        } QuestRewardMoney;

        // ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY             = 67
        struct
        {
            uint32 _unused;
            uint32 GoldInCopper;
        } LootMoney;

        // ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT         = 68
        struct
        {
            uint32 GameObject;
            uint32 UseCount;
        } UseGameObject;

        // ACHIEVEMENT_CRITERIA_TYPE_SPECIAL_PVP_KILL       = 70
        struct
        {
            uint32 _unused;
            uint32 KillCount;
        } SpecialPvPKill;

        // ACHIEVEMENT_CRITERIA_TYPE_FISH_IN_GAMEOBJECT     = 72
        struct
        {
            uint32 GameObject;
            uint32 LootCount;
        } FishInGameObject;

        // ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILLLINE_SPELLS = 75
        struct
        {
            uint32 SkillLine;
            uint32 SpellCount;
        } LearnSkillLineSpell;

        // ACHIEVEMENT_CRITERIA_TYPE_WIN_DUEL               = 76
        struct
        {
            uint32 _unused;
            uint32 DuelCount;
        } WinDuel;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_POWER          = 96
        struct
        {
            uint32 PowerType;  // mana=0, 1=rage, 3=energy, 6=runic power
        } HighestPower;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_STAT           = 97
        struct
        {
            uint32 StatType;  // 4=spirit, 3=int, 2=stamina, 1=agi, 0=strength
        } HighestStat;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_SPELL_POWER     = 98
        struct
        {
            uint32 SpellSchool;
        } HighestSpellPower;

        // ACHIEVEMENT_CRITERIA_TYPE_HIGHEST_RATING         = 100
        struct
        {
            uint32 RatingType;
        } HighestRating;

        // ACHIEVEMENT_CRITERIA_TYPE_LOOT_TYPE              = 109
        struct
        {
            uint32 LootType;  // 3=fishing, 2=pickpocket, 4=disenchant
            uint32 LootTypeCount;
        } LootType;

        // ACHIEVEMENT_CRITERIA_TYPE_LEARN_SKILL_LINE       = 112
        struct
        {
            uint32 SkillLine;
            uint32 SpellCount;
        } LearnSkillLine;

        // ACHIEVEMENT_CRITERIA_TYPE_EARN_HONORABLE_KILL    = 113
        struct
        {
            uint32 _unused;
            uint32 KillCount;
        } HonorableKill;

        struct
        {
            uint32 _unused;
            uint32 DungeonsComplete;
        } UseLFG;

        struct
        {
            uint32 Req;  // Main requirement
            uint32 Count;  // Main requirement count
        } Raw{};
    };

    struct
    {
        uint32 AdditionalRequirementType;
        uint32 AdditionalRequirementValue;
    } AdditionalRequirements[MAX_CRITERIA_REQUIREMENTS]{};

    uint32 Flags;
    uint32 TimedType;
    uint32 TimerStartEvent;  // Always appears with timed events: for timed spells it is spell id; for timed kills it is creature id
    uint32 TimeLimit;  // Time limit in seconds
};

struct AreaTableEntry
{
    explicit AreaTableEntry(const QueryResult& result);

    uint32 ID;
    uint32 MapID;
    uint32 Zone;  // If 0 then it's zone, else it's zone id of this area
    uint32 ExploreFlag;  // Main index
    uint32 Flags;
    int32 AreaLevel;
    std::string AreaName;
    uint32 Team;
    uint32 LiquidTypeOverride[4]{};  // Liquid override by type: [Water, Ocean, Magma, Slime]

    [[nodiscard]] bool IsSanctuary() const
    {
        if (MapID == 609) // MAP_EBON_HOLD
            return true;
        return Flags & AREA_FLAG_SANCTUARY;
    }

    [[nodiscard]] bool IsFlyable() const { return Flags & AREA_FLAG_OUTLAND; }
};

struct AreaGroupEntry
{
    explicit AreaGroupEntry(const QueryResult& result);

    uint32 ID;
    uint32 AreaId[MAX_GROUP_AREA_IDS]{};
    uint32 NextGroup;  // Index of next group
};

struct AuctionHouseEntry
{
    explicit AuctionHouseEntry(const QueryResult& result);

    uint32 ID;
    uint32 Faction;  // ID of Faction.dbc for player factions associated with city
    uint32 DepositPercent;  // 1/3 from real
    uint32 CutPercent;
};

struct BankBagSlotPricesEntry
{
    explicit BankBagSlotPricesEntry(const QueryResult& result);

    uint32 ID;
    uint32 Price;
};

struct BarberShopStyleEntry
{
    explicit BarberShopStyleEntry(const QueryResult& result);

    uint32 ID;
    uint32 Type;  // 0 -> Hair, 2 -> FacialHair
    uint32 Race;
    uint32 Gender;  // 0 -> Male, 1 -> Female
    uint32 HairID;  // Real ID to hair/facial hair
};

struct BattlemasterListEntry
{
    explicit BattlemasterListEntry(const QueryResult& result);

    uint32 ID;
    int32 MapID[8]{};
    uint32 Type;  // 3 - BG, 4 - Arena
    std::string Name;
    uint32 MaxGroupSize;  // Used for checking if queue as group
    uint32 HolidayWorldStateID;
};

struct CharStartOutfitEntry
{
    explicit CharStartOutfitEntry(const QueryResult& result);

    uint32 ID;
    uint8 Race;
    uint8 Class;
    uint8 Gender;
    int32 ItemID[MAX_OUTFIT_ITEMS]{};
};

struct CharTitlesEntry
{
    explicit CharTitlesEntry(const QueryResult& result);

    uint32 ID;  // Title ids, for example in Quest::GetCharTitleId()
    std::string NameMale;
    std::string NameFemale;
    uint32 BitIndex;  // Used in PLAYER_CHOSEN_TITLE and 1<<index in PLAYER__FIELD_KNOWN_TITLES
};

struct ChatChannelsEntry
{
    explicit ChatChannelsEntry(const QueryResult& result);

    uint32 ID;
    uint32 Flags;
    std::string Pattern;
};

struct ChrClassesEntry
{
    explicit ChrClassesEntry(const QueryResult& result);

    uint32 ID;
    uint32 PowerType;
    uint32 SpellFamily;
    uint32 CinematicSequence;  // ID from CinematicSequences.dbc
    uint32 Expansion;
};

struct ChrRacesEntry
{
    explicit ChrRacesEntry(const QueryResult& result);

    uint32 ID;
    uint32 Flags;
    uint32 FactionID;  // FactionTemplate ID
    uint32 ModelMale;
    uint32 ModelFemale;
    uint32 TeamID;  // 7-Alliance, 1-Horde
    uint32 CinematicSequence;  // ID from CinematicSequences.dbc
    uint32 Team;  // Faction (0 alliance, 1 horde, 2 not available?)
    uint32 Expansion;

    bool HasFlag(const ChrRacesFlags flag) const { return (Flags & flag) != 0; }
};

struct CinematicCameraEntry
{
    explicit CinematicCameraEntry(const QueryResult& result);

    uint32 ID;
    std::string Model;  // Model filename (translate .mdx to .m2)
    uint32 SoundID;  // Sound ID (voiceover for cinematic)
    DBCPosition3D Origin;  // Position in map used for basis for M2 co-ordinates
    float OriginFacing;  // Orientation in map used for basis for M2 co-ordinates
};

struct CinematicSequencesEntry
{
    explicit CinematicSequencesEntry(const QueryResult& result);

    uint32 ID;
    uint32 CinematicCameraID;  // ID in CinematicCamera.dbc
};

struct CreatureDisplayInfoEntry
{
    explicit CreatureDisplayInfoEntry(const QueryResult& result);

    uint32 ID;
    uint32 ModelID;
    uint32 ExtendedDisplayInfoID;
    float Scale;
};

struct CreatureDisplayInfoExtraEntry
{
    explicit CreatureDisplayInfoExtraEntry(const QueryResult& result);

    uint32 ID;
    uint32 DisplayRaceID;
};

struct CreatureFamilyEntry
{
    explicit CreatureFamilyEntry(const QueryResult& result);

    uint32 ID;
    float MinScale;
    uint32 MinScaleLevel;
    float MaxScale;
    uint32 MaxScaleLevel;
    uint32 SkillLine[2]{};
    uint32 PetFoodMask;
    int32 PetTalentType;
    std::string Name;
};

struct CreatureModelDataEntry
{
    explicit CreatureModelDataEntry(const QueryResult& result);

    uint32 ID;
    uint32 Flags;
    float Scale;  // Used in calculation of unit collision data
    float CollisionWidth;
    float CollisionHeight;
    float MountHeight;  // Used in calculation of unit collision data when mounted

    bool HasFlag(const CreatureModelDataFlags flag) const { return (Flags & flag) != 0; }
};

struct CreatureSpellDataEntry
{
    explicit CreatureSpellDataEntry(const QueryResult& result);

    uint32 ID;
    uint32 SpellID[MAX_CREATURE_SPELL_DATA_SLOT]{};
};

struct CreatureTypeEntry
{
    explicit CreatureTypeEntry(const QueryResult& result);

    uint32 ID;
};

struct CurrencyTypesEntry
{
    explicit CurrencyTypesEntry(const QueryResult& result);

    uint32 ID;  // An item entry, not id
    uint32 BitIndex;  // Bit index in PLAYER_FIELD_KNOWN_CURRENCIES (1 << (index - 1))
};

struct DestructibleModelDataEntry
{
    explicit DestructibleModelDataEntry(const QueryResult& result);

    uint32 ID;
    uint32 DamagedDisplayId;
    uint32 DestroyedDisplayId;
    uint32 RebuildingDisplayId;
    uint32 SmokeDisplayId;
};

struct DungeonEncounterEntry
{
    explicit DungeonEncounterEntry(const QueryResult& result);

    uint32 ID;
    uint32 MapID;
    uint32 Difficulty;
    uint32 EncounterIndex;  // Encounter index for creating completed mask
    std::string EncounterName;
};

struct DurabilityCostsEntry
{
    explicit DurabilityCostsEntry(const QueryResult& result);

    uint32 ID;
    uint32 Multiplier[29]{};
};

struct DurabilityQualityEntry
{
    explicit DurabilityQualityEntry(const QueryResult& result);

    uint32 ID;
    float QualityMod;
};

struct EmotesEntry
{
    explicit EmotesEntry(const QueryResult& result);

    uint32 ID;
    uint32 Flags;
    uint32 EmoteType;  // Can be 0, 1 or 2 (determine how emote are shown)
    uint32 UnitStandState;  // May be enum UnitStandStateType
};

struct EmotesTextEntry
{
    explicit EmotesTextEntry(const QueryResult& result);

    uint32 ID;
    uint32 TextID;
};

struct FactionEntry
{
    explicit FactionEntry(const QueryResult& result);

    uint32 ID;
    int32 ReputationListID;
    uint32 BaseRepRaceMask[4]{};
    uint32 BaseRepClassMask[4]{};
    int32 BaseRepValue[4]{};
    uint32 ReputationFlags[4]{};
    uint32 Team;
    float SpilloverRateIn; // Faction gains incoming rep * spilloverRateIn
    float SpilloverRateOut;  // Faction outputs rep * spilloverRateOut as spillover reputation
    uint32 SpilloverMaxRankIn; // The highest rank the faction will profit from incoming spillover
    std::string Name;

    // Helpers
    [[nodiscard]] bool CanHaveReputation() const { return ReputationListID >= 0; }
    [[nodiscard]] bool CanBeSetAtWar() const { return ReputationListID >= 0 && BaseRepRaceMask[0] == 1791; }
};

struct FactionTemplateEntry
{
    explicit FactionTemplateEntry(const QueryResult& result);

    uint32 ID;
    uint32 Faction;
    uint32 FactionFlags;
    uint32 OurMask;
    uint32 FriendlyMask;
    uint32 HostileMask;
    uint32 EnemyFaction[MAX_FACTION_RELATIONS]{};
    uint32 FriendFaction[MAX_FACTION_RELATIONS]{};

    [[nodiscard]] bool IsFriendlyTo(FactionTemplateEntry const& entry) const;
    [[nodiscard]] bool IsHostileTo(FactionTemplateEntry const& entry) const;
    [[nodiscard]] bool IsHostileToPlayers() const;
    [[nodiscard]] bool IsHostileToAlliancePlayers() const;
    [[nodiscard]] bool IsHostileToHordePlayers() const;
    [[nodiscard]] bool IsNeutralToAll() const;
    [[nodiscard]] bool IsContestedGuardFaction() const;
    [[nodiscard]] bool FactionRespondsToCallForHelp() const;
};

struct GameObjectArtKitEntry
{
    explicit GameObjectArtKitEntry(const QueryResult& result);

    uint32 ID;
};

struct GameObjectDisplayInfoEntry
{
    explicit GameObjectDisplayInfoEntry(const QueryResult& result);

    uint32 ID;
    float MinX;
    float MinY;
    float MinZ;
    float MaxX;
    float MaxY;
    float MaxZ;
};

struct GemPropertiesEntry
{
    explicit GemPropertiesEntry(const QueryResult& result);

    uint32 ID;
    uint32 SpellItemEnchantment;
    uint32 Color;
};

struct GlyphPropertiesEntry
{
    explicit GlyphPropertiesEntry(const QueryResult& result);

    uint32 ID;
    uint32 SpellID;
    uint32 TypeFlags;
};

struct GlyphSlotEntry
{
    explicit GlyphSlotEntry(const QueryResult& result);

    uint32 ID;
    uint32 TypeFlags;
    uint32 Order;
};

struct GtBarberShopCostBaseEntry
{
    explicit GtBarberShopCostBaseEntry(const QueryResult& result);

    uint32 ID;
    float Cost;
};

struct GtCombatRatingsEntry
{
    explicit GtCombatRatingsEntry(const QueryResult& result);

    uint32 ID;
    float Ratio;
};

struct GtChanceToMeleeCritBaseEntry
{
    explicit GtChanceToMeleeCritBaseEntry(const QueryResult& result);

    uint32 ID;
    float Base;
};

struct GtChanceToMeleeCritEntry
{
    explicit GtChanceToMeleeCritEntry(const QueryResult& result);

    uint32 ID;
    float Ratio;
};

struct GtChanceToSpellCritBaseEntry
{
    explicit GtChanceToSpellCritBaseEntry(const QueryResult& result);

    uint32 ID;
    float Base;
};

struct GtChanceToSpellCritEntry
{
    explicit GtChanceToSpellCritEntry(const QueryResult& result);

    uint32 ID;
    float Ratio;
};

struct GtNPCManaCostScalerEntry
{
    explicit GtNPCManaCostScalerEntry(const QueryResult& result);

    uint32 ID;
    float Ratio;
};

struct GtOCTClassCombatRatingScalarEntry
{
    explicit GtOCTClassCombatRatingScalarEntry(const QueryResult& result);

    uint32 ID;
    float Ratio;
};

struct GtOCTRegenHPEntry
{
    explicit GtOCTRegenHPEntry(const QueryResult& result);

    uint32 ID;
    float Ratio;
};

struct GtRegenHPPerSptEntry
{
    explicit GtRegenHPPerSptEntry(const QueryResult& result);

    uint32 ID;
    float Ratio;
};

struct GtRegenMPPerSptEntry
{
    explicit GtRegenMPPerSptEntry(const QueryResult& result);

    uint32 ID;
    float Ratio;
};

struct HolidaysEntry
{
    explicit HolidaysEntry(const QueryResult& result);

    uint32 ID;
    uint32 Duration[MAX_HOLIDAY_DURATIONS]{};
    uint32 Date[MAX_HOLIDAY_DATES]{};  // Dates in unix time starting at January, 1, 2000
    uint32 Region;
    uint32 Looping;
    uint32 CalendarFlags[MAX_HOLIDAY_FLAGS]{};
    std::string TextureFilename;
    uint32 Priority;
    int32 CalendarFilterType;  //-1 = Fishing Contest, 0 = Unk, 1 = DarkMoon Festival, 2 = Yearly holiday
};

struct ItemEntry
{
    explicit ItemEntry(const QueryResult& result);

    uint32 ID;
    uint32 ClassID;
    uint32 SubclassID;
    int32 SoundOverrideSubclassID;
    int32 Material;
    uint32 DisplayInfoID;
    uint32 InventoryType;
    uint32 SheatheType;
};

struct ItemBagFamilyEntry
{
    explicit ItemBagFamilyEntry(const QueryResult& result);

    uint32 ID;
};

struct ItemExtendedCostEntry
{
    explicit ItemExtendedCostEntry(const QueryResult& result);

    uint32 ID;
    uint32 ReqHonorPoints;
    uint32 ReqArenaPoints;
    uint32 ReqArenaSlot;
    uint32 ReqItem[MAX_ITEM_EXTENDED_COST_REQUIREMENTS]{};
    uint32 ReqItemCount[MAX_ITEM_EXTENDED_COST_REQUIREMENTS]{};
    uint32 ReqPersonalArenaRating;
};

struct ItemLimitCategoryEntry
{
    explicit ItemLimitCategoryEntry(const QueryResult& result);

    uint32 ID;
    uint32 MaxCount;  // Max allowed equipped as item or in gem slot
    uint32 Mode;  // 0 = have, 1 = equip (enum ItemLimitCategoryMode)
};

struct ItemRandomPropertiesEntry
{
    explicit ItemRandomPropertiesEntry(const QueryResult& result);

    uint32 ID;
    std::string Name;
    std::array<uint32, MAX_ITEM_ENCHANTMENT_EFFECTS> Enchantment{};
};

struct ItemRandomSuffixEntry
{
    explicit ItemRandomSuffixEntry(const QueryResult& result);

    uint32 ID;
    std::string Name;
    std::array<uint32, MAX_ITEM_ENCHANTMENT_EFFECTS> Enchantment{};
    std::array<uint32, MAX_ITEM_ENCHANTMENT_EFFECTS> AllocationPct{};
};

struct ItemSetEntry
{
    explicit ItemSetEntry(const QueryResult& result);

    uint32 ID;
    std::string Name;
    uint32 ItemID[MAX_ITEM_SET_ITEMS]{};
    uint32 Spells[MAX_ITEM_SET_SPELLS]{};
    uint32 ItemsToTriggerSpell[MAX_ITEM_SET_SPELLS]{};
    uint32 RequiredSkillID;
    uint32 RequiredSkillValue;
};

struct LFGDungeonEntry
{
    explicit LFGDungeonEntry(const QueryResult& result);

    uint32 ID;
    std::string Name;
    uint32 MinLevel;
    uint32 MaxLevel;
    uint32 TargetLevel;
    uint32 TargetLevelMin;
    uint32 TargetLevelMax;
    uint32 MapID;
    uint32 Difficulty;
    uint32 Flags;
    uint32 TypeID;
    uint32 ExpansionLevel;
    uint32 GroupID;

    [[nodiscard]] uint32 Entry() const { return ID + (TypeID << 24); }
};

struct LightEntry
{
    explicit LightEntry(const QueryResult& result);

    uint32 ID;
    uint32 MapID;
    float X;
    float Y;
    float Z;
};

struct LiquidTypeEntry
{
    explicit LiquidTypeEntry(const QueryResult& result);

    uint32 ID;
    uint32 Type;
    uint32 SpellID;
};

struct LockEntry
{
    explicit LockEntry(const QueryResult& result);

    uint32 ID;
    uint32 Type[MAX_LOCK_CASE]{};
    uint32 Index[MAX_LOCK_CASE]{};
    uint32 Skill[MAX_LOCK_CASE]{};
};

struct MailTemplateEntry
{
    explicit MailTemplateEntry(const QueryResult& result);

    uint32 ID;
    std::string Content;
};

struct MapEntry
{
    explicit MapEntry(const QueryResult& result);

    uint32 ID;
    uint32 MapType;
    uint32 Flags;
    std::string Name;
    uint32 Area;  // Common zone for instance and continent map
    int32 EntranceMap;  // MapEntry.ID of entrance map
    float EntranceX;  // Entrance x coordinate (if exist single entry)
    float EntranceY;  // Entrance y coordinate (if exist single entry)
    uint32 Expansion;  // 0: Vanilla, 1:TBC, 2:WotLK
    uint32 MaxPlayers;  // Max players, fallback if not present in MapDifficulty.dbc

    [[nodiscard]] bool IsDungeon() const { return MapType == MAP_INSTANCE || MapType == MAP_RAID; }
    [[nodiscard]] bool IsNonRaidDungeon() const { return MapType == MAP_INSTANCE; }
    [[nodiscard]] bool InstanceAble() const { return MapType == MAP_INSTANCE || MapType == MAP_RAID || MapType == MAP_BATTLEGROUND || MapType == MAP_ARENA; }
    [[nodiscard]] bool IsRaid() const { return MapType == MAP_RAID; }
    [[nodiscard]] bool IsBattleground() const { return MapType == MAP_BATTLEGROUND; }
    [[nodiscard]] bool IsBattleArena() const { return MapType == MAP_ARENA; }
    [[nodiscard]] bool IsBattlegroundOrArena() const { return MapType == MAP_BATTLEGROUND || MapType == MAP_ARENA; }
    [[nodiscard]] bool IsWorldMap() const { return MapType == MAP_COMMON; }
    [[nodiscard]] bool IsContinent() const;
    [[nodiscard]] bool IsDynamicDifficultyMap() const { return Flags & MAP_FLAG_DYNAMIC_DIFFICULTY; }

    bool GetEntrancePos(int32& mapID, float& x, float& y) const;
};

struct MapDifficultyEntry
{
    explicit MapDifficultyEntry(const QueryResult& result);

    uint32 ID;
    uint16 MapID;
    uint16 Difficulty;  // For arenas - arena slot
    std::string AreaTriggerText;  // Text shown when transfer to map failed (missing requirements)
    uint32 ResetTime;
    uint32 MaxPlayers;
};

struct MovieEntry
{
    explicit MovieEntry(const QueryResult& result);

    uint32 ID;
};

struct NamesReservedEntry
{
    explicit NamesReservedEntry(const QueryResult& result);

    uint32 ID;
    std::string Pattern;
};

struct NamesProfanityEntry
{
    explicit NamesProfanityEntry(const QueryResult& result);

    uint32 ID;
    std::string Pattern;
};

#define MAX_OVERRIDE_SPELL 10

struct OverrideSpellDataEntry
{
    explicit OverrideSpellDataEntry(const QueryResult& result);

    uint32 ID;
    uint32 SpellID[MAX_OVERRIDE_SPELL]{};
};

struct PowerDisplayEntry
{
    explicit PowerDisplayEntry(const QueryResult& result);

    uint32 ID;
    uint32 PowerType;
};

struct PvPDifficultyEntry
{
    explicit PvPDifficultyEntry(const QueryResult& result);

    PvPDifficultyEntry(const uint32 mapId, const uint32 bracketId, const uint32 minLevel, const uint32 maxLevel, const uint32 difficulty):
        ID(0), MapID(mapId), BracketID(bracketId), MinLevel(minLevel), MaxLevel(maxLevel), Difficulty(difficulty) {}

    uint32 ID;
    uint32 MapID;
    uint32 BracketID;
    uint32 MinLevel;
    uint32 MaxLevel;
    uint32 Difficulty;

    [[nodiscard]] BattlegroundBracketID GetBracketId() const { return static_cast<BattlegroundBracketID>(BracketID); }
};

struct QuestFactionRewEntry
{
    explicit QuestFactionRewEntry(const QueryResult& result);

    uint32 ID;
    int32 QuestRewFactionValue[10]{};
};

struct QuestSortEntry
{
    explicit QuestSortEntry(const QueryResult& result);

    uint32 ID;
};

struct QuestXPEntry
{
    explicit QuestXPEntry(const QueryResult& result);

    uint32 ID;
    uint32 Exp[10]{};
};

struct RandomPropertiesPointsEntry
{
    explicit RandomPropertiesPointsEntry(const QueryResult& result);

    uint32 ID;
    uint32 EpicPropertiesPoints[5]{};
    uint32 RarePropertiesPoints[5]{};
    uint32 UncommonPropertiesPoints[5]{};
};

struct ScalingStatDistributionEntry
{
    explicit ScalingStatDistributionEntry(const QueryResult& result);

    uint32 ID;
    int32 StatMod[10]{};
    uint32 Modifier[10]{};
    uint32 MaxLevel;
};

struct ScalingStatValuesEntry
{
    explicit ScalingStatValuesEntry(const QueryResult& result);

    uint32 ID;  // `level` column
    uint32 SsdMultiplier[4]{};
    uint32 ArmorMod[4]{};
    uint32 DPSMod[6]{};
    uint32 SpellPower;
    uint32 SsdMultiplier2;
    uint32 SsdMultiplier3;
    uint32 ArmorMod2[5]{};

    static bool IsTwoHand(uint32 mask);
    [[nodiscard]] uint32 GetSSDMultiplier(uint32 mask) const;
    [[nodiscard]] uint32 GetArmorMod(uint32 mask) const;
    [[nodiscard]] uint32 GetDPSMod(uint32 mask) const;
    [[nodiscard]] uint32 GetSpellBonus(uint32 mask) const;
    [[nodiscard]] uint32 GetFeralBonus(uint32 mask) const;
};

struct SkillLineEntry
{
    explicit SkillLineEntry(const QueryResult& result);

    uint32 ID;
    int32 CategoryID;
    std::string Name;
    uint32 CanLink;
};

struct SkillLineAbilityEntry
{
    explicit SkillLineAbilityEntry(const QueryResult& result);

    uint32 ID;
    uint32 SkillLine;
    uint32 Spell;
    uint32 RaceMask;
    uint32 ClassMask;
    uint32 MinSkillLineRank;
    uint32 SupersededBySpell;
    uint32 AcquireMethod;
    uint32 TrivialSkillLineRankHigh;
    uint32 TrivialSkillLineRankLow;
};

struct SkillRaceClassInfoEntry
{
    explicit SkillRaceClassInfoEntry(const QueryResult& result);

    uint32 ID;
    uint32 SkillID;
    uint32 RaceMask;
    uint32 ClassMask;
    uint32 Flags;
    uint32 SkillTierID;
};

struct SkillTiersEntry
{
    explicit SkillTiersEntry(const QueryResult& result);

    uint32 ID;
    uint32 Value[MAX_SKILL_STEP]{};
};

struct SoundEntriesEntry
{
    explicit SoundEntriesEntry(const QueryResult& result);

    uint32 ID;
};

struct SpellEntry
{
    explicit SpellEntry(const QueryResult& result);

    uint32 ID;
    uint32 Category;
    uint32 Dispel;
    uint32 Mechanic;
    uint32 Attributes;
    uint32 AttributesEx;
    uint32 AttributesEx2;
    uint32 AttributesEx3;
    uint32 AttributesEx4;
    uint32 AttributesEx5;
    uint32 AttributesEx6;
    uint32 AttributesEx7;
    uint32 Stances;
    uint32 StancesNot;
    uint32 Targets;
    uint32 TargetCreatureType;
    uint32 RequiresSpellFocus;
    uint32 FacingCasterFlags;
    uint32 CasterAuraState;
    uint32 TargetAuraState;
    uint32 CasterAuraStateNot;
    uint32 TargetAuraStateNot;
    uint32 CasterAuraSpell;
    uint32 TargetAuraSpell;
    uint32 ExcludeCasterAuraSpell;
    uint32 ExcludeTargetAuraSpell;
    uint32 CastingTimeIndex;
    uint32 RecoveryTime;
    uint32 CategoryRecoveryTime;
    uint32 InterruptFlags;
    uint32 AuraInterruptFlags;
    uint32 ChannelInterruptFlags;
    uint32 ProcFlags;
    uint32 ProcChance;
    uint32 ProcCharges;
    uint32 MaxLevel;
    uint32 BaseLevel;
    uint32 SpellLevel;
    uint32 DurationIndex;
    uint32 PowerType;
    uint32 ManaCost;
    uint32 ManaCostPerLevel;
    uint32 ManaPerSecond;
    uint32 ManaPerSecondPerLevel;
    uint32 RangeIndex;
    float Speed;
    uint32 StackAmount;
    std::array<uint32, 2> Totem{};
    std::array<int32, MAX_SPELL_REAGENTS> Reagent{};
    std::array<uint32, MAX_SPELL_REAGENTS> ReagentCount{};
    int32 EquippedItemClass;
    int32 EquippedItemSubClassMask;
    int32 EquippedItemInventoryTypeMask;
    std::array<uint32, MAX_SPELL_EFFECTS> Effect{};
    std::array<int32, MAX_SPELL_EFFECTS> EffectDieSides{};
    std::array<float, MAX_SPELL_EFFECTS> EffectRealPointsPerLevel{};
    std::array<int32, MAX_SPELL_EFFECTS> EffectBasePoints{};
    std::array<uint32, MAX_SPELL_EFFECTS> EffectMechanic{};
    std::array<uint32, MAX_SPELL_EFFECTS> EffectImplicitTargetA{};
    std::array<uint32, MAX_SPELL_EFFECTS> EffectImplicitTargetB{};
    std::array<uint32, MAX_SPELL_EFFECTS> EffectRadiusIndex{};
    std::array<uint32, MAX_SPELL_EFFECTS> EffectApplyAuraName{};
    std::array<uint32, MAX_SPELL_EFFECTS> EffectAmplitude{};
    std::array<float, MAX_SPELL_EFFECTS> EffectValueMultiplier{};
    std::array<uint32, MAX_SPELL_EFFECTS> EffectChainTarget{};
    std::array<uint32, MAX_SPELL_EFFECTS> EffectItemType{};
    std::array<int32, MAX_SPELL_EFFECTS> EffectMiscValue{};
    std::array<int32, MAX_SPELL_EFFECTS> EffectMiscValueB{};
    std::array<uint32, MAX_SPELL_EFFECTS> EffectTriggerSpell{};
    std::array<float, MAX_SPELL_EFFECTS> EffectPointsPerComboPoint{};
    std::array<flag96, MAX_SPELL_EFFECTS> EffectSpellClassMask{};
    std::array<float, MAX_SPELL_EFFECTS> EffectDamageMultiplier{};
    std::array<float, MAX_SPELL_EFFECTS> EffectBonusMultiplier{};
    std::array<uint32, 2> SpellVisual{};
    uint32 SpellIconID;
    uint32 ActiveIconID;
    uint32 SpellPriority;
    std::string SpellName;
    std::string Rank;
    uint32 ManaCostPercentage;
    uint32 StartRecoveryCategory;
    uint32 StartRecoveryTime;
    uint32 MaxTargetLevel;
    uint32 SpellFamilyName;
    flag96 SpellFamilyFlags;
    uint32 MaxAffectedTargets;
    uint32 DmgClass;
    uint32 PreventionType;
    std::array<uint32, 2> TotemCategory{};
    int32 AreaGroupId;
    uint32 SchoolMask;
    uint32 RuneCostID;
};

struct SpellCastTimesEntry
{
    explicit SpellCastTimesEntry(const QueryResult& result);

    uint32 ID;
    int32 CastTime;
};

struct SpellCategoryEntry
{
    explicit SpellCategoryEntry(const QueryResult& result);

    uint32 ID;
    uint32 Flags;
};

struct SpellDifficultyEntry
{
    explicit SpellDifficultyEntry(const QueryResult& result);

    SpellDifficultyEntry()
    {
        ID = 0;
        std::ranges::fill(SpellID, 0);
    }

    uint32 ID;
    int32 SpellID[MAX_DIFFICULTY]{}; // Instance modes: 10N, 25N, 10H, 25H or Normal/Heroic if only 1-2 is set, if 3-4 is 0 then Mode-2
};

struct SpellDurationEntry
{
    explicit SpellDurationEntry(const QueryResult& result);

    uint32 ID;
    int32 Duration[3]{};
};


struct SpellFocusObjectEntry
{
    explicit SpellFocusObjectEntry(const QueryResult& result);

    uint32 ID;
};

struct SpellItemEnchantmentEntry
{
    explicit SpellItemEnchantmentEntry(const QueryResult& result);

    uint32 ID;
    uint32 Charges;
    uint32 Type[MAX_SPELL_ITEM_ENCHANTMENT_EFFECTS]{};
    uint32 Amount[MAX_SPELL_ITEM_ENCHANTMENT_EFFECTS]{};
    uint32 SpellID[MAX_SPELL_ITEM_ENCHANTMENT_EFFECTS]{};
    uint32 AuraID;
    uint32 Slot;
    uint32 GemID;
    uint32 EnchantmentCondition;
    uint32 RequiredSkill;
    uint32 RequiredSkillValue;
    uint32 RequiredLevel;
};

struct SpellItemEnchantmentConditionEntry
{
    explicit SpellItemEnchantmentConditionEntry(const QueryResult& result);

    uint32 ID;
    uint8 Color[5];
    uint8 Comparator[5];
    uint8 CompareColor[5];
    uint32 Value[5];
};

struct SpellRadiusEntry
{
    explicit SpellRadiusEntry(const QueryResult& result);

    uint32 ID;
    float RadiusMin;
    float RadiusPerLevel;
    float RadiusMax;
};

struct SpellRangeEntry
{
    explicit SpellRangeEntry(const QueryResult& result);

    uint32 ID;
    float RangeMin[2]{};
    float RangeMax[2]{};
    uint32 Flags;
};

struct SpellRuneCostEntry
{
    explicit SpellRuneCostEntry(const QueryResult& result);

    uint32 ID;
    uint32 RuneCost[3]{};  // Blood, Unholy, Frost
    uint32 RunePowerGain;

    [[nodiscard]] bool NoRuneCost() const { return RuneCost[0] == 0 && RuneCost[1] == 0 && RuneCost[2] == 0; }
    [[nodiscard]] bool NoRunicPowerGain() const { return RunePowerGain == 0; }
};

struct SpellShapeshiftFormEntry
{
    explicit SpellShapeshiftFormEntry(const QueryResult& result);

    uint32 ID;
    uint32 Flags;
    int32 CreatureType;  // <= 0 humanoid, other normal creature types
    uint32 AttackSpeed;
    uint32 AllianceModelID;
    uint32 HordeModelID;
    uint32 StanceSpell[MAX_SHAPESHIFT_SPELLS]{};
};

struct SpellVisualEntry
{
    explicit SpellVisualEntry(const QueryResult& result);

    uint32 ID;
    bool HasMissile;
    int32 MissileModel;
};

struct StableSlotPricesEntry
{
    explicit StableSlotPricesEntry(const QueryResult& result);

    uint32 ID;
    uint32 Price;
};

struct SummonPropertiesEntry
{
    explicit SummonPropertiesEntry(const QueryResult& result);

    uint32 ID;
    uint32 Category;  // 0 - can't be controlled?, 1 - something guardian?, 2 - pet?, 3 - something controllable?, 4 - taxi/mount?
    uint32 Faction;
    uint32 Type;
    uint32 Slot;
    uint32 Flags;
};

struct TalentEntry
{
    explicit TalentEntry(const QueryResult& result);

    uint32 ID;
    uint32 TalentTab;  // ID in TalentTab.dbc (TalentTabEntry)
    uint32 Row;
    uint32 Col;
    std::array<uint32, MAX_TALENT_RANK> RankID{};
    uint32 DependsOn;  // ID in Talent.dbc (TalentEntry)
    uint32 DependsOnRank;
    bool AddToSpellBook;  // Also need disable highest ranks on reset talent tree
};

struct TalentTabEntry
{
    explicit TalentTabEntry(const QueryResult& result);

    uint32 ID;
    uint32 ClassMask;
    uint32 PetTalentMask;
    uint32 TabPage;
};

struct TaxiNodesEntry
{
    explicit TaxiNodesEntry(const QueryResult& result);

    uint32 ID;
    uint32 MapID;
    float X;
    float Y;
    float Z;
    std::string Name;
    uint32 MountCreatureID[2]{};
};

struct TaxiPathEntry
{
    explicit TaxiPathEntry(const QueryResult& result);

    uint32 ID;
    uint32 From;
    uint32 To;
    uint32 Price;
};

struct TaxiPathNodeEntry
{
    explicit TaxiPathNodeEntry(const QueryResult& result);

    uint32 ID;
    uint32 Path;
    uint32 Index;
    uint32 MapID;
    float X;
    float Y;
    float Z;
    uint32 ActionFlag;
    uint32 Delay;
    uint32 ArrivalEventID;
    uint32 DepartureEventID;
};

struct TeamContributionPointsEntry
{
    explicit TeamContributionPointsEntry(const QueryResult& result);

    uint32 ID;
    float Value;
};

struct TotemCategoryEntry
{
    explicit TotemCategoryEntry(const QueryResult& result);

    uint32 ID;
    uint32 CategoryType;  // One for specialization
    uint32 CategoryMask;  // Compatibility mask for same type: different for totems, compatible from high to low for rods
};

struct TransportAnimationEntry
{
    explicit TransportAnimationEntry(const QueryResult& result);

    uint32 ID;
    uint32 TransportEntry;
    uint32 TimeSeg;
    float X;
    float Y;
    float Z;
};

struct TransportRotationEntry
{
    explicit TransportRotationEntry(const QueryResult& result);

    uint32 ID;
    uint32 TransportEntry;
    uint32 TimeSeg;
    float X;
    float Y;
    float Z;
    float W;
};

struct VehicleEntry
{
    explicit VehicleEntry(const QueryResult& result);

    uint32 ID;
    uint32 Flags;
    uint32 SeatID[MAX_VEHICLE_SEATS]{};
    uint32 PowerDisplayID;
};

struct VehicleSeatEntry
{
    explicit VehicleSeatEntry(const QueryResult& result);

    uint32 ID;
    uint32 Flags;
    uint32 FlagsB;
    float AttachmentOffsetX;
    float AttachmentOffsetY;
    float AttachmentOffsetZ;

    [[nodiscard]] bool CanEnterOrExit() const;
    [[nodiscard]] bool CanSwitchFromSeat() const;
    [[nodiscard]] bool IsUsableByOverride() const;
    [[nodiscard]] bool IsEjectable() const;
    [[nodiscard]] bool CanControl() const;
};

struct WMOAreaTableEntry
{
    explicit WMOAreaTableEntry(const QueryResult& result);

    uint32 ID;
    uint16 RootID; // Used in root WMO
    uint8 AdtID; // Used in ADT file
    int32 GroupID; // Used in group WMO
    uint32 Flags;  // Used for indoor/outdoor determination
    uint32 AreaID;  // Link to AreaTableEntry.ID
};

struct WorldMapAreaEntry
{
    explicit WorldMapAreaEntry(const QueryResult& result);

    uint32 ID;  // `area_table` entry (continent 0 areas ignored)
    uint32 MapID;
    float LeftCoord;
    float RightCoord;
    float TopCoord;
    float BottomCoord;
    int32 VirtualMapID;  // -1: MapID have correct map; other: virtual map where zone show (MapID - where zone in fact internally)
};

struct WorldMapOverlayEntry
{
    explicit WorldMapOverlayEntry(const QueryResult& result);

    uint32 ID;
    uint32 AreaID[MAX_WORLD_MAP_OVERLAY_AREA_IDX]{};
};

struct MapDifficulty
{
    MapDifficulty() = default;
    MapDifficulty(const uint32 resetTime, const uint32 maxPlayers, const bool hasErrorMessage) :
        ResetTime(resetTime), MaxPlayers(maxPlayers), HasErrorMessage(hasErrorMessage) {}

    uint32 ResetTime{0};
    uint32 MaxPlayers{0};
    bool HasErrorMessage{false};
};

struct TalentSpellPos
{
    TalentSpellPos()  = default;
    TalentSpellPos(const uint16 talent_id, const uint8 rank) : TalentID(talent_id), Rank(rank) {}

    uint16 TalentID{0};
    uint8  Rank{0};
};

#endif
