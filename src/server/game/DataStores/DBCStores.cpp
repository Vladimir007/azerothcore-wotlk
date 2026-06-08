#include "DBCStores.h"

#include <map>

#include "BattlegroundMgr.h"
#include "Errors.h"
#include "LFGMgr.h"
#include "Log.h"
#include "SharedDefines.h"
#include "SpellMgr.h"
#include "TransportMgr.h"
#include "World.h"

typedef std::map<uint16, uint32> AreaFlagByAreaID;
typedef std::map<uint32, uint32> AreaFlagByMapID;
typedef std::map<uint32, SimpleFactionsList> FactionTeamMap;
typedef std::tuple<uint16, uint8, int32> WMOAreaTableKey;
typedef std::map<WMOAreaTableKey, const WMOAreaTableEntry*> WMOAreaInfoByTriple;
typedef std::list<std::string> StoreProblemList;
typedef std::map<uint32, TalentSpellPos> TalentSpellPosMap;

static WMOAreaInfoByTriple sWMOAreaInfoByTriple;
static FactionTeamMap sFactionTeamMap;
static uint32 sTalentTabPages[MAX_CLASSES][3];  // Store absolute bit position for first rank for talent inspect

std::map<uint32, const CharStartOutfitEntry*> sCharStartOutfitMap;

DBCStorage<AchievementEntry>                   sAchievementStore;
DBCStorage<AchievementCategoryEntry>           sAchievementCategoryStore;
DBCStorage<AchievementCriteriaEntry>           sAchievementCriteriaStore;
DBCStorage<AreaTableEntry>                     sAreaTableStore;
DBCStorage<AreaGroupEntry>                     sAreaGroupStore;
DBCStorage<AuctionHouseEntry>                  sAuctionHouseStore;
DBCStorage<BankBagSlotPricesEntry>             sBankBagSlotPricesStore;
DBCStorage<BattlemasterListEntry>              sBattlemasterListStore;
DBCStorage<BarberShopStyleEntry>               sBarberShopStyleStore;
DBCStorage<CharStartOutfitEntry>               sCharStartOutfitStore;
DBCStorage<CharTitlesEntry>                    sCharTitlesStore;
DBCStorage<ChatChannelsEntry>                  sChatChannelsStore;
DBCStorage<ChrClassesEntry>                    sChrClassesStore;
DBCStorage<ChrRacesEntry>                      sChrRacesStore;
DBCStorage<CinematicCameraEntry>               sCinematicCameraStore;
DBCStorage<CinematicSequencesEntry>            sCinematicSequencesStore;
DBCStorage<CreatureDisplayInfoEntry>           sCreatureDisplayInfoStore;
DBCStorage<CreatureDisplayInfoExtraEntry>      sCreatureDisplayInfoExtraStore;
DBCStorage<CreatureFamilyEntry>                sCreatureFamilyStore;
DBCStorage<CreatureModelDataEntry>             sCreatureModelDataStore;
DBCStorage<CreatureSpellDataEntry>             sCreatureSpellDataStore;
DBCStorage<CreatureTypeEntry>                  sCreatureTypeStore;
DBCStorage<CurrencyTypesEntry>                 sCurrencyTypesStore;
DBCStorage<DestructibleModelDataEntry>         sDestructibleModelDataStore;
DBCStorage<DungeonEncounterEntry>              sDungeonEncounterStore;
DBCStorage<DurabilityQualityEntry>             sDurabilityQualityStore;
DBCStorage<DurabilityCostsEntry>               sDurabilityCostsStore;
DBCStorage<EmotesEntry>                        sEmotesStore;
DBCStorage<EmotesTextEntry>                    sEmotesTextStore;
DBCStorage<FactionEntry>                       sFactionStore;
DBCStorage<FactionTemplateEntry>               sFactionTemplateStore;
DBCStorage<GameObjectArtKitEntry>              sGameObjectArtKitStore;
DBCStorage<GameObjectDisplayInfoEntry>         sGameObjectDisplayInfoStore;
DBCStorage<GemPropertiesEntry>                 sGemPropertiesStore;
DBCStorage<GlyphPropertiesEntry>               sGlyphPropertiesStore;
DBCStorage<GlyphSlotEntry>                     sGlyphSlotStore;
DBCStorage<GtBarberShopCostBaseEntry>          sGtBarberShopCostBaseStore;
DBCStorage<GtCombatRatingsEntry>               sGtCombatRatingsStore;
DBCStorage<GtChanceToMeleeCritBaseEntry>       sGtChanceToMeleeCritBaseStore;
DBCStorage<GtChanceToMeleeCritEntry>           sGtChanceToMeleeCritStore;
DBCStorage<GtChanceToSpellCritBaseEntry>       sGtChanceToSpellCritBaseStore;
DBCStorage<GtChanceToSpellCritEntry>           sGtChanceToSpellCritStore;
DBCStorage<GtNPCManaCostScalerEntry>           sGtNPCManaCostScalerStore;
DBCStorage<GtOCTClassCombatRatingScalarEntry>  sGtOCTClassCombatRatingScalarStore;
DBCStorage<GtOCTRegenHPEntry>                  sGtOCTRegenHPStore;
DBCStorage<GtRegenHPPerSptEntry>               sGtRegenHPPerSptStore;
DBCStorage<GtRegenMPPerSptEntry>               sGtRegenMPPerSptStore;
DBCStorage<HolidaysEntry>                      sHolidaysStore;
DBCStorage<ItemEntry>                          sItemStore;
DBCStorage<ItemBagFamilyEntry>                 sItemBagFamilyStore;
DBCStorage<ItemExtendedCostEntry>              sItemExtendedCostStore;
DBCStorage<ItemLimitCategoryEntry>             sItemLimitCategoryStore;
DBCStorage<ItemRandomPropertiesEntry>          sItemRandomPropertiesStore;
DBCStorage<ItemRandomSuffixEntry>              sItemRandomSuffixStore;
DBCStorage<ItemSetEntry>                       sItemSetStore;
DBCStorage<LFGDungeonEntry>                    sLFGDungeonStore;
DBCStorage<LightEntry>                         sLightStore;
DBCStorage<LiquidTypeEntry>                    sLiquidTypeStore;
DBCStorage<LockEntry>                          sLockStore;
DBCStorage<MailTemplateEntry>                  sMailTemplateStore;
DBCStorage<MapEntry>                           sMapStore;
DBCStorage<MapDifficultyEntry>                 sMapDifficultyStore; // Used only for initialization sMapDifficultyMap at startup.
DBCStorage<MovieEntry>                         sMovieStore;
DBCStorage<NamesReservedEntry>                 sNamesReservedStore;
DBCStorage<NamesProfanityEntry>                sNamesProfanityStore;
DBCStorage<OverrideSpellDataEntry>             sOverrideSpellDataStore;
DBCStorage<PowerDisplayEntry>                  sPowerDisplayStore;
DBCStorage<PvPDifficultyEntry>                 sPvPDifficultyStore;
DBCStorage<QuestSortEntry>                     sQuestSortStore;
DBCStorage<QuestXPEntry>                       sQuestXPStore;
DBCStorage<QuestFactionRewEntry>               sQuestFactionRewardStore;
DBCStorage<RandomPropertiesPointsEntry>        sRandomPropertiesPointsStore;
DBCStorage<ScalingStatDistributionEntry>       sScalingStatDistributionStore;
DBCStorage<ScalingStatValuesEntry>             sScalingStatValuesStore;
DBCStorage<SkillLineEntry>                     sSkillLineStore;
DBCStorage<SkillLineAbilityEntry>              sSkillLineAbilityStore;
DBCStorage<SkillRaceClassInfoEntry>            sSkillRaceClassInfoStore;
DBCStorage<SkillTiersEntry>                    sSkillTiersStore;
DBCStorage<SoundEntriesEntry>                  sSoundEntriesStore;
DBCStorage<SpellItemEnchantmentEntry>          sSpellItemEnchantmentStore;
DBCStorage<SpellItemEnchantmentConditionEntry> sSpellItemEnchantmentConditionStore;
DBCStorage<SpellEntry>                         sSpellStore;
DBCStorage<SpellCastTimesEntry>                sSpellCastTimesStore;
DBCStorage<SpellCategoryEntry>                 sSpellCategoryStore;
DBCStorage<SpellDifficultyEntry>               sSpellDifficultyStore;
DBCStorage<SpellDurationEntry>                 sSpellDurationStore;
DBCStorage<SpellFocusObjectEntry>              sSpellFocusObjectStore;
DBCStorage<SpellRadiusEntry>                   sSpellRadiusStore;
DBCStorage<SpellRangeEntry>                    sSpellRangeStore;
DBCStorage<SpellRuneCostEntry>                 sSpellRuneCostStore;
DBCStorage<SpellShapeshiftFormEntry>           sSpellShapeshiftFormStore;
DBCStorage<SpellVisualEntry>                   sSpellVisualStore;
DBCStorage<StableSlotPricesEntry>              sStableSlotPricesStore;
DBCStorage<SummonPropertiesEntry>              sSummonPropertiesStore;
DBCStorage<TalentEntry>                        sTalentStore;
DBCStorage<TalentTabEntry>                     sTalentTabStore;
DBCStorage<TaxiNodesEntry>                     sTaxiNodesStore;
DBCStorage<TaxiPathEntry>                      sTaxiPathStore;  // Used only for initialization sTaxiPathSetBySource at startup.
DBCStorage<TaxiPathNodeEntry>                  sTaxiPathNodeStore;  // Used only for initialization sTaxiPathNodeStore at startup.
DBCStorage<TeamContributionPointsEntry>        sTeamContributionPointsStore;
DBCStorage<TotemCategoryEntry>                 sTotemCategoryStore;
DBCStorage<TransportAnimationEntry>            sTransportAnimationStore;
DBCStorage<TransportRotationEntry>             sTransportRotationStore;
DBCStorage<VehicleEntry>                       sVehicleStore;
DBCStorage<VehicleSeatEntry>                   sVehicleSeatStore;
DBCStorage<WMOAreaTableEntry>                  sWMOAreaTableStore;
DBCStorage<WorldMapAreaEntry>                  sWorldMapAreaStore;
DBCStorage<WorldMapOverlayEntry>               sWorldMapOverlayStore;

MapDifficultyMap sMapDifficultyMap;
SkillLineAbilityIndexBySkillLine sSkillLineAbilityIndexBySkillLine;
SkillRaceClassInfoMap SkillRaceClassInfoBySkill;
SpellCategoryStore sSpellsByCategoryStore;
PetFamilySpellsStore sPetFamilySpellsStore;
TalentSpellPosMap sTalentSpellPosMap;
std::unordered_set<uint32> sPetTalentSpells;

TaxiMask sTaxiNodesMask;
TaxiMask sOldContinentsNodesMask;
TaxiMask sHordeTaxiNodesMask;
TaxiMask sAllianceTaxiNodesMask;
TaxiMask sDeathKnightTaxiNodesMask;

TaxiPathSetBySource sTaxiPathSetBySource;
TaxiPathNodesByPath sTaxiPathNodesByPath;

uint32 DBCFileCount = 0;

template<class T>
void LoadDBC(DBCStorage<T>& storage, const std::string& table, const std::string& fields, const std::string& ordering="id")
{
    ++DBCFileCount;
    storage.Load(table, fields, ordering);
}

void LoadDBCStores()
{
    const uint32 oldMSTime = getMSTime();

    LoadDBC(sAchievementStore, "dbc_achievement", "id, faction, map, name, category, points, flags, min_criteria_demanded, linked_achievement");
    LoadDBC(sAchievementCategoryStore, "dbc_achievement_category", "id, parent");
    LoadDBC(sAchievementCriteriaStore, "dbc_achievement_criterion",
        "id, achievement, type, required_asset, required_amount, start_type, start_asset, "
        "fail_type, fail_asset, flags, timer_type, timer_asset, timer_time");
    LoadDBC(sAreaTableStore, "dbc_area",
        "id, map, parent, explore_flag, flags, area_level, name, faction_group, liquid_water, liquid_ocean, liquid_magma, liquid_slime");
    LoadDBC(sAreaGroupStore, "dbc_area_group", "id, areas, group_next");
    LoadDBC(sAuctionHouseStore, "dbc_auction_house", "id, faction, deposit_rate, auction_cut");
    LoadDBC(sBankBagSlotPricesStore, "dbc_bank_bag_slot_price", "id, price");
    LoadDBC(sBarberShopStyleStore, "dbc_barber_shop_style", "id, type, race, gender, data");
    LoadDBC(sBattlemasterListStore, "dbc_battlemaster", "id, maps, instance_type, name, max_group_size, holiday_world_state");
    LoadDBC(sCharStartOutfitStore, "dbc_character_start_outfit", "id, race, character_class, gender, items");
    LoadDBC(sCharTitlesStore, "dbc_character_title", "id, name_male, name_female, bit_index");
    LoadDBC(sChatChannelsStore, "dbc_chat_channel", "id, flags, name");
    LoadDBC(sChrClassesStore, "dbc_character_class", "id, power_type, spell_class_set, cinematic_sequence, required_expansion");
    LoadDBC(sChrRacesStore, "dbc_character_race",
        "id, flags, faction_template, male_display, female_display, base_language, cinematic_sequence, team, required_expansion");
    LoadDBC(sCinematicCameraStore, "dbc_cinematic_camera", "id, file, voiceover, origin, rotation");
    LoadDBC(sCinematicSequencesStore, "dbc_cinematic_sequence", "id, cameras");
    LoadDBC(sCreatureDisplayInfoStore, "dbc_creature_display_info", "id, creature_model, extra_display_info, scale");
    LoadDBC(sCreatureDisplayInfoExtraStore, "dbc_creature_display_info_extra", "id, race");
    LoadDBC(sCreatureFamilyStore, "dbc_creature_family",
        "id, min_scale, min_scale_level, max_scale, max_scale_level, skill_line1, skill_line2, pet_food_mask, pet_talent_type, name");
    LoadDBC(sCreatureModelDataStore, "dbc_creature_model_data", "id, flags, model_scale, collision_width, collision_height, mount_height");
    LoadDBC(sCreatureSpellDataStore, "dbc_creature_spell_data", "id, spells");
    LoadDBC(sCreatureTypeStore, "dbc_creature_type", "id");
    LoadDBC(sCurrencyTypesStore, "dbc_currency_type", "item, bit_index", "item");
    LoadDBC(sDestructibleModelDataStore, "dbc_destructible_model_data", "id, state1_wmo, state2_wmo, state3_wmo, repair_ground_fx");
    LoadDBC(sDungeonEncounterStore, "dbc_dungeon_encounter", "id, map, difficulty, bit, name");
    LoadDBC(sDurabilityCostsStore, "dbc_durability_cost", "id, weapon_subclass_cost, armor_subclass_cost");
    LoadDBC(sDurabilityQualityStore, "dbc_durability_quality", "id, quality");
    LoadDBC(sEmotesStore, "dbc_emote", "id, flags, spec_proc, spec_proc_param");
    LoadDBC(sEmotesTextStore, "dbc_emote_texts", "id, emote");
    LoadDBC(sFactionStore, "dbc_faction",
        "id, reputation_index, reputation_race_mask, reputation_class_mask, reputation_base, "
        "reputation_flags, parent, spillover_rate_in, spillover_rate_out, spillover_max_rank_in, name");
    LoadDBC(sFactionTemplateStore, "dbc_faction_template", "id, faction, flags, faction_group, friend_group, enemy_group, enemies, friends");
    LoadDBC(sGameObjectArtKitStore, "dbc_game_object_art_kit", "id");
    LoadDBC(sGameObjectDisplayInfoStore, "dbc_game_object_display_info", "id, geo_box_min, geo_box_max");
    LoadDBC(sGemPropertiesStore, "dbc_gem_properties", "id, enchantment, type");
    LoadDBC(sGlyphPropertiesStore, "dbc_glyph_properties", "id, spell, type_flags");
    LoadDBC(sGlyphSlotStore, "dbc_glyph_slot", "id, type, order");
    LoadDBC(sGtBarberShopCostBaseStore, "dbc_barber_shop_cost_base", "id, cost");
    LoadDBC(sGtCombatRatingsStore, "dbc_combat_rating", "id, ratio");
    LoadDBC(sGtChanceToMeleeCritBaseStore, "dbc_chance_to_melee_crit_base", "id, base");
    LoadDBC(sGtChanceToMeleeCritStore, "dbc_chance_to_melee_crit", "id, ratio");
    LoadDBC(sGtChanceToSpellCritBaseStore, "dbc_chance_to_spell_crit_base", "id, base");
    LoadDBC(sGtChanceToSpellCritStore, "dbc_chance_to_spell_crit", "id, ratio");
    LoadDBC(sGtNPCManaCostScalerStore, "dbc_npc_mana_cost_scaler", "id, ratio");
    LoadDBC(sGtOCTClassCombatRatingScalarStore, "dbc_oct_class_combat_rating_scalar", "id, ratio");
    LoadDBC(sGtOCTRegenHPStore, "dbc_oct_regen_hp", "id, ratio");
    LoadDBC(sGtRegenHPPerSptStore, "dbc_regen_hp_per_spirit", "id, ratio");
    LoadDBC(sGtRegenMPPerSptStore, "dbc_regen_mp_per_spirit", "id, ratio");
    LoadDBC(sHolidaysStore, "dbc_holiday",
        "id, stage_durations, dates, region, looping, calendar_flags, texture_filename, priority, filter_type");
    LoadDBC(sItemStore, "dbc_item",
        "id, item_class, item_subclass, sound_override_subclass, material, display_info, inventory_type, sheath_type");
    LoadDBC(sItemBagFamilyStore, "dbc_item_bag_family", "id");
    LoadDBC(sItemExtendedCostStore, "dbc_item_extended_cost",
        "id, honor_points, arena_points, arena_bracket, required_items, required_items_counts, required_arena_rating");
    LoadDBC(sItemLimitCategoryStore, "dbc_item_limit_category", "id, quantity, type");
    LoadDBC(sItemRandomPropertiesStore, "dbc_item_random_property", "id, suffix, spell_item_enchantment");
    LoadDBC(sItemRandomSuffixStore, "dbc_item_random_suffix", "id, name, enchantments, allocation_pct");
    LoadDBC(sItemSetStore, "dbc_item_set", "id, name, items, set_spells, set_thresholds, required_skill, required_skill_rank");
    LoadDBC(sLFGDungeonStore, "dbc_lfg_dungeon",
        "id, name, level_min, level_max, target_level, target_level_min, target_level_max, map, difficulty, flags, type, expansion, group");
    LoadDBC(sLightStore, "dbc_light", "id, map, position");
    LoadDBC(sLiquidTypeStore, "dbc_liquid", "id, type, spell");
    LoadDBC(sLockStore, "dbc_lock", "id, types, indexes, skills");
    LoadDBC(sMailTemplateStore, "dbc_mail_template", "id, body");
    LoadDBC(sMapStore, "dbc_map", "id, type, flags, name, area, entrance_map, entrance_position, expansion, max_players");
    LoadDBC(sMapDifficultyStore, "dbc_map_difficulty", "id, map, difficulty, message, raid_duration, max_players");
    LoadDBC(sMovieStore, "dbc_movie", "id");
    LoadDBC(sNamesReservedStore, "dbc_name_reserved", "id, pattern");
    LoadDBC(sNamesProfanityStore, "dbc_name_profanity", "id, name");
    LoadDBC(sOverrideSpellDataStore, "dbc_override_spell_data", "id, spells");
    LoadDBC(sPowerDisplayStore, "dbc_power_display", "id, type");
    LoadDBC(sPvPDifficultyStore, "dbc_pvp_difficulty", "id, map, bracket, min_level, max_level, difficulty");
    LoadDBC(sQuestFactionRewardStore, "dbc_quest_faction_reward", "id, values");
    LoadDBC(sQuestSortStore, "dbc_quest_sort", "id");
    LoadDBC(sQuestXPStore, "dbc_quest_xp", "id, values");
    LoadDBC(sRandomPropertiesPointsStore, "dbc_rand_prop_points", "id, epic, superior, good");
    LoadDBC(sScalingStatDistributionStore, "dbc_scaling_stat_distribution", "id, types, bonuses, max_level");
    LoadDBC(sScalingStatValuesStore, "dbc_scaling_stat_values",
        "level, multiplier, shoulder_modifiers, dps_modifiers, spell_power, primary_mul, tertiary_mul, armor_modifiers", "level");
    LoadDBC(sSkillLineStore, "dbc_skill_line", "id, category, name, can_link");
    LoadDBC(sSkillLineAbilityStore, "dbc_skill_line_ability",
        "id, skill_line, spell, race_mask, class_mask, min_skill_line_rank, superseded_by_spell, "
        "acquire_method, trivial_skill_line_rank_high, trivial_skill_line_rank_low");
    LoadDBC(sSkillRaceClassInfoStore, "dbc_skill_race_class_info", "id, skill_line, race_mask, class_mask, flags, skill_tier");
    LoadDBC(sSkillTiersStore, "dbc_skill_tier", "id, values");
    LoadDBC(sSoundEntriesStore, "dbc_sound", "id");
    LoadDBC(sSpellStore, "dbc_spell",
        "id, spell_category, dispel_type, mechanic, "
        "attributes0, attributes1, attributes2, attributes3, attributes4, attributes5, attributes6, attributes7, "
        "stances, excluded_stances, target, target_creature_type, spell_focus_object, facing_caster_flags, "
        "caster_aura_state, target_aura_state, caster_aura_state_exclude, target_aura_state_exclude, "
        "caster_aura_spell, target_aura_spell, caster_aura_spell_exclude, target_aura_spell_exclude, "
        "cast_time, cooldown, cooldown_category, interrupt, aura_interrupt, channel_interrupt, "
        "proc_flags, proc_chance, proc_charges, max_level, min_level, level, duration, power_type, "
        "mana_cost, mana_cost_per_level, mana_per_second, mana_per_second_per_level, "
        "range, speed, max_stacks, totem0, totem1, reagents, reagents_counts, "
        "equipped_item_class, equipped_item_subclass, equipped_item_inventory_type_mask, "
        "effects_type, effects_die_sides, effects_real_points_per_level, effects_base_points, effects_mechanic, "
        "effects_implicit_targets_a, effects_implicit_targets_b, effects_radius, "
        "effects_apply_aura, effects_aura_period, effects_amplitude, effects_chain_targets, "
        "effects_item, effects_misc_value_a, effects_misc_value_b, effects_trigger_spell, effects_points_per_combo_point, "
        "effects_class_mask_a, effects_class_mask_b, effects_class_mask_c, "
        "effects_chain_amplitude, effects_bonus_multiplier, "
        "visual1, visual2, icon, icon_active, priority, name, subtitle, mana_cost_percentage, "
        "start_recovery_category, start_recovery_time, max_target_level, "
        "class_set, class_mask_a, class_mask_b, class_mask_c, "
        "max_affected_targets, defense_type, prevention_type, "
        "totem_category_a, totem_category_b, required_area_group, school_mask, rune_cost");

    LoadDBC(sSpellCastTimesStore, "dbc_spell_cast_time", "id, cast_time_base");
    LoadDBC(sSpellCategoryStore, "dbc_spell_category", "id, flags");
    LoadDBC(sSpellDifficultyStore, "dbc_spell_difficulty", "id, normal_10man, normal_25man, heroic_10man, heroic_25man");
    LoadDBC(sSpellDurationStore, "dbc_spell_duration", "id, duration, duration_per_level, max_duration");
    LoadDBC(sSpellFocusObjectStore, "dbc_spell_focus_object", "id");
    LoadDBC(sSpellItemEnchantmentStore, "dbc_spell_item_enchantment",
        "id, charges, types, min_amounts, param0, param1, param2, item_visual, flags, "
        "source_item, condition, required_skill, required_skill_rank, required_level");
    LoadDBC(sSpellItemEnchantmentConditionStore, "dbc_spell_item_enchantment_condition", "id, socket_color, operator, value_types, values");
    LoadDBC(sSpellRadiusStore, "dbc_spell_radius", "id, radius, radius_per_level, radius_max");
    LoadDBC(sSpellRangeStore, "dbc_spell_range", "id, min_range_hostile, min_range_friend, max_range_hostile, max_range_friend, flags");
    LoadDBC(sSpellRuneCostStore, "dbc_spell_rune_cost", "id, blood_rune_cost, unholy_rune_cost, frost_rune_cost, rune_power_gain");
    LoadDBC(sSpellShapeshiftFormStore, "dbc_spell_shapeshift_form", "id, flags, creature_type, attack_speed, displays, preset_spells");
    LoadDBC(sSpellVisualStore, "dbc_spell_visual", "id, has_missile, missile_model");
    LoadDBC(sStableSlotPricesStore, "dbc_stable_spot_price", "id, cost");
    LoadDBC(sSummonPropertiesStore, "dbc_summon_properties", "id, category, faction, type, slot, flags");
    LoadDBC(sTalentStore, "dbc_talent", "id, talent_tab, row, col, spell_ranks, prereq_talents, prereq_ranks, one_point");
    LoadDBC(sTalentTabStore, "dbc_talent_tab", "id, class_mask, pet_talent_mask, tab_page");
    LoadDBC(sTaxiNodesStore, "dbc_taxi_node", "id, map, position, name, mount_creature_alliance, mount_creature_horde");
    LoadDBC(sTaxiPathStore, "dbc_taxi_path", "id, from_node, to_node, cost");
    LoadDBC(sTaxiPathNodeStore, "dbc_taxi_path_node", "id, path, node_index, map, position, flags, delay, arrival_event, departure_event");
    LoadDBC(sTeamContributionPointsStore, "dbc_team_contribution_points", "id, value");
    LoadDBC(sTotemCategoryStore, "dbc_totem_category", "id, category, category_mask");
    LoadDBC(sTransportAnimationStore, "dbc_transport_animation", "id, transport, time_index, position");
    LoadDBC(sTransportRotationStore, "dbc_transport_rotation", "id, game_object, time_index, rotation");
    LoadDBC(sVehicleStore, "dbc_vehicle", "id, flags, seats, power_display");
    LoadDBC(sVehicleSeatStore, "dbc_vehicle_seat", "id, flags, flags_b, attachment_offset");
    LoadDBC(sWMOAreaTableStore, "dbc_wmo_area", "id, root_id, name_set, group_id, flags, area_table");
    LoadDBC(sWorldMapAreaStore, "dbc_world_map_area WHERE area_table > 0",
        "area_table, map, pos_left, pos_right, pos_top, pos_bottom, display_map", "area_table");
    LoadDBC(sWorldMapOverlayStore, "dbc_world_map_overlay", "id, areas");

    for (const CharStartOutfitEntry* outfit : sCharStartOutfitStore)
        sCharStartOutfitMap[outfit->Race | (outfit->Class << 8) | (outfit->Gender << 16)] = outfit;

    for (FactionEntry const* faction : sFactionStore)
    {
        if (faction->Team)
        {
            SimpleFactionsList& flist = sFactionTeamMap[faction->Team];
            flist.push_back(faction->ID);
        }
    }

    for (GameObjectDisplayInfoEntry const* info : sGameObjectDisplayInfoStore)
    {
        if (info->MaxX < info->MinX)
            std::swap(*const_cast<float*>(&info->MaxX), *const_cast<float*>(&info->MinX));
        if (info->MaxY < info->MinY)
            std::swap(*const_cast<float*>(&info->MaxY), *const_cast<float*>(&info->MinY));
        if (info->MaxZ < info->MinZ)
            std::swap(*const_cast<float*>(&info->MaxZ), *const_cast<float*>(&info->MinZ));
    }

    for (const MapDifficultyEntry* entry : sMapDifficultyStore)
        sMapDifficultyMap[MAKE_PAIR32(entry->MapID, entry->Difficulty)] = MapDifficulty(entry->ResetTime, entry->MaxPlayers, !entry->AreaTriggerText.empty());

    for (const PvPDifficultyEntry* entry : sPvPDifficultyStore)
        if (entry->BracketID > MAX_BATTLEGROUND_BRACKETS)
            ASSERT(false, "Need update MAX_BATTLEGROUND_BRACKETS by DBC data");

    for (const auto spell : sSpellStore)
        if (spell->Category)
            sSpellsByCategoryStore[spell->Category].emplace(false, spell->ID);

    for (SkillRaceClassInfoEntry const* entry : sSkillRaceClassInfoStore)
    {
        if (sSkillLineStore.LookupEntry(entry->SkillID))
            SkillRaceClassInfoBySkill.emplace(entry->SkillID, entry);
    }

    for (SkillLineAbilityEntry const* skillLine : sSkillLineAbilityStore)
    {
        if (const SpellEntry* spellEntry = sSpellStore.LookupEntry(skillLine->Spell); spellEntry && spellEntry->Attributes & SPELL_ATTR0_PASSIVE)
        {
            for (CreatureFamilyEntry const* cFamily : sCreatureFamilyStore)
            {
                if (skillLine->SkillLine != cFamily->SkillLine[0] && skillLine->SkillLine != cFamily->SkillLine[1])
                    continue;
                if (spellEntry->SpellLevel)
                    continue;
                if (skillLine->AcquireMethod != SKILL_LINE_ABILITY_LEARNED_ON_SKILL_LEARN)
                    continue;

                sPetFamilySpellsStore[cFamily->ID].insert(spellEntry->ID);
            }
        }
    }

    for (SkillLineAbilityEntry const* skillLine : sSkillLineAbilityStore)
        sSkillLineAbilityIndexBySkillLine[skillLine->SkillLine].push_back(skillLine);

    // Create SpellDifficulty searcher
    for (SpellDifficultyEntry const* spellDiff : sSpellDifficultyStore)
    {
        SpellDifficultyEntry newEntry;

        for (uint8 x = 0; x < MAX_DIFFICULTY; ++x)
        {
            if (spellDiff->SpellID[x] <= 0 || !sSpellStore.LookupEntry(spellDiff->SpellID[x]))
            {
                if (spellDiff->SpellID[x] > 0) // Don't show error if spell is <= 0, not all modes have spells and there are unknown negative values
                    LOG_ERROR("sql.sql", "SpellDifficulty.dbc: spell {} at field id: {} at SpellID {} does not exist in SpellStore (spell.dbc), loaded as 0", spellDiff->SpellID[x], spellDiff->ID, x);

                newEntry.SpellID[x] = 0; // Spell was <= 0 or invalid, set to 0
            }
            else
                newEntry.SpellID[x] = spellDiff->SpellID[x];
        }

        if (newEntry.SpellID[0] <= 0 || newEntry.SpellID[1] <= 0) // ID 0-1 must be always set!
            continue;

        for (uint8 x = 0; x < MAX_DIFFICULTY; ++x)
            if (newEntry.SpellID[x])
                sSpellMgr->SetSpellDifficultyId(static_cast<uint32>(newEntry.SpellID[x]), spellDiff->ID);
    }

    // Create talent spells set
    for (const TalentEntry* talentInfo : sTalentStore)
    {
        const TalentTabEntry* talentTab = sTalentTabStore.LookupEntry(talentInfo->TalentTab);

        for (uint8 j = 0; j < MAX_TALENT_RANK; ++j)
        {
            if (talentInfo->RankID[j])
            {
                sTalentSpellPosMap[talentInfo->RankID[j]] = TalentSpellPos(talentInfo->ID, j);

                if (talentTab && talentTab->PetTalentMask)
                    sPetTalentSpells.insert(talentInfo->RankID[j]);
            }
        }
    }

    // Prepare fast data access to bit pos of talent ranks for use at inspecting
    {
        // Now have all max ranks (and then bit amount used for store talent ranks in inspect)
        for (const TalentTabEntry* talentTabInfo : sTalentTabStore)
        {
            // Prevent memory corruption; otherwise cls will become 12 below
            if ((talentTabInfo->ClassMask & CLASS_MASK_ALL_PLAYABLE) == 0)
                continue;

            // Store class talent tab pages
            for (uint32 cls = 1; cls < MAX_CLASSES; ++cls)
                if (talentTabInfo->ClassMask & (1 << (cls - 1)))
                    sTalentTabPages[cls][talentTabInfo->TabPage] = talentTabInfo->ID;
        }
    }

    for (const TaxiPathEntry* entry : sTaxiPathStore)
        sTaxiPathSetBySource[entry->From][entry->To] = entry;

    // Calculate path nodes count
    const uint32 pathCount = sTaxiPathStore.GetNumRows();
    std::vector<uint32> pathLength;
    pathLength.resize(pathCount);  // 0 and some other indexes not used

    for (const TaxiPathNodeEntry* entry : sTaxiPathNodeStore)
        if (pathLength[entry->Path] < entry->Index + 1)
            pathLength[entry->Path] = entry->Index + 1;

    // Set path length
    sTaxiPathNodesByPath.resize(pathCount);  // 0 and some other indexes not used
    for (uint32 i = 1; i < sTaxiPathNodesByPath.size(); ++i)
        sTaxiPathNodesByPath[i].resize(pathLength[i]);

    // fill data
    for (const TaxiPathNodeEntry* entry : sTaxiPathNodeStore)
        sTaxiPathNodesByPath[entry->Path][entry->Index] = entry;

    // Initialize global TaxiNodes mask
    // Include existed nodes that have at least single not spell base (scripted) path
    {
        std::set<uint32> spellPaths;
        for (SpellEntry const* sInfo : sSpellStore)
            for (uint8 j = 0; j < MAX_SPELL_EFFECTS; ++j)
                if (sInfo->Effect[j] == SPELL_EFFECT_SEND_TAXI)
                    spellPaths.insert(sInfo->EffectMiscValue[j]);

        sTaxiNodesMask.fill(0);
        sOldContinentsNodesMask.fill(0);
        sHordeTaxiNodesMask.fill(0);
        sAllianceTaxiNodesMask.fill(0);
        sDeathKnightTaxiNodesMask.fill(0);

        for (const TaxiNodesEntry* node : sTaxiNodesStore)
        {
            if (!node)
                continue;

            TaxiPathSetBySource::const_iterator src_i = sTaxiPathSetBySource.find(node->ID);
            if (src_i != sTaxiPathSetBySource.end() && !src_i->second.empty())
            {
                bool ok = false;
                for (auto dest_i = src_i->second.begin(); dest_i != src_i->second.end(); ++dest_i)
                {
                    // Not spell path
                    if (dest_i->second->Price || !spellPaths.contains(dest_i->second->ID))
                    {
                        ok = true;
                        break;
                    }
                }

                if (!ok)
                    continue;
            }

            // Valid taxi network node
            const uint8 field = static_cast<uint8>((node->ID - 1) / 32);
            const uint32 subMask = 1 << ((node->ID - 1) % 32);
            sTaxiNodesMask[field] |= subMask;

            if (node->MountCreatureID[0] && node->MountCreatureID[0] != 32981)
                sHordeTaxiNodesMask[field] |= subMask;

            if (node->MountCreatureID[1] && node->MountCreatureID[1] != 32981)
                sAllianceTaxiNodesMask[field] |= subMask;

            if (node->MountCreatureID[0] == 32981 || node->MountCreatureID[1] == 32981)
                sDeathKnightTaxiNodesMask[field] |= subMask;

            // Old continent node (+ nodes virtually at old continents, check explicitly to avoid loading map files for zone info)
            if (node->MapID < 2 || node->ID == 82 || node->ID == 83 || node->ID == 93 || node->ID == 94)
                sOldContinentsNodesMask[field] |= subMask;

            // fix DK node at Ebon Hold and Shadow Vault flight master
            if (node->ID == 315 || node->ID == 333)
                const_cast<TaxiNodesEntry*>(node)->MountCreatureID[1] = 32981;
        }
    }

    for (TransportAnimationEntry const* anim : sTransportAnimationStore)
        sTransportMgr->AddPathNodeToTransport(anim->TransportEntry, anim->TimeSeg, anim);

    for (TransportRotationEntry const* rot : sTransportRotationStore)
        sTransportMgr->AddPathRotationToTransport(rot->TransportEntry, rot->TimeSeg, rot);

    for (WMOAreaTableEntry const* entry : sWMOAreaTableStore)
        sWMOAreaInfoByTriple[WMOAreaTableKey(entry->RootID, entry->AdtID, entry->GroupID)] = entry;

    // Check loaded DBC files proper version
    if (!sAreaTableStore.LookupEntry(4987)             ||       // last area added in 3.3.5a
            !sCharTitlesStore.LookupEntry(177)         ||       // last char title added in 3.3.5a
            !sGemPropertiesStore.LookupEntry(1629)     ||       // last added spell in 3.3.5a
            !sItemStore.LookupEntry(56806)             ||       // last client known item added in 3.3.5a
            !sItemExtendedCostStore.LookupEntry(2997)  ||       // last item extended cost added in 3.3.5a
            !sMapStore.LookupEntry(724)                ||       // last map added in 3.3.5a
            !sSpellStore.LookupEntry(80864)            )        // last client known item added in 3.3.5a
    {
        LOG_ERROR("dbc", "You have _outdated_ DBC data. Please extract correct versions from current using client.");
        exit(1);
    }

    LOG_INFO("server.loading", ">> Initialized {} Data Stores in {} ms", DBCFileCount, GetMSTimeDiffToNow(oldMSTime));
    LOG_INFO("server.loading", " ");
}

const SimpleFactionsList* GetFactionTeamList(const uint32 faction)
{
    const FactionTeamMap::const_iterator itr = sFactionTeamMap.find(faction);
    if (itr == sFactionTeamMap.end())
        return nullptr;
    return &itr->second;
}

std::string GetPetName(const uint32 petFamily)
{
    if (!petFamily)
        return nullptr;

    const CreatureFamilyEntry* pet_family = sCreatureFamilyStore.LookupEntry(petFamily);
    if (!pet_family)
        return nullptr;

    return pet_family->Name;
}

const TalentSpellPos* GetTalentSpellPos(const uint32 spellID)
{
    const TalentSpellPosMap::const_iterator itr = sTalentSpellPosMap.find(spellID);
    if (itr == sTalentSpellPosMap.end())
        return nullptr;
    return &itr->second;
}

uint32 GetTalentSpellCost(const uint32 spellId)
{
    if (const TalentSpellPos* pos = GetTalentSpellPos(spellId))
        return pos->Rank + 1;
    return 0;
}

const WMOAreaTableEntry* GetWMOAreaTableEntryByTriple(const int32 rootID, const int32 adtID, const int32 groupID)
{
    const auto entry = sWMOAreaInfoByTriple.find(WMOAreaTableKey(static_cast<uint16>(rootID), static_cast<uint8>(adtID), groupID));
    return entry == sWMOAreaInfoByTriple.end() ? nullptr : entry->second;
}

uint32 GetVirtualMapForMapAndZone(const uint32 mapID, const uint32 zoneID)
{
    if (mapID != MAP_OUTLAND && mapID != MAP_NORTHREND) // Speed for most cases
        return mapID;

    if (WorldMapAreaEntry const* wma = sWorldMapAreaStore.LookupEntry(zoneID))
        return wma->VirtualMapID >= 0 ? wma->VirtualMapID : wma->MapID;

    return mapID;
}

ContentLevels GetContentLevelsForMapAndZone(uint32 mapID, const uint32 zoneID)
{
    mapID = GetVirtualMapForMapAndZone(mapID, zoneID);
    if (mapID < 2)
        return CONTENT_1_60;

    const MapEntry* mapEntry = sMapStore.LookupEntry(mapID);
    if (!mapEntry)
        return CONTENT_1_60;

    switch (mapEntry->Expansion)
    {
        default:
            return CONTENT_1_60;
        case 1:
            return CONTENT_61_70;
        case 2:
            return CONTENT_71_80;
    }
}

void Zone2MapCoordinates(float& x, float& y, const uint32 zone)
{
    const WorldMapAreaEntry* maEntry = sWorldMapAreaStore.LookupEntry(zone);

    // If not listed then map coordinates (instance)
    if (!maEntry)
        return;

    std::swap(x, y);  // At client map coords swapped
    x = x * ((maEntry->BottomCoord - maEntry->TopCoord) / 100) + maEntry->TopCoord;
    y = y * ((maEntry->RightCoord - maEntry->LeftCoord) / 100) + maEntry->LeftCoord; // Client y coord from top to down
}

void Map2ZoneCoordinates(float& x, float& y, const uint32 zone)
{
    WorldMapAreaEntry const* maEntry = sWorldMapAreaStore.LookupEntry(zone);

    // If not listed then map coordinates (instance)
    if (!maEntry)
        return;

    x = (x - maEntry->TopCoord) / ((maEntry->BottomCoord - maEntry->TopCoord) / 100);
    y = (y - maEntry->LeftCoord) / ((maEntry->RightCoord - maEntry->LeftCoord) / 100); // Client y coord from top to down
    std::swap(x, y);  // Client have map coords swapped
}

const MapDifficulty* GetMapDifficultyData(const uint32 mapID, const Difficulty difficulty)
{
    const MapDifficultyMap::const_iterator itr = sMapDifficultyMap.find(MAKE_PAIR32(mapID, difficulty));
    return itr != sMapDifficultyMap.end() ? &itr->second : nullptr;
}

const MapDifficulty* GetDownscaledMapDifficultyData(const uint32 mapID, Difficulty& difficulty)
{
    uint32 tmpDiff = difficulty;

    MapDifficulty const* mapDiff = GetMapDifficultyData(mapID, static_cast<Difficulty>(tmpDiff));
    if (!mapDiff)
    {
        if (tmpDiff > RAID_DIFFICULTY_25MAN_NORMAL) // Heroic, downscale to normal
            tmpDiff -= 2;
        else
            tmpDiff -= 1;   // Any non-normal mode for raids like tbc (only one mode)

        // Pull new data
        mapDiff = GetMapDifficultyData(mapID, static_cast<Difficulty>(tmpDiff)); // We are 10 normal or 25 normal
        if (!mapDiff)
        {
            tmpDiff -= 1;
            mapDiff = GetMapDifficultyData(mapID, static_cast<Difficulty>(tmpDiff)); // 10 normal
        }
    }

    difficulty = static_cast<Difficulty>(tmpDiff);

    return mapDiff;
}

const PvPDifficultyEntry* GetBattlegroundBracketByLevel(const uint32 mapID, const uint32 level)
{
    PvPDifficultyEntry const* maxEntry = nullptr;  // Used for level > max listed level case

    for (PvPDifficultyEntry const* entry : sPvPDifficultyStore)
    {
        // Skip unrelated and too-high brackets
        if (entry->MapID != mapID || entry->MinLevel > level)
            continue;

        // Exactly fit
        if (entry->MaxLevel >= level)
            return entry;

        // Remember for possible out-of-range case (search higher from existed)
        if (!maxEntry || maxEntry->MaxLevel < entry->MaxLevel)
            maxEntry = entry;
    }

    return maxEntry;
}

const PvPDifficultyEntry* GetBattlegroundBracketById(const uint32 mapID, const BattlegroundBracketID id)
{
    for (PvPDifficultyEntry const* entry : sPvPDifficultyStore)
        if (entry->MapID == mapID && entry->GetBracketId() == id)
            return entry;

    return nullptr;
}

const uint32* GetTalentTabPages(const uint8 cls)
{
    return sTalentTabPages[cls];
}

bool IsSharedDifficultyMap(const uint32 mapID)
{
    return sWorld->getBoolConfig(CONFIG_INSTANCE_SHARED_ID) && (mapID == 631 || mapID == 724);
}

uint32 GetLiquidFlags(const uint32 liquidType)
{
    if (LiquidTypeEntry const* liq = sLiquidTypeStore.LookupEntry(liquidType))
        return 1 << liq->Type;
    return 0;
}

const CharStartOutfitEntry* GetCharStartOutfitEntry(const uint8 race, const uint8 class_, const uint8 gender)
{
    const std::map<uint32, CharStartOutfitEntry const*>::const_iterator itr = sCharStartOutfitMap.find(race | (class_ << 8) | (gender << 16));
    if (itr == sCharStartOutfitMap.end())
        return nullptr;
    return itr->second;
}

/// Returns LFGDungeonEntry for a specific map and difficulty.
/// Will return first found entry if multiple dungeons use the same map (such as Scarlet Monastery)
const LFGDungeonEntry* GetLFGDungeon(const uint32 mapID, const Difficulty difficulty)
{
    for (const LFGDungeonEntry* dungeon : sLFGDungeonStore)
    {
        if (dungeon->MapID == mapID && static_cast<Difficulty>(dungeon->Difficulty) == difficulty)
            return dungeon;
    }
    return nullptr;
}

const LFGDungeonEntry* GetZoneLFGDungeonEntry(const std::string& zoneName)
{
    for (const LFGDungeonEntry* dungeon : sLFGDungeonStore)
    {
        if (dungeon->TypeID == lfg::LFG_TYPE_ZONE && zoneName.find(dungeon->Name) != std::string::npos)
            return dungeon;
    }
    return nullptr;
}

uint32 GetDefaultMapLight(const uint32 mapID)
{
    for (const LightEntry* light : sLightStore)
    {
        if (!light)
            continue;
        if (light->MapID == mapID && light->X == 0.0f && light->Y == 0.0f && light->Z == 0.0f)
            return light->ID;
    }
    return 0;
}

const SkillRaceClassInfoEntry* GetSkillRaceClassInfo(const uint32 skill, const uint8 race, const uint8 class_)
{
    auto [fst, lst] = SkillRaceClassInfoBySkill.equal_range(skill);
    for (auto itr = fst; itr != lst; ++itr)
    {
        if (itr->second->RaceMask && !(itr->second->RaceMask & (1 << (race - 1))))
            continue;
        if (itr->second->ClassMask && !(itr->second->ClassMask & (1 << (class_ - 1))))
            continue;

        return itr->second;
    }
    return nullptr;
}

const std::vector<const SkillLineAbilityEntry*>& GetSkillLineAbilitiesBySkillLine(const uint32 skillLine)
{
    const auto it = sSkillLineAbilityIndexBySkillLine.find(skillLine);
    if (it == sSkillLineAbilityIndexBySkillLine.end())
    {
        static constexpr std::vector<const SkillLineAbilityEntry*> emptyVector;
        return emptyVector;
    }
    return it->second;
}
