#include "DBCStructure.h"

AchievementEntry::AchievementEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    int i = 0;
    ID = fields[i++].Get<uint32>();
    RequiredFaction = fields[i++].Get<int32>();
    MapID = fields[i++].Get<int32>();
    Name = fields[i++].Get<std::string>();
    CategoryID = fields[i++].Get<uint32>();
    Points = fields[i++].Get<uint32>();
    Flags = fields[i++].Get<uint32>();
    Count = fields[i++].Get<uint32>();
    RefAchievement = fields[i].Get<uint32>();
}

AchievementCategoryEntry::AchievementCategoryEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    ParentCategory = fields[1].Get<int32>();
}

AchievementCriteriaEntry::AchievementCriteriaEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    int i = 0;
    ID = fields[i++].Get<uint32>();
    ReferredAchievement = fields[i++].Get<uint32>();
    RequiredType = fields[i++].Get<uint32>();
    Raw = {fields[i++].Get<uint32>(), fields[i++].Get<uint32>()};
    AdditionalRequirements[0] = {fields[i++].Get<uint32>(), fields[i++].Get<uint32>()};
    AdditionalRequirements[1] = {fields[i++].Get<uint32>(), fields[i++].Get<uint32>()};
    Flags = fields[i++].Get<uint32>();
    TimedType = fields[i++].Get<uint32>();
    TimerStartEvent = fields[i++].Get<uint32>();
    TimeLimit = fields[i].Get<uint32>();
}

AreaTableEntry::AreaTableEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    int i = 0;
    ID = fields[i++].Get<uint32>();
    MapID = fields[i++].Get<uint32>();
    Zone = fields[i++].Get<uint32>();
    ExploreFlag = fields[i++].Get<uint32>();
    Flags = fields[i++].Get<uint32>();
    AreaLevel = fields[i++].Get<uint32>();
    AreaName = fields[i++].Get<std::string>();
    Team = fields[i++].Get<uint32>();
    LiquidTypeOverride[0] = fields[i++].Get<uint32>();
    LiquidTypeOverride[1] = fields[i++].Get<uint32>();
    LiquidTypeOverride[2] = fields[i++].Get<uint32>();
    LiquidTypeOverride[3] = fields[i].Get<uint32>();
}

AreaGroupEntry::AreaGroupEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    std::ranges::copy(fields[1].GetArray<uint32, MAX_GROUP_AREA_IDS>(), AreaId);
    NextGroup = fields[2].Get<uint32>();
}

AuctionHouseEntry::AuctionHouseEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Faction = fields[1].Get<uint32>();
    DepositPercent = fields[2].Get<uint32>();
    CutPercent = fields[3].Get<uint32>();
}

BankBagSlotPricesEntry::BankBagSlotPricesEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Price = fields[1].Get<uint32>();
}

BarberShopStyleEntry::BarberShopStyleEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Type = fields[1].Get<uint32>();
    Race = fields[2].Get<uint32>();
    Gender = fields[3].Get<uint32>();
    HairID = fields[4].Get<uint32>();
}

BattlemasterListEntry::BattlemasterListEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    std::ranges::copy(fields[1].GetArray<int32, 8>(), MapID);
    Type = fields[2].Get<uint32>();
    Name = fields[3].Get<std::string>();
    MaxGroupSize = fields[4].Get<uint32>();
    HolidayWorldStateID = fields[5].Get<uint32>();
}

CharStartOutfitEntry::CharStartOutfitEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Race = fields[1].Get<uint8>();
    Class = fields[2].Get<uint8>();
    Gender = fields[3].Get<uint8>();
    std::ranges::copy(fields[4].GetArray<int32, MAX_OUTFIT_ITEMS>(), ItemID);
}

CharTitlesEntry::CharTitlesEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    NameMale = fields[1].Get<std::string>();
    NameFemale = fields[2].Get<std::string>();
    BitIndex = fields[3].Get<uint32>();
}

ChatChannelsEntry::ChatChannelsEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Flags = fields[1].Get<uint32>();
    Pattern = fields[2].Get<std::string>();
}

ChrClassesEntry::ChrClassesEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    PowerType = fields[1].Get<uint32>();
    SpellFamily = fields[2].Get<uint32>();
    CinematicSequence = fields[3].Get<uint32>();
    Expansion = fields[4].Get<uint32>();
}

ChrRacesEntry::ChrRacesEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    int i = 0;
    ID = fields[i++].Get<uint32>();
    Flags = fields[i++].Get<uint32>();
    FactionID = fields[i++].Get<uint32>();
    ModelMale = fields[i++].Get<uint32>();
    ModelFemale = fields[i++].Get<uint32>();
    TeamID = fields[i++].Get<uint32>();
    CinematicSequence = fields[i++].Get<uint32>();
    Team = fields[i++].Get<uint32>();
    Expansion = fields[i].Get<uint32>();
}

CinematicCameraEntry::CinematicCameraEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Model = fields[1].Get<std::string>();
    SoundID = fields[2].Get<uint32>();
    const auto position = fields[3].GetArray<float, 3>();
    Origin = {position[0], position[1], position[2]};
    OriginFacing = fields[4].Get<float>();
}

CinematicSequencesEntry::CinematicSequencesEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    const auto cameras = fields[1].GetArray<uint32, 8>();
    CinematicCameraID = cameras[0];  // Others are always 0
}

CreatureDisplayInfoEntry::CreatureDisplayInfoEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    ModelID = fields[1].Get<uint32>();
    ExtendedDisplayInfoID = fields[2].Get<uint32>();
    Scale = fields[3].Get<float>();
}

CreatureDisplayInfoExtraEntry::CreatureDisplayInfoExtraEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    DisplayRaceID = fields[1].Get<uint32>();
}

CreatureFamilyEntry::CreatureFamilyEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    int i = 0;
    ID = fields[i++].Get<uint32>();
    MinScale = fields[i++].Get<float>();
    MinScaleLevel = fields[i++].Get<uint32>();
    MaxScale = fields[i++].Get<float>();
    MaxScaleLevel = fields[i++].Get<uint32>();
    SkillLine[0] = fields[i++].Get<uint32>();
    SkillLine[1] = fields[i++].Get<uint32>();
    PetFoodMask = fields[i++].Get<uint32>();
    PetTalentType = fields[i++].Get<int32>();
    Name = fields[i].Get<std::string>();
}

CreatureModelDataEntry::CreatureModelDataEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    int i = 0;
    ID = fields[i++].Get<uint32>();
    Flags = fields[i++].Get<uint32>();
    Scale = fields[i++].Get<float>();
    CollisionWidth = fields[i++].Get<float>();
    CollisionHeight = fields[i++].Get<float>();
    MountHeight = fields[i].Get<float>();
}

CreatureSpellDataEntry::CreatureSpellDataEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    std::ranges::copy(fields[1].GetArray<uint32, MAX_CREATURE_SPELL_DATA_SLOT>(), SpellID);
}

CreatureTypeEntry::CreatureTypeEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
}

CurrencyTypesEntry::CurrencyTypesEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    BitIndex = fields[1].Get<uint32>();
}

DestructibleModelDataEntry::DestructibleModelDataEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    DamagedDisplayId = fields[1].Get<uint32>();
    DestroyedDisplayId = fields[2].Get<uint32>();
    RebuildingDisplayId = fields[3].Get<uint32>();
    SmokeDisplayId = fields[4].Get<uint32>();
}


DungeonEncounterEntry::DungeonEncounterEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    MapID = fields[1].Get<uint32>();
    Difficulty = fields[2].Get<uint32>();
    EncounterIndex = fields[3].Get<uint32>();
    EncounterName = fields[4].Get<std::string>();
}

DurabilityCostsEntry::DurabilityCostsEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    std::ranges::copy(fields[1].GetArray<uint32, 21>(), Multiplier);
    std::ranges::copy(fields[2].GetArray<uint32, 8>(), Multiplier + 21);
}

DurabilityQualityEntry::DurabilityQualityEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    QualityMod = fields[1].Get<float>();
}

EmotesEntry::EmotesEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Flags = fields[1].Get<uint32>();
    EmoteType = fields[2].Get<uint32>();
    UnitStandState = fields[3].Get<uint32>();
}

EmotesTextEntry::EmotesTextEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    TextID = fields[1].Get<uint32>();
}

FactionEntry::FactionEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    int i = 0;
    ID = fields[i++].Get<uint32>();
    ReputationListID = fields[i++].Get<int32>();
    std::ranges::copy(fields[i++].GetArray<uint32, 4>(), BaseRepRaceMask);
    std::ranges::copy(fields[i++].GetArray<uint32, 4>(), BaseRepClassMask);
    std::ranges::copy(fields[i++].GetArray<int32, 4>(), BaseRepValue);
    std::ranges::copy(fields[i++].GetArray<uint32, 4>(), ReputationFlags);
    Team = fields[i++].Get<uint32>();
    SpilloverRateIn = fields[i++].Get<float>();
    SpilloverRateOut = fields[i++].Get<float>();
    SpilloverMaxRankIn = fields[i++].Get<uint32>();
    Name = fields[i].Get<std::string>();
}

FactionTemplateEntry::FactionTemplateEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    int i = 0;
    ID = fields[i++].Get<uint32>();
    Faction = fields[i++].Get<uint32>();
    FactionFlags = fields[i++].Get<uint32>();
    OurMask = fields[i++].Get<uint32>();
    FriendlyMask = fields[i++].Get<uint32>();
    HostileMask = fields[i++].Get<uint32>();
    std::ranges::copy(fields[i++].GetArray<uint32, MAX_FACTION_RELATIONS>(), EnemyFaction);
    std::ranges::copy(fields[i].GetArray<uint32, MAX_FACTION_RELATIONS>(), FriendFaction);
}

bool FactionTemplateEntry::IsFriendlyTo(FactionTemplateEntry const& entry) const
{
    // Always friendly to self faction
    if (Faction == entry.Faction)
        return true;

    if (entry.Faction)
    {
        for (const unsigned int i : EnemyFaction)
            if (i == entry.Faction)
                return false;
        for (const unsigned int i : FriendFaction)
            if (i == entry.Faction)
                return true;
    }
    return FriendlyMask & entry.OurMask || OurMask & entry.FriendlyMask;
}

bool FactionTemplateEntry::IsHostileTo(FactionTemplateEntry const& entry) const
{
    if (entry.Faction)
    {
        for (const unsigned int i : EnemyFaction)
            if (i == entry.Faction)
                return true;
        for (const unsigned int i : FriendFaction)
            if (i == entry.Faction)
                return false;
    }
    return (HostileMask & entry.OurMask) != 0;
}

bool FactionTemplateEntry::IsHostileToPlayers() const
{
    return (HostileMask & FACTION_MASK_PLAYER) != 0;
}

bool FactionTemplateEntry::IsHostileToAlliancePlayers() const
{
    return (HostileMask & FACTION_MASK_ALLIANCE) != 0;
}

bool FactionTemplateEntry::IsHostileToHordePlayers() const
{
    return (HostileMask & FACTION_MASK_HORDE) != 0;
}

bool FactionTemplateEntry::IsNeutralToAll() const
{
    for (const unsigned int i : EnemyFaction)
        if (i != 0)
            return false;
    return HostileMask == 0 && FriendlyMask == 0;
}

bool FactionTemplateEntry::IsContestedGuardFaction() const
{
    return (FactionFlags & FACTION_TEMPLATE_FLAG_ATTACK_PVP_ACTIVE_PLAYERS) != 0;
}

bool FactionTemplateEntry::FactionRespondsToCallForHelp() const
{
    return (FactionFlags & FACTION_TEMPLATE_FLAG_RESPOND_TO_CALL_FOR_HELP) != 0;
}

GameObjectArtKitEntry::GameObjectArtKitEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
}

GameObjectDisplayInfoEntry::GameObjectDisplayInfoEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    const auto geoBoxMin = fields[1].GetArray<float, 3>();
    MinX = geoBoxMin[0];
    MinY = geoBoxMin[1];
    MinZ = geoBoxMin[2];
    const auto geoBoxMax = fields[2].GetArray<float, 3>();
    MaxX = geoBoxMax[0];
    MaxY = geoBoxMax[1];
    MaxZ = geoBoxMax[2];
}

GemPropertiesEntry::GemPropertiesEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    SpellItemEnchantment = fields[1].Get<uint32>();
    Color = fields[2].Get<uint32>();
}

GlyphPropertiesEntry::GlyphPropertiesEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    SpellID = fields[1].Get<uint32>();
    TypeFlags = fields[2].Get<uint32>();
}

GlyphSlotEntry::GlyphSlotEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    TypeFlags = fields[1].Get<uint32>();
    Order = fields[2].Get<uint32>();
}

GtBarberShopCostBaseEntry::GtBarberShopCostBaseEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Cost = fields[1].Get<float>();
}

GtCombatRatingsEntry::GtCombatRatingsEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Ratio = fields[1].Get<float>();
}

GtChanceToMeleeCritBaseEntry::GtChanceToMeleeCritBaseEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Base = fields[1].Get<float>();
}

GtChanceToMeleeCritEntry::GtChanceToMeleeCritEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Ratio = fields[1].Get<float>();
}

GtChanceToSpellCritBaseEntry::GtChanceToSpellCritBaseEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Base = fields[1].Get<float>();
}

GtChanceToSpellCritEntry::GtChanceToSpellCritEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Ratio = fields[1].Get<float>();
}

GtNPCManaCostScalerEntry::GtNPCManaCostScalerEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Ratio = fields[1].Get<float>();
}

GtOCTClassCombatRatingScalarEntry::GtOCTClassCombatRatingScalarEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Ratio = fields[1].Get<float>();
}

GtOCTRegenHPEntry::GtOCTRegenHPEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Ratio = fields[1].Get<float>();
}

GtRegenHPPerSptEntry::GtRegenHPPerSptEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Ratio = fields[1].Get<float>();
}

GtRegenMPPerSptEntry::GtRegenMPPerSptEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Ratio = fields[1].Get<float>();
}

HolidaysEntry::HolidaysEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    int i = 0;
    ID = fields[i++].Get<uint32>();
    std::ranges::copy(fields[i++].GetArray<uint32, MAX_HOLIDAY_DURATIONS>(), Duration);
    std::ranges::copy(fields[i++].GetArray<uint32, MAX_HOLIDAY_DATES>(), Date);
    Region = fields[i++].Get<uint32>();
    Looping = fields[i++].Get<uint32>();
    std::ranges::copy(fields[i++].GetArray<uint32, MAX_HOLIDAY_FLAGS>(), CalendarFlags);
    TextureFilename = fields[i++].Get<std::string>();
    Priority = fields[i++].Get<uint32>();
    CalendarFilterType = fields[i].Get<int32>();
}

ItemEntry::ItemEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    int i = 0;
    ID = fields[i++].Get<uint32>();
    ClassID = fields[i++].Get<uint32>();
    SubclassID = fields[i++].Get<uint32>();
    SoundOverrideSubclassID = fields[i++].Get<int32>();
    Material = fields[i++].Get<int32>();
    DisplayInfoID = fields[i++].Get<uint32>();
    InventoryType = fields[i++].Get<uint32>();
    SheatheType = fields[i].Get<uint32>();
}

ItemBagFamilyEntry::ItemBagFamilyEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
}

ItemExtendedCostEntry::ItemExtendedCostEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    int i = 0;
    ID = fields[i++].Get<uint32>();
    ReqHonorPoints = fields[i++].Get<uint32>();
    ReqArenaPoints = fields[i++].Get<uint32>();
    ReqArenaSlot = fields[i++].Get<uint32>();
    std::ranges::copy(fields[i++].GetArray<uint32, MAX_ITEM_EXTENDED_COST_REQUIREMENTS>(), ReqItem);
    std::ranges::copy(fields[i++].GetArray<uint32, MAX_ITEM_EXTENDED_COST_REQUIREMENTS>(), ReqItemCount);
    ReqPersonalArenaRating = fields[i].Get<uint32>();
}

ItemLimitCategoryEntry::ItemLimitCategoryEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    MaxCount = fields[1].Get<uint32>();
    Mode = fields[2].Get<uint32>();
}

ItemRandomPropertiesEntry::ItemRandomPropertiesEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Name = fields[1].Get<std::string>();
    std::ranges::copy(fields[2].GetArray<uint32, MAX_ITEM_ENCHANTMENT_EFFECTS>(), Enchantment.begin());
}

ItemRandomSuffixEntry::ItemRandomSuffixEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Name = fields[1].Get<std::string>();
    std::ranges::copy(fields[2].GetArray<uint32, MAX_ITEM_ENCHANTMENT_EFFECTS>(), Enchantment.begin());
    std::ranges::copy(fields[3].GetArray<uint32, MAX_ITEM_ENCHANTMENT_EFFECTS>(), AllocationPct.begin());
}

ItemSetEntry::ItemSetEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Name = fields[1].Get<std::string>();

    auto items = fields[2].GetArray<uint32, DBC_ITEM_SET_ITEMS>();
    std::ranges::copy_n(items.begin(), MAX_ITEM_SET_ITEMS, ItemID);

    std::ranges::copy(fields[3].GetArray<uint32, MAX_ITEM_SET_SPELLS>(), Spells);
    std::ranges::copy(fields[4].GetArray<uint32, MAX_ITEM_SET_SPELLS>(), ItemsToTriggerSpell);
    RequiredSkillID = fields[5].Get<uint32>();
    RequiredSkillValue = fields[6].Get<uint32>();
}

LFGDungeonEntry::LFGDungeonEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    int i = 0;
    ID = fields[i++].Get<uint32>();
    Name = fields[i++].Get<std::string>();
    MinLevel = fields[i++].Get<uint32>();
    MaxLevel = fields[i++].Get<uint32>();
    TargetLevel = fields[i++].Get<uint32>();
    TargetLevelMin = fields[i++].Get<uint32>();
    TargetLevelMax = fields[i++].Get<uint32>();
    MapID = fields[i++].Get<uint32>();
    Difficulty = fields[i++].Get<uint32>();
    Flags = fields[i++].Get<uint32>();
    TypeID = fields[i++].Get<uint32>();
    ExpansionLevel = fields[i++].Get<uint32>();
    GroupID = fields[i].Get<uint32>();
}

LightEntry::LightEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    MapID = fields[1].Get<uint32>();
    const auto position = fields[2].GetArray<float, 3>();
    X = position[0];
    Y = position[1];
    Z = position[2];
}

LiquidTypeEntry::LiquidTypeEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Type = fields[1].Get<uint32>();
    SpellID = fields[2].Get<uint32>();
}

LockEntry::LockEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    std::ranges::copy(fields[1].GetArray<uint32, MAX_LOCK_CASE>(), Type);
    std::ranges::copy(fields[2].GetArray<uint32, MAX_LOCK_CASE>(), Index);
    std::ranges::copy(fields[3].GetArray<uint32, MAX_LOCK_CASE>(), Skill);
}

MailTemplateEntry::MailTemplateEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Content = fields[1].Get<std::string>();
}

MapEntry::MapEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    int i = 0;
    ID = fields[i++].Get<uint32>();
    MapType = fields[i++].Get<uint32>();
    Flags = fields[i++].Get<uint32>();
    Name = fields[i++].Get<std::string>();
    Area = fields[i++].Get<uint32>();
    EntranceMap = fields[i++].Get<int32>();
    const auto entrancePos = fields[i++].GetArray<float, 2>();
    EntranceX = entrancePos[0];
    EntranceY = entrancePos[1];
    Expansion = fields[i++].Get<uint32>();
    MaxPlayers = fields[i].Get<uint32>();
}

bool MapEntry::IsContinent() const
{
    // MAP_EASTERN_KINGDOMS or MAP_KALIMDOR or MAP_OUTLAND or MAP_NORTHREND
    return ID == 0 || ID == 1 || ID == 530 || ID == 571;
}

bool MapEntry::GetEntrancePos(int32& mapID, float& x, float& y) const
{
    if (EntranceMap < 0)
        return false;
    mapID = EntranceMap;
    x = EntranceX;
    y = EntranceY;
    return true;
}

MapDifficultyEntry::MapDifficultyEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    MapID = fields[1].Get<uint16>();
    Difficulty = fields[2].Get<uint16>();
    AreaTriggerText = fields[3].Get<std::string>();
    ResetTime = fields[4].Get<uint32>();
    MaxPlayers = fields[5].Get<uint32>();
}

MovieEntry::MovieEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
}

NamesReservedEntry::NamesReservedEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Pattern = fields[1].Get<std::string>();
}

NamesProfanityEntry::NamesProfanityEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Pattern = fields[1].Get<std::string>();
}

OverrideSpellDataEntry::OverrideSpellDataEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    std::ranges::copy(fields[1].GetArray<uint32, MAX_OVERRIDE_SPELL>(), SpellID);
}

PowerDisplayEntry::PowerDisplayEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    PowerType = fields[1].Get<uint32>();
}

PvPDifficultyEntry::PvPDifficultyEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    MapID = fields[1].Get<uint32>();
    BracketID = fields[2].Get<uint32>();
    MinLevel = fields[3].Get<uint32>();
    MaxLevel = fields[4].Get<uint32>();
    Difficulty = fields[5].Get<uint32>();
}

QuestFactionRewEntry::QuestFactionRewEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    std::ranges::copy(fields[1].GetArray<int32, 10>(), QuestRewFactionValue);
}

QuestSortEntry::QuestSortEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
}

QuestXPEntry::QuestXPEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    std::ranges::copy(fields[1].GetArray<uint32, 10>(), Exp);
}

RandomPropertiesPointsEntry::RandomPropertiesPointsEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    std::ranges::copy(fields[1].GetArray<uint32, 5>(), EpicPropertiesPoints);
    std::ranges::copy(fields[2].GetArray<uint32, 5>(), RarePropertiesPoints);
    std::ranges::copy(fields[3].GetArray<uint32, 5>(), UncommonPropertiesPoints);
}

ScalingStatDistributionEntry::ScalingStatDistributionEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    std::ranges::copy(fields[1].GetArray<int32, 10>(), StatMod);
    std::ranges::copy(fields[2].GetArray<uint32, 10>(), Modifier);
    MaxLevel = fields[3].Get<uint32>();
}

ScalingStatValuesEntry::ScalingStatValuesEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    int i = 0;
    ID = fields[i++].Get<uint32>();
    std::ranges::copy(fields[i++].GetArray<uint32, 4>(), SsdMultiplier);
    std::ranges::copy(fields[i++].GetArray<uint32, 4>(), ArmorMod);
    std::ranges::copy(fields[i++].GetArray<uint32, 6>(), DPSMod);
    SpellPower = fields[i++].Get<uint32>();
    SsdMultiplier2 = fields[i++].Get<uint32>();
    SsdMultiplier3 = fields[i++].Get<uint32>();
    std::ranges::copy(fields[i].GetArray<uint32, 5>(), ArmorMod2);
}


bool ScalingStatValuesEntry::IsTwoHand(const uint32 mask)
{
    if (mask & 0x7E00)
    {
        if (mask & 0x00000400) return true;
        if (mask & 0x00001000) return true;
    }
    return false;
}

uint32 ScalingStatValuesEntry::GetSSDMultiplier(const uint32 mask) const
{
    if (mask & 0x4001F)
    {
        if (mask & 0x00000001) return SsdMultiplier[0]; // Shoulder
        if (mask & 0x00000002) return SsdMultiplier[1]; // Trinket
        if (mask & 0x00000004) return SsdMultiplier[2]; // Weapon1H
        if (mask & 0x00000008) return SsdMultiplier2;
        if (mask & 0x00000010) return SsdMultiplier[3]; // Ranged
        if (mask & 0x00040000) return SsdMultiplier3;
    }
    return 0;
}

uint32 ScalingStatValuesEntry::GetArmorMod(const uint32 mask) const
{
    if (mask & 0x00F801E0)
    {
        if (mask & 0x00000020) return ArmorMod[0];      // Cloth shoulder
        if (mask & 0x00000040) return ArmorMod[1];      // Leather shoulder
        if (mask & 0x00000080) return ArmorMod[2];      // Mail shoulder
        if (mask & 0x00000100) return ArmorMod[3];      // Plate shoulder

        if (mask & 0x00080000) return ArmorMod2[0];      // Cloak
        if (mask & 0x00100000) return ArmorMod2[1];      // Cloth
        if (mask & 0x00200000) return ArmorMod2[2];      // Leather
        if (mask & 0x00400000) return ArmorMod2[3];      // Mail
        if (mask & 0x00800000) return ArmorMod2[4];      // Plate
    }
    return 0;
}

uint32 ScalingStatValuesEntry::GetDPSMod(const uint32 mask) const
{
    if (mask & 0x7E00)
    {
        if (mask & 0x00000200) return DPSMod[0];        // Weapon 1h
        if (mask & 0x00000400) return DPSMod[1];        // Weapon 2h
        if (mask & 0x00000800) return DPSMod[2];        // Caster dps 1h
        if (mask & 0x00001000) return DPSMod[3];        // Caster dps 2h
        if (mask & 0x00002000) return DPSMod[4];        // Ranged
        if (mask & 0x00004000) return DPSMod[5];        // Wand
    }
    return 0;
}

uint32 ScalingStatValuesEntry::GetSpellBonus(const uint32 mask) const
{
    if (mask & 0x00008000) return SpellPower;
    return 0;
}

// ReSharper disable once CppDFAConstantFunctionResult
// ReSharper disable once CppMemberFunctionMayBeStatic
uint32 ScalingStatValuesEntry::GetFeralBonus(const uint32 mask) const
{
    if (mask & 0x00010000) return 0;  // not used?
    return 0;
}

SkillLineEntry::SkillLineEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    CategoryID = fields[1].Get<int32>();
    Name = fields[2].Get<std::string>();
    CanLink = fields[3].Get<uint32>();
}

SkillLineAbilityEntry::SkillLineAbilityEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    int i = 0;
    ID = fields[i++].Get<uint32>();
    SkillLine = fields[i++].Get<uint32>();
    Spell = fields[i++].Get<uint32>();
    RaceMask = fields[i++].Get<uint32>();
    ClassMask = fields[i++].Get<uint32>();
    MinSkillLineRank = fields[i++].Get<uint32>();
    SupersededBySpell = fields[i++].Get<uint32>();
    AcquireMethod = fields[i++].Get<uint32>();
    TrivialSkillLineRankHigh = fields[i++].Get<uint32>();
    TrivialSkillLineRankLow = fields[i].Get<uint32>();
}

SkillRaceClassInfoEntry::SkillRaceClassInfoEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    int i = 0;
    ID = fields[i++].Get<uint32>();
    SkillID = fields[i++].Get<uint32>();
    RaceMask = fields[i++].Get<uint32>();
    ClassMask = fields[i++].Get<uint32>();
    Flags = fields[i++].Get<uint32>();
    SkillTierID = fields[i].Get<uint32>();
}

SkillTiersEntry::SkillTiersEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    std::ranges::copy(fields[1].GetArray<uint32, MAX_SKILL_STEP>(), Value);
}

SoundEntriesEntry::SoundEntriesEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
}

SpellEntry::SpellEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    int i = 0;
    ID = fields[i++].Get<uint32>();
    Category = fields[i++].Get<uint32>();
    Dispel = fields[i++].Get<uint32>();
    Mechanic = fields[i++].Get<uint32>();
    Attributes = fields[i++].Get<uint32>();
    AttributesEx = fields[i++].Get<uint32>();
    AttributesEx2 = fields[i++].Get<uint32>();
    AttributesEx3 = fields[i++].Get<uint32>();
    AttributesEx4 = fields[i++].Get<uint32>();
    AttributesEx5 = fields[i++].Get<uint32>();
    AttributesEx6 = fields[i++].Get<uint32>();
    AttributesEx7 = fields[i++].Get<uint32>();
    Stances = fields[i++].Get<uint32>();
    StancesNot = fields[i++].Get<uint32>();
    Targets = fields[i++].Get<uint32>();
    TargetCreatureType = fields[i++].Get<uint32>();
    RequiresSpellFocus = fields[i++].Get<uint32>();
    FacingCasterFlags = fields[i++].Get<uint32>();
    CasterAuraState = fields[i++].Get<uint32>();
    TargetAuraState = fields[i++].Get<uint32>();
    CasterAuraStateNot = fields[i++].Get<uint32>();
    TargetAuraStateNot = fields[i++].Get<uint32>();
    CasterAuraSpell = fields[i++].Get<uint32>();
    TargetAuraSpell = fields[i++].Get<uint32>();
    ExcludeCasterAuraSpell = fields[i++].Get<uint32>();
    ExcludeTargetAuraSpell = fields[i++].Get<uint32>();
    CastingTimeIndex = fields[i++].Get<uint32>();
    RecoveryTime = fields[i++].Get<uint32>();
    CategoryRecoveryTime = fields[i++].Get<uint32>();
    InterruptFlags = fields[i++].Get<uint32>();
    AuraInterruptFlags = fields[i++].Get<uint32>();
    ChannelInterruptFlags = fields[i++].Get<uint32>();
    ProcFlags = fields[i++].Get<uint32>();
    ProcChance = fields[i++].Get<uint32>();
    ProcCharges = fields[i++].Get<uint32>();
    MaxLevel = fields[i++].Get<uint32>();
    BaseLevel = fields[i++].Get<uint32>();
    SpellLevel = fields[i++].Get<uint32>();
    DurationIndex = fields[i++].Get<uint32>();
    PowerType = fields[i++].Get<uint32>();
    ManaCost = fields[i++].Get<uint32>();
    ManaCostPerLevel = fields[i++].Get<uint32>();
    ManaPerSecond = fields[i++].Get<uint32>();
    ManaPerSecondPerLevel = fields[i++].Get<uint32>();
    RangeIndex = fields[i++].Get<uint32>();
    Speed = fields[i++].Get<float>();
    StackAmount = fields[i++].Get<uint32>();
    Totem[0] = fields[i++].Get<uint32>();
    Totem[1] = fields[i++].Get<uint32>();
    std::ranges::copy(fields[i++].GetArray<int32, MAX_SPELL_REAGENTS>(), Reagent.begin());
    std::ranges::copy(fields[i++].GetArray<uint32, MAX_SPELL_REAGENTS>(), ReagentCount.begin());
    EquippedItemClass = fields[i++].Get<int32>();
    EquippedItemSubClassMask = fields[i++].Get<int32>();
    EquippedItemInventoryTypeMask = fields[i++].Get<int32>();
    std::ranges::copy(fields[i++].GetArray<uint32, MAX_SPELL_EFFECTS>(), Effect.begin());
    std::ranges::copy(fields[i++].GetArray<int32, MAX_SPELL_EFFECTS>(), EffectDieSides.begin());
    std::ranges::copy(fields[i++].GetArray<float, MAX_SPELL_EFFECTS>(), EffectRealPointsPerLevel.begin());
    std::ranges::copy(fields[i++].GetArray<int32, MAX_SPELL_EFFECTS>(), EffectBasePoints.begin());
    std::ranges::copy(fields[i++].GetArray<uint32, MAX_SPELL_EFFECTS>(), EffectMechanic.begin());
    std::ranges::copy(fields[i++].GetArray<uint32, MAX_SPELL_EFFECTS>(), EffectImplicitTargetA.begin());
    std::ranges::copy(fields[i++].GetArray<uint32, MAX_SPELL_EFFECTS>(), EffectImplicitTargetB.begin());
    std::ranges::copy(fields[i++].GetArray<uint32, MAX_SPELL_EFFECTS>(), EffectRadiusIndex.begin());
    std::ranges::copy(fields[i++].GetArray<uint32, MAX_SPELL_EFFECTS>(), EffectApplyAuraName.begin());
    std::ranges::copy(fields[i++].GetArray<uint32, MAX_SPELL_EFFECTS>(), EffectAmplitude.begin());
    std::ranges::copy(fields[i++].GetArray<float, MAX_SPELL_EFFECTS>(), EffectValueMultiplier.begin());
    std::ranges::copy(fields[i++].GetArray<uint32, MAX_SPELL_EFFECTS>(), EffectChainTarget.begin());
    std::ranges::copy(fields[i++].GetArray<uint32, MAX_SPELL_EFFECTS>(), EffectItemType.begin());
    std::ranges::copy(fields[i++].GetArray<int32, MAX_SPELL_EFFECTS>(), EffectMiscValue.begin());
    std::ranges::copy(fields[i++].GetArray<int32, MAX_SPELL_EFFECTS>(), EffectMiscValueB.begin());
    std::ranges::copy(fields[i++].GetArray<uint32, MAX_SPELL_EFFECTS>(), EffectTriggerSpell.begin());
    std::ranges::copy(fields[i++].GetArray<float, MAX_SPELL_EFFECTS>(), EffectPointsPerComboPoint.begin());
    const auto effectsClassMaskA = fields[i++].GetArray<uint32, 3>();
    EffectSpellClassMask[0] = flag96(effectsClassMaskA[0], effectsClassMaskA[1], effectsClassMaskA[2]);
    const auto effectsClassMaskB = fields[i++].GetArray<uint32, 3>();
    EffectSpellClassMask[1] = flag96(effectsClassMaskB[0], effectsClassMaskB[1], effectsClassMaskB[2]);
    const auto effectsClassMaskC = fields[i++].GetArray<uint32, 3>();
    EffectSpellClassMask[2] = flag96(effectsClassMaskC[0], effectsClassMaskC[1], effectsClassMaskC[2]);
    std::ranges::copy(fields[i++].GetArray<float, MAX_SPELL_EFFECTS>(), EffectDamageMultiplier.begin());
    std::ranges::copy(fields[i++].GetArray<float, MAX_SPELL_EFFECTS>(), EffectBonusMultiplier.begin());
    SpellVisual[0] = fields[i++].Get<uint32>();
    SpellVisual[1] = fields[i++].Get<uint32>();
    SpellIconID = fields[i++].Get<uint32>();
    ActiveIconID = fields[i++].Get<uint32>();
    SpellPriority = fields[i++].Get<uint32>();
    SpellName = fields[i++].Get<std::string>();
    Rank = fields[i++].Get<std::string>();
    ManaCostPercentage = fields[i++].Get<uint32>();
    StartRecoveryCategory = fields[i++].Get<uint32>();
    StartRecoveryTime = fields[i++].Get<uint32>();
    MaxTargetLevel = fields[i++].Get<uint32>();
    SpellFamilyName = fields[i++].Get<uint32>();
    const auto spellFamilyFlagsArr = fields[i++].GetArray<uint32, 3>();
    SpellFamilyFlags = flag96(spellFamilyFlagsArr[0], spellFamilyFlagsArr[1], spellFamilyFlagsArr[2]);
    MaxAffectedTargets = fields[i++].Get<uint32>();
    DmgClass = fields[i++].Get<uint32>();
    PreventionType = fields[i++].Get<uint32>();
    TotemCategory[0] = fields[i++].Get<uint32>();
    TotemCategory[1] = fields[i++].Get<uint32>();
    AreaGroupId = fields[i++].Get<int32>();
    SchoolMask = fields[i++].Get<uint32>();
    RuneCostID = fields[i++].Get<uint32>();
}

SpellCastTimesEntry::SpellCastTimesEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    CastTime = fields[1].Get<int32>();
}

SpellCategoryEntry::SpellCategoryEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Flags = fields[1].Get<uint32>();
}

SpellDifficultyEntry::SpellDifficultyEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    SpellID[0] = fields[1].Get<int32>();
    SpellID[1] = fields[2].Get<int32>();
    SpellID[2] = fields[3].Get<int32>();
    SpellID[3] = fields[4].Get<int32>();
}

SpellDurationEntry::SpellDurationEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Duration[0] = fields[1].Get<int32>();
    Duration[1] = fields[2].Get<int32>();
    Duration[2] = fields[3].Get<int32>();
}

SpellFocusObjectEntry::SpellFocusObjectEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
}

SpellItemEnchantmentEntry::SpellItemEnchantmentEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    int i = 0;
    ID = fields[i++].Get<uint32>();
    Charges = fields[i++].Get<uint32>();
    std::ranges::copy(fields[i++].GetArray<uint32, MAX_SPELL_ITEM_ENCHANTMENT_EFFECTS>(), Type);
    std::ranges::copy(fields[i++].GetArray<uint32, MAX_SPELL_ITEM_ENCHANTMENT_EFFECTS>(), Amount);
    SpellID[0] = fields[i++].Get<uint32>();
    SpellID[1] = fields[i++].Get<uint32>();
    SpellID[2] = fields[i++].Get<uint32>();
    AuraID = fields[i++].Get<uint32>();
    Slot = fields[i++].Get<uint32>();
    GemID = fields[i++].Get<uint32>();
    EnchantmentCondition = fields[i++].Get<uint32>();
    RequiredSkill = fields[i++].Get<uint32>();
    RequiredSkillValue = fields[i++].Get<uint32>();
    RequiredLevel = fields[i].Get<uint32>();
}

SpellItemEnchantmentConditionEntry::SpellItemEnchantmentConditionEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    std::ranges::copy(fields[1].GetArray<uint8, 5>(), Color);
    std::ranges::copy(fields[2].GetArray<uint8, 5>(), Comparator);
    std::ranges::copy(fields[3].GetArray<uint8, 5>(), CompareColor);
    std::ranges::copy(fields[4].GetArray<uint32, 5>(), Value);
}

SpellRadiusEntry::SpellRadiusEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    RadiusMin = fields[1].Get<float>();
    RadiusPerLevel = fields[2].Get<float>();
    RadiusMax = fields[3].Get<float>();
}

SpellRangeEntry::SpellRangeEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    RangeMin[0] = fields[1].Get<float>();
    RangeMin[1] = fields[2].Get<float>();
    RangeMax[0] = fields[3].Get<float>();
    RangeMax[1] = fields[4].Get<float>();
    Flags = fields[5].Get<uint32>();
}

SpellRuneCostEntry::SpellRuneCostEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    RuneCost[0] = fields[1].Get<uint32>();
    RuneCost[1] = fields[2].Get<uint32>();
    RuneCost[2] = fields[3].Get<uint32>();
    RunePowerGain = fields[4].Get<uint32>();
}

SpellShapeshiftFormEntry::SpellShapeshiftFormEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    int i = 0;
    ID = fields[i++].Get<uint32>();
    Flags = fields[i++].Get<uint32>();
    CreatureType = fields[i++].Get<int32>();
    AttackSpeed = fields[i++].Get<uint32>();
    const auto displays = fields[i++].GetArray<uint32, 4>();
    AllianceModelID = displays[0];
    HordeModelID = displays[1];
    std::ranges::copy(fields[i].GetArray<uint32, MAX_SHAPESHIFT_SPELLS>(), StanceSpell);
}

SpellVisualEntry::SpellVisualEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    HasMissile = fields[1].Get<bool>();
    MissileModel = fields[2].Get<int32>();
}

StableSlotPricesEntry::StableSlotPricesEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Price = fields[1].Get<uint32>();
}

SummonPropertiesEntry::SummonPropertiesEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Category = fields[1].Get<uint32>();
    Faction = fields[2].Get<uint32>();
    Type = fields[3].Get<uint32>();
    Slot = fields[4].Get<uint32>();
    Flags = fields[5].Get<uint32>();
}

TalentEntry::TalentEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    int i = 0;
    ID = fields[i++].Get<uint32>();
    TalentTab = fields[i++].Get<uint32>();
    Row = fields[i++].Get<uint32>();
    Col = fields[i++].Get<uint32>();
    const auto rankValues = fields[i++].GetArray<uint32, 9>();
    std::ranges::copy_n(rankValues.begin(), MAX_TALENT_RANK, RankID.begin());
    DependsOn = fields[i++].GetArray<uint32, 3>()[0];
    DependsOnRank = fields[i++].GetArray<uint32, 3>()[0];
    AddToSpellBook = fields[i].Get<bool>();
}

TalentTabEntry::TalentTabEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    ClassMask = fields[1].Get<uint32>();
    PetTalentMask = fields[2].Get<uint32>();
    TabPage = fields[3].Get<uint32>();
}

TaxiNodesEntry::TaxiNodesEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    MapID = fields[1].Get<uint32>();
    const auto position = fields[2].GetArray<float, 3>();
    X = position[0];
    Y = position[1];
    Z = position[2];
    Name = fields[3].Get<std::string>();
    MountCreatureID[0] = fields[4].Get<uint32>();
    MountCreatureID[1] = fields[5].Get<uint32>();

}

TaxiPathEntry::TaxiPathEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    From = fields[1].Get<uint32>();
    const auto toNode = fields[2].Get<int32>();
    To = toNode < 0 ? 0 : static_cast<uint32>(toNode);
    Price = fields[3].Get<uint32>();
}

TaxiPathNodeEntry::TaxiPathNodeEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    int i = 0;
    ID = fields[i++].Get<uint32>();
    Path = fields[i++].Get<uint32>();
    Index = fields[i++].Get<uint32>();
    MapID = fields[i++].Get<uint32>();
    const auto position = fields[i++].GetArray<float, 3>();
    X = position[0];
    Y = position[1];
    Z = position[2];
    ActionFlag = fields[i++].Get<uint32>();
    Delay = fields[i++].Get<uint32>();
    ArrivalEventID = fields[i++].Get<uint32>();
    DepartureEventID = fields[i].Get<uint32>();
}

TeamContributionPointsEntry::TeamContributionPointsEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Value = fields[1].Get<float>();
}

TotemCategoryEntry::TotemCategoryEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    CategoryType = fields[1].Get<uint32>();
    CategoryMask = fields[2].Get<uint32>();
}

TransportAnimationEntry::TransportAnimationEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    TransportEntry = fields[1].Get<uint32>();
    TimeSeg = fields[2].Get<uint32>();
    const auto position = fields[3].GetArray<float, 3>();
    X = position[0];
    Y = position[1];
    Z = position[2];
}

TransportRotationEntry::TransportRotationEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    TransportEntry = fields[1].Get<uint32>();
    TimeSeg = fields[2].Get<uint32>();
    const auto position = fields[3].GetArray<float, 4>();
    X = position[0];
    Y = position[1];
    Z = position[2];
    W = position[3];
}

VehicleEntry::VehicleEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Flags = fields[1].Get<uint32>();
    std::ranges::copy(fields[2].GetArray<uint32, MAX_VEHICLE_SEATS>(), SeatID);
    PowerDisplayID = fields[3].Get<uint32>();
}

VehicleSeatEntry::VehicleSeatEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    Flags = fields[1].Get<uint32>();
    FlagsB = fields[2].Get<uint32>();
    const auto attachmentOffset = fields[3].GetArray<float, 3>();
    AttachmentOffsetX = attachmentOffset[0];
    AttachmentOffsetY = attachmentOffset[1];
    AttachmentOffsetZ = attachmentOffset[2];
}

bool VehicleSeatEntry::CanEnterOrExit() const
{
    // If it has animation for enter/ride, means it can be entered/exited by logic
    return Flags & VEHICLE_SEAT_FLAG_CAN_ENTER_OR_EXIT ||
        Flags & (VEHICLE_SEAT_FLAG_HAS_LOWER_ANIM_FOR_ENTER | VEHICLE_SEAT_FLAG_HAS_LOWER_ANIM_FOR_RIDE);
}

bool VehicleSeatEntry::CanSwitchFromSeat() const
{
    return Flags & VEHICLE_SEAT_FLAG_CAN_SWITCH;
}

bool VehicleSeatEntry::IsUsableByOverride() const
{
    return Flags & (VEHICLE_SEAT_FLAG_UNCONTROLLED | VEHICLE_SEAT_FLAG_UNK18) ||
        FlagsB & (
            VEHICLE_SEAT_FLAG_B_USABLE_FORCED | VEHICLE_SEAT_FLAG_B_USABLE_FORCED_2 |
            VEHICLE_SEAT_FLAG_B_USABLE_FORCED_3 | VEHICLE_SEAT_FLAG_B_USABLE_FORCED_4);
}

bool VehicleSeatEntry::IsEjectable() const
{
    return FlagsB & VEHICLE_SEAT_FLAG_B_EJECTABLE;
}

bool VehicleSeatEntry::CanControl() const
{
    return Flags & VEHICLE_SEAT_FLAG_CAN_CONTROL;
}

WMOAreaTableEntry::WMOAreaTableEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    int i = 0;
    ID = fields[i++].Get<uint32>();
    RootID = fields[i++].Get<uint16>();
    AdtID = fields[i++].Get<uint8>();
    GroupID = fields[i++].Get<int32>();
    Flags = fields[i++].Get<uint32>();
    AreaID = fields[i].Get<uint32>();
}

WorldMapAreaEntry::WorldMapAreaEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    int i = 0;
    ID = fields[i++].Get<uint32>();
    MapID = fields[i++].Get<uint32>();
    LeftCoord = fields[i++].Get<float>();
    RightCoord = fields[i++].Get<float>();
    TopCoord = fields[i++].Get<float>();
    BottomCoord = fields[i++].Get<float>();
    VirtualMapID = fields[i++].Get<int32>();
}

WorldMapOverlayEntry::WorldMapOverlayEntry(const QueryResult& result)
{
    const Field* fields = result->Fetch();
    ID = fields[0].Get<uint32>();
    std::ranges::copy(fields[1].GetArray<uint32, MAX_WORLD_MAP_OVERLAY_AREA_IDX>(), AreaID);
}
