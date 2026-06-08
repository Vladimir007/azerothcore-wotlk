#ifndef DBC_STORES_H
#define DBC_STORES_H

#include <list>
#include <map>
#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "DBCDefines.h"
#include "DBCStorage.h"
#include "DBCStructure.h"

static constexpr std::size_t TaxiMaskSize = 14;

typedef std::list<uint32> SimpleFactionsList;
typedef std::map<uint32, MapDifficulty> MapDifficultyMap;
typedef std::unordered_multimap<uint32, const SkillRaceClassInfoEntry*> SkillRaceClassInfoMap;
typedef std::pair<SkillRaceClassInfoMap::iterator, SkillRaceClassInfoMap::iterator> SkillRaceClassInfoBounds;
typedef std::unordered_map<uint32 /* SkillLine */, std::vector<const SkillLineAbilityEntry*>> SkillLineAbilityIndexBySkillLine;
typedef std::set<std::pair<bool, uint32>> SpellCategorySet;
typedef std::unordered_map<uint32, SpellCategorySet> SpellCategoryStore;
typedef std::set<uint32> PetFamilySpellsSet;
typedef std::map<uint32, PetFamilySpellsSet> PetFamilySpellsStore;
typedef std::map<uint32, TaxiPathEntry const*> TaxiPathSetForSource;
typedef std::map<uint32, TaxiPathSetForSource> TaxiPathSetBySource;
typedef std::vector<TaxiPathNodeEntry const*> TaxiPathNodeList;
typedef std::vector<TaxiPathNodeList> TaxiPathNodesByPath;
typedef std::array<uint32, TaxiMaskSize> TaxiMask;

extern DBCStorage<AchievementEntry>                   sAchievementStore;
extern DBCStorage<AchievementCategoryEntry>           sAchievementCategoryStore;
extern DBCStorage<AchievementCriteriaEntry>           sAchievementCriteriaStore;
extern DBCStorage<AreaTableEntry>                     sAreaTableStore;
extern DBCStorage<AreaGroupEntry>                     sAreaGroupStore;
extern DBCStorage<AuctionHouseEntry>                  sAuctionHouseStore;
extern DBCStorage<BankBagSlotPricesEntry>             sBankBagSlotPricesStore;
extern DBCStorage<BarberShopStyleEntry>               sBarberShopStyleStore;
extern DBCStorage<BattlemasterListEntry>              sBattlemasterListStore;
extern DBCStorage<CharStartOutfitEntry>               sCharStartOutfitStore;
extern DBCStorage<CharTitlesEntry>                    sCharTitlesStore;
extern DBCStorage<ChatChannelsEntry>                  sChatChannelsStore;
extern DBCStorage<ChrClassesEntry>                    sChrClassesStore;
extern DBCStorage<ChrRacesEntry>                      sChrRacesStore;
extern DBCStorage<CinematicCameraEntry>               sCinematicCameraStore;
extern DBCStorage<CinematicSequencesEntry>            sCinematicSequencesStore;
extern DBCStorage<CreatureDisplayInfoEntry>           sCreatureDisplayInfoStore;
extern DBCStorage<CreatureDisplayInfoExtraEntry>      sCreatureDisplayInfoExtraStore;
extern DBCStorage<CreatureFamilyEntry>                sCreatureFamilyStore;
extern DBCStorage<CreatureModelDataEntry>             sCreatureModelDataStore;
extern DBCStorage<CreatureSpellDataEntry>             sCreatureSpellDataStore;
extern DBCStorage<CreatureTypeEntry>                  sCreatureTypeStore;
extern DBCStorage<CurrencyTypesEntry>                 sCurrencyTypesStore;
extern DBCStorage<DestructibleModelDataEntry>         sDestructibleModelDataStore;
extern DBCStorage<DungeonEncounterEntry>              sDungeonEncounterStore;
extern DBCStorage<DurabilityCostsEntry>               sDurabilityCostsStore;
extern DBCStorage<DurabilityQualityEntry>             sDurabilityQualityStore;
extern DBCStorage<EmotesEntry>                        sEmotesStore;
extern DBCStorage<EmotesTextEntry>                    sEmotesTextStore;
extern DBCStorage<FactionEntry>                       sFactionStore;
extern DBCStorage<FactionTemplateEntry>               sFactionTemplateStore;
extern DBCStorage<GameObjectArtKitEntry>              sGameObjectArtKitStore;
extern DBCStorage<GameObjectDisplayInfoEntry>         sGameObjectDisplayInfoStore;
extern DBCStorage<GemPropertiesEntry>                 sGemPropertiesStore;
extern DBCStorage<GlyphPropertiesEntry>               sGlyphPropertiesStore;
extern DBCStorage<GlyphSlotEntry>                     sGlyphSlotStore;
extern DBCStorage<GtBarberShopCostBaseEntry>          sGtBarberShopCostBaseStore;
extern DBCStorage<GtCombatRatingsEntry>               sGtCombatRatingsStore;
extern DBCStorage<GtChanceToMeleeCritBaseEntry>       sGtChanceToMeleeCritBaseStore;
extern DBCStorage<GtChanceToMeleeCritEntry>           sGtChanceToMeleeCritStore;
extern DBCStorage<GtChanceToSpellCritBaseEntry>       sGtChanceToSpellCritBaseStore;
extern DBCStorage<GtChanceToSpellCritEntry>           sGtChanceToSpellCritStore;
extern DBCStorage<GtNPCManaCostScalerEntry>           sGtNPCManaCostScalerStore;
extern DBCStorage<GtOCTClassCombatRatingScalarEntry>  sGtOCTClassCombatRatingScalarStore;
extern DBCStorage<GtOCTRegenHPEntry>                  sGtOCTRegenHPStore;
extern DBCStorage<GtRegenHPPerSptEntry>               sGtRegenHPPerSptStore;
extern DBCStorage<GtRegenMPPerSptEntry>               sGtRegenMPPerSptStore;
extern DBCStorage<HolidaysEntry>                      sHolidaysStore;
extern DBCStorage<ItemBagFamilyEntry>                 sItemBagFamilyStore;
extern DBCStorage<ItemEntry>                          sItemStore;
extern DBCStorage<ItemExtendedCostEntry>              sItemExtendedCostStore;
extern DBCStorage<ItemLimitCategoryEntry>             sItemLimitCategoryStore;
extern DBCStorage<ItemRandomPropertiesEntry>          sItemRandomPropertiesStore;
extern DBCStorage<ItemRandomSuffixEntry>              sItemRandomSuffixStore;
extern DBCStorage<ItemSetEntry>                       sItemSetStore;
extern DBCStorage<LFGDungeonEntry>                    sLFGDungeonStore;
extern DBCStorage<LiquidTypeEntry>                    sLiquidTypeStore;
extern DBCStorage<LockEntry>                          sLockStore;
extern DBCStorage<MailTemplateEntry>                  sMailTemplateStore;
extern DBCStorage<MapEntry>                           sMapStore;
extern DBCStorage<MovieEntry>                         sMovieStore;
extern DBCStorage<NamesReservedEntry>                 sNamesReservedStore;
extern DBCStorage<NamesProfanityEntry>                sNamesProfanityStore;
extern DBCStorage<OverrideSpellDataEntry>             sOverrideSpellDataStore;
extern DBCStorage<PowerDisplayEntry>                  sPowerDisplayStore;
extern DBCStorage<QuestSortEntry>                     sQuestSortStore;
extern DBCStorage<QuestXPEntry>                       sQuestXPStore;
extern DBCStorage<QuestFactionRewEntry>               sQuestFactionRewardStore;
extern DBCStorage<RandomPropertiesPointsEntry>        sRandomPropertiesPointsStore;
extern DBCStorage<ScalingStatDistributionEntry>       sScalingStatDistributionStore;
extern DBCStorage<ScalingStatValuesEntry>             sScalingStatValuesStore;
extern DBCStorage<SkillLineEntry>                     sSkillLineStore;
extern DBCStorage<SkillLineAbilityEntry>              sSkillLineAbilityStore;
extern DBCStorage<SkillTiersEntry>                    sSkillTiersStore;
extern DBCStorage<SoundEntriesEntry>                  sSoundEntriesStore;
extern DBCStorage<SpellCastTimesEntry>                sSpellCastTimesStore;
extern DBCStorage<SpellCategoryEntry>                 sSpellCategoryStore;
extern DBCStorage<SpellDifficultyEntry>               sSpellDifficultyStore;
extern DBCStorage<SpellDurationEntry>                 sSpellDurationStore;
extern DBCStorage<SpellFocusObjectEntry>              sSpellFocusObjectStore;
extern DBCStorage<SpellItemEnchantmentEntry>          sSpellItemEnchantmentStore;
extern DBCStorage<SpellItemEnchantmentConditionEntry> sSpellItemEnchantmentConditionStore;
extern DBCStorage<SpellRadiusEntry>                   sSpellRadiusStore;
extern DBCStorage<SpellRangeEntry>                    sSpellRangeStore;
extern DBCStorage<SpellRuneCostEntry>                 sSpellRuneCostStore;
extern DBCStorage<SpellShapeshiftFormEntry>           sSpellShapeshiftFormStore;
extern DBCStorage<SpellEntry>                         sSpellStore;
extern DBCStorage<SpellVisualEntry>                   sSpellVisualStore;
extern DBCStorage<StableSlotPricesEntry>              sStableSlotPricesStore;
extern DBCStorage<SummonPropertiesEntry>              sSummonPropertiesStore;
extern DBCStorage<TalentEntry>                        sTalentStore;
extern DBCStorage<TalentTabEntry>                     sTalentTabStore;
extern DBCStorage<TaxiNodesEntry>                     sTaxiNodesStore;
extern DBCStorage<TaxiPathEntry>                      sTaxiPathStore;
extern DBCStorage<TeamContributionPointsEntry>        sTeamContributionPointsStore;
extern DBCStorage<TotemCategoryEntry>                 sTotemCategoryStore;
extern DBCStorage<VehicleEntry>                       sVehicleStore;
extern DBCStorage<VehicleSeatEntry>                   sVehicleSeatStore;
extern DBCStorage<WMOAreaTableEntry>                  sWMOAreaTableStore;
extern DBCStorage<WorldMapOverlayEntry>               sWorldMapOverlayStore;

extern MapDifficultyMap sMapDifficultyMap;
extern SkillLineAbilityIndexBySkillLine sSkillLineAbilityIndexBySkillLine;
extern SpellCategoryStore sSpellsByCategoryStore;
extern PetFamilySpellsStore sPetFamilySpellsStore;
extern std::unordered_set<uint32> sPetTalentSpells;

extern TaxiMask sTaxiNodesMask;
extern TaxiMask sOldContinentsNodesMask;
extern TaxiMask sHordeTaxiNodesMask;
extern TaxiMask sAllianceTaxiNodesMask;
extern TaxiMask sDeathKnightTaxiNodesMask;

extern TaxiPathSetBySource sTaxiPathSetBySource;
extern TaxiPathNodesByPath sTaxiPathNodesByPath;

void LoadDBCStores();

const SimpleFactionsList* GetFactionTeamList(uint32 faction);
std::string GetPetName(uint32 petFamily);
uint32 GetTalentSpellCost(uint32 spellID);
const TalentSpellPos* GetTalentSpellPos(uint32 spellID);
const WMOAreaTableEntry* GetWMOAreaTableEntryByTriple(int32 rootID, int32 adtID, int32 groupID);
uint32 GetVirtualMapForMapAndZone(uint32 mapID, uint32 zoneID);
ContentLevels GetContentLevelsForMapAndZone(uint32 mapID, uint32 zoneID);
void Zone2MapCoordinates(float& x, float& y, uint32 zone);
void Map2ZoneCoordinates(float& x, float& y, uint32 zone);
const MapDifficulty* GetMapDifficultyData(uint32 mapID, Difficulty difficulty);
const MapDifficulty* GetDownscaledMapDifficultyData(uint32 mapID, Difficulty& difficulty);
bool IsSharedDifficultyMap(uint32 mapID);
const uint32* GetTalentTabPages(uint8 cls);
uint32 GetLiquidFlags(uint32 liquidType);
const PvPDifficultyEntry* GetBattlegroundBracketByLevel(uint32 mapID, uint32 level);
const PvPDifficultyEntry* GetBattlegroundBracketById(uint32 mapID, BattlegroundBracketID id);
const CharStartOutfitEntry* GetCharStartOutfitEntry(uint8 race, uint8 class_, uint8 gender);
const LFGDungeonEntry* GetLFGDungeon(uint32 mapID, Difficulty difficulty);
const LFGDungeonEntry* GetZoneLFGDungeonEntry(const std::string& zoneName);
uint32 GetDefaultMapLight(uint32 mapID);
const SkillRaceClassInfoEntry* GetSkillRaceClassInfo(uint32 skill, uint8 race, uint8 class_);
const std::vector<const SkillLineAbilityEntry*>& GetSkillLineAbilitiesBySkillLine(uint32 skillLine);

#endif
