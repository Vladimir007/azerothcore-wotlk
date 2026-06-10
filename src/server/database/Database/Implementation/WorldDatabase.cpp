#include "WorldDatabase.h"

void WorldDatabaseConnection::DoPrepareStatements()
{
    if (!m_reconnecting)
        m_stmts.resize(MAX_WORLD_DATABASE_STATEMENTS);

    PrepareStatement(WORLD_SEL_QUEST_POOLS, "SELECT id, pool FROM world_pool_quest", CONNECTION_SYNCH);
    PrepareStatement(WORLD_DEL_CREATURE_LINKED_RESPAWN, "DELETE FROM world_linked_respawn WHERE guid = $1", CONNECTION_ASYNC);
    PrepareStatement(WORLD_REP_CREATURE_LINKED_RESPAWN,
        "INSERT INTO world_linked_respawn (guid, link_type, linked_guid) VALUES ($1, 0, $2) ON CONFLICT (guid, link_type) DO UPDATE SET linked_guid=$2", CONNECTION_ASYNC);
    PrepareStatement(WORLD_SEL_CREATURE_TEXT,
        "SELECT creature, group, id, text, type, language, probability, emote, duration, sound, broadcast_text, text_range FROM world_creature_text", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_SMART_SCRIPTS,
        "SELECT entry, source_type, id, link, event_type, event_phase_mask, event_chance, event_flags, event_params, "
        "action_type, action_params, target_type, target_params, target_position, target_orientation FROM world_smart_script ORDER BY entry, source_type, id, link", CONNECTION_SYNCH);
    PrepareStatement(WORLD_DEL_GAMEOBJECT, "DELETE FROM world_game_object WHERE guid = $1", CONNECTION_ASYNC);
    PrepareStatement(WORLD_DEL_EVENT_GAMEOBJECT, "DELETE FROM world_game_event_game_object WHERE guid = $1", CONNECTION_ASYNC);
    PrepareStatement(WORLD_INS_GRAVEYARD_ZONE, "INSERT INTO world_graveyard_zone (id, ghost_zone, faction, comment) VALUES ($1, $2, $3, '')", CONNECTION_ASYNC);
    PrepareStatement(WORLD_DEL_GRAVEYARD_ZONE, "DELETE FROM world_graveyard_zone WHERE id=$1 AND ghost_zone=$2 AND faction=$3", CONNECTION_ASYNC);
    PrepareStatement(WORLD_INS_GAME_TELE, "INSERT INTO world_game_teleport (id, position, orientation, map, name) VALUES ($1, ARRAY[$2, $3, $4], $5, $6, $7)", CONNECTION_ASYNC);
    PrepareStatement(WORLD_DEL_GAME_TELE, "DELETE FROM world_game_teleport WHERE name=$1", CONNECTION_ASYNC);
    PrepareStatement(WORLD_INS_NPC_VENDOR, "INSERT INTO world_npc_vendor (entry, item, max_count, incr_time, extended_cost, slot) VALUES($1, $2, $3, $4, $5, 0)", CONNECTION_ASYNC);
    PrepareStatement(WORLD_DEL_NPC_VENDOR, "DELETE FROM world_npc_vendor WHERE entry=$1 AND item=$2", CONNECTION_ASYNC);
    PrepareStatement(WORLD_SEL_NPC_VENDOR_REF, "SELECT item, max_count, incr_time, extended_cost FROM world_npc_vendor WHERE entry=$1 ORDER BY slot ASC", CONNECTION_SYNCH);
    PrepareStatement(WORLD_UPD_CREATURE_MOVEMENT_TYPE, "UPDATE world_creature SET movement_type=$1 WHERE guid=$2", CONNECTION_ASYNC);
    PrepareStatement(WORLD_UPD_CREATURE_FACTION, "UPDATE world_creature_template SET faction=$1 WHERE id=$2", CONNECTION_ASYNC);
    PrepareStatement(WORLD_UPD_CREATURE_NPC_FLAG, "UPDATE world_creature_template SET npc_flag=$1 WHERE id=$2", CONNECTION_ASYNC);
    PrepareStatement(WORLD_UPD_CREATURE_POSITION, "UPDATE world_creature SET position=ARRAY[$1, $2, $3], orientation=$4 WHERE guid=$5", CONNECTION_ASYNC);
    PrepareStatement(WORLD_UPD_CREATURE_WANDER_DISTANCE, "UPDATE world_creature SET wander_distance=$1, movement_type=$2 WHERE guid=$3", CONNECTION_ASYNC);
    PrepareStatement(WORLD_UPD_CREATURE_SPAWN_TIME_SECS, "UPDATE world_creature SET spawn_time_secs=$1 WHERE guid=$2", CONNECTION_ASYNC);
    PrepareStatement(WORLD_DEL_CREATURE, "DELETE FROM world_creature WHERE guid=$1", CONNECTION_ASYNC);
    PrepareStatement(WORLD_SEL_CREATURE_BY_ID, "SELECT guid FROM world_creature WHERE id1=$1 OR id2=$2 OR id3=$3", CONNECTION_SYNCH);
    PrepareStatement(WORLD_INS_CREATURE_FORMATION,
        "INSERT INTO world_creature_formation (leader_guid, member_guid, dist, angle, group_ai, point1, point2) VALUES ($1, $2, $3, $4, $5, 0, 0)", CONNECTION_ASYNC);
    PrepareStatement(WORLD_SEL_SMARTAI_WP, "SELECT id, point, position, orientation, delay FROM world_waypoint ORDER BY id, point", CONNECTION_SYNCH);
    PrepareStatement(WORLD_INS_WAYPOINT_DATA,
        "INSERT INTO world_waypoint_data (id, point, position, orientation, velocity, delay, smooth_transition, move_type, action, action_chance, guid) "
        "VALUES ($1, $2, ARRAY[$3, $4, $5], NULL, 0.0, 0, FALSE, 0, 0, 100, 0)", CONNECTION_ASYNC);
    PrepareStatement(WORLD_DEL_WAYPOINT_DATA, "DELETE FROM world_waypoint_data WHERE id=$1 AND point=$2", CONNECTION_ASYNC);
    PrepareStatement(WORLD_UPD_WAYPOINT_DATA_POINT, "UPDATE world_waypoint_data SET point=(point - 1) WHERE id=$1 AND point > $2", CONNECTION_ASYNC);
    PrepareStatement(WORLD_UPD_WAYPOINT_DATA_POSITION, "UPDATE world_waypoint_data SET position=ARRAY[$1, $2, $3] WHERE id=$4 AND point=$5", CONNECTION_ASYNC);
    PrepareStatement(WORLD_UPD_WAYPOINT_DATA_WP_GUID, "UPDATE world_waypoint_data SET guid=$1 WHERE id=$2 and point=$3", CONNECTION_ASYNC);
    PrepareStatement(WORLD_SEL_WAYPOINT_DATA_MAX_ID, "SELECT MAX(id) FROM world_waypoint_data", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_WAYPOINT_DATA_MAX_POINT, "SELECT MAX(point) FROM world_waypoint_data WHERE id=$1", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_WAYPOINT_DATA_BY_ID,
        "SELECT point, position, orientation, velocity, delay, smooth_transition, move_type, action, action_chance FROM world_waypoint_data WHERE id=$1 ORDER BY point", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_WAYPOINT_DATA_POS_BY_ID, "SELECT point, position FROM world_waypoint_data WHERE id=$1", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_WAYPOINT_DATA_POS_FIRST_BY_ID, "SELECT position FROM world_waypoint_data WHERE point=1 AND id=$1", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_WAYPOINT_DATA_POS_LAST_BY_ID, "SELECT position, orientation FROM world_waypoint_data WHERE id=$1 ORDER BY point DESC LIMIT 1", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_WAYPOINT_DATA_BY_WP_GUID, "SELECT id, point FROM world_waypoint_data WHERE guid=$1", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_WAYPOINT_DATA_ALL_BY_WP_GUID, "SELECT id, point, delay, move_type, action, action_chance FROM world_waypoint_data WHERE guid=$1", CONNECTION_SYNCH);
    PrepareStatement(WORLD_UPD_WAYPOINT_DATA_ALL_WP_GUID, "UPDATE world_waypoint_data SET guid=0", CONNECTION_ASYNC);
    PrepareStatement(WORLD_SEL_WAYPOINT_DATA_BY_POS,
        "SELECT id, point FROM world_waypoint_data WHERE ABS(position[1] - $1) <= $4 AND ABS(position[2] - $2) <= $4 AND ABS(position[3] - $3) <= $4", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_WAYPOINT_DATA_WP_GUID_BY_ID, "SELECT guid FROM world_waypoint_data WHERE id=$1 and guid <> 0", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_WAYPOINT_DATA_ACTION, "SELECT DISTINCT action FROM world_waypoint_data", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_WAYPOINT_SCRIPTS_MAX_ID, "SELECT MAX(guid) FROM world_waypoint_script", CONNECTION_SYNCH);
    PrepareStatement(WORLD_INS_WAYPOINT_SCRIPT,
        "INSERT INTO world_waypoint_script (guid, entry, delay, command, data_i, data_f) VALUES ($1, 0, 0, 0, ARRAY[0, 0, 0], ARRAY[0.0, 0.0, 0.0, 0.0])", CONNECTION_ASYNC);
    PrepareStatement(WORLD_DEL_WAYPOINT_SCRIPT, "DELETE FROM world_waypoint_script WHERE guid=$1", CONNECTION_ASYNC);
    PrepareStatement(WORLD_UPD_WAYPOINT_SCRIPT_ID, "UPDATE world_waypoint_script SET entry=$1 WHERE guid=$2", CONNECTION_ASYNC);
    PrepareStatement(WORLD_UPD_WAYPOINT_SCRIPT_X, "UPDATE world_waypoint_script SET data_f[1]=$1 WHERE guid=$2", CONNECTION_ASYNC);
    PrepareStatement(WORLD_UPD_WAYPOINT_SCRIPT_Y, "UPDATE world_waypoint_script SET data_f[2]=$1 WHERE guid=$2", CONNECTION_ASYNC);
    PrepareStatement(WORLD_UPD_WAYPOINT_SCRIPT_Z, "UPDATE world_waypoint_script SET data_f[3]=$1 WHERE guid=$2", CONNECTION_ASYNC);
    PrepareStatement(WORLD_UPD_WAYPOINT_SCRIPT_O, "UPDATE world_waypoint_script SET data_f[4]=$1 WHERE guid=$2", CONNECTION_ASYNC);
    PrepareStatement(WORLD_SEL_WAYPOINT_SCRIPT_ID_BY_GUID, "SELECT entry FROM world_waypoint_script WHERE guid=$1", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_WAYPOINT_SCRIPT_BY_ID, "SELECT guid, delay, command, data_i, data_f FROM world_waypoint_script WHERE entry=$1", CONNECTION_SYNCH);
    PrepareStatement(WORLD_INS_CREATURE_ADDON,
        "INSERT INTO world_creature_addon (guid, path, mount, bytes1, bytes2, emote, visibility_distance_type, auras) VALUES ($1, $2, 0, 0, 0, 0, 0, '')", CONNECTION_ASYNC);
    PrepareStatement(WORLD_UPD_CREATURE_ADDON_PATH, "UPDATE world_creature_addon SET path=$1 WHERE guid=$2", CONNECTION_ASYNC);
    PrepareStatement(WORLD_DEL_CREATURE_ADDON, "DELETE FROM world_creature_addon WHERE guid=$1", CONNECTION_ASYNC);
    PrepareStatement(WORLD_SEL_CREATURE_ADDON_BY_GUID, "SELECT guid FROM world_creature_addon WHERE guid=$1", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_COMMANDS, "SELECT name, superuser_only, help FROM world_command", CONNECTION_SYNCH);

    PrepareStatement(WORLD_SEL_CREATURE_TEMPLATE,
        "SELECT id, difficulty, kill_credit, name, title, icon_name, gossip_menu, min_level, max_level, expansion, "
        "faction, npc_flag, flags_extra, speed_walk, speed_run, speed_swim, speed_flight, detection_range, rank, "
        "damage_school, damage_modifier, base_attack_time, range_attack_time, base_variance, range_variance, "
        "unit_class, unit_flags, unit_flags2, dynamic_flags, family, type, type_flags, loot, "
        "pickpocket_loot, skin_loot, pet_spell_data, vehicle, gold_min, gold_max, name_ai, movement_type, movement, "
        "ctm.ground, ctm.swim, ctm.flight, ctm.rooted, ctm.chase, ctm.random, ctm.interaction_pause_timer, "
        "hover_height, health_modifier, mana_modifier, armor_modifier, experience_modifier, racial_leader, regen_health, immunity, script_name "
        "FROM world_creature_template ct LEFT JOIN world_creature_template_movement ctm ON ct.id = ctm.creature WHERE id=$1", CONNECTION_SYNCH);


    PrepareStatement(WORLD_SEL_ITEM_TEMPLATE_BY_NAME, "SELECT id FROM world_item_template WHERE name=$1", CONNECTION_SYNCH);

    PrepareStatement(WORLD_SEL_GAMEOBJECT_NEAREST,
        "SELECT guid, id, position, map"
        "FROM world_game_object WHERE map=$1 AND (POW(position[1] - $2, 2) + POW(position[2] - $3, 2) + POW(position[3] - $4, 2)) <= $5 AND (phase_mask & $6) <> 0 "
        "ORDER BY (POW(position[1] - $2, 2) + POW(position[2] - $3, 2) + POW(position[3] - $4, 2))", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_CREATURE_NEAREST,
        "SELECT guid, id1, id2, id3, position, map "
        "FROM world_creature WHERE map=$1 AND (POW(position[1] - $2, 2) + POW(position[2] - $3, 2) + POW(position[3] - $4, 2)) <= $5 AND (phaseMask & $6) <> 0 "
        "ORDER BY (POW(position[1] - $2, 2) + POW(position[2] - $3, 2) + POW(position[3] - $4, 2))", CONNECTION_SYNCH);
    PrepareStatement(WORLD_INS_CREATURE,
        "INSERT INTO world_creature (guid, id1, id2, id3, map, zone, area, spawn_mask, phase_mask, equipment, position, orientation, spawn_time_secs, "
        "wander_distance, waypoint, health, mana, movement_type, npc_flag, unit_flags, dynamic_flags) "
        "VALUES ($1, $2, $3, $4, $5, 0, 0, $6, $7, $8, ARRAY[$9, $10, $11], $12, $13, $14, $15, $16, $17, $18, $19, $20, $21)", CONNECTION_ASYNC);
    PrepareStatement(WORLD_SEL_GAME_EVENTS,
        "SELECT id, EXTRACT(epoch FROM start_time), EXTRACT(epoch FROM end_time), occurrence, length, "
        "holiday, holiday_stage, description, world_event, announce FROM world_game_event", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_GAME_EVENT_PREREQUISITE_DATA, "SELECT event, prerequisite FROM world_game_event_prerequisite", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_GAME_EVENT_CREATURE_DATA, "SELECT guid, event FROM world_game_event_creature", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_GAME_EVENT_GAMEOBJECT_DATA, "SELECT guid, event FROM world_game_event_game_object", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_GAME_EVENT_MODEL_EQUIPMENT_DATA,
        "SELECT cr.guid, cr.id1, cr.id2, cr.id3, eq.event, eq.model, eq.equipment FROM world_creature cr JOIN world_game_event_model_equip eq ON cr.guid=eq.guid", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_GAME_EVENT_QUEST_DATA, "SELECT id, quest, event FROM world_game_event_creature_quest", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_GAME_EVENT_GAMEOBJECT_QUEST_DATA, "SELECT id, quest, event FROM world_game_event_game_object_quest", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_GAME_EVENT_QUEST_CONDITION_DATA, "SELECT quest, event, condition, num FROM world_game_event_quest_condition", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_GAME_EVENT_CONDITION_DATA, "SELECT event, condition, req_num, world_state_field_max, world_state_field_done FROM world_game_event_condition", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_GAME_EVENT_NPC_FLAGS, "SELECT guid, event, flag FROM world_game_event_npc_flag", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_GAME_EVENT_QUEST_SEASONAL_RELATIONS, "SELECT quest, event FROM world_game_event_seasonal_quest_relation", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_GAME_EVENT_BATTLEGROUND_DATA, "SELECT id, flag FROM world_game_event_battleground_holiday", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_GAME_EVENT_POOL_DATA, "SELECT t1.id, t2.event FROM world_pool_template t1 JOIN world_game_event_pool t2 ON t1.id = t2.id", CONNECTION_SYNCH);
    PrepareStatement(WORLD_SEL_GAME_EVENT_ARENA_SEASON, "SELECT event FROM world_game_event_arena_season WHERE season=$1", CONNECTION_SYNCH);
    PrepareStatement(WORLD_DEL_GAME_EVENT_CREATURE, "DELETE FROM world_game_event_creature WHERE guid=$1", CONNECTION_ASYNC);
    PrepareStatement(WORLD_DEL_GAME_EVENT_MODEL_EQUIP, "DELETE FROM world_game_event_model_equip WHERE guid=$1", CONNECTION_ASYNC);
    PrepareStatement(WORLD_SEL_GAME_EVENT_NPC_VENDOR,
        "SELECT event, guid, item, max_count, incr_time, extended_cost FROM world_game_event_npc_vendor ORDER BY guid, slot ASC", CONNECTION_SYNCH);
    PrepareStatement(WORLD_INS_GAMEOBJECT,
        "INSERT INTO world_game_object (guid, id, map, zone, area, spawn_mask, phase_mask, position, orientation, rotation, spawn_time, anim_progress, state) "
        "VALUES ($1, $2, $3, 0, 0, $4, $5, ARRAY[$6, $7, $8], $9, ARRAY[$10, $11, $12, $13], $14, $15, $16)", CONNECTION_ASYNC);
    PrepareStatement(WORLD_INS_DISABLES, "INSERT INTO world_disable_data (entry, type, flags, params0, params1, comment) VALUES ($1, $2, $3, '{}', '{}', $4)", CONNECTION_ASYNC);
    PrepareStatement(WORLD_SEL_DISABLES, "SELECT entry FROM world_disable_data WHERE entry=$1 AND type=$2", CONNECTION_SYNCH);
    PrepareStatement(WORLD_DEL_DISABLES, "DELETE FROM world_disable_data WHERE entry=$1 AND type=$2", CONNECTION_ASYNC);
    PrepareStatement(WORLD_UPD_CREATURE_ZONE_AREA_DATA, "UPDATE world_creature SET zone=$1, area=$2 WHERE guid=$3", CONNECTION_ASYNC);
    PrepareStatement(WORLD_UPD_GAMEOBJECT_ZONE_AREA_DATA, "UPDATE world_game_object SET zone=$1, area=$2 WHERE guid=$3", CONNECTION_ASYNC);
    PrepareStatement(WORLD_INS_GAMEOBJECT_ADDON,
        "INSERT INTO world_game_object_addon (guid, invisibility_type, invisibility_value, parent_rotation) VALUES ($1, 0, 0, ARRAY[0, 0, 0, 1])", CONNECTION_ASYNC);
    PrepareStatement(WORLD_SEL_REQ_XP, "SELECT experience FROM world_player_xp_for_level WHERE level=$1", CONNECTION_SYNCH);
    PrepareStatement(WORLD_DEL_SPAWNGROUP_MEMBER, "DELETE FROM spawn_group WHERE type=$1 AND spawn=$2", CONNECTION_ASYNC);
}

WorldDatabaseConnection::WorldDatabaseConnection(const std::string& connectionStr) : PSQLConnection(connectionStr) { }

WorldDatabaseConnection::WorldDatabaseConnection(ProducerConsumerQueue<SQLOperation*>* queue, const std::string& connectionStr):
    PSQLConnection(queue, connectionStr) { }

WorldDatabaseConnection::~WorldDatabaseConnection() = default;
