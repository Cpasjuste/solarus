/*
 * Copyright (C) 2006-2018 Christopho, Solarus - http://www.solarus-games.org
 *
 * Solarus is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Solarus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "solarus/audio/Sound.h"
#include "solarus/core/CurrentQuest.h"
#include "solarus/core/Debug.h"
#include "solarus/core/Geometry.h"
#include "solarus/core/Equipment.h"
#include "solarus/core/EquipmentItem.h"
#include "solarus/core/Game.h"
#include "solarus/core/Map.h"
#include "solarus/core/Savegame.h"
#include "solarus/core/Timer.h"
#include "solarus/entities/Block.h"
#include "solarus/entities/CarriedObject.h"
#include "solarus/entities/Chest.h"
#include "solarus/entities/CustomEntity.h"
#include "solarus/entities/Destination.h"
#include "solarus/entities/Destructible.h"
#include "solarus/entities/DynamicTile.h"
#include "solarus/entities/Door.h"
#include "solarus/entities/Enemy.h"
#include "solarus/entities/Entities.h"
#include "solarus/entities/EntityTypeInfo.h"
#include "solarus/entities/GroundInfo.h"
#include "solarus/entities/Hero.h"
#include "solarus/entities/Npc.h"
#include "solarus/entities/Pickable.h"
#include "solarus/entities/Sensor.h"
#include "solarus/entities/Separator.h"
#include "solarus/entities/ShopTreasure.h"
#include "solarus/entities/Stream.h"
#include "solarus/entities/StreamAction.h"
#include "solarus/entities/Switch.h"
#include "solarus/entities/Teletransporter.h"
#include "solarus/entities/Tileset.h"
#include "solarus/graphics/Sprite.h"
#include "solarus/hero/HeroSprites.h"
#include "solarus/lua/ExportableToLuaPtr.h"
#include "solarus/lua/LuaContext.h"
#include "solarus/lua/LuaTools.h"
#include "solarus/movements/Movement.h"
#include <sstream>

namespace Solarus {

namespace {

/**
 * \brief Returns the Lua metatable names of each entity type.
 * \return The Lua metatable name of each entity type.
 */
const std::map<EntityType, std::string>& get_entity_internal_type_names() {

  static std::map<EntityType, std::string> result;
  if (result.empty()) {
    for (const auto& kvp : EnumInfoTraits<EntityType>::names) {
      std::string internal_type_name = std::string("sol.") + kvp.second;
      result.emplace(kvp.first, internal_type_name);
    }
  }

  return result;
}
/**
 * \brief Returns a set of the Lua metatable names of all entity types.
 * \return The Lua metatable name of all entity types.
 */
const std::set<std::string>& get_entity_internal_type_names_set() {

  static std::set<std::string> result;
  if (result.empty()) {
    for (const auto& kvp : EnumInfoTraits<EntityType>::names) {
      result.insert(LuaContext::get_entity_internal_type_name(kvp.first));
    }
  }

  return result;
}

}

/**
 * \brief Initializes the map entity features provided to Lua.
 */
void LuaContext::register_entity_module() {

  // Methods common to all entity types.
  std::vector<luaL_Reg> common_methods = {
      { "get_type", entity_api_get_type },
      { "get_map", entity_api_get_map },
      { "get_game", entity_api_get_game },
      { "get_name", entity_api_get_name },
      { "exists", entity_api_exists },
      { "remove", entity_api_remove },
      { "is_enabled", entity_api_is_enabled },
      { "set_enabled", entity_api_set_enabled },
      { "get_size", entity_api_get_size },
      { "get_origin", entity_api_get_origin },
      { "get_position", entity_api_get_position },
      { "set_position", entity_api_set_position },
      { "get_center_position", entity_api_get_center_position },
      { "get_facing_position", entity_api_get_facing_position },
      { "get_facing_entity", entity_api_get_facing_entity },
      { "get_ground_position", entity_api_get_ground_position },
      { "get_ground_below", entity_api_get_ground_below },
      { "get_bounding_box", entity_api_get_bounding_box },
      { "get_max_bounding_box", entity_api_get_max_bounding_box },
      { "overlaps", entity_api_overlaps },
      { "get_distance", entity_api_get_distance },
      { "get_angle", entity_api_get_angle },
      { "get_direction4_to", entity_api_get_direction4_to },
      { "get_direction8_to", entity_api_get_direction8_to },
      { "snap_to_grid", entity_api_snap_to_grid },
      { "bring_to_front", entity_api_bring_to_front },
      { "bring_to_back", entity_api_bring_to_back },
      { "is_drawn_in_y_order", entity_api_is_drawn_in_y_order },
      { "set_drawn_in_y_order", entity_api_set_drawn_in_y_order },
      { "get_optimization_distance", entity_api_get_optimization_distance },
      { "set_optimization_distance", entity_api_set_optimization_distance },
      { "is_in_same_region", entity_api_is_in_same_region },
      { "test_obstacles", entity_api_test_obstacles },
      { "get_sprite", entity_api_get_sprite },
      { "get_sprites", entity_api_get_sprites },
      { "bring_sprite_to_front", entity_api_bring_sprite_to_front },
      { "bring_sprite_to_back", entity_api_bring_sprite_to_back },
      { "is_visible", entity_api_is_visible },
      { "set_visible", entity_api_set_visible },
      { "get_movement", entity_api_get_movement },
      { "stop_movement", entity_api_stop_movement },
  };
  if (CurrentQuest::is_format_at_least({ 1, 6 })) {
    common_methods.insert(common_methods.end(), {
        { "get_layer", entity_api_get_layer },
        { "set_layer", entity_api_set_layer },
        { "set_size", entity_api_set_size },
        { "set_origin", entity_api_set_origin },
        { "get_weight", entity_api_get_weight },
        { "set_weight", entity_api_set_weight },
        { "get_controlling_stream", entity_api_get_controlling_stream },
        { "get_property", entity_api_get_property },
        { "set_property", entity_api_set_property },
        { "get_properties", entity_api_get_properties },
        { "set_properties", entity_api_set_properties },
    });
  }

  // Metamethods of all entity types.
  std::vector<luaL_Reg> metamethods = {
      { "__gc", userdata_meta_gc },
      { "__newindex", userdata_meta_newindex_as_table },
      { "__index", userdata_meta_index_as_table },
  };

  // Hero.
  std::vector<luaL_Reg> hero_methods = {
      { "teleport", hero_api_teleport },
      { "get_direction", hero_api_get_direction },
      { "set_direction", hero_api_set_direction },
      { "get_walking_speed", hero_api_get_walking_speed },
      { "set_walking_speed", hero_api_set_walking_speed },
      { "save_solid_ground", hero_api_save_solid_ground },
      { "reset_solid_ground", hero_api_reset_solid_ground },
      { "get_solid_ground_position", hero_api_get_solid_ground_position },
      { "get_animation", hero_api_get_animation },
      { "set_animation", hero_api_set_animation },
      { "get_tunic_sprite_id", hero_api_get_tunic_sprite_id },
      { "set_tunic_sprite_id", hero_api_set_tunic_sprite_id },
      { "get_sword_sprite_id", hero_api_get_sword_sprite_id },
      { "set_sword_sprite_id", hero_api_set_sword_sprite_id },
      { "get_sword_sound_id", hero_api_get_sword_sound_id },
      { "set_sword_sound_id", hero_api_set_sword_sound_id },
      { "get_shield_sprite_id", hero_api_get_shield_sprite_id },
      { "set_shield_sprite_id", hero_api_set_shield_sprite_id },
      { "is_blinking", hero_api_is_blinking },
      { "set_blinking", hero_api_set_blinking },
      { "is_invincible", hero_api_is_invincible },
      { "set_invincible", hero_api_set_invincible },
      { "get_state", entity_api_get_state },
      { "freeze", hero_api_freeze },
      { "unfreeze", hero_api_unfreeze },
      { "walk", hero_api_walk },  // TODO use the more general movement:start
      { "start_attack", hero_api_start_attack },
      { "start_item", hero_api_start_item },
      { "start_jumping", hero_api_start_jumping },
      { "start_treasure", hero_api_start_treasure },
      { "start_victory", hero_api_start_victory},
      { "start_boomerang", hero_api_start_boomerang },
      { "start_bow", hero_api_start_bow },
      { "start_hookshot", hero_api_start_hookshot },
      { "start_running", hero_api_start_running },
      { "start_hurt", hero_api_start_hurt },
  };
  if (CurrentQuest::is_format_at_least({ 1, 6 })) {
    hero_methods.insert(hero_methods.end(), {
        { "get_carried_object", hero_api_get_carried_object },
    });
  }

  hero_methods.insert(hero_methods.end(), common_methods.begin(), common_methods.end());
  register_type(
      get_entity_internal_type_name(EntityType::HERO),
      {},
      hero_methods,
      metamethods
  );

  // Camera.
  std::vector<luaL_Reg> camera_methods = {
      { "get_position_on_screen", camera_api_get_position_on_screen },
      { "set_position_on_screen", camera_api_set_position_on_screen },
      { "get_state", entity_api_get_state },
      { "start_tracking", camera_api_start_tracking },
      { "start_manual", camera_api_start_manual },
      { "get_position_to_track", camera_api_get_position_to_track },
      { "get_tracked_entity", camera_api_get_tracked_entity },
  };
  if (CurrentQuest::is_format_at_most({ 1, 5 })) {
    camera_methods.insert(camera_methods.end(), {
        // Available to all entities since 1.6.
        { "set_size", entity_api_set_size },
    });
  }

  camera_methods.insert(camera_methods.end(), common_methods.begin(), common_methods.end());
  register_type(
      get_entity_internal_type_name(EntityType::CAMERA),
      {},
      camera_methods,
      metamethods
  );

  // Destination.
  std::vector<luaL_Reg> destination_methods = {
      { "get_starting_location_mode", destination_api_get_starting_location_mode },
      { "set_starting_location_mode", destination_api_set_starting_location_mode },
  };

  destination_methods.insert(destination_methods.end(), common_methods.begin(), common_methods.end());
  register_type(
      get_entity_internal_type_name(EntityType::DESTINATION),
      {},
      destination_methods,
      metamethods
  );

  // Teletransporter.
  std::vector<luaL_Reg> teletransporter_methods = {
      { "get_sound", teletransporter_api_get_sound },
      { "set_sound", teletransporter_api_set_sound },
      { "get_transition", teletransporter_api_get_transition },
      { "set_transition", teletransporter_api_set_transition },
      { "get_destination_map", teletransporter_api_get_destination_map },
      { "set_destination_map", teletransporter_api_set_destination_map},
      { "get_destination_name", teletransporter_api_get_destination_name },
      { "set_destination_name", teletransporter_api_set_destination_name },
  };

  teletransporter_methods.insert(teletransporter_methods.end(), common_methods.begin(), common_methods.end());
  register_type(
      get_entity_internal_type_name(EntityType::TELETRANSPORTER),
      {},
      teletransporter_methods,
      metamethods
  );

  // NPC.
  std::vector<luaL_Reg> npc_methods = {
      { "is_traversable", npc_api_is_traversable },
      { "set_traversable", npc_api_set_traversable },
  };

  npc_methods.insert(npc_methods.end(), common_methods.begin(), common_methods.end());
  register_type(
      get_entity_internal_type_name(EntityType::NPC),
      {},
      npc_methods,
      metamethods
  );

  // Chest.
  std::vector<luaL_Reg> chest_methods = {
      { "is_open", chest_api_is_open },
      { "set_open", chest_api_set_open },
      { "get_treasure", chest_api_get_treasure },
      { "set_treasure", chest_api_set_treasure },
  };

  chest_methods.insert(chest_methods.end(), common_methods.begin(), common_methods.end());
  register_type(
      get_entity_internal_type_name(EntityType::CHEST),
      {},
      chest_methods,
      metamethods
  );

  // Block.
  std::vector<luaL_Reg> block_methods = {
      { "reset", block_api_reset },
      { "is_pushable", block_api_is_pushable },
      { "set_pushable", block_api_set_pushable },
      { "is_pullable", block_api_is_pullable },
      { "set_pullable", block_api_set_pullable },
      { "get_maximum_moves", block_api_get_maximum_moves },
      { "set_maximum_moves", block_api_set_maximum_moves },
  };

  block_methods.insert(block_methods.end(), common_methods.begin(), common_methods.end());
  register_type(
      get_entity_internal_type_name(EntityType::BLOCK),
      {},
      block_methods,
      metamethods
  );

  // Switch.
  std::vector<luaL_Reg> switch_methods = {
      { "is_activated", switch_api_is_activated },
      { "set_activated", switch_api_set_activated },
      { "is_locked", switch_api_is_locked },
      { "set_locked", switch_api_set_locked },
      { "is_walkable", switch_api_is_walkable },
  };

  switch_methods.insert(switch_methods.end(), common_methods.begin(), common_methods.end());
  register_type(
      get_entity_internal_type_name(EntityType::SWITCH),
      {},
      switch_methods,
      metamethods
  );

  // Stream.
  std::vector<luaL_Reg> stream_methods = {
      { "get_direction", stream_api_get_direction },
      { "set_direction", stream_api_set_direction },
      { "get_speed", stream_api_get_speed },
      { "set_speed", stream_api_set_speed },
      { "get_allow_movement", stream_api_get_allow_movement },
      { "set_allow_movement", stream_api_set_allow_movement },
      { "get_allow_attack", stream_api_get_allow_attack },
      { "set_allow_attack", stream_api_set_allow_attack },
      { "get_allow_item", stream_api_get_allow_item },
      { "set_allow_item", stream_api_set_allow_item },
  };

  stream_methods.insert(stream_methods.end(), common_methods.begin(), common_methods.end());
  register_type(
      get_entity_internal_type_name(EntityType::STREAM),
      {},
      stream_methods,
      metamethods
  );

  // Door.
  std::vector<luaL_Reg> door_methods = {
      { "is_open", door_api_is_open },
      { "is_opening", door_api_is_opening },
      { "is_closed", door_api_is_closed },
      { "is_closing", door_api_is_closing },
  };
  if (CurrentQuest::is_format_at_least({ 1, 6 })) {
    door_methods.insert(door_methods.end(), {
        { "open", door_api_open },
        { "close", door_api_close },
        { "set_open", door_api_set_open },
    });
  }

  door_methods.insert(door_methods.end(), common_methods.begin(), common_methods.end());
  register_type(
      get_entity_internal_type_name(EntityType::DOOR),
      {},
      door_methods,
      metamethods
  );

  // Pickable.
  std::vector<luaL_Reg> pickable_methods = {
      { "has_layer_independent_collisions", entity_api_has_layer_independent_collisions },
      { "set_layer_independent_collisions", entity_api_set_layer_independent_collisions },
      { "get_followed_entity", pickable_api_get_followed_entity },
      { "get_falling_height", pickable_api_get_falling_height },
      { "get_treasure", pickable_api_get_treasure },
  };

  pickable_methods.insert(pickable_methods.end(), common_methods.begin(), common_methods.end());
  register_type(
      get_entity_internal_type_name(EntityType::PICKABLE),
      {},
      pickable_methods,
      metamethods
  );

  // Destructible.
  std::vector<luaL_Reg> destructible_methods = {
      { "get_treasure", destructible_api_get_treasure },
      { "set_treasure", destructible_api_set_treasure },
      { "get_destruction_sound", destructible_api_get_destruction_sound },
      { "set_destruction_sound", destructible_api_set_destruction_sound },
      { "get_can_be_cut", destructible_api_get_can_be_cut },
      { "set_can_be_cut", destructible_api_set_can_be_cut },
      { "get_can_explode", destructible_api_get_can_explode },
      { "set_can_explode", destructible_api_set_can_explode },
      { "get_can_regenerate", destructible_api_get_can_regenerate },
      { "set_can_regenerate", destructible_api_set_can_regenerate },
      { "get_damage_on_enemies", destructible_api_get_damage_on_enemies },
      { "set_damage_on_enemies", destructible_api_set_damage_on_enemies },
      { "get_modified_ground", destructible_api_get_modified_ground },
  };
  if (CurrentQuest::is_format_at_most({ 1, 5 })) {
    destructible_methods.insert(destructible_methods.end(), {
        // Available to all entities since 1.6.
        { "get_weight", entity_api_get_weight },
        { "set_weight", entity_api_set_weight },
    });
  }

  destructible_methods.insert(destructible_methods.end(), common_methods.begin(), common_methods.end());
  register_type(
      get_entity_internal_type_name(EntityType::DESTRUCTIBLE),
      {},
      destructible_methods,
      metamethods
  );

  // Carried object.
  std::vector<luaL_Reg> carried_object_methods = {
  };
  if (CurrentQuest::is_format_at_least({ 1, 6 })) {
    carried_object_methods.insert(carried_object_methods.end(), {
        { "get_destruction_sound", carried_object_api_get_destruction_sound },
        { "set_destruction_sound", carried_object_api_set_destruction_sound },
        { "get_damage_on_enemies", carried_object_api_get_damage_on_enemies },
        { "set_damage_on_enemies", carried_object_api_set_damage_on_enemies },
    });
  }

  carried_object_methods.insert(carried_object_methods.end(), common_methods.begin(), common_methods.end());
  register_type(
      get_entity_internal_type_name(EntityType::CARRIED_OBJECT),
      {},
      carried_object_methods,
      metamethods
  );

  // Dynamic tile.
  std::vector<luaL_Reg> dynamic_tile_methods = {
      { "get_pattern_id", dynamic_tile_api_get_pattern_id },
      { "get_modified_ground", dynamic_tile_api_get_modified_ground },
  };

  dynamic_tile_methods.insert(dynamic_tile_methods.end(), common_methods.begin(), common_methods.end());
  register_type(
      get_entity_internal_type_name(EntityType::DYNAMIC_TILE),
      {},
      dynamic_tile_methods,
      metamethods
  );

  // Enemy.
  std::vector<luaL_Reg> enemy_methods = {
      { "get_breed", enemy_api_get_breed },
      { "get_life", enemy_api_get_life },
      { "set_life", enemy_api_set_life },
      { "add_life", enemy_api_add_life },
      { "remove_life", enemy_api_remove_life },
      { "get_damage", enemy_api_get_damage },
      { "set_damage", enemy_api_set_damage },
      { "is_pushed_back_when_hurt", enemy_api_is_pushed_back_when_hurt },
      { "set_pushed_back_when_hurt", enemy_api_set_pushed_back_when_hurt },
      { "get_push_hero_on_sword", enemy_api_get_push_hero_on_sword },
      { "set_push_hero_on_sword", enemy_api_set_push_hero_on_sword },
      { "get_can_hurt_hero_running", enemy_api_get_can_hurt_hero_running },
      { "set_can_hurt_hero_running", enemy_api_set_can_hurt_hero_running },
      { "get_hurt_style", enemy_api_get_hurt_style },
      { "set_hurt_style", enemy_api_set_hurt_style },
      { "get_can_attack", enemy_api_get_can_attack },
      { "set_can_attack", enemy_api_set_can_attack },
      { "get_minimum_shield_needed", enemy_api_get_minimum_shield_needed },
      { "set_minimum_shield_needed", enemy_api_set_minimum_shield_needed },
      { "get_attack_consequence", enemy_api_get_attack_consequence },
      { "set_attack_consequence", enemy_api_set_attack_consequence },
      { "get_attack_consequence_sprite", enemy_api_get_attack_consequence_sprite },
      { "set_attack_consequence_sprite", enemy_api_set_attack_consequence_sprite },
      { "set_default_attack_consequences", enemy_api_set_default_attack_consequences },
      { "set_default_attack_consequences_sprite", enemy_api_set_default_attack_consequences_sprite },
      { "set_invincible", enemy_api_set_invincible },
      { "set_invincible_sprite", enemy_api_set_invincible_sprite },
      { "has_layer_independent_collisions", entity_api_has_layer_independent_collisions },
      { "set_layer_independent_collisions", entity_api_set_layer_independent_collisions },
      { "get_treasure", enemy_api_get_treasure },
      { "set_treasure", enemy_api_set_treasure },
      { "is_traversable", enemy_api_is_traversable },
      { "set_traversable", enemy_api_set_traversable },
      { "get_obstacle_behavior", enemy_api_get_obstacle_behavior },
      { "set_obstacle_behavior", enemy_api_set_obstacle_behavior },
      { "restart", enemy_api_restart },
      { "hurt", enemy_api_hurt },
      { "immobilize", enemy_api_immobilize },
      { "create_sprite", entity_api_create_sprite },
      { "remove_sprite", entity_api_remove_sprite },
      { "create_enemy", enemy_api_create_enemy },
  };
  if (CurrentQuest::is_format_at_most({ 1, 5 })) {
    enemy_methods.insert(enemy_methods.end(), {
        // Available to all entities since 1.6.
        { "set_size", entity_api_set_size },
        { "set_origin", entity_api_set_origin },
    });
  }
  if (CurrentQuest::is_format_at_least({ 1, 6 })) {
    enemy_methods.insert(enemy_methods.end(), {
        { "get_attacking_collision_mode", enemy_api_get_attacking_collision_mode },
        { "set_attacking_collision_mode", enemy_api_set_attacking_collision_mode },
    });
  }

  enemy_methods.insert(enemy_methods.end(), common_methods.begin(), common_methods.end());
  register_type(
      get_entity_internal_type_name(EntityType::ENEMY),
      {},
      enemy_methods,
      metamethods
  );

  // Custom entity.
  std::vector<luaL_Reg> custom_entity_methods = {
      { "get_model", custom_entity_api_get_model },
      { "set_size", entity_api_set_size },
      { "set_origin", entity_api_set_origin },
      { "get_direction", custom_entity_api_get_direction },
      { "set_direction", custom_entity_api_set_direction },
      { "create_sprite", entity_api_create_sprite },
      { "remove_sprite", entity_api_remove_sprite },
      { "set_traversable_by", custom_entity_api_set_traversable_by },
      { "set_can_traverse", custom_entity_api_set_can_traverse },
      { "can_traverse_ground", custom_entity_api_can_traverse_ground },
      { "set_can_traverse_ground", custom_entity_api_set_can_traverse_ground },
      { "add_collision_test", custom_entity_api_add_collision_test },
      { "clear_collision_tests", custom_entity_api_clear_collision_tests },
      { "has_layer_independent_collisions", entity_api_has_layer_independent_collisions },
      { "set_layer_independent_collisions", entity_api_set_layer_independent_collisions },
      { "get_modified_ground", custom_entity_api_get_modified_ground },
      { "set_modified_ground", custom_entity_api_set_modified_ground },
  };
  if (CurrentQuest::is_format_at_most({ 1, 5 })) {
    // Available to all entities since 1.6.
    custom_entity_methods.insert(custom_entity_methods.end(), {
        { "set_size", entity_api_set_size },
        { "set_origin", entity_api_set_origin },
        { "is_drawn_in_y_order", entity_api_is_drawn_in_y_order },
        { "set_drawn_in_y_order", entity_api_set_drawn_in_y_order },
    });
  }
  if (CurrentQuest::is_format_at_least({ 1, 6 })) {
    custom_entity_methods.insert(custom_entity_methods.end(), {
        { "is_tiled", custom_entity_api_is_tiled },
        { "set_tiled", custom_entity_api_set_tiled },
        { "get_follow_streams", custom_entity_api_get_follow_streams },
        { "set_follow_streams", custom_entity_api_set_follow_streams },
    });
  }

  custom_entity_methods.insert(custom_entity_methods.end(), common_methods.begin(), common_methods.end());
  register_type(
      get_entity_internal_type_name(EntityType::CUSTOM),
      {},
      custom_entity_methods,
      metamethods
  );

  // Also register all other types of entities that have no specific methods.
  register_type(get_entity_internal_type_name(EntityType::TILE), {}, common_methods, metamethods);
  register_type(get_entity_internal_type_name(EntityType::JUMPER), {}, common_methods, metamethods);
  register_type(get_entity_internal_type_name(EntityType::SENSOR), {}, common_methods, metamethods);
  register_type(get_entity_internal_type_name(EntityType::SEPARATOR), {}, common_methods, metamethods);
  register_type(get_entity_internal_type_name(EntityType::WALL), {}, common_methods, metamethods);
  register_type(get_entity_internal_type_name(EntityType::CRYSTAL), {}, common_methods, metamethods);
  register_type(get_entity_internal_type_name(EntityType::CRYSTAL_BLOCK), {}, common_methods, metamethods);
  register_type(get_entity_internal_type_name(EntityType::SHOP_TREASURE), {}, common_methods, metamethods);
  register_type(get_entity_internal_type_name(EntityType::STAIRS), {}, common_methods, metamethods);
  register_type(get_entity_internal_type_name(EntityType::BOMB), {}, common_methods, metamethods);
  register_type(get_entity_internal_type_name(EntityType::EXPLOSION), {}, common_methods, metamethods);
  register_type(get_entity_internal_type_name(EntityType::FIRE), {}, common_methods, metamethods);
  register_type(get_entity_internal_type_name(EntityType::ARROW), {}, common_methods, metamethods);
  register_type(get_entity_internal_type_name(EntityType::HOOKSHOT), {}, common_methods, metamethods);
  register_type(get_entity_internal_type_name(EntityType::BOOMERANG), {}, common_methods, metamethods);
}

/**
 * \brief Returns whether a value is a userdata of type entity.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return true if the value at this index is a entity.
 */
bool LuaContext::is_entity(lua_State* l, int index) {

  // We could return is_hero() || is_tile() || is_dynamic_tile() || ...
  // but this would be tedious, costly and error prone.

  void* udata = lua_touserdata(l, index);
  if (udata == nullptr) {
    // This is not a userdata.
    return false;
  }

  if (!lua_getmetatable(l, index)) {
    // The userdata has no metatable.
    return false;
  }

  // Get the name of the Solarus type from this userdata.
  lua_pushstring(l, "__solarus_type");
  lua_rawget(l, -2);
  if (!lua_isstring(l, -1)) {
    // This is probably a userdata from some library other than Solarus.
    lua_pop(l, 2);
    return false;
  }

  // Check if the type name is one of the entity type names.
  const std::string& type_name = lua_tostring(l, -1);
  lua_pop(l, 2);

  return get_entity_internal_type_names_set().find(type_name) != get_entity_internal_type_names_set().end();
}

/**
 * \brief Checks that the userdata at the specified index of the stack is an
 * entity and returns it.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return The entity.
 */
EntityPtr LuaContext::check_entity(lua_State* l, int index) {

  if (is_entity(l, index)) {
    const ExportableToLuaPtr& userdata = *(static_cast<ExportableToLuaPtr*>(
      lua_touserdata(l, index)
    ));
    return std::static_pointer_cast<Entity>(userdata);
  }
  else {
    LuaTools::type_error(l, index, "entity");
    throw;
  }
}

/**
 * \brief Pushes an entity userdata onto the stack.
 *
 * If the entity or its map does not exist anymore, pushes nil.
 *
 * \param l A Lua context.
 * \param entity An entity.
 */
void LuaContext::push_entity(lua_State* l, Entity& entity) {

  push_userdata(l, entity);
}

/**
 * \brief Pushes a list of entities as an iterator onto the stack.
 *
 * The iterator is pushed onto the stack as one value of type function.
 *
 * \param l A Lua context.
 * \param entity A list of entities. The iterator preserves their order.
 */
void LuaContext::push_entity_iterator(lua_State* l, const EntityVector& entities) {

  // Create a Lua table with the list of entities, preserving their order.
  int i = 0;
  lua_newtable(l);
  for (const EntityPtr& entity: entities) {
    ++i;
    lua_pushinteger(l, i);
    push_entity(l, *entity);
    lua_rawset(l, -3);
  }

  lua_pushinteger(l, entities.size());
  lua_pushinteger(l, 1);
  // 3 upvalues: entities table, size, current index.

  lua_pushcclosure(l, l_entity_iterator_next, 3);
}

/**
 * \brief Returns the Lua metatable name corresponding to a type of map entity.
 * \param entity_type A type of map entity.
 * \return The corresponding Lua metatable name, e.g. "sol.enemy".
 */
const std::string& LuaContext::get_entity_internal_type_name(
    EntityType entity_type) {

  const std::map<EntityType, std::string>& names = get_entity_internal_type_names();
  const auto& it = names.find(entity_type);
  SOLARUS_ASSERT(it != names.end(), "Missing entity internal type name");

  return it->second;
}

/**
 * \brief Closure of an iterator over a list of sprites and their names.
 *
 * This closure expects 3 upvalues in this order:
 * - An array of { name, sprite } pairs.
 * - The size of the array (for performance).
 * - The current index in the array.
 *
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::l_named_sprite_iterator_next(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {

    // Get upvalues.
    const int table_index = lua_upvalueindex(1);
    const int size = lua_tointeger(l, lua_upvalueindex(2));
    int index = lua_tointeger(l, lua_upvalueindex(3));

    if (index > size) {
      // Finished.
      return 0;
    }

    // Get the next value.
    lua_rawgeti(l, table_index, index);   // pair
    lua_rawgeti(l, -1, 1);                // pair name
    lua_rawgeti(l, -2, 2);                // pair name sprite

    // Increment index.
    ++index;
    lua_pushinteger(l, index);
    lua_replace(l, lua_upvalueindex(3));

    return 2;
  });
}

/**
 * \brief Pushes a list of sprites and their names as an iterator onto the stack.
 *
 * The iterator is pushed onto the stack as one value of type function.
 *
 * \param l A Lua context.
 * \param sprites A list of sprites and their names. The iterator preserves their order.
 */
void LuaContext::push_named_sprite_iterator(
    lua_State* l,
    const std::vector<Entity::NamedSprite>& sprites
) {
  // Create a Lua table with the list of name-sprite pairs, preserving their order.
  int i = 0;
  lua_newtable(l);
                                                 // sprites
  for (const Entity::NamedSprite& named_sprite: sprites) {
    if (named_sprite.removed) {
      continue;
    }
    ++i;
    lua_newtable(l);                             // sprites pair
    push_string(l, named_sprite.name);           // sprites pair name
    lua_rawseti(l, -2, 1);                       // sprites pair
    push_sprite(l, *named_sprite.sprite);        // sprites pair sprite
    lua_rawseti(l, -2, 2);                       // sprites pair

    lua_rawseti(l, -2, i);                       // sprites
  }

  lua_pushinteger(l, i);
  lua_pushinteger(l, 1);
  // 3 upvalues: sprites table, size, current index.

  lua_pushcclosure(l, l_named_sprite_iterator_next, 3);
}

/**
 * \brief Implementation of entity:get_type().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_type(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);

    const std::string& type_name = enum_to_name(entity.get_type());
    push_string(l, type_name);
    return 1;
  });
}

/**
 * \brief Implementation of entity:get_map().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_map(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);

    push_map(l, entity.get_map());
    return 1;
  });
}

/**
 * \brief Implementation of entity:get_game().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_game(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);

    push_game(l, entity.get_game().get_savegame());
    return 1;
  });
}

/**
 * \brief Implementation of entity:get_name().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_name(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);

    const std::string& name = entity.get_name();
    if (name.empty()) {
      lua_pushnil(l);
    }
    else {
      push_string(l, name);
    }
    return 1;
  });
}

/**
 * \brief Implementation of entity:exists().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_exists(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);

    lua_pushboolean(l, !entity.is_being_removed());
    return 1;
  });
}

/**
 * \brief Implementation of entity:remove().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_remove(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);

    entity.remove_from_map();

    return 0;
  });
}

/**
 * \brief Implementation of entity:is_enabled().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_is_enabled(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);

    lua_pushboolean(l, entity.is_enabled());
    return 1;
  });
}

/**
 * \brief Implementation of entity:set_enabled().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_set_enabled(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);
    bool enabled = LuaTools::opt_boolean(l, 2, true);

    entity.set_enabled(enabled);

    return 0;
  });
}

/**
 * \brief Implementation of entity:get_size().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_size(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);

    lua_pushinteger(l, entity.get_width());
    lua_pushinteger(l, entity.get_height());
    return 2;
  });
}

/**
 * \brief Implementation of entity:set_size().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_set_size(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);
    int width = LuaTools::check_int(l, 2);
    int height = LuaTools::check_int(l, 3);

    if (width < 0 || width % 8 != 0) {
      std::ostringstream oss;
      oss << "Invalid width: " << width << ": should be a positive multiple of 8";
      LuaTools::arg_error(l, 2, oss.str());
    }
    if (height < 0 || height % 8 != 0) {
      std::ostringstream oss;
      oss << "Invalid height: " << height << ": should be a positive multiple of 8";
      LuaTools::arg_error(l, 3, oss.str());
    }

    entity.set_size(width, height);
    entity.notify_position_changed();

    return 0;
  });
}

/**
 * \brief Implementation of entity:get_origin().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_origin(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);

    const Point& origin = entity.get_origin();

    lua_pushinteger(l, origin.x);
    lua_pushinteger(l, origin.y);
    return 2;
  });
}

/**
 * \brief Implementation of enemy:set_origin().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_set_origin(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);
    int x = LuaTools::check_int(l, 2);
    int y = LuaTools::check_int(l, 3);

    entity.set_origin(x, y);
    entity.notify_position_changed();

    return 0;
  });
}

/**
 * \brief Implementation of entity:get_position().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_position(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);

    lua_pushinteger(l, entity.get_x());
    lua_pushinteger(l, entity.get_y());
    lua_pushinteger(l, entity.get_layer());
    return 3;
  });
}

/**
 * \brief Implementation of entity:set_position().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_set_position(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);
    int x = LuaTools::check_int(l, 2);
    int y = LuaTools::check_int(l, 3);
    int layer = LuaTools::opt_layer(l, 4, entity.get_map(), entity.get_layer());

    Entities& entities = entity.get_map().get_entities();
    entity.set_xy(x, y);
    entities.set_entity_layer(entity, layer);
    entity.notify_position_changed();

    return 0;
  });
}

/**
 * \brief Implementation of entity:get_center_position().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_center_position(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);

    const Point& center_point = entity.get_center_point();
    lua_pushinteger(l, center_point.x);
    lua_pushinteger(l, center_point.y);
    lua_pushinteger(l, entity.get_layer());
    return 3;
  });
}

/**
 * \brief Implementation of entity:get_facing_position().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_facing_position(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);

    const Point& facing_point = entity.get_facing_point();
    lua_pushinteger(l, facing_point.x);
    lua_pushinteger(l, facing_point.y);
    lua_pushinteger(l, entity.get_layer());
    return 3;
  });
}

/**
 * \brief Implementation of entity:get_facing_entity().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_facing_entity(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);

    Entity* facing_entity = entity.get_facing_entity();
    if (facing_entity == nullptr) {
      lua_pushnil(l);
    }
    else {
      push_entity(l, *facing_entity);
    }
    return 1;
  });
}

/**
 * \brief Implementation of entity:get_ground_position().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_ground_position(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);

    const Point& ground_point = entity.get_ground_point();
    lua_pushinteger(l, ground_point.x);
    lua_pushinteger(l, ground_point.y);
    lua_pushinteger(l, entity.get_layer());
    return 3;
  });
}

/**
 * \brief Implementation of entity:get_ground_below().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_ground_below(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);

    Ground ground = entity.get_ground_below();

    push_string(l, enum_to_name(ground));
    return 1;
  });
}

/**
 * \brief Implementation of entity:get_bounding_box().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_bounding_box(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);

    const Rectangle& bounding_box = entity.get_bounding_box();
    lua_pushinteger(l, bounding_box.get_x());
    lua_pushinteger(l, bounding_box.get_y());
    lua_pushinteger(l, bounding_box.get_width());
    lua_pushinteger(l, bounding_box.get_height());
    return 4;
  });
}

/**
 * \brief Implementation of entity:get_max_bounding_box().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_max_bounding_box(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);

    const Rectangle& max_bounding_box = entity.get_max_bounding_box();
    lua_pushinteger(l, max_bounding_box.get_x());
    lua_pushinteger(l, max_bounding_box.get_y());
    lua_pushinteger(l, max_bounding_box.get_width());
    lua_pushinteger(l, max_bounding_box.get_height());
    return 4;
  });
}

/**
 * \brief Implementation of entity:get_layer().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_layer(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);

    lua_pushinteger(l, entity.get_layer());
    return 1;
  });
}

/**
 * \brief Implementation of entity:set_layer().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_set_layer(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);
    int layer = LuaTools::check_layer(l, 2, entity.get_map());

    Entities& entities = entity.get_map().get_entities();
    entities.set_entity_layer(entity, layer);
    entity.notify_position_changed();

    return 0;
  });
}

/**
 * \brief Implementation of entity:overlaps().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_overlaps(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);

    bool overlaps = false;
    if (is_entity(l, 2)) {
      Entity& other_entity = *check_entity(l, 2);
      std::string collision_mode_name = LuaTools::opt_string(l, 3, "overlapping");

      CollisionMode collision_mode = CollisionMode::COLLISION_NONE;
      if (collision_mode_name == "overlapping") {
        collision_mode = CollisionMode::COLLISION_OVERLAPPING;
      }
      else if (collision_mode_name == "containing") {
        collision_mode = CollisionMode::COLLISION_CONTAINING;
      }
      else if (collision_mode_name == "origin") {
        collision_mode = CollisionMode::COLLISION_ORIGIN;
      }
      else if (collision_mode_name == "facing") {
        collision_mode = CollisionMode::COLLISION_FACING;
      }
      else if (collision_mode_name == "touching") {
        collision_mode = CollisionMode::COLLISION_TOUCHING;
      }
      else if (collision_mode_name == "center") {
        collision_mode = CollisionMode::COLLISION_CENTER;
      }
      else if (collision_mode_name == "sprite") {
        collision_mode = CollisionMode::COLLISION_SPRITE;
      }
      else {
        LuaTools::arg_error(l, 3,
            std::string("Invalid name '") + lua_tostring(l, 2) + "'"
        );
      }

      overlaps = entity.test_collision(other_entity, collision_mode);
    }
    else {
      int x = LuaTools::check_int(l, 2);
      int y = LuaTools::check_int(l, 3);
      int width = LuaTools::opt_int(l, 4, 1);
      int height = LuaTools::opt_int(l, 5, 1);
      overlaps = entity.overlaps(Rectangle(x, y, width, height));
    }

    lua_pushboolean(l, overlaps);
    return 1;
  });
}

/**
 * \brief Implementation of entity:snap_to_grid().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_snap_to_grid(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);

    entity.set_aligned_to_grid();

    return 0;
  });
}

/**
 * \brief Implementation of entity:get_distance().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_distance(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);
    int distance;
    if (lua_gettop(l) >= 3) {
      int x = LuaTools::check_number(l, 2);
      int y = LuaTools::check_number(l, 3);
      distance = entity.get_distance(x, y);
    }
    else {
      const Entity& other_entity = *check_entity(l, 2);
      distance = entity.get_distance(other_entity);
    }

    lua_pushinteger(l, distance);
    return 1;
  });
}

/**
 * \brief Implementation of entity:get_angle().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_angle(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);
    double angle;
    if (lua_gettop(l) >= 3) {
      int x = LuaTools::check_number(l, 2);
      int y = LuaTools::check_number(l, 3);
      angle = entity.get_angle(x, y);
    }
    else {
      const Entity& other_entity = *check_entity(l, 2);
      angle = entity.get_angle(other_entity);
    }

    lua_pushnumber(l, angle);
    return 1;
  });
}

/**
 * \brief Implementation of entity:get_direction4_to().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_direction4_to(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);
    double angle;
    if (lua_gettop(l) >= 3) {
      int x = LuaTools::check_number(l, 2);
      int y = LuaTools::check_number(l, 3);
      angle = entity.get_angle(x, y);
    }
    else {
      const Entity& other_entity = *check_entity(l, 2);
      angle = entity.get_angle(other_entity);
    }

    // Convert from radians.
    int direction4 = (angle + Geometry::PI_OVER_4) / Geometry::PI_OVER_2;

    // Normalize.
    direction4 = (direction4 + 4) % 4;

    lua_pushnumber(l, direction4);
    return 1;
  });
}

/**
 * \brief Implementation of entity:get_direction8_to().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_direction8_to(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);
    double angle;
    if (lua_gettop(l) >= 3) {
      int x = LuaTools::check_number(l, 2);
      int y = LuaTools::check_number(l, 3);
      angle = entity.get_angle(x, y);
    }
    else {
      const Entity& other_entity = *check_entity(l, 2);
      angle = entity.get_angle(other_entity);
    }

    // Convert from radians.
    int direction8 = (angle + Geometry::PI_OVER_4 / 2) / Geometry::PI_OVER_4;

    // Normalize.
    direction8 = (direction8 + 8) % 8;

    lua_pushnumber(l, direction8);
    return 1;
  });
}

/**
 * \brief Implementation of entity:bring_to_front().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_bring_to_front(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);

    entity.get_map().get_entities().bring_to_front(entity);

    return 0;
  });
}

/**
 * \brief Implementation of entity:bring_to_back().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_bring_to_back(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);

    entity.get_map().get_entities().bring_to_back(entity);

    return 0;
  });
}

/**
 * \brief Implementation of entity:is_drawn_in_y_order().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_is_drawn_in_y_order(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);

    lua_pushboolean(l, entity.is_drawn_in_y_order());
    return 1;
  });
}

/**
 * \brief Implementation of entity:set_drawn_in_y_order().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_set_drawn_in_y_order(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);
    bool y_order = LuaTools::opt_boolean(l, 2, true);

    entity.set_drawn_in_y_order(y_order);

    return 0;
  });
}

/**
 * \brief Implementation of entity:get_sprite().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_sprite(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);
    std::string sprite_name = LuaTools::opt_string(l, 2 ,"");

    const SpritePtr& sprite = entity.get_sprite(sprite_name);
    if (sprite != nullptr) {
      push_sprite(l, *sprite);
    }
    else {
      lua_pushnil(l);
    }
    return 1;
  });
}

/**
 * \brief Implementation of entity:get_sprites().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_sprites(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);

    const std::vector<Entity::NamedSprite> named_sprites = entity.get_named_sprites();
    push_named_sprite_iterator(l, named_sprites);
    return 1;
  });
}

/**
 * \brief Implementation of enemy:create_sprite().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_create_sprite(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);
    const std::string& animation_set_id = LuaTools::check_string(l, 2);
    const std::string& sprite_name = LuaTools::opt_string(l, 3, "");

    const SpritePtr& sprite = entity.create_sprite(animation_set_id, sprite_name);
    sprite->enable_pixel_collisions();
    if (entity.is_suspended()) {
      sprite->set_suspended(true);
    }

    push_sprite(l, *sprite);
    return 1;
  });
}

/**
 * \brief Implementation of enemy:remove_sprite() and custom_entity:remove_sprite().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_remove_sprite(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);

    if (lua_gettop(l) >= 2) {
      Sprite& sprite = *check_sprite(l, 2);
      bool success = entity.remove_sprite(sprite);
      if (!success) {
        LuaTools::arg_error(l, 2, "This sprite does not belong to this entity");
      }
    }
    else {
      const SpritePtr& sprite = entity.get_sprite();
      if (sprite == nullptr) {
        LuaTools::error(l, "This entity has no sprite");
      }
      entity.remove_sprite(*sprite);
    }

    return 0;
  });
}

/**
 * \brief Implementation of entity:bring_sprite_to_front().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_bring_sprite_to_front(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);
    Sprite& sprite = *check_sprite(l, 2);
    bool success = entity.bring_sprite_to_front(sprite);
    if (!success) {
      LuaTools::arg_error(l, 2, "This sprite does not belong to this entity");
    }

    return 0;
  });
}

/**
 * \brief Implementation of entity:bring_sprite_to_back().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_bring_sprite_to_back(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);
    Sprite& sprite = *check_sprite(l, 2);
    bool success = entity.bring_sprite_to_back(sprite);
    if (!success) {
      LuaTools::arg_error(l, 2, "This sprite does not belong to this entity");
    }

    return 0;
  });
}

/**
 * \brief Implementation of entity:is_visible().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_is_visible(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);

    lua_pushboolean(l, entity.is_visible());
    return 1;
  });
}

/**
 * \brief Implementation of entity:set_visible().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_set_visible(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);
    bool visible = LuaTools::opt_boolean(l, 2, true);

    entity.set_visible(visible);

    return 0;
  });
}

/**
 * \brief Implementation of entity:get_weight().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_weight(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);

    int weight = entity.get_weight();

    lua_pushinteger(l, weight);
    return 1;
  });
}

/**
 * \brief Implementation of entity:set_weight().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_set_weight(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);
    int weight = LuaTools::check_int(l, 2);

    entity.set_weight(weight);

    return 0;
  });
}

/**
 * \brief Implementation of entity:entity_api_get_controlling_stream().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_controlling_stream(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);

    StreamAction* stream_action = entity.get_stream_action();
    if (stream_action == nullptr) {
      lua_pushnil(l);
    }
    else {
      push_stream(l, stream_action->get_stream());
    }
    return 1;
  });
}

/**
 * \brief Implementation of entity:get_movement().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_movement(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);

    const std::shared_ptr<Movement>& movement = entity.get_movement();
    if (movement == nullptr) {
      lua_pushnil(l);
    }
    else {
      push_userdata(l, *movement);
    }

    return 1;
  });
}

/**
 * \brief Implementation of entity:stop_movement().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_stop_movement(lua_State* l) {

  Entity& entity = *check_entity(l, 1);

  entity.clear_movement();

  return 0;
}

/**
 * \brief Implementation of
 * pickable:has_layer_independent_collisions() and
 * enemy:has_layer_independent_collisions().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_has_layer_independent_collisions(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);

    bool independent = entity.has_layer_independent_collisions();

    lua_pushboolean(l, independent);
    return 1;
  });
}

/**
 * \brief Implementation of
 * pickable:set_layer_independent_collisions() and
 * enemy:set_layer_independent_collisions().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_set_layer_independent_collisions(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);
    bool independent = LuaTools::opt_boolean(l, 2, true);

    entity.set_layer_independent_collisions(independent);

    return 0;
  });
}

/**
 * \brief Implementation of entity:test_obstacles().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_test_obstacles(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);
    int dx = LuaTools::opt_int(l, 2, 0);
    int dy = LuaTools::opt_int(l, 3, 0);
    int layer = entity.get_layer();
    if (lua_gettop(l) >= 4) {
      layer = LuaTools::check_layer(l, 4, entity.get_map());
    }

    Rectangle bounding_box = entity.get_bounding_box();
    bounding_box.add_xy(dx, dy);

    lua_pushboolean(l, entity.get_map().test_collision_with_obstacles(
        layer, bounding_box, entity));
    return 1;
  });
}

/**
 * \brief Implementation of entity:get_optimization_distance().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_optimization_distance(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);

    lua_pushinteger(l, entity.get_optimization_distance());
    return 1;
  });
}

/**
 * \brief Implementation of entity:set_optimization_distance().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_set_optimization_distance(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);
    int distance = LuaTools::check_int(l, 2);

    entity.set_optimization_distance(distance);

    return 0;
  });
}

/**
 * \brief Implementation of entity:is_in_same_region().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_is_in_same_region(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);
    const Entity& other_entity = *check_entity(l, 2);

    lua_pushboolean(l, entity.is_in_same_region(other_entity));
    return 1;
  });
}

/**
 * \brief Implementation of hero:get_state() and camera:get_state().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_state(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);

    std::string state_name = entity.get_state_name();
    if (state_name.empty()) {
      lua_pushnil(l);
    }
    else {
      push_string(l, state_name);
    }
    return 1;
  });
}

/**
 * \brief Implementation of entity:get_property().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_property(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);
    const std::string& key = LuaTools::check_string(l, 2);

    if (!entity.has_user_property(key)) {
      lua_pushnil(l);
    }
    else {
      const std::string& value = entity.get_user_property_value(key);
      push_string(l, value);
    }
    return 1;
  });
}

/**
 * \brief Implementation of entity:set_property().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_set_property(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);
    const std::string& key = LuaTools::check_string(l, 2);

    if (lua_isnil(l, 3)) {
      entity.remove_user_property(key);
    }
    else {
      const std::string& value = LuaTools::check_string(l, 3);

      if (!EntityData::is_user_property_key_valid(key)) {
        LuaTools::arg_error(l, 2, "Invalid property key: '" + key + "'");
      }
      entity.set_user_property_value(key, value);
    }

    return 0;
  });
}

/**
 * \brief Implementation of entity:get_properties().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_get_properties(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Entity& entity = *check_entity(l, 1);

    const std::vector<Entity::UserProperty>& properties = entity.get_user_properties();
    lua_createtable(l, properties.size(), 0);
    int i = 1;
    for (const Entity::UserProperty& property : properties) {
      lua_createtable(l, 0, 2);
      push_string(l, property.first);
      lua_setfield(l, -2, "key");
      push_string(l, property.second);
      lua_setfield(l, -2, "value");
      lua_rawseti(l, -2, i);
      ++i;
    }

    return 1;
  });
}

/**
 * \brief Implementation of entity:set_properties().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::entity_api_set_properties(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Entity& entity = *check_entity(l, 1);
    LuaTools::check_type(l, 2, LUA_TTABLE);

    entity.set_user_properties({});
    lua_pushnil(l);
    while (lua_next(l, 2) != 0) {
      LuaTools::check_type(l, -1, LUA_TTABLE);
      const std::string& key = LuaTools::check_string_field(l, -1, "key");
      const std::string& value = LuaTools::check_string_field(l, -1, "value");
      if (entity.has_user_property(key)) {
        LuaTools::error(l, "Duplicate property '" + key + "'");
      }
      if (!EntityData::is_user_property_key_valid(key)) {
        LuaTools::error(l, "Invalid property key: '" + key + "'");
      }
      entity.set_user_property_value(key, value);
      lua_pop(l, 1);
    }

    return 1;
  });
}

/**
 * \brief Returns whether a value is a userdata of type hero.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return true if the value at this index is a hero.
 */
bool LuaContext::is_hero(lua_State* l, int index) {
  return is_userdata(l, index, get_entity_internal_type_name(EntityType::HERO));
}

/**
 * \brief Checks that the userdata at the specified index of the stack is a
 * hero and returns it.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return The hero.
 */
std::shared_ptr<Hero> LuaContext::check_hero(lua_State* l, int index) {
  return std::static_pointer_cast<Hero>(check_userdata(
      l, index, get_entity_internal_type_name(EntityType::HERO)
  ));
}

/**
 * \brief Pushes a hero userdata onto the stack.
 * \param l A Lua context.
 * \param hero A hero.
 */
void LuaContext::push_hero(lua_State* l, Hero& hero) {
  push_userdata(l, hero);
}

/**
 * \brief Implementation of hero:teleport().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_teleport(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);
    const std::string& map_id = LuaTools::check_string(l, 2);
    const std::string& destination_name = LuaTools::opt_string(l, 3, "");
    Transition::Style transition_style = LuaTools::opt_enum<Transition::Style>(
        l, 4, Transition::Style::FADE);

    if (!CurrentQuest::resource_exists(ResourceType::MAP, map_id)) {
      LuaTools::arg_error(l, 2, std::string("No such map: '") + map_id + "'");
    }

    hero.get_game().set_current_map(map_id, destination_name, transition_style);

    return 0;
  });
}

/**
 * \brief Implementation of hero:get_direction().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_get_direction(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Hero& hero = *check_hero(l, 1);

    lua_pushinteger(l, hero.get_animation_direction());
    return 1;
  });
}

/**
 * \brief Implementation of hero:set_direction().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_set_direction(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);
    int direction = LuaTools::check_int(l, 2);

    hero.set_animation_direction(direction);

    return 0;
  });
}

/**
 * \brief Implementation of hero:get_walking_speed().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_get_walking_speed(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Hero& hero = *check_hero(l, 1);

    lua_pushinteger(l, hero.get_normal_walking_speed());
    return 1;
  });
}

/**
 * \brief Implementation of hero:set_walking_speed().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_set_walking_speed(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);
    int normal_walking_speed = LuaTools::check_int(l, 2);

    hero.set_normal_walking_speed(normal_walking_speed);

    return 0;
  });
}

/**
 * \brief Implementation of hero:save_solid_ground().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_save_solid_ground(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);

    ScopedLuaRef callback;
    if (lua_gettop(l) == 2) {
      // Function parameter.
      if (lua_isnil(l, 2)) {
        hero.reset_target_solid_ground_callback();
        return 0;
      }
      else {
        callback = LuaTools::check_function(l, 2);
      }
    }
    else {
      // Coordinates and layer.
      int x = 0;
      int y = 0;
      int layer = 0;
      if (lua_gettop(l) >= 2) {
        x = LuaTools::check_int(l, 2);
        y = LuaTools::check_int(l, 3);
        layer = LuaTools::check_layer(l, 4, hero.get_map());
      }
      else {
        x = hero.get_x();
        y = hero.get_y();
        layer = hero.get_layer();
      }
      callback = hero.make_solid_ground_callback(Point(x, y), layer);
    }
    hero.set_target_solid_ground_callback(callback);

    return 0;
  });
}

/**
 * \brief Implementation of hero:reset_solid_ground().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_reset_solid_ground(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);

    hero.reset_target_solid_ground_callback();

    return 0;
  });
}

/**
 * \brief Implementation of hero:get_solid_ground().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_get_solid_ground_position(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Hero& hero = *check_hero(l, 1);

    Point xy;
    int layer = 0;

    const ScopedLuaRef& solid_ground_callback = hero.get_target_solid_ground_callback();
    if (!solid_ground_callback.is_empty()) {
      // Coordinates memorized by hero:save_solid_ground().
      solid_ground_callback.push();
      bool success = LuaTools::call_function(l, 0, 3, "Solid ground callback");
      if (!success) {
        // Fallback: use the last solid ground position.
        xy = hero.get_last_solid_ground_coords();
        layer = hero.get_last_solid_ground_layer();
        lua_pushinteger(l, xy.x);
        lua_pushinteger(l, xy.y);
        lua_pushinteger(l, layer);
        return 3;
      }
      else {
        // Normal case: use the result of the function.
        return 3;
      }
    }
    else if (hero.get_last_solid_ground_coords().x != -1) {
      xy = hero.get_last_solid_ground_coords();
      layer = hero.get_last_solid_ground_layer();
      // Last solid ground coordinates.
      lua_pushinteger(l, xy.x);
      lua_pushinteger(l, xy.y);
      lua_pushinteger(l, layer);
      return 3;
    }
    else {
      // No solid ground coordinates.
      // Maybe the map started in water.
      lua_pushnil(l);
      return 1;
    }
  });
}

/**
 * \brief Implementation of hero:get_animation().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_get_animation(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);

    const std::string& animation = hero.get_hero_sprites().get_tunic_animation();

    push_string(l, animation);
    return 1;
  });
}

/**
 * \brief Implementation of hero:set_animation().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_set_animation(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);
    const std::string& animation = LuaTools::check_string(l, 2);
    const ScopedLuaRef& callback_ref = LuaTools::opt_function(l, 3);

    HeroSprites& sprites = hero.get_hero_sprites();
    if (!sprites.has_tunic_animation(animation)) {
      LuaTools::arg_error(l, 2,
          std::string("No such animation in tunic sprite: '") + animation + "'"
      );
    }

    sprites.set_animation(animation, callback_ref);

    return 0;
  });
}

/**
 * \brief Implementation of hero:get_tunic_sprite_id().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_get_tunic_sprite_id(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);

    const std::string& sprite_id = hero.get_hero_sprites().get_tunic_sprite_id();

    push_string(l, sprite_id);
    return 1;
  });
}

/**
 * \brief Implementation of hero:set_tunic_sprite_id().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_set_tunic_sprite_id(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);
    const std::string& sprite_id = LuaTools::check_string(l, 2);

    // TODO check the existence of the sprite animation set
    // (see also sol.sprite.create()).
    hero.get_hero_sprites().set_tunic_sprite_id(sprite_id);

    return 0;
  });
}

/**
 * \brief Implementation of hero:get_sword_sprite_id().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_get_sword_sprite_id(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);

    const std::string& sprite_id = hero.get_hero_sprites().get_sword_sprite_id();

    push_string(l, sprite_id);
    return 1;
  });
}

/**
 * \brief Implementation of hero:set_sword_sprite_id().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_set_sword_sprite_id(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);
    const std::string& sprite_id = LuaTools::check_string(l, 2);

    hero.get_hero_sprites().set_sword_sprite_id(sprite_id);

    return 0;
  });
}

/**
 * \brief Implementation of hero:get_sword_sound_id().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_get_sword_sound_id(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);

    const std::string& sound_id = hero.get_hero_sprites().get_sword_sound_id();

    push_string(l, sound_id);
    return 1;
  });
}

/**
 * \brief Implementation of hero:set_sword_sound_id().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_set_sword_sound_id(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);
    const std::string& sound_id = LuaTools::check_string(l, 2);

    hero.get_hero_sprites().set_sword_sound_id(sound_id);

    return 0;
  });
}

/**
 * \brief Implementation of hero:get_shield_sprite_id().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_get_shield_sprite_id(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);

    const std::string& sprite_id = hero.get_hero_sprites().get_shield_sprite_id();

    push_string(l, sprite_id);
    return 1;
  });
}

/**
 * \brief Implementation of hero:set_shield_sprite_id().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_set_shield_sprite_id(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);
    const std::string& sprite_id = LuaTools::check_string(l, 2);

    hero.get_hero_sprites().set_shield_sprite_id(sprite_id);

    return 0;
  });
}

/**
 * \brief Implementation of hero:is_blinking().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_is_blinking(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);

    lua_pushboolean(l, hero.get_hero_sprites().is_blinking());
    return 1;
  });
}

/**
 * \brief Implementation of hero:set_blinking().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_set_blinking(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);
    bool blinking = LuaTools::opt_boolean(l, 2, true);
    uint32_t duration = LuaTools::opt_int(l, 3, 0);

    if (blinking) {
      hero.get_hero_sprites().blink(duration);
    }
    else {
      hero.get_hero_sprites().stop_blinking();
    }

    return 0;
  });
}

/**
 * \brief Implementation of hero:is_invincible().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_is_invincible(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Hero& hero = *check_hero(l, 1);

    lua_pushboolean(l, hero.is_invincible());
    return 1;
  });
}

/**
 * \brief Implementation of hero:set_invincible().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_set_invincible(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);
    bool invincible = LuaTools::opt_boolean(l, 2, true);
    uint32_t duration = LuaTools::opt_int(l, 3, 0);

    hero.set_invincible(invincible, duration);

    return 0;
  });
}

/**
 * \brief Implementation of hero:get_carried_object().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_get_carried_object(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);

    const std::shared_ptr<CarriedObject>& carried_object = hero.get_carried_object();
    if (carried_object == nullptr) {
      lua_pushnil(l);
    }
    else {
      push_carried_object(l, *carried_object);
    }
    return 1;
  });
}

/**
 * \brief Implementation of hero:freeze().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_freeze(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);

    hero.start_frozen();

    return 0;
  });
}

/**
 * \brief Implementation of hero:unfreeze().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_unfreeze(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);

    hero.start_state_from_ground();

    return 0;
  });
}

/**
 * \brief Implementation of hero:walk().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_walk(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);
    const std::string& path = LuaTools::check_string(l, 2);
    bool loop = LuaTools::opt_boolean(l, 3, false);
    bool ignore_obstacles = LuaTools::opt_boolean(l, 4, false);

    hero.start_forced_walking(path, loop, ignore_obstacles);

    return 0;
  });
}

/**
 * \brief Implementation of hero:start_attack().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_start_attack(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);

    if (hero.can_start_sword()) {
      hero.start_sword();
    }

    return 0;
  });
}

/**
 * \brief Implementation of hero:start_item().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_start_item(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);
    EquipmentItem& item = *check_item(l, 2);

    if (!item.is_saved()) {
      LuaTools::arg_error(l, 2,
          std::string("Cannot use item '" + item.get_name() + "': this item is not saved"));
    }
    if (hero.can_start_item(item)) {
      hero.start_item(item);
    }

    return 0;
  });
}

/**
 * \brief Implementation of hero:start_jumping().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_start_jumping(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);
    int direction = LuaTools::check_int(l, 2);
    int length = LuaTools::check_int(l, 3);
    bool ignore_obstacles = LuaTools::opt_boolean(l, 4, false);

    hero.start_jumping(direction, length, ignore_obstacles, false);

    return 0;
  });
}

/**
 * \brief Implementation of hero:start_treasure().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_start_treasure(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);
    const std::string& item_name = LuaTools::check_string(l, 2);
    int variant = LuaTools::opt_int(l, 3, 1);
    const std::string& savegame_variable = LuaTools::opt_string(l, 4, "");

    if (!savegame_variable.empty()
        && !LuaTools::is_valid_lua_identifier(savegame_variable)) {
      LuaTools::arg_error(l, 4, std::string(
          "savegame variable identifier expected, got '") +
          savegame_variable + "'");
    }

    if (!hero.get_game().get_equipment().item_exists(item_name)) {
      LuaTools::arg_error(l, 2, std::string("No such item: '") + item_name + "'");
    }

    Treasure treasure(hero.get_game(), item_name, variant, savegame_variable);
    if (treasure.is_found()) {
      LuaTools::arg_error(l, 4, "This treasure is already found");
    }
    if (!treasure.is_obtainable()) {
      LuaTools::arg_error(l, 4, "This treasure is not obtainable");
    }

    const ScopedLuaRef& callback_ref = LuaTools::opt_function(l, 5);

    hero.start_treasure(treasure, callback_ref);

    return 0;
  });
}

/**
 * \brief Implementation of hero:start_victory().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_start_victory(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);
    ScopedLuaRef callback_ref = LuaTools::opt_function(l, 2);

    hero.start_victory(callback_ref);

    return 0;
  });
}

/**
 * \brief Implementation of hero:start_boomerang().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_start_boomerang(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);
    int max_distance = LuaTools::check_int(l, 2);
    int speed = LuaTools::check_int(l, 3);
    const std::string& tunic_preparing_animation = LuaTools::check_string(l, 4);
    const std::string& sprite_name = LuaTools::check_string(l, 5);

    hero.start_boomerang(max_distance, speed,
        tunic_preparing_animation, sprite_name);

    return 0;
  });
}

/**
 * \brief Implementation of hero:start_bow().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_start_bow(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);

    hero.start_bow();

    return 0;
  });
}

/**
 * \brief Implementation of hero:start_hookshot().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_start_hookshot(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);

    hero.start_hookshot();

    return 0;
  });
}

/**
 * \brief Implementation of hero:start_running().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_start_running(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Hero& hero = *check_hero(l, 1);

    hero.start_running();

    return 0;
  });
}

/**
 * \brief Implementation of hero:start_hurt().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::hero_api_start_hurt(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    // There are three possible prototypes:
    // - hero:start_hurt(damage)
    // - hero:start_hurt(source_x, source_y, damage)
    // - hero:start_hurt(source_entity, [source_sprite], damage)
    Hero& hero = *check_hero(l, 1);

    if (lua_gettop(l) <= 2) {
      // hero:start_hurt(damage)
      int damage = LuaTools::check_int(l, 2);
      hero.hurt(damage);
    }
    else if (lua_isnumber(l, 2)) {
      // hero:start_hurt(source_x, source_y, damage)
      int source_x = LuaTools::check_int(l, 2);
      int source_y = LuaTools::check_int(l, 3);
      int damage = LuaTools::check_int(l, 4);
      hero.hurt(Point(source_x, source_y), damage);
    }
    else {
      // hero:start_hurt(source_entity, [source_sprite], damage)
      Entity& source_entity = *check_entity(l, 2);
      SpritePtr source_sprite;
      int index = 3;
      if (is_sprite(l, 3)) {
        source_sprite = check_sprite(l, 3);
        index = 4;
      }
      int damage = LuaTools::check_int(l, index);
      hero.hurt(source_entity, source_sprite.get(), damage);
    }

    return 0;
  });
}

/**
 * \brief Notifies Lua that the hero is brandishing a treasure.
 *
 * Lua then manages the treasure's dialog if any.
 *
 * \param treasure The treasure being brandished.
 * \param callback_ref Lua ref to a function to call when the
 * treasure's dialog finishes (possibly an empty ref).
 */
void LuaContext::notify_hero_brandish_treasure(
    const Treasure& treasure,
    const ScopedLuaRef& callback_ref
) {
  // This is getting tricky. We will define our own dialog callback
  // that will do some work and call callback_ref.
  std::ostringstream oss;
  oss << "_treasure." << treasure.get_item_name() << "." << treasure.get_variant();
  const std::string& dialog_id = oss.str();
  Game& game = treasure.get_game();

  push_item(l, treasure.get_item());
  lua_pushinteger(l, treasure.get_variant());
  push_string(l, treasure.get_savegame_variable());
  push_ref(l, callback_ref);
  lua_pushcclosure(l, l_treasure_brandish_finished, 4);
  const ScopedLuaRef& treasure_callback_ref = create_ref();

  if (!CurrentQuest::dialog_exists(dialog_id)) {
    // No treasure dialog: keep brandishing the treasure for some delay
    // and then execute the callback.
    TimerPtr timer = std::make_shared<Timer>(3000);
    push_map(l, game.get_current_map());
    add_timer(timer, -1, treasure_callback_ref);
    lua_pop(l, 1);
  }
  else {
    // A treasure dialog exists. Show it and then execute the callback.
    game.start_dialog(dialog_id, ScopedLuaRef(), treasure_callback_ref);
  }
}

/**
 * \brief Callback function executed after the animation of brandishing
 * a treasure.
 *
 * Upvalues: item, variant, savegame variable, callback/nil.
 *
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::l_treasure_brandish_finished(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    LuaContext& lua_context = get_lua_context(l);

    // The treasure's dialog is over.
    EquipmentItem& item = *check_item(l, lua_upvalueindex(1));
    int treasure_variant = LuaTools::check_int(l, lua_upvalueindex(2));
    const std::string& treasure_savegame_variable =
        LuaTools::check_string(l, lua_upvalueindex(3));
    lua_pushvalue(l, lua_upvalueindex(4));

    // Check upvalues. Any error here would be the fault of the C++ side
    // because the user cannot call this function.
    Debug::check_assertion(item.get_game() != nullptr,
        "Equipment item without game");

    Debug::check_assertion(lua_isnil(l, -1) || lua_isfunction(l, -1),
        "Expected function or nil for treasure callback");

    Game& game = *item.get_game();
    Hero& hero = *game.get_hero();
    const Treasure treasure(game, item.get_name(), treasure_variant, treasure_savegame_variable);

    // Notify the Lua item and the Lua map.
    if (!lua_isnil(l, -1)) {
      // There is a user callback for this treasure.
      lua_context.call_function(0, 0, "treasure callback");
    }
    lua_context.item_on_obtained(item, treasure);
    lua_context.map_on_obtained_treasure(game.get_current_map(), treasure);

    if (hero.is_brandishing_treasure()) {
      // The script may have changed the hero's state.
      // If not, stop the treasure state.
      hero.start_free();
    }

    return 0;
  });
}

/**
 * \brief Returns whether a value is a userdata of type camera.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return true if the value at this index is a camera.
 */
bool LuaContext::is_camera(lua_State* l, int index) {
  return is_userdata(l, index, get_entity_internal_type_name(EntityType::CAMERA));
}

/**
 * \brief Checks that the userdata at the specified index of the stack is a
 * camera and returns it.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return The camera.
 */
std::shared_ptr<Camera> LuaContext::check_camera(lua_State* l, int index) {
  return std::static_pointer_cast<Camera>(check_userdata(
      l, index, get_entity_internal_type_name(EntityType::CAMERA)
  ));
}

/**
 * \brief Pushes a camera userdata onto the stack.
 * \param l A Lua context.
 * \param camera A camera.
 */
void LuaContext::push_camera(lua_State* l, Camera& camera) {
  push_userdata(l, camera);
}

/**
 * \brief Implementation of camera:get_position_on_screen().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::camera_api_get_position_on_screen(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Camera& camera = *check_camera(l, 1);

    const Point& position_on_screen = camera.get_position_on_screen();

    lua_pushinteger(l, position_on_screen.x);
    lua_pushinteger(l, position_on_screen.y);

    return 2;
  });
}

/**
 * \brief Implementation of camera:set_position_on_screen().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::camera_api_set_position_on_screen(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Camera& camera = *check_camera(l, 1);
    int x = LuaTools::check_int(l, 2);
    int y = LuaTools::check_int(l, 3);

    camera.set_position_on_screen({ x, y });

    return 0;
  });
}

/**
 * \brief Implementation of camera:start_tracking().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::camera_api_start_tracking(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Camera& camera = *check_camera(l, 1);
    EntityPtr entity = check_entity(l, 2);

    camera.start_tracking(entity);

    return 0;
  });
}

/**
 * \brief Implementation of camera:start_manual().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::camera_api_start_manual(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Camera& camera = *check_camera(l, 1);

    camera.start_manual();

    return 0;
  });
}

/**
 * \brief Implementation of camera:get_position_to_track().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::camera_api_get_position_to_track(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Camera& camera = *check_camera(l, 1);

    Point xy;
    if (lua_isnumber(l, 2)) {
      xy.x = LuaTools::check_int(l, 2);
      xy.y = LuaTools::check_int(l, 3);
    }
    else if (is_entity(l, 2)) {
      const Entity& entity = *check_entity(l, 2);
      xy = entity.get_center_point();
    }
    else {
      LuaTools::type_error(l, 2, "number or entity");
    }
    const Point& position_to_track = camera.get_position_to_track(xy);

    lua_pushinteger(l, position_to_track.x);
    lua_pushinteger(l, position_to_track.y);

    return 2;
  });
}

/**
 * \brief Implementation of camera:get_tracked_entity().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::camera_api_get_tracked_entity(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Camera& camera = *check_camera(l, 1);

    EntityPtr entity = camera.get_tracked_entity();
    if (entity == nullptr) {
      lua_pushnil(l);
    }
    else {
      push_entity(l, *entity);
    }
    return 1;
  });
}

/**
 * \brief Returns whether a value is a userdata of type destination.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return \c true if the value at this index is an destination.
 */
bool LuaContext::is_destination(lua_State* l, int index) {
  return is_userdata(l, index, get_entity_internal_type_name(EntityType::DESTINATION));
}

/**
 * \brief Checks that the userdata at the specified index of the stack is a
 * destination and returns it.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return The destination.
 */
std::shared_ptr<Destination> LuaContext::check_destination(lua_State* l, int index) {
  return std::static_pointer_cast<Destination>(check_userdata(
      l, index, get_entity_internal_type_name(EntityType::DESTINATION)
  ));
}

/**
 * \brief Pushes an destination userdata onto the stack.
 * \param l A Lua context.
 * \param destination A destination.
 */
void LuaContext::push_destination(lua_State* l, Destination& destination) {
  push_userdata(l, destination);
}

/**
 * \brief Implementation of destination:get_starting_location_mode().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::destination_api_get_starting_location_mode(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Destination& destination = *check_destination(l, 1);

    StartingLocationMode mode = destination.get_starting_location_mode();

    push_string(l, enum_to_name(mode));
    return 1;
  });
}

/**
 * \brief Implementation of destination:set_starting_location_mode().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::destination_api_set_starting_location_mode(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Destination& destination = *check_destination(l, 1);
    StartingLocationMode mode = StartingLocationMode::WHEN_WORLD_CHANGES;

    if (lua_gettop(l) == 1) {
      LuaTools::type_error(l, 2, "string or nil");
    }
    if (!lua_isnil(l, 2)) {
      mode = LuaTools::check_enum<StartingLocationMode>(l, 2);
    }

    destination.set_starting_location_mode(mode);
    return 0;
  });
}

/**
 * \brief Returns whether a value is a userdata of type teletransporter.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return \c true if the value at this index is an teletransporter.
 */
bool LuaContext::is_teletransporter(lua_State* l, int index) {
  return is_userdata(l, index, get_entity_internal_type_name(EntityType::TELETRANSPORTER));
}

/**
 * \brief Checks that the userdata at the specified index of the stack is a
 * teletransporter and returns it.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return The teletransporter.
 */
std::shared_ptr<Teletransporter> LuaContext::check_teletransporter(lua_State* l, int index) {
  return std::static_pointer_cast<Teletransporter>(check_userdata(
      l, index, get_entity_internal_type_name(EntityType::TELETRANSPORTER)
  ));
}

/**
 * \brief Pushes an teletransporter userdata onto the stack.
 * \param l A Lua context.
 * \param teletransporter A teletransporter.
 */
void LuaContext::push_teletransporter(lua_State* l, Teletransporter& teletransporter) {
  push_userdata(l, teletransporter);
}

/**
 * \brief Implementation of teletransporter:get_sound().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::teletransporter_api_get_sound(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Teletransporter& teletransporter = *check_teletransporter(l, 1);

    const std::string& sound_id = teletransporter.get_sound_id();

    if (sound_id.empty()) {
      lua_pushnil(l);
    }
    else {
      push_string(l, sound_id);
    }
    return 1;
  });
}

/**
 * \brief Implementation of teletransporter:set_sound().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::teletransporter_api_set_sound(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Teletransporter& teletransporter = *check_teletransporter(l, 1);

    std::string sound_id;
    if (lua_gettop(l) > 1) {
      sound_id = LuaTools::check_string(l, 2);
    }

    teletransporter.set_sound_id(sound_id);
    return 0;
  });
}

/**
 * \brief Implementation of teletransporter:get_transition().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::teletransporter_api_get_transition(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Teletransporter& teletransporter = *check_teletransporter(l, 1);

    push_string(l, enum_to_name(teletransporter.get_transition_style()));
    return 1;
  });
}

/**
 * \brief Implementation of teletransporter:set_transition().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::teletransporter_api_set_transition(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Teletransporter& teletransporter = *check_teletransporter(l, 1);
    Transition::Style transition_style = LuaTools::check_enum<Transition::Style>(
        l, 2
    );

    teletransporter.set_transition_style(transition_style);

    return 0;
  });
}

/**
 * \brief Implementation of teletransporter:get_destination_map().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::teletransporter_api_get_destination_map(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Teletransporter& teletransporter = *check_teletransporter(l, 1);

    const std::string& map_id = teletransporter.get_destination_map_id();

    push_string(l, map_id);
    return 1;
  });
}

/**
 * \brief Implementation of teletransporter:set_destination_map().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::teletransporter_api_set_destination_map(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Teletransporter& teletransporter = *check_teletransporter(l, 1);
    const std::string& map_id = LuaTools::check_string(l, 2);

    teletransporter.set_destination_map_id(map_id);

    return 0;
  });
}

/**
 * \brief Implementation of teletransporter:get_destination_name().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::teletransporter_api_get_destination_name(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Teletransporter& teletransporter = *check_teletransporter(l, 1);

    const std::string& destination_name = teletransporter.get_destination_name();

    push_string(l, destination_name);
    return 1;
  });
}

/**
 * \brief Implementation of teletransporter:set_destination_name().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::teletransporter_api_set_destination_name(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Teletransporter& teletransporter = *check_teletransporter(l, 1);
    const std::string& destination_name = LuaTools::check_string(l, 2);

    teletransporter.set_destination_name(destination_name);

    return 0;
  });
}

/**
 * \brief Returns whether a value is a userdata of type NPC.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return true if the value at this index is an NPC.
 */
bool LuaContext::is_npc(lua_State* l, int index) {
  return is_userdata(l, index, get_entity_internal_type_name(EntityType::NPC));
}

/**
 * \brief Checks that the userdata at the specified index of the stack is an
 * NPC and returns it.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return The NPC.
 */
std::shared_ptr<Npc> LuaContext::check_npc(lua_State* l, int index) {
  return std::static_pointer_cast<Npc>(check_userdata(
      l, index, get_entity_internal_type_name(EntityType::NPC))
  );
}

/**
 * \brief Pushes an NPC userdata onto the stack.
 * \param l A Lua context.
 * \param npc An NPC.
 */
void LuaContext::push_npc(lua_State* l, Npc& npc) {
  push_userdata(l, npc);
}

/**
 * \brief Implementation of npc:is_traversable().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::npc_api_is_traversable(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Npc& npc = *check_npc(l, 1);

    lua_pushboolean(l, npc.is_traversable());
    return 1;
  });
}

/**
 * \brief Implementation of npc:set_traversable().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::npc_api_set_traversable(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Npc& npc = *check_npc(l, 1);

    bool traversable = LuaTools::opt_boolean(l, 2, true);

    npc.set_traversable(traversable);

    return 0;
  });
}

/**
 * \brief Returns whether a value is a userdata of type chest.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return true if the value at this index is a chest.
 */
bool LuaContext::is_chest(lua_State* l, int index) {
  return is_userdata(l, index, get_entity_internal_type_name(EntityType::CHEST));
}

/**
 * \brief Checks that the userdata at the specified index of the stack is a
 * chest and returns it.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return The chest.
 */
std::shared_ptr<Chest> LuaContext::check_chest(lua_State* l, int index) {
  return std::static_pointer_cast<Chest>(check_userdata(
      l, index, get_entity_internal_type_name(EntityType::CHEST)
  ));
}

/**
 * \brief Pushes a chest userdata onto the stack.
 * \param l A Lua context.
 * \param chest A chest.
 */
void LuaContext::push_chest(lua_State* l, Chest& chest) {
  push_userdata(l, chest);
}

/**
 * \brief Implementation of chest:is_open().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::chest_api_is_open(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Chest& chest = *check_chest(l, 1);

    lua_pushboolean(l, chest.is_open());
    return 1;
  });
}

/**
 * \brief Implementation of chest:set_open().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::chest_api_set_open(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Chest& chest = *check_chest(l, 1);
    bool open = LuaTools::opt_boolean(l, 2, true);

    chest.set_open(open);

    return 0;
  });
}

/**
 * \brief Implementation of chest:get_treasure().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::chest_api_get_treasure(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Chest& chest = *check_chest(l, 1);
    const Treasure& treasure = chest.get_treasure();

    if (treasure.is_empty()) {
      // No treasure.
      lua_pushnil(l);
      lua_pushnil(l);
    }
    else {
      push_string(l, treasure.get_item_name());
      lua_pushinteger(l, treasure.get_variant());
    }
    if (!treasure.is_saved()) {
      lua_pushnil(l);
    }
    else {
      push_string(l, treasure.get_savegame_variable());
    }
    return 3;
  });
}

/**
 * \brief Implementation of chest:set_treasure().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::chest_api_set_treasure(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Chest& chest = *check_chest(l, 1);
    std::string item_name;
    int variant = 1;
    std::string savegame_variable;

    if (lua_gettop(l) >= 2 && !lua_isnil(l, 2)) {
      item_name = LuaTools::check_string(l, 2);
    }
    if (lua_gettop(l) >= 3 && !lua_isnil(l, 3)) {
      variant = LuaTools::check_int(l, 3);
    }
    if (lua_gettop(l) >= 4 && !lua_isnil(l, 4)) {
      savegame_variable = LuaTools::check_string(l, 4);
    }

    if (!savegame_variable.empty()
        && !LuaTools::is_valid_lua_identifier(savegame_variable)) {
      LuaTools::arg_error(l, 4,
          std::string("savegame variable identifier expected, got '")
      + savegame_variable + "'");
    }

    Treasure treasure(chest.get_game(), item_name, variant, savegame_variable);
    chest.set_treasure(treasure);

    return 0;
  });
}

/**
 * \brief Returns whether a value is a userdata of type block.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return true if the value at this index is a block.
 */
bool LuaContext::is_block(lua_State* l, int index) {
  return is_userdata(l, index, get_entity_internal_type_name(EntityType::BLOCK));
}

/**
 * \brief Checks that the userdata at the specified index of the stack is a
 * block and returns it.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return The block.
 */
std::shared_ptr<Block> LuaContext::check_block(lua_State* l, int index) {
  return std::static_pointer_cast<Block>(check_userdata(
      l, index, get_entity_internal_type_name(EntityType::BLOCK)
  ));
}

/**
 * \brief Pushes a block userdata onto the stack.
 * \param l A Lua context.
 * \param block A block.
 */
void LuaContext::push_block(lua_State* l, Block& block) {
  push_userdata(l, block);
}

/**
 * \brief Implementation of block:reset().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::block_api_reset(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Block& block = *check_block(l, 1);

    block.reset();

    return 0;
  });
}

/**
 * \brief Implementation of block:is_pushable().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::block_api_is_pushable(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Block& block = *check_block(l, 1);

    lua_pushboolean(l, block.is_pushable());
    return 1;
  });
}

/**
 * \brief Implementation of block:set_pushable().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::block_api_set_pushable(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Block& block = *check_block(l, 1);
    bool pushable = LuaTools::opt_boolean(l, 2, true);

    block.set_pushable(pushable);

    return 0;
  });
}

/**
 * \brief Implementation of block:is_pullable().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::block_api_is_pullable(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Block& block = *check_block(l, 1);

    lua_pushboolean(l, block.is_pullable());
    return 1;
  });
}

/**
 * \brief Implementation of block:set_pullable().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::block_api_set_pullable(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Block& block = *check_block(l, 1);
    bool pullable = LuaTools::opt_boolean(l, 2, true);

    block.set_pullable(pullable);

    return 0;
  });
}

/**
 * \brief Implementation of block:get_maximum_moves().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::block_api_get_maximum_moves(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Block& block = *check_block(l, 1);

    const int maximum_moves = block.get_maximum_moves();

    if (maximum_moves == 2) {
      // 2 means no maximum in the side C++ side (for now).
      lua_pushnil(l);
    }
    else {
      lua_pushinteger(l, maximum_moves);
    }
    return 1;
  });
}

/**
 * \brief Implementation of block:set_maximum_moves().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::block_api_set_maximum_moves(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Block& block = *check_block(l, 1);
    if (lua_type(l, 2) != LUA_TNUMBER && lua_type(l, 2) != LUA_TNIL) {
      LuaTools::type_error(l, 2, "number or nil");
    }

    if (lua_isnumber(l, 2)) {
      const int maximum_moves = LuaTools::check_int(l, 2);
      if (maximum_moves < 0 || maximum_moves > 1) {
        LuaTools::arg_error(l, 2, "maximum_moves should be 0, 1 or nil");
      }
      block.set_maximum_moves(maximum_moves);
    }
    else if (lua_isnil(l, 2)) {
      // 2 means no maximum in C++.
      block.set_maximum_moves(2);
    }

    return 0;
  });
}

/**
 * \brief Returns whether a value is a userdata of type switch.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return true if the value at this index is a switch.
 */
bool LuaContext::is_switch(lua_State* l, int index) {
  return is_userdata(l, index, get_entity_internal_type_name(EntityType::SWITCH));
}

/**
 * \brief Checks that the userdata at the specified index of the stack is a
 * switch and returns it.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return The switch.
 */
std::shared_ptr<Switch> LuaContext::check_switch(lua_State* l, int index) {
  return std::static_pointer_cast<Switch>(check_userdata(
      l, index, get_entity_internal_type_name(EntityType::SWITCH)
  ));
}

/**
 * \brief Pushes a switch userdata onto the stack.
 * \param l A Lua context.
 * \param sw A switch.
 */
void LuaContext::push_switch(lua_State* l, Switch& sw) {
  push_userdata(l, sw);
}

/**
 * \brief Implementation of switch:is_activated().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::switch_api_is_activated(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Switch& sw = *check_switch(l, 1);

    lua_pushboolean(l, sw.is_activated());
    return 1;
  });
}

/**
 * \brief Implementation of switch:set_activated().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::switch_api_set_activated(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Switch& sw = *check_switch(l, 1);
    bool activated = LuaTools::opt_boolean(l, 2, true);

    sw.set_activated(activated);

    return 0;
  });
}

/**
 * \brief Implementation of switch:is_locked().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::switch_api_is_locked(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Switch& sw = *check_switch(l, 1);

    lua_pushboolean(l, sw.is_locked());
    return 1;
  });
}

/**
 * \brief Implementation of switch:set_locked().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::switch_api_set_locked(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Switch& sw = *check_switch(l, 1);
    bool locked = LuaTools::opt_boolean(l, 2, true);

    sw.set_locked(locked);

    return 0;
  });
}

/**
 * \brief Implementation of switch:is_walkable().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::switch_api_is_walkable(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Switch& sw = *check_switch(l, 1);

    lua_pushboolean(l, sw.is_walkable());
    return 1;
  });
}

/**
 * \brief Returns whether a value is a userdata of type stream.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return true if the value at this index is a stream.
 */
bool LuaContext::is_stream(lua_State* l, int index) {
  return is_userdata(l, index, get_entity_internal_type_name(EntityType::STREAM));
}

/**
 * \brief Checks that the userdata at the specified index of the stack is a
 * stream and returns it.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return The stream.
 */
std::shared_ptr<Stream> LuaContext::check_stream(lua_State* l, int index) {
  return std::static_pointer_cast<Stream>(check_userdata(
      l, index, get_entity_internal_type_name(EntityType::STREAM)
  ));
}

/**
 * \brief Pushes a stream userdata onto the stack.
 * \param l A Lua context.
 * \param stream A stream.
 */
void LuaContext::push_stream(lua_State* l, Stream& stream) {
  push_userdata(l, stream);
}

/**
 * \brief Implementation of stream:get_direction().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::stream_api_get_direction(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Stream& stream = *check_stream(l, 1);

    lua_pushinteger(l, stream.get_direction());
    return 1;
  });
}

/**
 * \brief Implementation of stream:set_direction().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::stream_api_set_direction(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Stream& stream = *check_stream(l, 1);
    int direction = LuaTools::check_int(l, 2);

    if (direction < 0 || direction >= 8) {
      LuaTools::arg_error(l, 2, "Invalid stream direction: must be between 0 and 7");
    }

    stream.set_direction(direction);

    return 0;
  });
}

/**
 * \brief Implementation of stream:get_speed().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::stream_api_get_speed(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Stream& stream = *check_stream(l, 1);

    lua_pushinteger(l, stream.get_speed());
    return 1;
  });
}

/**
 * \brief Implementation of stream:set_speed().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::stream_api_set_speed(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Stream& stream = *check_stream(l, 1);
    int speed = LuaTools::check_int(l, 2);

    stream.set_speed(speed);

    return 0;
  });
}

/**
 * \brief Implementation of stream:get_allow_movement().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::stream_api_get_allow_movement(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Stream& stream = *check_stream(l, 1);

    lua_pushboolean(l, stream.get_allow_movement());
    return 1;
  });
}

/**
 * \brief Implementation of stream:set_allow_movement().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::stream_api_set_allow_movement(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Stream& stream = *check_stream(l, 1);
    bool allow_movement = LuaTools::opt_boolean(l, 2, true);

    stream.set_allow_movement(allow_movement);

    return 0;
  });
}

/**
 * \brief Implementation of stream:get_allow_attack().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::stream_api_get_allow_attack(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Stream& stream = *check_stream(l, 1);

    lua_pushboolean(l, stream.get_allow_attack());
    return 1;
  });
}

/**
 * \brief Implementation of stream:set_allow_attack().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::stream_api_set_allow_attack(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Stream& stream = *check_stream(l, 1);
    bool allow_attack = LuaTools::opt_boolean(l, 2, true);

    stream.set_allow_attack(allow_attack);

    return 0;
  });
}

/**
 * \brief Implementation of stream:get_allow_item().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::stream_api_get_allow_item(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Stream& stream = *check_stream(l, 1);

    lua_pushboolean(l, stream.get_allow_item());
    return 1;
  });
}

/**
 * \brief Implementation of stream:set_allow_item().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::stream_api_set_allow_item(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Stream& stream = *check_stream(l, 1);
    bool allow_item = LuaTools::opt_boolean(l, 2, true);

    stream.set_allow_item(allow_item);

    return 0;
  });
}

/**
 * \brief Returns whether a value is a userdata of type door.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return true if the value at this index is a door.
 */
bool LuaContext::is_door(lua_State* l, int index) {
  return is_userdata(l, index, get_entity_internal_type_name(EntityType::DOOR));
}

/**
 * \brief Checks that the userdata at the specified index of the stack is a
 * door and returns it.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return The door.
 */
std::shared_ptr<Door> LuaContext::check_door(lua_State* l, int index) {
  return std::static_pointer_cast<Door>(check_userdata(
      l, index, get_entity_internal_type_name(EntityType::DOOR))
  );
}

/**
 * \brief Pushes a door userdata onto the stack.
 * \param l A Lua context.
 * \param door A door.
 */
void LuaContext::push_door(lua_State* l, Door& door) {
  push_userdata(l, door);
}

/**
 * \brief Implementation of door:is_open().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::door_api_is_open(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Door& door = *check_door(l, 1);

    lua_pushboolean(l, door.is_open());
    return 1;
  });
}

/**
 * \brief Implementation of door:is_opening().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::door_api_is_opening(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Door& door = *check_door(l, 1);

    lua_pushboolean(l, door.is_opening());
    return 1;
  });
}

/**
 * \brief Implementation of door:is_closed().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::door_api_is_closed(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Door& door = *check_door(l, 1);

    lua_pushboolean(l, door.is_closed());
    return 1;
  });
}

/**
 * \brief Implementation of door:is_closing().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::door_api_is_closing(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Door& door = *check_door(l, 1);

    lua_pushboolean(l, door.is_closing());
    return 1;
  });
}

/**
 * \brief Implementation of door:open().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::door_api_open(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Door& door = *check_door(l, 1);

    if (!door.is_open() && !door.is_opening()) {
      door.open();
      Sound::play("door_open");
    }

    return 0;
  });
}

/**
 * \brief Implementation of door:close().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::door_api_close(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Door& door = *check_door(l, 1);

    if (!door.is_closed() && !door.is_closing()) {
      door.close();
      Sound::play("door_closed");
    }

    return 0;
  });
}

/**
 * \brief Implementation of door:set_open().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::door_api_set_open(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Door& door = *check_door(l, 1);
    bool open = LuaTools::opt_boolean(l, 2, true);

    door.set_open(open);

    return 0;
  });
}

/**
 * \brief Returns whether a value is a userdata of type shop treasure.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return true if the value at this index is a shop treasure.
 */
bool LuaContext::is_shop_treasure(lua_State* l, int index) {
  return is_userdata(l, index, get_entity_internal_type_name(EntityType::SHOP_TREASURE));
}

/**
 * \brief Checks that the userdata at the specified index of the stack is a
 * shop treasure and returns it.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return The shop treasure.
 */
std::shared_ptr<ShopTreasure> LuaContext::check_shop_treasure(lua_State* l, int index) {
  return std::static_pointer_cast<ShopTreasure>(check_userdata(
      l, index, get_entity_internal_type_name(EntityType::SHOP_TREASURE)
  ));
}

/**
 * \brief Pushes a shop treasure userdata onto the stack.
 * \param l A Lua context.
 * \param shop_treasure A shop treasure.
 */
void LuaContext::push_shop_treasure(lua_State* l, ShopTreasure& shop_treasure) {
  push_userdata(l, shop_treasure);
}

/**
 * \brief Notifies Lua that the hero interacts with a shop treasure.
 *
 * Lua then manages the dialogs shown to the player.
 *
 * \param shop_treasure A shop treasure.
 */
void LuaContext::notify_shop_treasure_interaction(ShopTreasure& shop_treasure) {

  push_shop_treasure(l, shop_treasure);
  lua_pushcclosure(l, l_shop_treasure_description_dialog_finished, 1);
  const ScopedLuaRef& callback_ref = create_ref();

  shop_treasure.get_game().start_dialog(
      shop_treasure.get_dialog_id(),
      ScopedLuaRef(),
      callback_ref
  );
}

/**
 * \brief Callback function executed after the description dialog of
 * a shop treasure.
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::l_shop_treasure_description_dialog_finished(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {

    // The description message has just finished.
    // The shop treasure is the first upvalue.
    ShopTreasure& shop_treasure = *check_shop_treasure(l, lua_upvalueindex(1));
    Game& game = shop_treasure.get_game();

    if (shop_treasure.is_being_removed()) {
      // The shop treasure was removed during the dialog.
      return 0;
    }

    lua_pushinteger(l, shop_treasure.get_price());
    const ScopedLuaRef& price_ref = LuaTools::create_ref(l);

    push_shop_treasure(l, shop_treasure);
    lua_pushcclosure(l, l_shop_treasure_question_dialog_finished, 1);
    ScopedLuaRef callback_ref = LuaTools::create_ref(l);

    game.start_dialog("_shop.question", price_ref, callback_ref);

    return 0;
  });
}

/**
 * \brief Callback function executed after the question dialog of
 * a shop treasure.
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::l_shop_treasure_question_dialog_finished(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    LuaContext& lua_context = get_lua_context(l);

    // The "do you want to buy?" question has just been displayed.
    // The shop treasure is the first upvalue.
    ShopTreasure& shop_treasure = *check_shop_treasure(l, lua_upvalueindex(1));

    if (shop_treasure.is_being_removed()) {
      // The shop treasure was removed during the dialog.
      return 0;
    }

    // The first parameter is the answer.
    bool wants_to_buy = lua_isboolean(l, 1) && lua_toboolean(l, 1);

    Game& game = shop_treasure.get_game();
    if (wants_to_buy) {

      // The player wants to buy the item.
      Equipment& equipment = game.get_equipment();
      const Treasure& treasure = shop_treasure.get_treasure();
      EquipmentItem& item = treasure.get_item();

      if (!treasure.is_obtainable()) {
        // This treasure is not allowed.
        Sound::play("wrong");
      }
      else if (equipment.get_money() < shop_treasure.get_price()) {
        // Not enough money.
        Sound::play("wrong");
        game.start_dialog("_shop.not_enough_money", ScopedLuaRef(), ScopedLuaRef());
      }
      else if (item.has_amount() && item.get_amount() >= item.get_max_amount()) {
        // The player already has the maximum amount of this item.
        Sound::play("wrong");
        game.start_dialog("_shop.amount_full", ScopedLuaRef(), ScopedLuaRef());
      }
      else {

        bool can_buy = lua_context.shop_treasure_on_buying(shop_treasure);
        if (can_buy) {

          // Give the treasure.
          equipment.remove_money(shop_treasure.get_price());

          game.get_hero()->start_treasure(treasure, ScopedLuaRef());
          if (treasure.is_saved()) {
            shop_treasure.remove_from_map();
            game.get_savegame().set_boolean(treasure.get_savegame_variable(), true);
          }
          lua_context.shop_treasure_on_bought(shop_treasure);
        }
      }
    }
    return 0;
  });
}

/**
 * \brief Returns whether a value is a userdata of type pickable.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return true if the value at this index is a pickable.
 */
bool LuaContext::is_pickable(lua_State* l, int index) {
  return is_userdata(l, index, get_entity_internal_type_name(EntityType::PICKABLE));
}

/**
 * \brief Checks that the userdata at the specified index of the stack is an
 * pickable and returns it.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return The pickable.
 */
std::shared_ptr<Pickable> LuaContext::check_pickable(lua_State* l, int index) {
  return std::static_pointer_cast<Pickable>(check_userdata(
      l, index, get_entity_internal_type_name(EntityType::PICKABLE)
  ));
}

/**
 * \brief Pushes an pickable userdata onto the stack.
 * \param l A Lua context.
 * \param pickable A pickable treasure.
 */
void LuaContext::push_pickable(lua_State* l, Pickable& pickable) {
  push_userdata(l, pickable);
}

/**
 * \brief Implementation of pickable:get_followed_entity().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::pickable_api_get_followed_entity(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Pickable& pickable = *check_pickable(l, 1);

    EntityPtr followed_entity = pickable.get_entity_followed();

    if (followed_entity != nullptr) {
      push_entity(l, *followed_entity);
    }
    else {
      lua_pushnil(l);
    }
    return 1;
  });
}

/**
 * \brief Implementation of pickable:get_falling_height().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::pickable_api_get_falling_height(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Pickable& pickable = *check_pickable(l, 1);

    lua_pushinteger(l, pickable.get_falling_height());
    return 1;
  });
}

/**
 * \brief Implementation of pickable:get_treasure().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::pickable_api_get_treasure(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Pickable& pickable = *check_pickable(l, 1);
    const Treasure& treasure = pickable.get_treasure();

    push_item(l, treasure.get_item());
    lua_pushinteger(l, treasure.get_variant());
    if (!treasure.is_saved()) {
      lua_pushnil(l);
    }
    else {
      push_string(l, treasure.get_savegame_variable());
    }
    return 3;
  });
}

/**
 * \brief Returns whether a value is a userdata of type destructible object.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return \c true if the value at this index is a destructible object.
 */
bool LuaContext::is_destructible(lua_State* l, int index) {
  return is_userdata(l, index, get_entity_internal_type_name(EntityType::DESTRUCTIBLE));
}

/**
 * \brief Checks that the userdata at the specified index of the stack is a
 * destructible object and returns it.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return The destructible object.
 */
std::shared_ptr<Destructible> LuaContext::check_destructible(lua_State* l, int index) {
  return std::static_pointer_cast<Destructible>(check_userdata(
      l, index, get_entity_internal_type_name(EntityType::DESTRUCTIBLE)
  ));
}

/**
 * \brief Pushes a destructible userdata onto the stack.
 * \param l A Lua context.
 * \param destructible A destructible object.
 */
void LuaContext::push_destructible(lua_State* l, Destructible& destructible) {
  push_userdata(l, destructible);
}

/**
 * \brief Implementation of destructible:get_treasure().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::destructible_api_get_treasure(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Destructible& destructible = *check_destructible(l, 1);
    const Treasure& treasure = destructible.get_treasure();

    if (treasure.get_item_name().empty()) {
      // No treasure: return nil.
      lua_pushnil(l);
      return 1;
    }

    push_string(l, treasure.get_item_name());
    lua_pushinteger(l, treasure.get_variant());
    if (!treasure.is_saved()) {
      lua_pushnil(l);
    }
    else {
      push_string(l, treasure.get_savegame_variable());
    }
    return 3;
  });
}

/**
 * \brief Implementation of destructible:set_treasure().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::destructible_api_set_treasure(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Destructible& destructible = *check_destructible(l, 1);
    std::string item_name, savegame_variable;
    int variant = 1;

    if (lua_gettop(l) >= 2 && !lua_isnil(l, 2)) {
      item_name = LuaTools::check_string(l, 2);
    }
    if (lua_gettop(l) >= 3 && !lua_isnil(l, 3)) {
      variant = LuaTools::check_int(l, 3);
    }
    if (lua_gettop(l) >= 4 && !lua_isnil(l, 4)) {
      savegame_variable = LuaTools::check_string(l, 4);
    }

    if (!savegame_variable.empty()
        && !LuaTools::is_valid_lua_identifier(savegame_variable)) {
      LuaTools::arg_error(l, 4,
          std::string("savegame variable identifier expected, got '")
      + savegame_variable + "'");
    }

    Treasure treasure(destructible.get_game(), item_name, variant, savegame_variable);
    destructible.set_treasure(treasure);

    return 0;
  });
}

/**
 * \brief Implementation of destructible:get_destruction_sound().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::destructible_api_get_destruction_sound(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Destructible& destructible = *check_destructible(l, 1);

    const std::string& destruction_sound_id = destructible.get_destruction_sound();

    if (destruction_sound_id.empty()) {
      lua_pushnil(l);
    }
    else {
      push_string(l, destruction_sound_id);
    }
    return 1;
  });
}

/**
 * \brief Implementation of destructible:set_destruction_sound().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::destructible_api_set_destruction_sound(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Destructible& destructible = *check_destructible(l, 1);
    std::string destruction_sound_id;
    if (!lua_isnil(l, 2)) {
      destruction_sound_id = LuaTools::check_string(l, 2);
    }

    destructible.set_destruction_sound(destruction_sound_id);
    return 0;
  });
}

/**
 * \brief Implementation of destructible:get_can_be_cut().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::destructible_api_get_can_be_cut(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Destructible& destructible = *check_destructible(l, 1);

    bool can_be_cut = destructible.get_can_be_cut();

    lua_pushboolean(l, can_be_cut);
    return 1;
  });
}

/**
 * \brief Implementation of destructible:set_can_be_cut().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::destructible_api_set_can_be_cut(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Destructible& destructible = *check_destructible(l, 1);
    bool can_be_cut = LuaTools::opt_boolean(l, 2, true);

    destructible.set_can_be_cut(can_be_cut);

    return 0;
  });
}

/**
 * \brief Implementation of destructible:get_can_explode().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::destructible_api_get_can_explode(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Destructible& destructible = *check_destructible(l, 1);

    bool can_explode = destructible.get_can_explode();

    lua_pushboolean(l, can_explode);
    return 1;
  });
}

/**
 * \brief Implementation of destructible:set_can_explode().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::destructible_api_set_can_explode(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Destructible& destructible = *check_destructible(l, 1);
    bool can_explode = LuaTools::opt_boolean(l, 2, true);

    destructible.set_can_explode(can_explode);

    return 0;
  });
}

/**
 * \brief Implementation of destructible:get_can_regenerate().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::destructible_api_get_can_regenerate(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Destructible& destructible = *check_destructible(l, 1);

    bool can_regenerate = destructible.get_can_regenerate();

    lua_pushboolean(l, can_regenerate);
    return 1;
  });
}

/**
 * \brief Implementation of destructible:set_can_regenerate().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::destructible_api_set_can_regenerate(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Destructible& destructible = *check_destructible(l, 1);
    bool can_regenerate = LuaTools::opt_boolean(l, 2, true);

    destructible.set_can_regenerate(can_regenerate);

    return 0;
  });
}

/**
 * \brief Implementation of destructible:get_damage_on_enemies().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::destructible_api_get_damage_on_enemies(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Destructible& destructible = *check_destructible(l, 1);

    int damage_on_enemies = destructible.get_damage_on_enemies();

    lua_pushinteger(l, damage_on_enemies);
    return 1;
  });
}

/**
 * \brief Implementation of destructible:set_damage_on_enemies().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::destructible_api_set_damage_on_enemies(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Destructible& destructible = *check_destructible(l, 1);
    int damage_on_enemies = LuaTools::check_int(l, 2);

    destructible.set_damage_on_enemies(damage_on_enemies);

    return 0;
  });
}

/**
 * \brief Implementation of destructible:get_modified_ground().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::destructible_api_get_modified_ground(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Destructible& destructible = *check_destructible(l, 1);

    Ground modified_ground = destructible.get_modified_ground();

    push_string(l, enum_to_name(modified_ground));
    return 1;
  });
}

/**
 * \brief Returns whether a value is a userdata of type carried object.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return \c true if the value at this index is a carried object.
 */
bool LuaContext::is_carried_object(lua_State* l, int index) {
  return is_userdata(l, index, get_entity_internal_type_name(EntityType::CARRIED_OBJECT));
}

/**
 * \brief Checks that the userdata at the specified index of the stack is a
 * carried object and returns it.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return The carried object.
 */
std::shared_ptr<CarriedObject> LuaContext::check_carried_object(lua_State* l, int index) {
  return std::static_pointer_cast<CarriedObject>(check_userdata(
      l, index, get_entity_internal_type_name(EntityType::CARRIED_OBJECT)
  ));
}

/**
 * \brief Pushes a carried object userdata onto the stack.
 * \param l A Lua context.
 * \param carried_object A carried object.
 */
void LuaContext::push_carried_object(lua_State* l, CarriedObject& carried_object) {
  push_userdata(l, carried_object);
}

/**
 * \brief Implementation of carried_object:get_destruction_sound().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::carried_object_api_get_destruction_sound(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const CarriedObject& carried_object = *check_carried_object(l, 1);

    const std::string& destruction_sound_id = carried_object.get_destruction_sound();

    if (destruction_sound_id.empty()) {
      lua_pushnil(l);
    }
    else {
      push_string(l, destruction_sound_id);
    }
    return 1;
  });
}

/**
 * \brief Implementation of carried_object:set_destruction_sound().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::carried_object_api_set_destruction_sound(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    CarriedObject& carried_object = *check_carried_object(l, 1);
    std::string destruction_sound_id;
    if (!lua_isnil(l, 2)) {
      destruction_sound_id = LuaTools::check_string(l, 2);
    }

    carried_object.set_destruction_sound(destruction_sound_id);
    return 0;
  });
}

/**
 * \brief Implementation of carried_object:get_damage_on_enemies().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::carried_object_api_get_damage_on_enemies(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const CarriedObject& carried_object = *check_carried_object(l, 1);

    int damage_on_enemies = carried_object.get_damage_on_enemies();

    lua_pushinteger(l, damage_on_enemies);
    return 1;
  });
}

/**
 * \brief Implementation of carried_object:set_damage_on_enemies().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::carried_object_api_set_damage_on_enemies(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    CarriedObject& carried_object = *check_carried_object(l, 1);
    int damage_on_enemies = LuaTools::check_int(l, 2);

    carried_object.set_damage_on_enemies(damage_on_enemies);

    return 0;
  });
}

/**
 * \brief Returns whether a value is a userdata of type dynamic tile.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return \c true if the value at this index is a dynamic tile.
 */
bool LuaContext::is_dynamic_tile(lua_State* l, int index) {
  return is_userdata(l, index, get_entity_internal_type_name(EntityType::DYNAMIC_TILE));
}

/**
 * \brief Checks that the userdata at the specified index of the stack is a
 * dynamic tile and returns it.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return The dynamic tile.
 */
std::shared_ptr<DynamicTile> LuaContext::check_dynamic_tile(lua_State* l, int index) {
  return std::static_pointer_cast<DynamicTile>(check_userdata(
      l, index, get_entity_internal_type_name(EntityType::DYNAMIC_TILE)
  ));
}

/**
 * \brief Pushes a dynamic tile userdata onto the stack.
 * \param l A Lua context.
 * \param dynamic_tile A dynamic tile.
 */
void LuaContext::push_dynamic_tile(lua_State* l, DynamicTile& dynamic_tile) {
  push_userdata(l, dynamic_tile);
}

/**
 * \brief Implementation of dynamic_tile:get_pattern_id().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::dynamic_tile_api_get_pattern_id(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const DynamicTile& dynamic_tile = *check_dynamic_tile(l, 1);

    const std::string& pattern_id = dynamic_tile.get_tile_pattern_id();

    push_string(l, pattern_id);
    return 1;
  });
}

/**
 * \brief Implementation of dynamic_tile:get_modified_ground().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::dynamic_tile_api_get_modified_ground(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const DynamicTile& dynamic_tile = *check_dynamic_tile(l, 1);

    Ground modified_ground = dynamic_tile.get_modified_ground();

    push_string(l, enum_to_name(modified_ground));
    return 1;
  });
}

/**
 * \brief Returns whether a value is a userdata of type enemy.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return true if the value at this index is an enemy.
 */
bool LuaContext::is_enemy(lua_State* l, int index) {
  return is_userdata(l, index, get_entity_internal_type_name(EntityType::ENEMY));
}

/**
 * \brief Checks that the userdata at the specified index of the stack is an
 * enemy and returns it.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return The enemy.
 */
std::shared_ptr<Enemy> LuaContext::check_enemy(lua_State* l, int index) {
  return std::static_pointer_cast<Enemy>(check_userdata(
      l, index, get_entity_internal_type_name(EntityType::ENEMY)
  ));
}

/**
 * \brief Pushes an enemy userdata onto the stack.
 * \param l A Lua context.
 * \param enemy An enemy.
 */
void LuaContext::push_enemy(lua_State* l, Enemy& enemy) {
  push_userdata(l, enemy);
}

/**
 * \brief Implementation of enemy:get_breed().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_get_breed(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Enemy& enemy = *check_enemy(l, 1);

    push_string(l, enemy.get_breed());
    return 1;
  });
}

/**
 * \brief Implementation of enemy:get_life().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_get_life(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Enemy& enemy = *check_enemy(l, 1);

    lua_pushinteger(l, enemy.get_life());
    return 1;
  });
}

/**
 * \brief Implementation of enemy:set_life().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_set_life(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Enemy& enemy = *check_enemy(l, 1);
    int life = LuaTools::check_int(l, 2);

    enemy.set_life(life);

    return 0;
  });
}

/**
 * \brief Implementation of enemy:add_life().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_add_life(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Enemy& enemy = *check_enemy(l, 1);
    int points = LuaTools::check_int(l, 2);

    enemy.set_life(enemy.get_life() + points);

    return 0;
  });
}

/**
 * \brief Implementation of enemy:remove_life().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_remove_life(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Enemy& enemy = *check_enemy(l, 1);
    int points = LuaTools::check_int(l, 2);

    enemy.set_life(enemy.get_life() - points);

    return 0;
  });
}

/**
 * \brief Implementation of enemy:get_damage().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_get_damage(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Enemy& enemy = *check_enemy(l, 1);

    lua_pushinteger(l, enemy.get_damage());
    return 1;
  });
}

/**
 * \brief Implementation of enemy:set_damage().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_set_damage(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Enemy& enemy = *check_enemy(l, 1);
    int damage = LuaTools::check_int(l, 2);

    enemy.set_damage(damage);

    return 0;
  });
}

/**
 * \brief Implementation of enemy:is_pushed_back_when_hurt().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_is_pushed_back_when_hurt(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Enemy& enemy = *check_enemy(l, 1);

    lua_pushboolean(l, enemy.get_pushed_back_when_hurt());
    return 1;
  });
}

/**
 * \brief Implementation of enemy:set_pushed_back_when_hurt().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_set_pushed_back_when_hurt(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Enemy& enemy = *check_enemy(l, 1);
    bool push_back = LuaTools::opt_boolean(l, 2, true);

    enemy.set_pushed_back_when_hurt(push_back);

    return 0;
  });
}

/**
 * \brief Implementation of enemy:get_push_hero_on_sword().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_get_push_hero_on_sword(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Enemy& enemy = *check_enemy(l, 1);

    lua_pushboolean(l, enemy.get_push_hero_on_sword());
    return 1;
  });
}

/**
 * \brief Implementation of enemy:set_push_hero_on_sword().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_set_push_hero_on_sword(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Enemy& enemy = *check_enemy(l, 1);
    bool push = LuaTools::opt_boolean(l, 2, true);

    enemy.set_push_hero_on_sword(push);

    return 0;
  });
}

/**
 * \brief Implementation of enemy:get_can_hurt_hero_running().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_get_can_hurt_hero_running(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Enemy& enemy = *check_enemy(l, 1);

    lua_pushboolean(l, enemy.get_can_hurt_hero_running());
    return 1;
  });
}

/**
 * \brief Implementation of enemy:set_can_hurt_hero_running().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_set_can_hurt_hero_running(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Enemy& enemy = *check_enemy(l, 1);
    bool can_hurt_hero_running = LuaTools::opt_boolean(l, 2, true);

    enemy.set_can_hurt_hero_running(can_hurt_hero_running);

    return 0;
  });
}

/**
 * \brief Implementation of enemy:get_hurt_style().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_get_hurt_style(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Enemy& enemy = *check_enemy(l, 1);

    Enemy::HurtStyle hurt_style = enemy.get_hurt_style();

    push_string(l, Enemy::hurt_style_names.find(hurt_style)->second);
    return 1;
  });
}

/**
 * \brief Implementation of enemy:set_hurt_style().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_set_hurt_style(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Enemy& enemy = *check_enemy(l, 1);
    Enemy::HurtStyle hurt_style = LuaTools::check_enum<Enemy::HurtStyle>(
        l, 2, Enemy::hurt_style_names);

    enemy.set_hurt_style(hurt_style);

    return 0;
  });
}

/**
 * \brief Implementation of enemy:get_can_attack().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_get_can_attack(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Enemy& enemy = *check_enemy(l, 1);

    lua_pushboolean(l, enemy.get_can_attack());
    return 1;
  });
}

/**
 * \brief Implementation of enemy:set_can_attack().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_set_can_attack(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Enemy& enemy = *check_enemy(l, 1);
    bool can_attack = LuaTools::opt_boolean(l, 2, true);

    enemy.set_can_attack(can_attack);

    return 0;
  });
}

/**
 * \brief Implementation of enemy:get_minimum_shield_needed().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_get_minimum_shield_needed(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Enemy& enemy = *check_enemy(l, 1);

    int shield_level = enemy.get_minimum_shield_needed();

    lua_pushinteger(l, shield_level);
    return 1;
  });
}

/**
 * \brief Implementation of enemy:set_minimum_shield_needed().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_set_minimum_shield_needed(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Enemy& enemy = *check_enemy(l, 1);
    int shield_level = LuaTools::check_int(l, 2);

    enemy.set_minimum_shield_needed(shield_level);

    return 0;
  });
}

/**
 * \brief Implementation of enemy:get_attack_consequence().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_get_attack_consequence(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Enemy& enemy = *check_enemy(l, 1);
    EnemyAttack attack = LuaTools::check_enum<EnemyAttack>(l, 2, Enemy::attack_names);

    const EnemyReaction::Reaction& reaction = enemy.get_attack_consequence(attack, nullptr);
    if (reaction.type == EnemyReaction::ReactionType::HURT) {
      // Return the life damage.
      lua_pushinteger(l, reaction.life_lost);
    }
    else if (reaction.type == EnemyReaction::ReactionType::LUA_CALLBACK) {
      // Return the callback.
      reaction.callback.push();
    }
    else {
      // Return a string.
      push_string(l, enum_to_name(reaction.type));
    }
    return 1;
  });
}

/**
 * \brief Implementation of enemy:set_attack_consequence().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_set_attack_consequence(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Enemy& enemy = *check_enemy(l, 1);
    EnemyAttack attack = LuaTools::check_enum<EnemyAttack>(l, 2, Enemy::attack_names);

    if (lua_isnumber(l, 3)) {
      int life_points = LuaTools::check_int(l, 3);
      if (life_points < 0) {
        std::ostringstream oss;
        oss << "Invalid life points number for attack consequence: '"
            << life_points << "'";
        LuaTools::arg_error(l, 3, oss.str());
      }
      enemy.set_attack_consequence(attack, EnemyReaction::ReactionType::HURT, life_points);
    }
    else if (lua_isstring(l, 3)) {
      EnemyReaction::ReactionType reaction = LuaTools::check_enum<EnemyReaction::ReactionType>(
          l, 3);
      enemy.set_attack_consequence(attack, reaction);
    }
    else if (lua_isfunction(l, 3)) {
      ScopedLuaRef callback = LuaTools::check_function(l, 3);
      enemy.set_attack_consequence(attack, EnemyReaction::ReactionType::LUA_CALLBACK, 0, callback);
    }
    else {
      LuaTools::type_error(l, 3, "number, string or function");
    }

    return 0;
  });
}

/**
 * \brief Implementation of enemy:get_attack_consequence_sprite().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_get_attack_consequence_sprite(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Enemy& enemy = *check_enemy(l, 1);
    Sprite& sprite = *check_sprite(l, 2);
    EnemyAttack attack = LuaTools::check_enum<EnemyAttack>(l, 3, Enemy::attack_names);

    const EnemyReaction::Reaction& reaction = enemy.get_attack_consequence(attack, &sprite);
    if (reaction.type == EnemyReaction::ReactionType::HURT) {
      // Return the life damage.
      lua_pushinteger(l, reaction.life_lost);
    }
    else if (reaction.type == EnemyReaction::ReactionType::LUA_CALLBACK) {
      // Return the callback.
      reaction.callback.push();
    }
    else {
      // Return a string.
      push_string(l, enum_to_name(reaction.type));
    }
    return 1;
  });
}

/**
 * \brief Implementation of enemy:set_attack_consequence_sprite().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_set_attack_consequence_sprite(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Enemy& enemy = *check_enemy(l, 1);
    Sprite& sprite = *check_sprite(l, 2);
    EnemyAttack attack = LuaTools::check_enum<EnemyAttack>(l, 3, Enemy::attack_names);

    if (lua_isnumber(l, 4)) {
      int life_points = LuaTools::check_int(l, 4);
      if (life_points < 0) {
        std::ostringstream oss;
        oss << "Invalid life points number for attack consequence: '"
            << life_points << "'";
        LuaTools::arg_error(l, 4, oss.str());
      }
      enemy.set_attack_consequence_sprite(sprite, attack, EnemyReaction::ReactionType::HURT, life_points);
    }
    else if (lua_isstring(l, 4)) {
      EnemyReaction::ReactionType reaction = LuaTools::check_enum<EnemyReaction::ReactionType>(
          l, 4);
      enemy.set_attack_consequence_sprite(sprite, attack, reaction);
    }
    else if (lua_isfunction(l, 4)) {
      ScopedLuaRef callback = LuaTools::check_function(l, 4);
      enemy.set_attack_consequence_sprite(sprite, attack, EnemyReaction::ReactionType::LUA_CALLBACK, 0, callback);
    }
    else {
      LuaTools::type_error(l, 3, "number, string or function");
    }

    return 0;
  });
}

/**
 * \brief Implementation of enemy:set_default_attack_consequences().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_set_default_attack_consequences(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Enemy& enemy = *check_enemy(l, 1);

    enemy.set_default_attack_consequences();

    return 0;
  });
}

/**
 * \brief Implementation of enemy:set_default_attack_consequences_sprite().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_set_default_attack_consequences_sprite(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Enemy& enemy = *check_enemy(l, 1);
    Sprite& sprite = *check_sprite(l, 2);

    enemy.set_default_attack_consequences_sprite(sprite);

    return 0;
  });
}

/**
 * \brief Implementation of enemy:set_invincible().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_set_invincible(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Enemy& enemy = *check_enemy(l, 1);

    enemy.set_no_attack_consequences();

    return 0;
  });
}

/**
 * \brief Implementation of enemy:set_invincible_sprite().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_set_invincible_sprite(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Enemy& enemy = *check_enemy(l, 1);
    Sprite& sprite = *check_sprite(l, 2);

    enemy.set_no_attack_consequences_sprite(sprite);

    return 0;
  });
}

/**
 * \brief Implementation of enemy:get_treasure().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_get_treasure(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Enemy& enemy = *check_enemy(l, 1);
    const Treasure& treasure = enemy.get_treasure();

    if (treasure.get_item_name().empty()) {
      // No treasure: return nil.
      lua_pushnil(l);
      return 1;
    }

    push_string(l, treasure.get_item_name());
    lua_pushinteger(l, treasure.get_variant());
    if (!treasure.is_saved()) {
      lua_pushnil(l);
    }
    else {
      push_string(l, treasure.get_savegame_variable());
    }
    return 3;
  });
}


/**
 * \brief Implementation of enemy:set_treasure().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_set_treasure(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Enemy& enemy = *check_enemy(l, 1);
    std::string item_name, savegame_variable;
    int variant = 1;

    if (lua_gettop(l) >= 2 && !lua_isnil(l, 2)) {
      item_name = LuaTools::check_string(l, 2);
    }
    if (lua_gettop(l) >= 3 && !lua_isnil(l, 3)) {
      variant = LuaTools::check_int(l, 3);
    }
    if (lua_gettop(l) >= 4 && !lua_isnil(l, 4)) {
      savegame_variable = LuaTools::check_string(l, 4);
    }

    if (!savegame_variable.empty()
        && !LuaTools::is_valid_lua_identifier(savegame_variable)) {
      LuaTools::arg_error(l, 4,
          std::string("savegame variable identifier expected, got '")
      + savegame_variable + "'");
    }

    Treasure treasure(enemy.get_game(), item_name, variant, savegame_variable);
    enemy.set_treasure(treasure);

    return 0;
  });
}

/**
 * \brief Implementation of enemy:is_traversable().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_is_traversable(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Enemy& enemy = *check_enemy(l, 1);

    lua_pushboolean(l, enemy.is_traversable());
    return 1;
  });
}

/**
 * \brief Implementation of enemy:set_traversable().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_set_traversable(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Enemy& enemy = *check_enemy(l, 1);

    bool traversable = LuaTools::opt_boolean(l, 2, true);

    enemy.set_traversable(traversable);

    return 0;
  });
}

/**
 * \brief Implementation of enemy:get_attacking_collision_mode().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_get_attacking_collision_mode(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Enemy& enemy = *check_enemy(l, 1);

    push_string(l, enum_to_name(enemy.get_attacking_collision_mode()));
    return 1;
  });
}

/**
 * \brief Implementation of enemy:set_attacking_collision_mode().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_set_attacking_collision_mode(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Enemy& enemy = *check_enemy(l, 1);
    CollisionMode attacking_collision_mode = LuaTools::check_enum<CollisionMode>(l, 2,
        EnumInfoTraits<CollisionMode>::names_no_none_no_custom
    );
    enemy.set_attacking_collision_mode(attacking_collision_mode);

    return 0;
  });
}

/**
 * \brief Implementation of enemy:get_obstacle_behavior().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_get_obstacle_behavior(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const Enemy& enemy = *check_enemy(l, 1);

    Enemy::ObstacleBehavior behavior = enemy.get_obstacle_behavior();

    push_string(l, Enemy::obstacle_behavior_names.find(behavior)->second);
    return 1;
  });
}

/**
 * \brief Implementation of enemy:set_obstacle_behavior().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_set_obstacle_behavior(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Enemy& enemy = *check_enemy(l, 1);
    Enemy::ObstacleBehavior behavior = LuaTools::check_enum<Enemy::ObstacleBehavior>(
        l, 2, Enemy::obstacle_behavior_names);

    enemy.set_obstacle_behavior(behavior);

    return 0;
  });
}

/**
 * \brief Implementation of enemy:restart().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_restart(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Enemy& enemy = *check_enemy(l, 1);

    enemy.restart();

    return 0;
  });
}

/**
 * \brief Implementation of enemy:hurt().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_hurt(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Enemy& enemy = *check_enemy(l, 1);
    int life_points = LuaTools::check_int(l, 2);

    if (enemy.is_in_normal_state() && !enemy.is_invulnerable()) {
      Hero& hero = enemy.get_map().get_entities().get_hero();
      enemy.set_attack_consequence(EnemyAttack::SCRIPT, EnemyReaction::ReactionType::HURT, life_points);
      enemy.try_hurt(EnemyAttack::SCRIPT, hero, nullptr);
    }

    return 0;
  });
}

/**
 * \brief Implementation of enemy:immobilize().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_immobilize(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Enemy& enemy = *check_enemy(l, 1);

    if (enemy.is_invulnerable()) {
      return 0;
    }

    if (enemy.is_in_normal_state() || enemy.is_immobilized()) {
      Hero& hero = enemy.get_map().get_entities().get_hero();
      enemy.set_attack_consequence(EnemyAttack::SCRIPT, EnemyReaction::ReactionType::IMMOBILIZED, 0);
      enemy.try_hurt(EnemyAttack::SCRIPT, hero, nullptr);
    }

    return 0;
  });
}

/**
 * \brief Implementation of enemy:create_enemy().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::enemy_api_create_enemy(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    Enemy& enemy = *check_enemy(l, 1);
    LuaTools::check_type(l, 2, LUA_TTABLE);
    const std::string& name = LuaTools::opt_string_field(l, 2, "name", "");
    int layer = LuaTools::opt_layer_field(l, 2, "layer", enemy.get_map(), enemy.get_layer());
    int x = LuaTools::opt_int_field(l, 2, "x", 0);
    int y = LuaTools::opt_int_field(l, 2, "y", 0);
    int direction = LuaTools::opt_int_field(l, 2, "direction", 3);
    const std::string& breed = LuaTools::check_string_field(l, 2, "breed");
    const std::string& savegame_variable = LuaTools::opt_string_field(l, 2, "savegame_variable", "");
    const std::string& treasure_name = LuaTools::opt_string_field(l, 2, "treasure_name", "");
    int treasure_variant = LuaTools::opt_int_field(l, 2, "treasure_variant", 1);
    const std::string& treasure_savegame_variable = LuaTools::opt_string_field(l, 2, "treasure_savegame_variable", "");

    if (!savegame_variable.empty()
        && !LuaTools::is_valid_lua_identifier(savegame_variable)) {
      LuaTools::arg_error(l, 2, std::string(
          "Bad field 'savegame_variable' (invalid savegame variable identifier '")
      + savegame_variable + "'");
    }

    if (!treasure_savegame_variable.empty()
        && !LuaTools::is_valid_lua_identifier(treasure_savegame_variable)) {
      LuaTools::arg_error(l, 2, std::string(
          "Bad field 'treasure_savegame_variable' (invalid savegame variable identifier '")
      + treasure_savegame_variable + "'");
    }

    // Make x and y relative to the existing enemy.
    x += enemy.get_x();
    y += enemy.get_y();

    // Create the new enemy.
    Map& map = enemy.get_map();

    if (!map.is_loaded()) {
      LuaTools::error(l, "Cannot create enemy: this map is not running");
    }

    Game& game = map.get_game();
    const EntityPtr& entity = Enemy::create(
        game,
        breed,
        savegame_variable,
        name,
        layer,
        { x, y },
        direction,
        Treasure(game, treasure_name, treasure_variant, treasure_savegame_variable)
    );

    if (entity == nullptr) {
      // The enemy is saved as already dead.
      lua_pushnil(l);
      return 1;
    }

    map.get_entities().add_entity(entity);

    push_entity(l, *entity);
    return 1;
  });
}

/**
 * \brief Returns whether a value is a userdata of type custom entity.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return true if the value at this index is a custom entity.
 */
bool LuaContext::is_custom_entity(lua_State* l, int index) {
  return is_userdata(l, index, get_entity_internal_type_name(EntityType::CUSTOM));
}

/**
 * \brief Checks that the userdata at the specified index of the stack is a
 * custom entity and returns it.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return The custom entity.
 */
std::shared_ptr<CustomEntity> LuaContext::check_custom_entity(lua_State* l, int index) {
  return std::static_pointer_cast<CustomEntity>(check_userdata(
      l, index, get_entity_internal_type_name(EntityType::CUSTOM)
  ));
}

/**
 * \brief Pushes a custom entity userdata onto the stack.
 * \param l A Lua context.
 * \param custom_entity A custom entity.
 */
void LuaContext::push_custom_entity(lua_State* l, CustomEntity& entity) {
  push_userdata(l, entity);
}

/**
 * \brief Calls the specified a Lua traversable test function.
 * \param traversable_test_ref Lua ref to a traversable test function.
 * \param custom_entity The custom entity that is testing if it can traverse
 * or be traversed by another entity.
 * \param other_entity The other entity.
 * \return \c true if the traversable test function returned \c true.
 */
bool LuaContext::do_custom_entity_traversable_test_function(
    const ScopedLuaRef& traversable_test_ref,
    CustomEntity& custom_entity,
    Entity& other_entity) {

  Debug::check_assertion(!traversable_test_ref.is_empty(),
      "Missing traversable test function ref"
  );

  // Call the test function.
  push_ref(l, traversable_test_ref);
  Debug::check_assertion(lua_isfunction(l, -1),
      "Traversable test is not a function"
  );
  push_custom_entity(l, custom_entity);
  push_entity(l, other_entity);
  if (!LuaTools::call_function(l, 2, 1, "traversable test function")) {
    // Error in the traversable test function.
    return false;
  }

  // See its result.
  bool traversable = lua_toboolean(l, -1);
  lua_pop(l, 1);

  return traversable;
}

/**
 * \brief Calls the specified a Lua collision test function.
 * \param collision_test_ref Lua ref to a collision test function.
 * \param custom_entity The custom entity that is testing the collision.
 * \param other_entity The entity to test a collision with.
 * \return \c true if the collision test function returned \c true.
 */
bool LuaContext::do_custom_entity_collision_test_function(
    const ScopedLuaRef& collision_test_ref,
    CustomEntity& custom_entity,
    Entity& other_entity
) {
  Debug::check_assertion(!collision_test_ref.is_empty(),
      "Missing collision test function"
  );

  // Call the test function.
  push_ref(l, collision_test_ref);
  Debug::check_assertion(lua_isfunction(l, -1),
      "Collision test is not a function"
  );
  push_custom_entity(l, custom_entity);
  push_entity(l, other_entity);
  if (!LuaTools::call_function(l, 2, 1, "collision test function")) {
    // Error in the collision test function.
    return false;
  }

  // See its result.
  bool collision = lua_toboolean(l, -1);
  lua_pop(l, 1);

  return collision;
}

/**
 * \brief Executes a callback after a custom entity detected a collision.
 * \param callback_ref Ref of the function to call.
 * \param custom_entity A custom entity that detected a collision.
 * \param other_entity The entity that was detected.
 */
void LuaContext::do_custom_entity_collision_callback(
    const ScopedLuaRef& callback_ref,
    CustomEntity& custom_entity,
    Entity& other_entity
) {
  Debug::check_assertion(!callback_ref.is_empty(),
      "Missing collision callback");

  push_ref(l, callback_ref);
  Debug::check_assertion(lua_isfunction(l, -1),
      "Collision callback is not a function");
  push_custom_entity(l, custom_entity);
  push_entity(l, other_entity);
  LuaTools::call_function(l, 2, 0, "collision callback");
}

/**
 * \brief Executes a callback after a custom entity detected a pixel-precise
 * collision.
 * \param callback_ref Ref of the function to call.
 * \param custom_entity A custom entity that detected a collision.
 * \param other_entity The entity that was detected.
 * \param custom_entity_sprite Sprite of the custom entity involved in the
 * collision.
 * \param other_entity_sprite Sprite of the detected entity involved in the
 * collision.
 */
void LuaContext::do_custom_entity_collision_callback(
    const ScopedLuaRef& callback_ref,
    CustomEntity& custom_entity,
    Entity& other_entity,
    Sprite& custom_entity_sprite,
    Sprite& other_entity_sprite) {

  Debug::check_assertion(!callback_ref.is_empty(),
      "Missing sprite collision callback"
  );

  push_ref(l, callback_ref);
  Debug::check_assertion(lua_isfunction(l, -1),
      "Sprite collision callback is not a function");
  push_custom_entity(l, custom_entity);
  push_entity(l, other_entity);
  push_sprite(l, custom_entity_sprite);
  push_sprite(l, other_entity_sprite);
  LuaTools::call_function(l, 4, 0, "collision callback");
}

/**
 * \brief Implementation of custom_entity:get_model().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::custom_entity_api_get_model(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const CustomEntity& entity = *check_custom_entity(l, 1);

    push_string(l, entity.get_model());
    return 1;
  });
}

/**
 * \brief Implementation of custom_entity:get_direction().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::custom_entity_api_get_direction(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const CustomEntity& entity = *check_custom_entity(l, 1);

    lua_pushinteger(l, entity.get_sprites_direction());
    return 1;
  });
}

/**
 * \brief Implementation of custom_entity:set_direction().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::custom_entity_api_set_direction(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    CustomEntity& entity = *check_custom_entity(l, 1);
    int direction = LuaTools::check_int(l, 2);

    entity.set_sprites_direction(direction);

    return 0;
  });
}

/**
 * \brief Implementation of custom_entity:set_traversable_by().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::custom_entity_api_set_traversable_by(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    CustomEntity& entity = *check_custom_entity(l, 1);

    bool type_specific = false;
    EntityType type = EntityType::TILE;
    int index = 2;
    if (lua_isstring(l, index)) {
      ++index;
      type_specific = true;
      type = LuaTools::check_enum<EntityType>(
          l, 2
      );
    }

    if (lua_isnil(l, index)) {
      // Reset the setting.
      if (!type_specific) {
        entity.reset_traversable_by_entities();
      }
      else {
        entity.reset_traversable_by_entities(type);
      }
    }
    else if (lua_isboolean(l, index)) {
      // Boolean value.
      bool traversable = lua_toboolean(l, index);
      if (!type_specific) {
        entity.set_traversable_by_entities(traversable);
      }
      else {
        entity.set_traversable_by_entities(type, traversable);
      }
    }
    else if (lua_isfunction(l, index)) {
      // Custom boolean function.

      const ScopedLuaRef& traversable_test_ref = LuaTools::check_function(l, index);
      if (!type_specific) {
        entity.set_traversable_by_entities(traversable_test_ref);
      }
      else {
        entity.set_traversable_by_entities(type, traversable_test_ref);
      }
    }
    else {
      LuaTools::type_error(l, index, "boolean, function or nil");
    }

    return 0;
  });
}

/**
 * \brief Implementation of custom_entity:set_can_traverse().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::custom_entity_api_set_can_traverse(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    CustomEntity& entity = *check_custom_entity(l, 1);

    bool type_specific = false;
    EntityType type = EntityType::TILE;
    int index = 2;
    if (lua_isstring(l, index)) {
      ++index;
      type_specific = true;
      type = LuaTools::check_enum<EntityType>(
          l, 2
      );
    }

    if (lua_isnil(l, index)) {
      // Reset the setting.
      if (!type_specific) {
        entity.reset_can_traverse_entities();
      }
      else {
        entity.reset_can_traverse_entities(type);
      }
    }
    else if (lua_isboolean(l, index)) {
      // Boolean value.
      bool traversable = lua_toboolean(l, index);
      if (!type_specific) {
        entity.set_can_traverse_entities(traversable);
      }
      else {
        entity.set_can_traverse_entities(type, traversable);
      }
    }
    else if (lua_isfunction(l, index)) {
      // Custom boolean function.

      const ScopedLuaRef& traversable_test_ref = LuaTools::check_function(l, index);
      if (!type_specific) {
        entity.set_can_traverse_entities(traversable_test_ref);
      }
      else {
        entity.set_can_traverse_entities(type, traversable_test_ref);
      }
    }
    else {
      LuaTools::type_error(l, index, "boolean, function or nil");
    }

    return 0;
  });
}

/**
 * \brief Implementation of custom_entity:can_traverse_ground().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::custom_entity_api_can_traverse_ground(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const CustomEntity& entity = *check_custom_entity(l, 1);
    Ground ground = LuaTools::check_enum<Ground>(l, 2);

    bool traversable = entity.can_traverse_ground(ground);

    lua_pushboolean(l, traversable);
    return 1;
  });
}

/**
 * \brief Implementation of custom_entity:set_can_traverse_ground().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::custom_entity_api_set_can_traverse_ground(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    CustomEntity& entity = *check_custom_entity(l, 1);
    Ground ground = LuaTools::check_enum<Ground>(l, 2);
    if (lua_isnil(l, 3)) {
      entity.reset_can_traverse_ground(ground);
    }
    else {
      if (!lua_isboolean(l, 3)) {
        LuaTools::type_error(l, 3, "boolean or nil");
      }
      bool traversable = lua_toboolean(l, 3);

      entity.set_can_traverse_ground(ground, traversable);
    }

    return 0;
  });
}

/**
 * \brief Implementation of custom_entity:add_collision_test().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::custom_entity_api_add_collision_test(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    CustomEntity& entity = *check_custom_entity(l, 1);

    const ScopedLuaRef& callback_ref = LuaTools::check_function(l, 3);

    if (lua_isstring(l, 2)) {
      // Built-in collision test.
      CollisionMode collision_mode = LuaTools::check_enum<CollisionMode>(l, 2,
          EnumInfoTraits<CollisionMode>::names_no_none_no_custom
      );
      entity.add_collision_test(collision_mode, callback_ref);
    }
    else if (lua_isfunction(l, 2)) {
      // Custom collision test.
      const ScopedLuaRef& collision_test_ref = LuaTools::check_function(l, 2);
      entity.add_collision_test(collision_test_ref, callback_ref);
    }
    else {
      LuaTools::type_error(l, 2, "string or function");
    }

    return 0;
  });
}

/**
 * \brief Implementation of custom_entity:clear_collision_tests().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::custom_entity_api_clear_collision_tests(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    CustomEntity& entity = *check_custom_entity(l, 1);

    entity.clear_collision_tests();

    return 0;
  });
}

/**
 * \brief Implementation of custom_entity:get_modified_ground().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::custom_entity_api_get_modified_ground(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const CustomEntity& entity = *check_custom_entity(l, 1);

    const Ground modified_ground = entity.get_modified_ground();

    if (modified_ground == Ground::EMPTY) {
      lua_pushnil(l);
    }
    else {
      push_string(l, enum_to_name(modified_ground));
    }
    return 1;
  });
}

/**
 * \brief Implementation of custom_entity:set_modified_ground().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::custom_entity_api_set_modified_ground(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    CustomEntity& entity = *check_custom_entity(l, 1);
    Ground modified_ground = Ground::EMPTY;

    if (lua_gettop(l) == 1) {
      LuaTools::type_error(l, 2, "string or nil");
    }
    if (!lua_isnil(l, 2)) {
      modified_ground = LuaTools::check_enum<Ground>(l, 2);
    }

    entity.set_modified_ground(modified_ground);
    return 0;
  });
}

/**
 * \brief Implementation of custom_entity:get_tiled().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::custom_entity_api_is_tiled(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const CustomEntity& entity = *check_custom_entity(l, 1);

    lua_pushboolean(l, entity.is_tiled());
    return 1;
  });
}

/**
 * \brief Implementation of custom_entity:set_tiled().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::custom_entity_api_set_tiled(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    CustomEntity& entity = *check_custom_entity(l, 1);
    bool tiled = LuaTools::opt_boolean(l, 2, true);

    entity.set_tiled(tiled);

    return 0;
  });
}

/**
 * \brief Implementation of custom_entity:get_follow_streams().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::custom_entity_api_get_follow_streams(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    const CustomEntity& entity = *check_custom_entity(l, 1);

    lua_pushboolean(l, entity.get_follow_streams());
    return 1;
  });
}

/**
 * \brief Implementation of custom_entity:set_follow_streams().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::custom_entity_api_set_follow_streams(lua_State* l) {

  return LuaTools::exception_boundary_handle(l, [&] {
    CustomEntity& entity = *check_custom_entity(l, 1);
    bool follow_streams = LuaTools::opt_boolean(l, 2, true);

    entity.set_follow_streams(follow_streams);

    return 0;
  });
}

/**
 * \brief Calls the on_update() method of a Lua map entity.
 *
 * Does nothing if the method is not defined.
 *
 * \param entity A map entity.
 */
void LuaContext::entity_on_update(Entity& entity) {

  // This particular method is tried so often that we want to optimize
  // the std::string construction.
  static const std::string method_name = "on_update";
  if (!userdata_has_field(entity, method_name)) {
    return;
  }

  push_entity(l, entity);
  on_update();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_suspended() method of a Lua map entity.
 *
 * Does nothing if the method is not defined.
 *
 * \param entity A map entity.
 * \param suspended \c true if the entity is suspended.
 */
void LuaContext::entity_on_suspended(Entity& entity, bool suspended) {

  if (!userdata_has_field(entity, "on_suspended")) {
    return;
  }

  push_entity(l, entity);
  on_suspended(suspended);
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_created() method of a Lua map entity.
 *
 * Does nothing if the method is not defined.
 *
 * \param entity A map entity.
 */
void LuaContext::entity_on_created(Entity& entity) {

  if (!userdata_has_field(entity, "on_created")) {
    return;
  }

  push_entity(l, entity);
  on_created();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_removed() method of a Lua map entity if it is defined.
 *
 * Also stops timers associated to the entity.
 *
 * \param entity A map entity.
 */
void LuaContext::entity_on_removed(Entity& entity) {

  push_entity(l, entity);
  if (userdata_has_field(entity, "on_removed")) {
    on_removed();
  }
  remove_timers(-1);  // Stop timers associated to this entity.
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_enabled() method of a Lua map entity.
 *
 * Does nothing if the method is not defined.
 *
 * \param entity A map entity.
 */
void LuaContext::entity_on_enabled(Entity& entity) {

  if (!userdata_has_field(entity, "on_enabled")) {
    return;
  }

  push_entity(l, entity);
  on_enabled();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_disabled() method of a Lua map entity.
 *
 * Does nothing if the method is not defined.
 *
 * \param entity A map entity.
 */
void LuaContext::entity_on_disabled(Entity& entity) {

  if (!userdata_has_field(entity, "on_disabled")) {
    return;
  }

  push_entity(l, entity);
  on_disabled();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_pre_draw() method of a Lua map entity.
 *
 * Does nothing if the method is not defined.
 *
 * \param entity A map entity.
 */
void LuaContext::entity_on_pre_draw(Entity& entity) {

  if (!userdata_has_field(entity, "on_pre_draw")) {
    return;
  }

  push_entity(l, entity);
  on_pre_draw();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_post_draw() method of a Lua map entity.
 *
 * Does nothing if the method is not defined.
 *
 * \param entity A map entity.
 */
void LuaContext::entity_on_post_draw(Entity& entity) {

  if (!userdata_has_field(entity, "on_post_draw")) {
    return;
  }

  push_entity(l, entity);
  on_post_draw();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_position_changed() method of a Lua map entity.
 *
 * Does nothing if the method is not defined.
 *
 * \param entity A map entity.
 * \param xy The new coordinates.
 * \param layer The new layer.
 */
void LuaContext::entity_on_position_changed(
    Entity& entity, const Point& xy, int layer) {

  if (!userdata_has_field(entity, "on_position_changed")) {
    return;
  }

  push_entity(l, entity);
  on_position_changed(xy, layer);
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_obstacle_reached() method of a Lua map entity.
 *
 * Does nothing if the method is not defined.
 *
 * \param entity A map entity.
 * \param movement The movement that reached an obstacle.
 */
void LuaContext::entity_on_obstacle_reached(
    Entity& entity, Movement& movement) {

  if (!userdata_has_field(entity, "on_obstacle_reached")) {
    return;
  }

  push_entity(l, entity);
  on_obstacle_reached(movement);
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_movement_started() method of a Lua map entity.
 *
 * Does nothing if the method is not defined.
 *
 * \param entity A map entity.
 */
  void LuaContext::entity_on_movement_started(
      Entity& entity, Movement& movement) {

  if (!userdata_has_field(entity, "on_movement_started")) {
    return;
  }

  push_entity(l, entity);
  on_movement_started(movement);
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_movement_changed() method of a Lua map entity.
 *
 * Does nothing if the method is not defined.
 *
 * \param entity A map entity.
 * \param movement Its movement.
 */
void LuaContext::entity_on_movement_changed(
    Entity& entity, Movement& movement) {

  if (!userdata_has_field(entity, "on_movement_changed")) {
    return;
  }

  push_entity(l, entity);
  on_movement_changed(movement);
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_movement_finished() method of a Lua map entity.
 *
 * Does nothing if the method is not defined.
 *
 * \param entity A map entity.
 */
void LuaContext::entity_on_movement_finished(Entity& entity) {

  if (!userdata_has_field(entity, "on_movement_finished")) {
    return;
  }

  push_entity(l, entity);
  on_movement_finished();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_interaction() method of a Lua map entity.
 *
 * Does nothing if the method is not defined.
 *
 * \param entity A map entity.
 * \return \c true if an interaction occurred.
 */
bool LuaContext::entity_on_interaction(Entity& entity) {

  if (!userdata_has_field(entity, "on_interaction")) {
    return false;
  }

  push_entity(l, entity);
  bool exists = on_interaction();
  lua_pop(l, 1);

  return exists;
}

/**
 * \brief Calls the on_interaction_item() method of a Lua map entity.
 *
 * Does nothing if the method is not defined.
 *
 * \param entity A map entity.
 * \param item_used The equipment item used.
 * \return \c true if an interaction occurred.
 */
bool LuaContext::entity_on_interaction_item(
    Entity& entity, EquipmentItem& item_used) {

  if (!userdata_has_field(entity, "on_interaction_item")) {
    return false;
  }

  push_entity(l, entity);
  bool result = on_interaction_item(item_used);
  lua_pop(l, 1);
  return result;
}

/**
 * \brief Calls the on_state_changed() method of a Lua entity.
 *
 * Does nothing if the method is not defined.
 *
 * \param entity A map entity.
 * \param state_name A name describing the new state.
 */
void LuaContext::entity_on_state_changed(
    Entity& entity, const std::string& state_name) {

  if (!userdata_has_field(entity, "on_state_changed")) {
    return;
  }

  push_entity(l, entity);
  on_state_changed(state_name);
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_lifting() method of a Lua entity.
 *
 * Does nothing if the method is not defined.
 *
 * \param entity The entity being lifted.
 * \param carrier Entity that is lifting the first one.
 * \param carried_object Carried object created to replace
 * the entity being lifted.
 */
void LuaContext::entity_on_lifting(
    Entity& entity,
    Entity& carrier,
    CarriedObject& carried_object
) {

  if (!userdata_has_field(entity, "on_lifting")) {
    return;
  }

  push_entity(l, entity);
  on_lifting(carrier, carried_object);
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_taking_damage() method of a Lua hero.
 *
 * Does nothing if the method is not defined.
 *
 * \param hero The hero.
 * \param damage The damage to take.
 * \return \c true if the event is defined.
 */
bool LuaContext::hero_on_taking_damage(Hero& hero, int damage) {

  if (!userdata_has_field(hero, "on_taking_damage")) {
    return false;
  }

  push_hero(l, hero);
  bool exists = on_taking_damage(damage);
  lua_pop(l, 1);
  return exists;
}

/**
 * \brief Calls the on_activated() method of a Lua destination.
 *
 * Does nothing if the method is not defined.
 *
 * \param destination A destination.
 */
void LuaContext::destination_on_activated(Destination& destination) {

  if (!userdata_has_field(destination, "on_activated")) {
    return;
  }

  push_entity(l, destination);
  on_activated();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_activated() method of a Lua teletransporter.
 *
 * Does nothing if the method is not defined.
 *
 * \param teletransporter A teletransporter.
 */
void LuaContext::teletransporter_on_activated(Teletransporter& teletransporter) {

  if (!userdata_has_field(teletransporter, "on_activated")) {
    return;
  }

  push_teletransporter(l, teletransporter);
  on_activated();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_collision_fire() method of a Lua NPC.
 *
 * Does nothing if the method is not defined.
 *
 * \param npc An NPC.
 */
void LuaContext::npc_on_collision_fire(Npc& npc) {

  if (!userdata_has_field(npc, "on_collision_fire")) {
    return;
  }

  push_npc(l, npc);
  on_collision_fire();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_moving() method of a Lua block.
 *
 * Does nothing if the method is not defined.
 *
 * \param block a block.
 */
void LuaContext::block_on_moving(Block& block) {

  if (!userdata_has_field(block, "on_moving")) {
    return;
  }

  push_block(l, block);
  on_moving();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_moved() method of a Lua block.
 *
 * Does nothing if the method is not defined.
 *
 * \param block a block.
 */
void LuaContext::block_on_moved(Block& block) {

  if (!userdata_has_field(block, "on_moved")) {
    return;
  }

  push_block(l, block);
  on_moved();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_opened() method of a Lua chest.
 *
 * Does nothing if the method is not defined.
 *
 * \param chest A chest.
 * \param treasure The treasure obtained.
 * \return \c true if the on_opened() method is defined.
 */
bool LuaContext::chest_on_opened(Chest& chest, const Treasure& treasure) {

  if (!userdata_has_field(chest, "on_opened")) {
    return false;
  }

  push_chest(l, chest);
  bool exists = on_opened(treasure);
  lua_pop(l, 1);
  return exists;
}

/**
 * \brief Calls the on_activated() method of a Lua switch.
 *
 * Does nothing if the method is not defined.
 *
 * \param sw A switch.
 */
void LuaContext::switch_on_activated(Switch& sw) {

  if (!userdata_has_field(sw, "on_activated")) {
    return;
  }

  push_switch(l, sw);
  on_activated();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_inactivated() method of a Lua switch.
 *
 * Does nothing if the method is not defined.
 *
 * \param sw A switch.
 */
void LuaContext::switch_on_inactivated(Switch& sw) {

  if (!userdata_has_field(sw, "on_inactivated")) {
    return;
  }

  push_switch(l, sw);
  on_inactivated();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_left() method of a Lua switch.
 *
 * Does nothing if the method is not defined.
 *
 * \param sw A switch.
 */
void LuaContext::switch_on_left(Switch& sw) {

  if (!userdata_has_field(sw, "on_left")) {
    return;
  }

  push_switch(l, sw);
  on_left();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_activated() method of a Lua sensor.
 *
 * Does nothing if the method is not defined.
 *
 * \param sensor A sensor.
 */
void LuaContext::sensor_on_activated(Sensor& sensor) {

  if (!userdata_has_field(sensor, "on_activated")) {
    return;
  }

  push_entity(l, sensor);
  on_activated();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_activated_repeat() method of a Lua sensor.
 *
 * Does nothing if the method is not defined.
 *
 * \param sensor A sensor.
 */
void LuaContext::sensor_on_activated_repeat(Sensor& sensor) {

  if (!userdata_has_field(sensor, "on_activated_repeat")) {
    return;
  }

  push_entity(l, sensor);
  on_activated_repeat();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_left() method of a Lua sensor.
 * \param sensor A sensor.
 */
void LuaContext::sensor_on_left(Sensor& sensor) {

  if (!userdata_has_field(sensor, "on_left")) {
    return;
  }

  push_entity(l, sensor);
  on_left();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_collision_explosion() method of a Lua sensor.
 *
 * Does nothing if the method is not defined.
 *
 * \param sensor A sensor.
 */
void LuaContext::sensor_on_collision_explosion(Sensor& sensor) {

  if (!userdata_has_field(sensor, "on_collision_explosion")) {
    return;
  }

  push_entity(l, sensor);
  on_collision_explosion();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_activating() method of a Lua separator.
 *
 * Does nothing if the method is not defined.
 *
 * \param separator A separator.
 * \param direction4 Direction of the traversal.
 */
void LuaContext::separator_on_activating(Separator& separator, int direction4) {

  if (!userdata_has_field(separator, "on_activating")) {
    return;
  }

  push_entity(l, separator);
  on_activating(direction4);
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_activated() method of a Lua separator.
 *
 * Does nothing if the method is not defined.
 *
 * \param separator A separator.
 * \param direction4 Direction of the traversal.
 */
void LuaContext::separator_on_activated(Separator& separator, int direction4) {

  if (!userdata_has_field(separator, "on_activated")) {
    return;
  }

  push_entity(l, separator);
  on_activated(direction4);
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_opened() method of a Lua door.
 *
 * Does nothing if the method is not defined.
 *
 * \param door A door.
 */
void LuaContext::door_on_opened(Door& door) {

  if (!userdata_has_field(door, "on_opened")) {
    return;
  }

  push_door(l, door);
  on_opened();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_closed() method of a Lua door.
 *
 * Does nothing if the method is not defined.
 *
 * \param door A door.
 */
void LuaContext::door_on_closed(Door& door) {

  if (!userdata_has_field(door, "on_closed")) {
    return;
  }

  push_door(l, door);
  on_closed();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_buying() method of a Lua shop treasure.
 *
 * Does nothing if the method is not defined.
 *
 * \param shop_treasure A shop treasure.
 * \return \c true if the player is allowed to buy the treasure.
 */
bool LuaContext::shop_treasure_on_buying(ShopTreasure& shop_treasure) {

  if (!userdata_has_field(shop_treasure, "on_buying")) {
    return true;
  }

  push_shop_treasure(l, shop_treasure);
  bool result = on_buying();
  lua_pop(l, 1);
  return result;
}

/**
 * \brief Calls the on_bought() method of a Lua shop treasure.
 *
 * Does nothing if the method is not defined.
 *
 * \param shop_treasure A shop treasure.
 */
void LuaContext::shop_treasure_on_bought(ShopTreasure& shop_treasure) {

  if (!userdata_has_field(shop_treasure, "on_bought")) {
    return;
  }

  push_shop_treasure(l, shop_treasure);
  on_bought();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_looked() method of a Lua destructible.
 *
 * Does nothing if the method is not defined.
 *
 * \param destructible A destructible object.
 */
void LuaContext::destructible_on_looked(Destructible& destructible) {

  if (!userdata_has_field(destructible, "on_looked")) {
    return;
  }

  push_destructible(l, destructible);
  on_looked();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_cut() method of a Lua destructible.
 *
 * Does nothing if the method is not defined.
 *
 * \param destructible A destructible object.
 */
void LuaContext::destructible_on_cut(Destructible& destructible) {

  if (!userdata_has_field(destructible, "on_cut")) {
    return;
  }

  push_destructible(l, destructible);
  on_cut();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_exploded() method of a Lua destructible.
 *
 * Does nothing if the method is not defined.
 *
 * \param destructible A destructible object.
 */
void LuaContext::destructible_on_exploded(Destructible& destructible) {

  if (!userdata_has_field(destructible, "on_exploded")) {
    return;
  }

  push_destructible(l, destructible);
  on_exploded();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_regenerating() method of a Lua destructible.
 *
 * Does nothing if the method is not defined.
 *
 * \param destructible A destructible object.
 */
void LuaContext::destructible_on_regenerating(Destructible& destructible) {

  if (!userdata_has_field(destructible, "on_regenerating")) {
    return;
  }

  push_destructible(l, destructible);
  on_regenerating();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_restarted() method of a Lua enemy if it is defined.
 *
 * Also stops timers associated to the entity.
 *
 * \param enemy An enemy.
 */
void LuaContext::enemy_on_restarted(Enemy& enemy) {

  push_enemy(l, enemy);
  remove_timers(-1);  // Stop timers associated to this enemy.
  if (userdata_has_field(enemy, "on_restarted")) {
    on_restarted();
  }
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_collision_enemy() method of a Lua enemy.
 *
 * Does nothing if the method is not defined.
 *
 * \param enemy An enemy.
 * \param other_enemy Another enemy colliding with the first one.
 * \param other_sprite Colliding sprite of the other enemy.
 * \param this_sprite Colliding sprite of the first enemy.
 */
void LuaContext::enemy_on_collision_enemy(Enemy& enemy,
    Enemy& other_enemy, Sprite& other_sprite, Sprite& this_sprite) {

  if (!userdata_has_field(enemy, "on_collision_enemy")) {
    return;
  }

  push_enemy(l, enemy);
  on_collision_enemy(other_enemy, other_sprite, this_sprite);
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_custom_attack_received() method of a Lua enemy.
 *
 * Does nothing if the method is not defined.
 *
 * \param enemy An enemy.
 * \param attack The attack received.
 * \param sprite The sprite that receives the attack if any.
 */
void LuaContext::enemy_on_custom_attack_received(Enemy& enemy,
    EnemyAttack attack, Sprite* sprite) {

  if (!userdata_has_field(enemy, "on_custom_attack_received")) {
    return;
  }

  push_enemy(l, enemy);
  on_custom_attack_received(attack, sprite);
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_hurt_by_sword() method of a Lua enemy if it is defined.
 * \param enemy An enemy.
 * \param hero The hero whose sword is hitting the enemy.
 * \param enemy_sprite Sprite of the enemy that gets hits.
 * \return \c true if the method is defined.
 */
bool LuaContext::enemy_on_hurt_by_sword(
    Enemy& enemy, Hero& hero, Sprite& enemy_sprite) {

  if (!userdata_has_field(enemy, "on_hurt_by_sword")) {
    return false;
  }

  push_enemy(l, enemy);
  bool exists = on_hurt_by_sword(hero, enemy_sprite);
  lua_pop(l, 1);
  return exists;
}

/**
 * \brief Calls the on_hurt() method of a Lua enemy if it is defined.
 *
 * Also stops timers associated to the enemy.
 *
 * \param enemy An enemy.
 * \param attack The attack received.
 */
void LuaContext::enemy_on_hurt(Enemy& enemy, EnemyAttack attack) {

  push_enemy(l, enemy);
  remove_timers(-1);  // Stop timers associated to this enemy.
  if (userdata_has_field(enemy, "on_hurt")) {
    on_hurt(attack);
  }
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_dying() method of a Lua enemy if it is defined.
 *
 * Also stops timers associated to the enemy.
 *
 * \param enemy An enemy.
 */
void LuaContext::enemy_on_dying(Enemy& enemy) {

  push_enemy(l, enemy);
  remove_timers(-1);  // Stop timers associated to this enemy.
  if (userdata_has_field(enemy, "on_dying")) {
    on_dying();
  }
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_dead() method of a Lua enemy.
 *
 * Does nothing if the method is not defined.
 *
 * \param enemy An enemy.
 */
void LuaContext::enemy_on_dead(Enemy& enemy) {

  if (!userdata_has_field(enemy, "on_dead")) {
    return;
  }

  push_enemy(l, enemy);
  on_dead();
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_immobilized() method of a Lua enemy if it is defined.
 *
 * Also stops timers associated to the enemy.
 *
 * \param enemy An enemy.
 */
void LuaContext::enemy_on_immobilized(Enemy& enemy) {

  push_enemy(l, enemy);
  remove_timers(-1);  // Stop timers associated to this enemy.
  if (userdata_has_field(enemy, "on_immobilized")) {
    on_immobilized();
  }
  lua_pop(l, 1);
}

/**
 * \brief Calls the on_attacking_hero() method of a Lua enemy.
 *
 * Does nothing if the method is not defined.
 *
 * \param enemy An enemy.
 * \param hero The hero attacked.
 * \param attacker_sprite Enemy's sprite that caused the collision or nullptr.
 * \return \c true if the method is defined.
 */
bool LuaContext::enemy_on_attacking_hero(Enemy& enemy, Hero& hero, Sprite* attacker_sprite) {

  if (!userdata_has_field(enemy, "on_attacking_hero")) {
    return false;
  }

  push_enemy(l, enemy);
  bool exists = on_attacking_hero(hero, attacker_sprite);
  lua_pop(l, 1);
  return exists;
}

/**
 * \brief Calls the on_ground_below_changed() method of a Lua custom entity.
 *
 * Does nothing if the method is not defined.
 *
 * \param custom_entity A custom entity.
 * \param ground_below The new ground below this entity.
 */
void LuaContext::custom_entity_on_ground_below_changed(
    CustomEntity& custom_entity, Ground ground_below) {

  if (!userdata_has_field(custom_entity, "on_ground_below_changed")) {
    return;
  }

  push_custom_entity(l, custom_entity);
  on_ground_below_changed(ground_below);
  lua_pop(l, 1);
}

}

