/*
 * Copyright (C) 2006-2019 Christopho, Solarus - http://www.solarus-games.org
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
#include "solarus/core/Debug.h"
#include "solarus/core/CommandsEffects.h"
#include "solarus/core/Equipment.h"
#include "solarus/core/EquipmentItem.h"
#include "solarus/core/Game.h"
#include "solarus/core/Map.h"
#include "solarus/core/System.h"
#include "solarus/entities/Block.h"
#include "solarus/entities/Bomb.h"
#include "solarus/entities/Boomerang.h"
#include "solarus/entities/Chest.h"
#include "solarus/entities/Crystal.h"
#include "solarus/entities/Destination.h"
#include "solarus/entities/Destructible.h"
#include "solarus/entities/Enemy.h"
#include "solarus/entities/Entities.h"
#include "solarus/entities/GroundInfo.h"
#include "solarus/entities/Hero.h"
#include "solarus/entities/Jumper.h"
#include "solarus/entities/Sensor.h"
#include "solarus/entities/Stairs.h"
#include "solarus/entities/Stream.h"
#include "solarus/entities/StreamAction.h"
#include "solarus/entities/Switch.h"
#include "solarus/entities/Teletransporter.h"
#include "solarus/graphics/Sprite.h"
#include "solarus/hero/BackToSolidGroundState.h"
#include "solarus/hero/BoomerangState.h"
#include "solarus/hero/BowState.h"
#include "solarus/hero/CarryingState.h"
#include "solarus/hero/CustomState.h"
#include "solarus/hero/FallingState.h"
#include "solarus/hero/ForcedWalkingState.h"
#include "solarus/hero/FreeState.h"
#include "solarus/hero/FrozenState.h"
#include "solarus/hero/GrabbingState.h"
#include "solarus/hero/HeroSprites.h"
#include "solarus/hero/HookshotState.h"
#include "solarus/hero/HurtState.h"
#include "solarus/hero/JumpingState.h"
#include "solarus/hero/LiftingState.h"
#include "solarus/hero/PlungingState.h"
#include "solarus/hero/PullingState.h"
#include "solarus/hero/PushingState.h"
#include "solarus/hero/RunningState.h"
#include "solarus/hero/StairsState.h"
#include "solarus/hero/SwordLoadingState.h"
#include "solarus/hero/SwordSwingingState.h"
#include "solarus/hero/SwimmingState.h"
#include "solarus/hero/TreasureState.h"
#include "solarus/hero/UsingItemState.h"
#include "solarus/hero/VictoryState.h"
#include "solarus/lua/LuaContext.h"
#include "solarus/lua/LuaTools.h"
#include "solarus/movements/StraightMovement.h"
#include <lua.hpp>
#include <algorithm>
#include <sstream>
#include <utility>

namespace Solarus {

namespace {

/**
 * \brief Implementation of the callback used to return solid ground coordinates.
 * \param l A Lua state.
 * \return Number of values to return to Lua.
 */
int l_solid_ground_callback(lua_State* l) {

  int x = LuaTools::check_int(l, lua_upvalueindex(1));
  int y = LuaTools::check_int(l, lua_upvalueindex(2));
  int layer = LuaTools::check_int(l, lua_upvalueindex(3));

  lua_pushinteger(l, x);
  lua_pushinteger(l, y);
  lua_pushinteger(l, layer);
  return 3;
}

}

/**
 * \brief Creates a hero.
 * \param equipment the equipment (needed to build the sprites even outside a game)
 */
Hero::Hero(Equipment& equipment):
  Entity("hero", 0, 0, Point(0, 0), Size(16, 16)),
  invincible(false),
  end_invincible_date(0),
  normal_walking_speed(88),
  walking_speed(normal_walking_speed),
  delayed_teletransporter(nullptr),
  on_raised_blocks(false),
  last_solid_ground_coords(0, 0),
  last_solid_ground_layer(0),
  target_solid_ground_callback(),
  next_ground_date(0),
  next_ice_date(0),
  ice_movement_direction8(0) {

  // position
  set_origin(8, 13);
  last_solid_ground_coords = { -1, -1 };
  last_solid_ground_layer = 0;

  // sprites
  set_drawn_in_y_order(true);
  sprites = std::unique_ptr<HeroSprites>(new HeroSprites(*this, equipment));
}

/**
 * \brief Returns the type of entity.
 * \return the type of entity
 */
EntityType Hero::get_type() const {
  return ThisType;
}

/**
 * \brief Returns the item currently carried by the hero, if any.
 *
 * This function is used internally to allow this item to be preserved between
 * different hero states.
 *
 * \return The carried object or nullptr.
 */
std::shared_ptr<CarriedObject> Hero::get_carried_object() {
  return get_state()->get_carried_object();
}

/**
 * \brief Suspends or resumes the animation and the movements of the hero.
 *
 * This function is called by the map when the game is suspended or resumed.
 *
 * \param suspended true to suspend the hero, false to resume it
 */
void Hero::set_suspended(bool suspended) {

  Entity::set_suspended(suspended);

  if (!suspended) {

    uint32_t diff = System::now() - get_when_suspended();
    next_ground_date += diff;

    if (end_invincible_date != 0) {
      end_invincible_date += diff;
    }
  }

  sprites->set_suspended(suspended);
  get_state()->set_suspended(suspended);
}

/**
 * \brief Updates the hero's position, movement and animation.
 *
 * This function is called repeatedly by the game loop.
 */
void Hero::update() {

  update_invincibility();
  update_movement();
  update_direction();
  sprites->update();

  // Update the state now because it may be impacted by movements and sprites.
  update_state();

  if (!is_suspended()) {
    update_ground_effects();
    check_collision_with_detectors();
    check_gameover();
  }
}

/**
 * \brief Updates the sprites direction according to the wanted movement direction.
 */
void Hero::update_direction() {

  int wanted_direction8 = get_wanted_movement_direction8();
  if (wanted_direction8 == -1) {
    return;
  }

  int old_animation_direction = sprites->get_animation_direction();
  int animation_direction = sprites->get_animation_direction(wanted_direction8, get_real_movement_direction8());

  if (animation_direction != old_animation_direction
      && animation_direction != -1
      && !is_direction_locked()) {
    // If the direction defined by the directional keys has changed,
    // update the sprite's direction of animation
    // (unless the direction is locked).
    sprites->set_animation_direction(animation_direction);
  }
}

/**
 * \brief Updates the hero's position according to its movement.
 *
 * This function is called repeatedly by update().
 */
void Hero::update_movement() {

  if (!get_map().is_loaded()) {
    // Can happen during transitions.
    return;
  }

  if (on_raised_blocks && !get_state()->is_touching_ground()) {
    // If the hero was already over raised blocks, keep it that way while he
    // is not touching ground.
  }
  else {
    on_raised_blocks = get_entities().overlaps_raised_blocks(get_layer(), get_bounding_box());
  }

  if (get_movement() != nullptr) {
    get_movement()->update();
  }
  // TODO clear_old_movements() is missing

  update_stream_action();
}

/**
 * \brief Updates the effects (if any) of the ground below the hero.
 *
 * This function is called repeatedly.
 */
void Hero::update_ground_effects() {

  // see if it's time to do something (depending on the ground)
  uint32_t now = System::now();
  if (now >= next_ground_date) {

    if (is_ground_visible() && get_movement() != nullptr) {

      // a special ground is displayed under the hero and it's time to play a sound
      const std::shared_ptr<StraightMovement> movement =
          std::dynamic_pointer_cast<StraightMovement>(get_movement());
      if (movement != nullptr) {
        // TODO replace the dynamic_pointer_cast by a virtual method get_speed() in Movement.
        double speed = movement->get_speed();
        next_ground_date = now + std::max(150, (int) (20000 / speed));
        if (sprites->is_walking() && get_state()->is_touching_ground()) {
          sprites->play_ground_sound();
        }
      }
    }

    else {

      Ground ground = get_ground_below();
      if (ground == Ground::HOLE && !get_state()->can_avoid_hole()) {
        // the hero is being attracted by a hole and it's time to move one more pixel into the hole

        next_ground_date = now + 60;

        if (get_distance(last_solid_ground_coords) >= 8) {
          // too far from the solid ground: make the hero fall
          set_walking_speed(normal_walking_speed);
          set_state(std::make_shared<FallingState>(*this));
        }
        else {
          // not too far yet
          apply_additional_ground_movement();
        }
      }
      else if (ground == Ground::ICE) {

        // Slide on ice.
        if (!get_state()->can_avoid_ice()) {
          apply_additional_ground_movement();
        }

        next_ground_date = now + 30;

        if (now >= next_ice_date) {
          // Time to update the additional movement.
          update_ice();
          ice_movement_direction8 = get_wanted_movement_direction8();
        }
      }
    }
  }
}

/**
 * \brief Updates the additional movement applied when the hero is on ice ground.
 */
void Hero::update_ice() {

  uint32_t now = System::now();
  int wanted_movement_direction8 = get_wanted_movement_direction8();
  if (wanted_movement_direction8 == -1) {
    // The player wants to stop.
    if (ice_movement_direction8 == -1) {
      // And he does for a while so stop.
      ground_dxy = { 0, 0 };
      next_ice_date = now + 300;
    }
    else {
      // But he was just moving on ice: continue the ice movement.
      ground_dxy = direction_to_xy_move(ice_movement_direction8) * 2;
      next_ice_date = now + 300;
    }
  }
  else {
    // The player wants to move.
    if (ice_movement_direction8 == -1) {
      // But he was not just moving on ice: resist to the wanted movement.
      ground_dxy = direction_to_xy_move((wanted_movement_direction8 + 4) % 8);
      next_ice_date = now + 300;
    }
    else if (ice_movement_direction8 != wanted_movement_direction8) {
      // He changed his direction: continue the ice movement strongly.
      ground_dxy = direction_to_xy_move(ice_movement_direction8) * 2;
      next_ice_date = now + 300;
    }
    else {
      // He continues in the same direction.
      ground_dxy = direction_to_xy_move(wanted_movement_direction8);
      next_ice_date = now + 300;
    }
  }
}

/**
 * \brief Stops the additional movement applied when the hero is on ice ground.
 */
void Hero::stop_ice_movement() {

  ice_movement_direction8 = 0;
  ground_dxy = { 0, 0 };
}

/**
 * \brief Changes the position of the hero as an effect of his current ground
 * (like hole or ice).
 */
void Hero::apply_additional_ground_movement() {

  if (ground_dxy.x == 0 && ground_dxy.y == 0) {
    return;
  }

  bool moved = false;
  Rectangle collision_box = get_bounding_box();
  collision_box.add_xy(ground_dxy);

  if (!get_map().test_collision_with_obstacles(get_layer(), collision_box, *this)) {
    set_bounding_box(collision_box);
    notify_position_changed();
    moved = true;
  }

  if (!moved && ground_dxy.x != 0) { // try x only
    collision_box = get_bounding_box();
    collision_box.add_xy(ground_dxy.x, 0);
    if (!get_map().test_collision_with_obstacles(get_layer(), collision_box, *this)) {
      set_bounding_box(collision_box);
      notify_position_changed();
      moved = true;
    }
  }

  if (!moved && ground_dxy.y != 0) { // try y only
    collision_box = get_bounding_box();
    collision_box.add_xy(0, ground_dxy.y);
    if (!get_map().test_collision_with_obstacles(get_layer(), collision_box, *this)) {
      set_bounding_box(collision_box);
      notify_position_changed();
      moved = true;
    }
  }

  if (!moved) {
    if (get_ground_below() == Ground::HOLE) {
      // the hero cannot be moved towards the direction previously calculated
      set_walking_speed(normal_walking_speed);
      set_state(std::make_shared<FallingState>(*this));
    }
  }
}

/**
 * \brief Checks that the hero has still some life.
 *
 * If his life reaches zero, this function starts the gameover sequence.
 */
void Hero::check_gameover() {

  if (get_equipment().get_life() <= 0 &&
      get_state()->can_start_gameover_sequence()) {
    sprites->stop_blinking();
    get_game().start_game_over();
  }
}

/**
 * \copydoc Entity::built_in_draw
 */
void Hero::built_in_draw(Camera& /* camera */) {

  get_state()->draw_on_map();
}

/**
 * \brief This function is called when a low-level input event occurs.
 * \param event The event to handle.
 * \return \c true if the event was handled.
 * If \c false, notify_command_pressed/released() can then still be called
 * if they apply.
 */
bool Hero::notify_input(const InputEvent& event) {
  return get_state()->notify_input(event);
}

/**
 * \brief This function is called when a game command is pressed
 * and the game is not suspended.
 * \param command The command pressed.
 */
void Hero::notify_command_pressed(GameCommand command) {
  get_state()->notify_command_pressed(command);
}

/**
 * \brief This function is called when a game command is released
 * if the game is not suspended.
 * \param command The command released.
 */
void Hero::notify_command_released(GameCommand command) {
  get_state()->notify_command_released(command);
}

/**
 * \brief Returns the sprites currently representing the hero.
 * \return the sprites
 */
const HeroSprites& Hero::get_hero_sprites() const {
  return *sprites;
}

/**
 * \overload Non-const version.
 */
HeroSprites& Hero::get_hero_sprites() {
  return *sprites;
}

/**
 * \brief Returns the direction of the hero's sprites.
 *
 * It is different from the movement direction.
 *
 * \return the direction of the sprites (0 to 3)
 */
int Hero::get_animation_direction() const {
  return sprites->get_animation_direction();
}

/**
 * \brief Changes the direction of the hero's sprites.
 *
 * It is different from the movement direction.
 *
 * \param direction4 the direction to set (0 to 3)
 */
void Hero::set_animation_direction(int direction4) {
  sprites->set_animation_direction(direction4);
}

/**
 * \brief Returns whether the sprites animations are finished.
 * \return true if the animation is finished
 */
bool Hero::is_animation_finished() const {
  return sprites->is_animation_finished();
}

/**
 * \brief Loads (or reloads) the sprites and sounds of the hero and his equipment.
 *
 * The sprites and sounds depend on its tunic, sword and shield as specified in the savegame.
 * This function must be called at the game beginning
 * and as soon as the hero's equipment is changed.
 */
void Hero::rebuild_equipment() {

  sprites->rebuild_equipment();
}

/**
 * \brief Returns whether the shadow should be currently displayed, separate from the tunic sprite.
 * \return true if the shadow should be currently displayed.
 */
bool Hero::is_shadow_visible() const {
  return get_displayed_xy().y != get_y();
}

/**
 * \copydoc Entity::notify_creating
 */
void Hero::notify_creating() {

  Entity::notify_creating();
  get_hero_sprites().notify_creating();

  // At this point the map is known and loaded. Notify the state.
  get_state()->set_map(get_map());
}

/**
 * \copydoc Entity::notify_map_starting
 */
void Hero::notify_map_starting(Map& map, const std::shared_ptr<Destination>& destination) {

  Entity::notify_map_starting(map, destination);

  // At this point the map is known and loaded. Notify the state.
  get_state()->set_map(get_map());
}

/**
 * \copydoc Entity::notify_map_started
 */
void Hero::notify_map_started(Map& map, const std::shared_ptr<Destination>& destination) {

  Entity::notify_map_started(map, destination);
  get_state()->notify_map_started(map, destination);
}

/**
 * \copydoc Entity::notify_map_opening_transition_finishing
 */
void Hero::notify_map_opening_transition_finishing(Map& map, const std::shared_ptr<Destination>& destination) {

  Entity::notify_map_opening_transition_finishing(map, destination);

  int side = get_map().get_destination_side();
  if (side != -1) {
    // the hero was placed on the side of the map:
    // there was a scrolling between the previous map and this one

    switch (side) {

    case 0: // right side
      set_x(get_map().get_width() - 8);
      break;

    case 1: // top side
      set_y(13);
      break;

    case 2: // left side
      set_x(8);
      break;

    case 3: // bottom side
      set_y(get_map().get_height() - 3);
      break;

    default:
      Debug::die("Invalid destination side");
    }
  }
  check_position();
  if (get_state()->is_touching_ground()) {  // Don't change the state during stairs.
    start_state_from_ground();
  }
}

/**
 * \copydoc Entity::notify_map_opening_transition_finished
 */
void Hero::notify_map_opening_transition_finished(Map& map, const std::shared_ptr<Destination>& destination) {

  Entity::notify_map_opening_transition_finished(map, destination);
  get_state()->notify_map_opening_transition_finished(map, destination);
}

/**
 * \copydoc Entity::notify_map_finished
 */
void Hero::notify_map_finished() {

  Entity::notify_map_finished();
  get_state()->notify_map_finished();
}

/**
 * \copydoc Entity::notify_tileset_changed
 */
void Hero::notify_tileset_changed() {

  Entity::notify_tileset_changed();
  get_hero_sprites().notify_tileset_changed();
}

/**
 * \brief Puts the hero on a map.
 *
 * This function is called when the current map is changed.
 * Does nothing if the hero is already on this map.
 *
 * \param map The map.
 */
void Hero::place_on_map(Map& map) {

  if (is_on_map() &&
      &get_map() == &map
  ) {
    // No change.
    return;
  }

  // Add the hero to the map.
  const HeroPtr& shared_hero = std::static_pointer_cast<Hero>(shared_from_this());
  map.get_entities().add_entity(shared_hero);

  last_solid_ground_coords = { -1, -1 };
  last_solid_ground_layer = 0;
  reset_target_solid_ground_callback();
  get_hero_sprites().set_clipping_rectangle();

  get_state()->set_map(map);

  Entity::set_map(map);
}

/**
 * \brief Places the hero on the map specified and at its destination point selected.
 * \param map The new map.
 * \param previous_map_location Position of the previous map in its world
 * (because the previous map is already destroyed).
 */
void Hero::place_on_destination(Map& map, const Rectangle& previous_map_location) {

  const std::string& destination_name = map.get_destination_name();

  if (destination_name == "_same") {

    // The hero's coordinates are the same as on the previous map
    // but 1) we need to take into account the location of both maps
    // and 2) we may have to change the layer.

    const Rectangle& next_map_location = map.get_location();
    int x = get_x() - next_map_location.get_x() + previous_map_location.get_x();
    int y = get_y() - next_map_location.get_y() + previous_map_location.get_y();

    int layer = map.get_max_layer();
    while (layer > map.get_min_layer() &&
        map.get_ground(layer, x, y, this) == Ground::EMPTY) {
      // TODO check the whole hero's bounding box rather than just a point.
      --layer;
    }
    place_on_map(map);
    set_xy(x, y);
    map.get_entities().notify_entity_bounding_box_changed(*this);
    map.get_entities().set_entity_layer(*this, layer);
    last_solid_ground_coords = { x, y };
    last_solid_ground_layer = get_layer();

    start_free();
    check_position();  // To appear initially swimming, for example.
  }
  else {
    int side = map.get_destination_side();

    if (side != -1) {

      // Go to a side of the other map.
      int layer = get_layer();
      if (!map.is_valid_layer(layer)) {
        std::ostringstream oss;
        oss << "Layer " << layer << " does not exist on map '" << map.get_id()
               + "'. Placing the hero on layer 0 instead.";
        Debug::error(oss.str());
        layer = 0;
        set_layer(layer);
      }
      place_on_map(map);

      switch (side) {

      case 0: // Right side.
        set_x(map.get_width());
        set_y(get_y() - map.get_location().get_y() + previous_map_location.get_y());
        break;

      case 1: // Top side.
        set_y(5);
        set_x(get_x() - map.get_location().get_x() + previous_map_location.get_x());
        break;

      case 2: // Left side.
        set_x(0);
        set_y(get_y() - map.get_location().get_y() + previous_map_location.get_y());
        break;

      case 3: // Bottom side.
        set_y(map.get_height() + 5);
        set_x(get_x() - map.get_location().get_x() + previous_map_location.get_x());
        break;

      default:
        Debug::die("Invalid destination side");
      }
      map.get_entities().notify_entity_bounding_box_changed(*this);
      map.get_entities().set_entity_layer(*this, layer);
      last_solid_ground_coords = get_xy();
      last_solid_ground_layer = get_layer();

      // Note that we keep the hero's state from the previous map.
      check_position();  // Correctly initialize the ground.
    }
    else {

      // Normal case: the location is specified by a destination point object.

      const std::shared_ptr<Destination> destination = map.get_destination();

      if (destination == nullptr) {
        // This is embarrassing: there is no valid destination that we can use.
        // The map is probably in an early development phase.
        // For now, let's place the hero at the top-left corner of the map.
        Debug::error(
            std::string("No valid destination on map '") + map.get_id()
            + "'. Placing the hero at (0,0) instead."
        );
        place_on_map(map);
        sprites->set_animation_direction(3);
        set_top_left_xy(0, 0);
        map.get_entities().notify_entity_bounding_box_changed(*this);
        map.get_entities().set_entity_layer(*this, map.get_max_layer());
      }
      else {
        // Normal case.
        place_on_map(map);
        if (destination->get_direction() != -1) {
          sprites->set_animation_direction(destination->get_direction());
        }
        set_xy(destination->get_xy());
        map.get_entities().notify_entity_bounding_box_changed(*this);
        map.get_entities().set_entity_layer(*this, destination->get_layer());
      }
      last_solid_ground_coords = get_xy();
      last_solid_ground_layer = get_layer();

      // Remove boomerangs in case the map remains the same.
      const std::set<std::shared_ptr<Boomerang>>& boomerangs =
          map.get_entities().get_entities_by_type<Boomerang>();
      for (const std::shared_ptr<Boomerang>& boomerang : boomerangs) {
        boomerang->remove_from_map();
      }

      if (destination != nullptr) {
        get_lua_context()->destination_on_activated(*destination);
      }

      const std::shared_ptr<const Stairs> stairs = get_stairs_overlapping();
      if (stairs != nullptr) {
        // The hero arrived on the map by stairs.
        set_state(std::make_shared<StairsState>(*this, stairs, Stairs::REVERSE_WAY));
      }
      else {
        // The hero arrived on the map by a usual destination point.
        start_free();
        check_position();  // To appear initially swimming, for example.
      }
    }
  }
}

/**
 * \copydoc Entity::get_facing_point
 *
 * TODO remove this override
 */
Point Hero::get_facing_point() const {

  return get_touching_point(get_animation_direction());
}

/**
 * \copydoc Entity::notify_facing_entity_changed
 */
void Hero::notify_facing_entity_changed(Entity* facing_entity) {

  CommandsEffects& commands_effects = get_commands_effects();
  if (facing_entity != nullptr) {
    if (facing_entity->can_be_lifted() &&
        is_free() &&
        commands_effects.get_action_key_effect() == CommandsEffects::ACTION_KEY_NONE) {
      commands_effects.set_action_key_effect(CommandsEffects::ACTION_KEY_LIFT);
    }
  }
  else {
    // No more facing entity.
    if (commands_effects.is_action_key_acting_on_facing_entity()) {
      // The hero just stopped facing an entity that was showing an action icon.
      commands_effects.set_action_key_effect(CommandsEffects::ACTION_KEY_NONE);
    }
  }
}

/**
 * \brief Returns whether there is an obstacle in front of the hero.
 *
 * This function returns whether he is touching an obstacle that
 * is just in front of him.
 * Note that even if this function returns true, the hero
 * may still be able to move in that direction due to the possibly sliding movement.
 * This information is calculated and not stored, so it is
 * always up to date.
 *
 * \return true if the hero is facing an obstacle
 */
bool Hero::is_facing_obstacle() {

  Rectangle collision_box = get_bounding_box();
  switch (sprites->get_animation_direction()) {

    case 0:
      collision_box.add_x(1);
      break;

    case 1:
      collision_box.add_y(-1);
      break;

    case 2:
      collision_box.add_x(-1);
      break;

    case 3:
      collision_box.add_y(1);
      break;

    default:
      Debug::die("Invalid animation direction");
      break;
  }

  return get_map().test_collision_with_obstacles(get_layer(), collision_box, *this);
}

/**
 * \brief Returns whether the facing point of the hero is overlapping an obstacle.
 *
 * This function returns whether his facing point is overlapping an obstacle of the map.
 * This information is calculated and not stored, so it is always up to date.
 *
 * \return true if the facing point is overlapping an obstacle
 */
bool Hero::is_facing_point_on_obstacle() {

  const Point& facing_point = get_facing_point();
  return get_map().test_collision_with_obstacles(
      get_layer(), facing_point, *this);
}

/**
 * \brief Returns whether the hero is looking towards the specified direction.
 * \param direction4 a direction (0 to 3)
 * \return true if the hero is looking towards the specified direction
 */
bool Hero::is_facing_direction4(int direction4) const {
  return get_animation_direction() == direction4;
}

/**
 * \brief Returns whether the hero is looking towards the specified direction.
 * \param direction8 a direction (0 to 7)
 * \return true if the hero is looking towards the specified direction
 * (always false for diagonal directions)
 */
bool Hero::is_facing_direction8(int direction8) const {
  return get_animation_direction() * 2 == direction8;
}

/**
 * \brief Returns whether the hero is currently on raised crystal blocks.
 * \return true if the hero is currently on raised crystal blocks
 */
bool Hero::is_on_raised_blocks() const {
  return on_raised_blocks;
}

/**
 * \brief Returns the stairs the hero may be currently overlapping.
 * \return the stairs the hero is currently overlapping, or nullptr
 */
std::shared_ptr<const Stairs> Hero::get_stairs_overlapping() const {

  std::set<std::shared_ptr<const Stairs>> all_stairs =
      get_entities().get_entities_by_type<Stairs>(get_layer());
  for (const std::shared_ptr<const Stairs>& stairs: all_stairs) {

    if (overlaps(*stairs)) {
      return stairs;
    }
  }

  return nullptr;
}

/**
 * \brief Returns whether the player can control his movements in the current state.
 * \return true if the player can control his movements
 */
bool Hero::can_control_movement() const {

  if (has_stream_action()
      && !get_stream_action()->get_stream().get_allow_movement()) {
    // A stream prevents the control.
    return false;
  }

  return get_state()->get_can_control_movement();
}

/**
 * \brief Returns the speed applied to the walking movement on normal ground.
 * \return The normal walking speed in pixels per second.
 */
int Hero::get_normal_walking_speed() const {
  return normal_walking_speed;
}

/**
 * \brief Sets the speed applied to the walking movement on normal ground.
 * \param normal_walking_speed The normal walking speed to set in pixels per second.
 */
void Hero::set_normal_walking_speed(int normal_walking_speed) {

  bool was_normal = (this->walking_speed == this->normal_walking_speed);
  this->normal_walking_speed = normal_walking_speed;
  if (was_normal) {
    set_walking_speed(normal_walking_speed);
  }
}

/**
 * \brief Returns the current speed applied to the hero's movements when he is walking.
 * \return The current walking speed.
 */
int Hero::get_walking_speed() const {
  return walking_speed;
}

/**
 * \brief Sets the speed to apply to the hero's movements when he is walking.
 * \param walking_speed the new walking speed
 */
void Hero::set_walking_speed(int walking_speed) {

  if (walking_speed != this->walking_speed) {
    this->walking_speed = walking_speed;
    get_state()->notify_walking_speed_changed();
  }
}

/**
 * \brief Returns the direction of the hero's movement as defined by the controls applied by the player.
 *
 * If he is not moving, -1 is returned.
 * This direction may be different from the real movement direction because of obstacles.
 *
 * \return the hero's wanted direction between 0 and 7, or -1 if he is stopped
 */
int Hero::get_wanted_movement_direction8() const {
  return get_state()->get_wanted_movement_direction8();
}

/**
 * \brief Returns the actual direction of the hero's movement.
 *
 * This function returns the actual direction of the hero's movement, which can be different from the one
 * defined by the directional keys pressed by the player because we consider obstacles here.
 * If he does not want to move, -1 is returned. If he is trying to move but cannot because of obstacles,
 * the direction he is trying to move toward is returned.
 * This function is not used to compute the hero's movement (PlayerMovement does that) but only
 * to decide what direction to give to its sprites once the movement is already computed.
 *
 * \return the hero's actual direction between 0 and 7, or -1 if he is stopped
 */
int Hero::get_real_movement_direction8() {

  int result;

  int wanted_direction8 = get_wanted_movement_direction8();
  if (wanted_direction8 == -1) {
    // the hero does not want to move
    result = -1;
  }
  else {
    // the hero wants to move

    Rectangle collision_box(get_bounding_box());

    // if we can move towards the wanted direction, no problem
    Point xy_move = direction_to_xy_move(wanted_direction8);
    collision_box.add_xy(xy_move);
    if (!get_map().test_collision_with_obstacles(get_layer(), collision_box, *this)) {
      result = wanted_direction8;
    }
    else {
      // otherwise, see if he can move in one of the two closest directions (i.e. he is sliding)

      int alternative_direction8 = (wanted_direction8 + 1) % 8;
      collision_box = get_bounding_box();
      xy_move = direction_to_xy_move(alternative_direction8);
      collision_box.add_xy(xy_move);
      if (!get_map().test_collision_with_obstacles(get_layer(), collision_box, *this)) {
        result = alternative_direction8;
      }
      else {
        alternative_direction8 = (wanted_direction8 - 1) % 8;
        collision_box = get_bounding_box();
        xy_move = direction_to_xy_move(alternative_direction8);
        collision_box.add_xy(xy_move);
        if (!get_map().test_collision_with_obstacles(get_layer(), collision_box, *this)) {
          result = alternative_direction8;
        }
        else {
          // he is not sliding, he wants to move but can't
          result = wanted_direction8;
        }
      }
    }
  }

  return result;
}

/**
 * \brief Returns whether the hero is moving towards the specified direction
 * among the four main directions.
 *
 * If the hero is not moving, false is returned.
 * If he is making a diagonal move, this function considers that he is moving
 * towards both directions.
 *
 * \param direction4 one of the four main directions (0 to 3)
 * \return true if the hero is moving in that direction, even if he is actually doing a diagonal move
 */
bool Hero::is_moving_towards(int direction4) const {

  const std::shared_ptr<const Movement>& movement = get_movement();
  if (movement == nullptr || movement->is_stopped()) {
    return false;
  }

  int direction8 = direction4 * 2;
  int movement_direction8 = get_wanted_movement_direction8();

  return movement_direction8 != -1
      && (movement_direction8 == direction8
          || (movement_direction8 + 1) % 8 == direction8
          || (movement_direction8 + 7) % 8 == direction8);
}

/**
 * \brief Returns whether the animation direction is locked.
 *
 * When this function returns false, which is the case most of the time,
 * it means that the animation direction is set to the movement direction.
 * When it returns true, it means that the animation direction is fixed
 * and do not depends on the movement direction anymore (this is the case
 * when the hero is loading his sword).
 *
 * \return true if the animation direction is locked
 */
bool Hero::is_direction_locked() const {
  return get_state()->is_direction_locked();
}

/**
 * \brief This function is called when the hero's position is changed.
 */
void Hero::notify_position_changed() {

  if (is_on_map()) {
    get_entities().notify_entity_bounding_box_changed(*this);
  }

  check_position();
  get_state()->notify_position_changed();

  if (are_movement_notifications_enabled()) {
    get_lua_context()->entity_on_position_changed(*this, get_xy(), get_layer());
  }
}

/**
 * \brief This function is called when the layer of this entity has just changed.
 */
void Hero::notify_layer_changed() {
  get_state()->notify_layer_changed();
}

/**
 * \brief Notifies this entity that it has just failed to change its position
 * because of obstacles.
 */
void Hero::notify_obstacle_reached() {

  Entity::notify_obstacle_reached();

  get_state()->notify_obstacle_reached();

  if (get_ground_below() == Ground::ICE) {
    ground_dxy = { 0, 0 };
    ice_movement_direction8 = -1;
  }
}

/**
 * \brief This function is called when the movement of the entity starts.
 */
void Hero::notify_movement_started() {

  Entity::notify_movement_started();
  get_state()->notify_movement_started();
}

/**
 * \brief This function is called when the movement of the entity is finished.
 */
void Hero::notify_movement_finished() {

  Entity::notify_movement_finished();
  get_state()->notify_movement_finished();
}

/**
 * \brief Updates the hero depending on its movement.
 *
 * This function is called when the hero's movement direction changes (for instance
 * because the player pressed or released a directional key, or the hero just reached an obstacle).
 * It updates the hero's animations and collisions according to the new movement.
 */
void Hero::notify_movement_changed() {

  update_direction();

  // let the state pick the animation corresponding to the movement tried by the player
  get_state()->notify_movement_changed();
  check_position();

  if (get_ground_below() == Ground::ICE) {
    update_ice();
  }
  Entity::notify_movement_changed();
}

/**
 * \brief Checks collisions with detectors, determines the facing entity
 * and the ground below the hero in its current position.
 *
 * This function is called when these checks have to be done again,
 * e.g. when the position, the direction or the state of the hero changes.
 */
void Hero::check_position() {

  if (!is_on_map()) {
    return;
  }

  if (get_state()->are_collisions_ignored()) {
    // Do not take care of the ground or detectors.
    return;
  }

  // Recompute the facing entity.
  set_facing_entity(nullptr);
  check_collision_with_detectors();

  if (is_suspended()
      && get_map().test_collision_with_border(get_ground_point())) {
    // When suspended, the hero may have invalid coordinates
    // (e.g. transition between maps).
    return;
  }

  // Determine the new ground if it has changed.
  update_ground_below();

  // Save the hero's last valid position.
  Ground ground = get_ground_below();
  if (ground != Ground::DEEP_WATER
      && ground != Ground::HOLE
      && ground != Ground::LAVA
      && ground != Ground::PRICKLE
      && ground != Ground::EMPTY
      && get_state()->get_can_come_from_bad_ground()
      && (get_xy() != last_solid_ground_coords)) {

    last_solid_ground_coords = get_xy();
    last_solid_ground_layer = get_layer();
  }

  // With empty ground, possibly go to the lower layer.
  if (ground == Ground::EMPTY && get_state()->is_touching_ground()) {

    int x = get_top_left_x();
    int y = get_top_left_y();
    int layer = get_layer();

    if (layer > get_map().get_min_layer()
        && get_map().get_ground(layer, x, y, this) == Ground::EMPTY
        && get_map().get_ground(layer, x + 15, y, this) == Ground::EMPTY
        && get_map().get_ground(layer, x, y + 15, this) == Ground::EMPTY
        && get_map().get_ground(layer, x + 15, y + 15, this) == Ground::EMPTY) {

      get_entities().set_entity_layer(*this, layer - 1);
      Ground new_ground = get_map().get_ground(get_layer(), x, y, this);
      if (get_state()->is_free() &&
          (new_ground == Ground::TRAVERSABLE
           || new_ground == Ground::GRASS
           || new_ground == Ground::LADDER)) {
        Sound::play("hero_lands");
      }
    }
  }
}

/**
 * \brief Stops the movement of the player and lets the player restart it when he can.
 *
 * This function is typically called when the player temporarily loses the control
 * (e.g. because of a script or a map change) whereas the movement remains the same.
 * Then the movement may want to move a few pixels more as soon as it is resumed.
 * This function removes such residual effects of the player's movement.
 * If the current movement is not controlled by the player, this function has no effect.
 */
void Hero::reset_movement() {

  if (can_control_movement()) {
    get_movement()->stop();
  }
}

/**
 * \brief Starts activating the new ground of the hero.
 */
void Hero::notify_ground_below_changed() {

  const bool suspended = get_game().is_suspended();

  Entity::notify_ground_below_changed();

  switch (get_ground_below()) {

  case Ground::TRAVERSABLE:
    // Traversable ground: remove any special sprite displayed under the hero.
    sprites->destroy_ground();
    set_walking_speed(normal_walking_speed);
    break;

  case Ground::DEEP_WATER:
    // Deep water: plunge if the hero is not jumping.
    if (!get_state()->can_avoid_deep_water()) {

      if (suspended) {
        // During a transition, it is okay to start swimming
        // but we don't want to start plunging right now.
        if (get_state()->is_touching_ground()) {
          start_deep_water();
        }
      }
      else {
        start_deep_water();
      }
    }
    break;

  case Ground::HOLE:
    // Hole: fall into the hole or get attracted to it.
    // But wait for the teletransporter opening transition to finish if any.
    if (!suspended
        && !get_state()->can_avoid_hole()) {
      start_hole();
    }
    break;

  case Ground::ICE:
    // Ice: make the hero slide.
    if (!get_state()->can_avoid_ice()) {
      start_ice();
    }
    break;

  case Ground::LAVA:
    // Lava: plunge into lava.
    if (!suspended
        && !get_state()->can_avoid_lava()) {
      start_lava();
    }
    break;

  case Ground::PRICKLE:
    // Prickles.
    if (!suspended
        && !get_state()->can_avoid_prickle()) {
      start_prickle(500);
    }
    break;

  case Ground::SHALLOW_WATER:
    if (get_state()->is_affected_by_shallow_water()) {
      start_shallow_water();
    }
    break;

  case Ground::GRASS:
    if (get_state()->is_affected_by_grass()) {
      start_grass();
    }
    break;

  case Ground::LADDER:
    if (!suspended &&
        get_state()->is_affected_by_ladder()) {
      set_walking_speed(normal_walking_speed * 3 / 5);
    }
    break;

  case Ground::WALL:
  case Ground::LOW_WALL:
  case Ground::WALL_TOP_RIGHT:
  case Ground::WALL_TOP_LEFT:
  case Ground::WALL_BOTTOM_LEFT:
  case Ground::WALL_BOTTOM_RIGHT:
  case Ground::WALL_TOP_RIGHT_WATER:
  case Ground::WALL_TOP_LEFT_WATER:
  case Ground::WALL_BOTTOM_LEFT_WATER:
  case Ground::WALL_BOTTOM_RIGHT_WATER:
    // The hero is stuck in a wall. Damn.
    // This is the fault of the quest maker, unless there is a bug in Solarus.
    // The user will have to save and quit his game.
    // TODO maybe we could use the back to solid ground mechanism here?
    break;

  case Ground::EMPTY:
    break;
  }

  // Notify the state.
  get_state()->notify_ground_below_changed();
}

/**
 * \brief Returns whether the hero is in a state such that
 * a ground can be displayed under him.
 */
bool Hero::is_ground_visible() const {

  Ground ground = get_ground_below();
  return (ground == Ground::GRASS || ground == Ground::SHALLOW_WATER)
    && get_state()->is_touching_ground();
}

/**
 * \brief Returns whether this entity is sensible to the ground below it.
 * \return \c true if this entity is sensible to its ground.
 */
bool Hero::is_ground_observer() const {
  return true;
}

/**
 * \brief Returns the last point of solid ground where the hero was.
 * \return The last solid ground coordinates, or
 * <tt>-1,-1</tt> if the hero never was on solid ground on this map yet.
 */
const Point& Hero::get_last_solid_ground_coords() const {
  return last_solid_ground_coords;
}

/**
 * \brief Returns the layer of the last of solid ground where the hero was.
 * \return The last solid ground layer, or
 * 0 if the hero never was on solid ground on this map yet.
 */
int Hero::get_last_solid_ground_layer() const {
  return last_solid_ground_layer;
}

/**
 * \brief Returns the function indicated by the last call to
 * set_target_solid_callback().
 * \return The Lua function that gives solid ground coordinates, or
 * an empty ref if set_target_solid_callback() was not called.
 */
const ScopedLuaRef& Hero::get_target_solid_ground_callback() const {
  return target_solid_ground_callback;
}

/**
 * \brief Creates a Lua function that returns the given coordinates and layer.
 *
 * This function can be used as callback for specifying the solid ground position.
 *
 * \param xy Coordinates the function should return.
 * \param layer Layer the function should return.
 * \return Lua ref to a function that returns this solid position.
 */
ScopedLuaRef Hero::make_solid_ground_callback(
    const Point& xy, int layer) const {

  LuaContext* lua_context = get_lua_context();
  if (lua_context == nullptr) {
    return ScopedLuaRef();
  }
  lua_State* l = lua_context->get_internal_state();

  lua_pushinteger(l, xy.x);
  lua_pushinteger(l, xy.y);
  lua_pushinteger(l, layer);
  lua_pushcclosure(l, l_solid_ground_callback, 3);

  ScopedLuaRef callback_ref = LuaTools::create_ref(l, -1);
  lua_pop(l, 1);
  return callback_ref;
}

/**
 * \brief Specifies a function indicating where the hero will go back
 * if he falls into a hole or some other bad ground.
 * \param target_solid_ground_callback A Lua function that gives the
 * coordinates and layer of where the hero should go if he falls into
 * a hole or some other bad ground.
 * An empty ref means returning automatically to the last solid ground
 * position.
 */
void Hero::set_target_solid_ground_callback(
    const ScopedLuaRef& target_solid_ground_callback) {

  this->target_solid_ground_callback = target_solid_ground_callback;
}

/**
 * \brief Forgets the function indicating where the hero was supposed to go
 * back if he falls into a hole or some other bad ground.
 *
 * The hero will now get back to the last solid ground instead of going back
 * to a custom position.
 */
void Hero::reset_target_solid_ground_callback() {

  set_target_solid_ground_callback(ScopedLuaRef());
}

/**
 * \brief Returns whether this entity is an obstacle for another one.
 * \param other another entity
 * \return true if this entity is an obstacle for the other one
 */
bool Hero::is_obstacle_for(Entity& other) {
  return other.is_hero_obstacle(*this);
}

/**
 * \copydoc Entity::is_traversable_obstacle
 */
bool Hero::is_traversable_obstacle() const {
  return get_state()->is_traversable_obstacle();
}

/**
 * \copydoc Entity::is_wall_obstacle
 */
bool Hero::is_wall_obstacle() const {
  return get_state()->is_wall_obstacle();
}

/**
 * \copydoc Entity::is_low_wall_obstacle
 */
bool Hero::is_low_wall_obstacle() const {
  return get_state()->is_low_wall_obstacle();
}

/**
 * \copydoc Entity::is_grass_obstacle
 */
bool Hero::is_grass_obstacle() const {
  return get_state()->is_grass_obstacle();
}

/**
 * \copydoc Entity::is_shallow_water_obstacle
 */
bool Hero::is_shallow_water_obstacle() const {
  return get_state()->is_shallow_water_obstacle();
}

/**
 * \copydoc Entity::is_deep_water_obstacle
 */
bool Hero::is_deep_water_obstacle() const {
  return get_state()->is_deep_water_obstacle();
}

/**
 * \copydoc Entity::is_hole_obstacle
 */
bool Hero::is_hole_obstacle() const {
  return get_state()->is_hole_obstacle();
}

/**
 * \copydoc Entity::is_ice_obstacle
 */
bool Hero::is_ice_obstacle() const {
  return get_state()->is_ice_obstacle();
}

/**
 * \copydoc Entity::is_lava_obstacle
 */
bool Hero::is_lava_obstacle() const {
  return get_state()->is_lava_obstacle();
}

/**
 * \copydoc Entity::is_prickle_obstacle
 */
bool Hero::is_prickle_obstacle() const {
  return get_state()->is_prickle_obstacle();
}

/**
 * \copydoc Entity::is_ladder_obstacle
 */
bool Hero::is_ladder_obstacle() const {
  return get_state()->is_ladder_obstacle();
}

/**
 * \copydoc Entity::is_hero_obstacle
 */
bool Hero::is_hero_obstacle(Hero& hero) {
  return get_state()->is_hero_obstacle(hero);
}

/**
 * \copydoc Entity::is_block_obstacle
 */
bool Hero::is_block_obstacle(Block& block) {
  return get_state()->is_block_obstacle(block);
}

/**
 * \copydoc Entity::is_teletransporter_obstacle
 */
bool Hero::is_teletransporter_obstacle(Teletransporter& teletransporter) {
  return get_state()->is_teletransporter_obstacle(teletransporter);
}

/**
 * \copydoc Entity::is_stream_obstacle
 */
bool Hero::is_stream_obstacle(Stream& stream) {
  return get_state()->is_stream_obstacle(stream);
}

/**
 * \copydoc Entity::is_stairs_obstacle
 */
bool Hero::is_stairs_obstacle(Stairs& stairs) {
  return get_state()->is_stairs_obstacle(stairs);
}

/**
 * \copydoc Entity::is_sensor_obstacle
 */
bool Hero::is_sensor_obstacle(Sensor& sensor) {
  return get_state()->is_sensor_obstacle(sensor);
}

/**
 * \copydoc Entity::is_switch_obstacle
 */
bool Hero::is_switch_obstacle(Switch& sw) {
  return get_state()->is_switch_obstacle(sw);
}

/**
 * \copydoc Entity::is_raised_block_obstacle
 */
bool Hero::is_raised_block_obstacle(CrystalBlock& raised_block) {
  return get_state()->is_raised_block_obstacle(raised_block);
}

/**
 * \copydoc Entity::is_crystal_obstacle
 */
bool Hero::is_crystal_obstacle(Crystal& crystal) {
  return get_state()->is_crystal_obstacle(crystal);
}

/**
 * \copydoc Entity::is_npc_obstacle
 */
bool Hero::is_npc_obstacle(Npc& npc) {
  return get_state()->is_npc_obstacle(npc);
}

/**
 * \copydoc Entity::is_door_obstacle
 */
bool Hero::is_door_obstacle(Door& door) {
  return get_state()->is_door_obstacle(door);
}

/**
 * \copydoc Entity::is_enemy_obstacle
 */
bool Hero::is_enemy_obstacle(Enemy& enemy) {
  return get_state()->is_enemy_obstacle(enemy);
}

/**
 * \copydoc Entity::is_jumper_obstacle
 */
bool Hero::is_jumper_obstacle(Jumper& jumper, const Rectangle& candidate_position) {
  return get_state()->is_jumper_obstacle(jumper, candidate_position);
}

/**
 * \copydoc Entity::is_destructible_obstacle
 */
bool Hero::is_destructible_obstacle(Destructible& destructible) {
  return get_state()->is_destructible_obstacle(destructible);
}

/**
 * \copydoc Entity::is_separator_obstacle
 */
bool Hero::is_separator_obstacle(Separator& separator, const Rectangle& /* candidate_position */) {
  return get_state()->is_separator_obstacle(separator);
}

/**
 * \copydoc Entity::notify_collision_with_destructible
 */
void Hero::notify_collision_with_destructible(
    Destructible& destructible, CollisionMode collision_mode) {
  destructible.notify_collision_with_hero(*this, collision_mode);
}

/**
 * \copydoc Enemy::notify_collision_with_enemy(Enemy&, CollisionMode)
 */
void Hero::notify_collision_with_enemy(Enemy& enemy, CollisionMode collision_mode) {

  if (enemy.get_attacking_collision_mode() != collision_mode) {
    // Not the collision mode used to attack the hero.
    return;
  }
  enemy.attack_hero(*this, nullptr);
}

/**
 * \copydoc Enemy::notify_collision_with_enemy(Enemy&, Sprite&, Sprite&))
 */
void Hero::notify_collision_with_enemy(
    Enemy& enemy, Sprite& this_sprite, Sprite& enemy_sprite) {

  const std::string& this_sprite_id = this_sprite.get_animation_set_id();
  if (this_sprite_id == get_hero_sprites().get_sword_sprite_id()) {
    // the hero's sword overlaps the enemy
    enemy.try_hurt(EnemyAttack::SWORD, *this, &enemy_sprite);
  }
  else if (this_sprite_id == get_hero_sprites().get_tunic_sprite_id()) {

    if (enemy.get_attacking_collision_mode() != CollisionMode::COLLISION_SPRITE) {
      // The enemy does not attack with sprite collisions.
      return;
    }

    // The hero's body sprite overlaps the enemy.
    // Check that the 16x16 rectangle of the hero also overlaps the enemy.
    const Size& enemy_sprite_size = enemy_sprite.get_size();
    const Point& enemy_sprite_origin = enemy_sprite.get_origin();
    const Point& enemy_sprite_offset = enemy_sprite.get_xy();
    Rectangle enemy_sprite_rectangle(
        enemy.get_x() - enemy_sprite_origin.x + enemy_sprite_offset.x,
        enemy.get_y() - enemy_sprite_origin.y + enemy_sprite_offset.y,
        enemy_sprite_size.width,
        enemy_sprite_size.height
    );

    if (overlaps(enemy_sprite_rectangle)) {
      enemy.attack_hero(*this, &enemy_sprite);
    }
  }
}

/**
 * \brief This function is called when a teletransporter detects a collision with the hero.
 * \param teletransporter the teletransporter
 * \param collision_mode the collision mode that detected the event
 */
void Hero::notify_collision_with_teletransporter(
    Teletransporter& teletransporter, CollisionMode /* collision_mode */) {

  if (!can_avoid_teletransporter(teletransporter)) {

    update_ground_below();  // Make sure the ground is up-to-date.
    bool on_hole = get_ground_below() == Ground::HOLE;
    if (on_hole || get_state()->is_teletransporter_delayed()) {
       // Fall into the hole (or do something else) first, transport later
      this->delayed_teletransporter = std::static_pointer_cast<Teletransporter>(
            teletransporter.shared_from_this()
      );
    }
    else {
      teletransporter.transport_hero(*this); // usual case: transport right now
    }
  }
}

/**
 * \brief Returns a teletransporter that has detected a collision with the hero
 * bu will be activated when the current action is finished
 * (e.g. falling into a hole or taking stairs).
 * \return The delayed teletransporter or nullptr.
 */
std::shared_ptr<Teletransporter> Hero::get_delayed_teletransporter() {
  return delayed_teletransporter;
}

/**
 * \copydoc Entity::notify_collision_with_stream
 */
void Hero::notify_collision_with_stream(
    Stream& stream, int dx, int dy) {

  if (has_stream_action()) {
    // There is already an active stream.
    return;
  }

  if (get_state()->can_avoid_stream(stream)) {
    // Streams are ignored in the current state of the hero.
    return;
  }

  // Check that the feet of the hero are on the stream.
  if (!stream.overlaps(get_ground_point())) {
    return;
  }

  bool activate_stream = false;

  if (stream.get_allow_movement()) {
    // The stream can be forced: there is no risk.
    // Even if the stream is faster than the player's movement,
    // he can still move perpendicularly.
    activate_stream = true;
  }
  else if (dx != 0 && dy != 0) {
    // Case of a diagonal stream.
    // TODO Diagonal streams are always activated for now, this could be improved.
    activate_stream = true;
  }
  else {
    // This is a blocking stream.
    // Check that the hero can go in the stream's direction.
    // Otherwise the hero, would be trapped forever if there
    // is an obstacle after the stream.
    Map& map = get_map();
    Rectangle collision_box(0, 0, 16, 16);

    // First check that the exit of the stream is clear.
    collision_box.set_xy(stream.get_bounding_box().get_xy());
    collision_box.add_xy(dx, dy);
    if (!map.test_collision_with_obstacles(get_layer(), collision_box, *this)) {
      // The stream's exit is clear.
      // Check that we can make at least one pixel with the stream.
      // Because maybe we have already passed the exit and cannot do more.
      const bool horizontal = dx != 0;
      if (horizontal) {
        collision_box.set_xy(
            get_top_left_x() + dx,
            get_top_left_y()
        );
      }
      else {
        collision_box.set_xy(
            get_top_left_x(),
            get_top_left_y() + dy
        );
      }

      if (!get_map().test_collision_with_obstacles(get_layer(), collision_box, *this)) {
        // We can make at least one pixel with the stream.
        activate_stream = true;
      }
      else {
        // We would be blocked if we took the stream straight.
        // Maybe we won't get blocked when getting aligned with the stream.
        // Let's check that.

        if (horizontal) {
          // Horizontal stream: try to align the Y coordinate.
          collision_box.set_xy(
              get_top_left_x() + dx,
              stream.get_top_left_y()
          );
        }
        else {
          // Vertical stream: try to align the X coordinate.
          collision_box.set_xy(
              stream.get_top_left_x(),
              get_top_left_y() + dy
          );
        }

        if (!map.test_collision_with_obstacles(get_layer(), collision_box, *this)) {
          // We can move into the direction when correctly aligned.
          activate_stream = true;
        }
      }
    }
  }

  if (activate_stream) {
    stream.activate(*this);
    if (!get_state()->can_persist_on_stream(stream)) {
      start_free();
    }
  }
}

/**
 * \brief This function is called when a stairs entity detects a collision
 * with this entity.
 * \param stairs The stairs.
 * \param collision_mode The collision mode that detected the event.
 */
void Hero::notify_collision_with_stairs(
    Stairs& stairs, CollisionMode collision_mode) {

  if (get_state()->get_can_take_stairs()) {

    Stairs::Way stairs_way;
    if (stairs.is_inside_floor()) {
      stairs_way = (get_layer() == stairs.get_layer()) ?
          Stairs::NORMAL_WAY : Stairs::REVERSE_WAY;
    }
    else {
      stairs_way = (collision_mode == COLLISION_TOUCHING) ?
          Stairs::NORMAL_WAY : Stairs::REVERSE_WAY;
    }

    // Check that the hero is exactly aligned with the stairs.
    if (stairs.get_direction() % 2 == 0) {
      // Horizontal stairs.
      if (get_top_left_y() != stairs.get_top_left_y()) {
        return;
      }
    }
    else {
      // Vertical stairs.
      if (get_top_left_x() != stairs.get_top_left_x()) {
        return;
      }
    }

    // Check whether the hero is trying to move in the direction of the stairs.
    int correct_direction = stairs.get_movement_direction(stairs_way);
    if (is_moving_towards(correct_direction / 2)) {
      std::shared_ptr<const Stairs> shared_stairs =
          std::static_pointer_cast<const Stairs>(stairs.shared_from_this());
      set_state(std::make_shared<StairsState>(*this, shared_stairs, stairs_way));
    }
  }
}

/**
 * \copydoc Entity::notify_collision_with_jumper
 */
void Hero::notify_collision_with_jumper(Jumper& jumper,
    CollisionMode collision_mode) {

  if (collision_mode == COLLISION_CUSTOM) {
    get_state()->notify_jumper_activated(jumper);
  }
}

/**
 * \brief This function is called when a sensor detects a collision with this entity.
 * \param sensor a sensor
 * \param collision_mode the collision mode that detected the collision
 */
void Hero::notify_collision_with_sensor(Sensor& sensor, CollisionMode collision_mode) {

  if (collision_mode == COLLISION_CONTAINING    // the hero is entirely inside the sensor
      && !get_state()->can_avoid_sensor()) {
    sensor.activate(*this);
  }
}

/**
 * \brief This function is called when a switch detects a collision with this entity.
 * \param sw the switch
 * \param collision_mode the collision mode that detected the event
 */
void Hero::notify_collision_with_switch(Switch& sw, CollisionMode /* collision_mode */) {

  // it's normally a walkable switch
  if (sw.is_walkable()
      && !get_state()->can_avoid_switch()) {
    sw.try_activate(*this);
  }
}

/**
 * \brief This function is called when a the sprite of a switch
 * detects a pixel-precise collision with a sprite of this entity.
 * \param sw the switch
 * \param sprite_overlapping the sprite of the current entity that collides with the crystal
 */
void Hero::notify_collision_with_switch(Switch& sw, Sprite& sprite_overlapping) {

  // it's normally a solid switch
  const std::string& sprite_id = sprite_overlapping.get_animation_set_id();
  if (sprite_id == get_hero_sprites().get_sword_sprite_id() && // the hero's sword is overlapping the switch
      sw.is_solid() &&
      get_state()->can_sword_hit_crystal()) {
    // note that solid switches and crystals have the same rules for the sword

    sw.try_activate();
  }
}

/**
 * \brief This function is called when a crystal detects a collision with this entity.
 * \param crystal the crystal
 * \param collision_mode the collision mode that detected the event
 */
void Hero::notify_collision_with_crystal(Crystal& /* crystal */, CollisionMode collision_mode) {

  if (collision_mode == COLLISION_FACING) {
    // The hero is touching the crystal and is looking in its direction.

    if (get_commands_effects().get_action_key_effect() == CommandsEffects::ACTION_KEY_NONE
        && is_free()) {

      // We show the action icon.
      get_commands_effects().set_action_key_effect(CommandsEffects::ACTION_KEY_LOOK);
    }
  }
}

/**
 * \brief This function is called when a the sprite of a crystal
 * detects a pixel-precise collision with a sprite of this entity.
 * \param crystal the crystal
 * \param sprite_overlapping the sprite of the current entity that collides with the crystal
 */
void Hero::notify_collision_with_crystal(Crystal& crystal, Sprite& sprite_overlapping) {

  const std::string sprite_id = sprite_overlapping.get_animation_set_id();
  if (sprite_id == get_hero_sprites().get_sword_sprite_id() && // the hero's sword is overlapping the crystal
      get_state()->can_sword_hit_crystal()) {

    crystal.activate(*this);
  }
}

/**
 * \brief This function is called when a chest detects a collision with this entity.
 * \param chest the chest
 */
void Hero::notify_collision_with_chest(Chest& chest) {

  if (get_commands_effects().get_action_key_effect() == CommandsEffects::ACTION_KEY_NONE
      && is_free()
      && is_facing_direction4(1)
      && !chest.is_open()) {

    // We show the 'open' icon even if the chest cannot be opened yet.
    get_commands_effects().set_action_key_effect(CommandsEffects::ACTION_KEY_OPEN);
  }
}

/**
 * \brief This function is called when a block detects a collision with this entity.
 * \param block the block
 */
void Hero::notify_collision_with_block(Block& /* block */) {

  if (get_commands_effects().get_action_key_effect() == CommandsEffects::ACTION_KEY_NONE &&
      is_free() &&
      can_grab()
  ) {
    // We allow to grab using the action command.
    get_commands_effects().set_action_key_effect(CommandsEffects::ACTION_KEY_GRAB);
  }
}

/**
 * \copydoc Entity::notify_collision_with_separator
 */
void Hero::notify_collision_with_separator(
    Separator& separator, CollisionMode /* collision_mode */) {

  const CameraPtr& camera = get_map().get_camera();
  if (camera == nullptr) {
    return;
  }
  if (camera->get_tracked_entity().get() == this) {
    camera->notify_tracked_entity_traversing_separator(separator);
  }
}

/**
 * \brief This function is called when an explosion's sprite detects a collision with a sprite of the hero.
 * \param explosion the explosion
 * \param sprite_overlapping the sprite of the hero that collides with the explosion
 */
void Hero::notify_collision_with_explosion(
    Explosion& explosion, Sprite& sprite_overlapping) {

  const std::string& sprite_id = sprite_overlapping.get_animation_set_id();
  if (!get_state()->can_avoid_explosion() &&
      sprite_id == get_hero_sprites().get_tunic_sprite_id() &&
      can_be_hurt(&explosion)) {
    hurt(explosion, nullptr, 2);
  }
}

/**
 * \brief Makes the hero escape from an entity that is overlapping him.
 *
 * This function is called when an entity that just appeared may overlap the hero
 * (e.g. a chest or a door).
 *
 * \param entity the entity that just appeared
 * \param direction the direction of the hero relative to the entity
 * (the hero will be moved into this direction): 0 to 3
 */
void Hero::avoid_collision(Entity& entity, int direction) {

  // fix the hero's position, whatever the entity's) size is
  switch (direction) {

    case 0:
      set_top_left_x(entity.get_top_left_x() + entity.get_width());
      set_top_left_y(entity.get_center_point().y - 8);
      break;

    case 1:
      set_top_left_y(entity.get_top_left_y() - this->get_height());
      set_top_left_x(entity.get_center_point().x - 8);
      break;

    case 2:
      set_top_left_x(entity.get_top_left_x() - this->get_width());
      set_top_left_y(entity.get_center_point().y - 8);
      break;

    case 3:
      set_top_left_y(entity.get_top_left_y() + entity.get_height());
      set_top_left_x(entity.get_center_point().x - 8);
      break;

    default:
      Debug::die("Invalid direction in Hero::avoid_collision()");
      break;
  }
  reset_movement();
}

/**
 * \brief Notifies the hero that the entity he is pushing or pulling
 * cannot move anymore because of a collision.
 */
void Hero::notify_grabbed_entity_collision() {
  get_state()->notify_grabbed_entity_collision();
}

/**
 * \brief Tests whether the hero is cutting with his sword the specified entity
 * for which a collision was detected.
 *
 * When the sword sprite collides with an entity,
 * this function can be called to determine whether the hero is
 * really cutting this particular entity precisely.
 * This depends on the hero's state, his direction and his
 * distance to the detector.
 * This function assumes that there is already a collision
 * between the sword sprite and the detector's sprite.
 * This function should be called to check whether the
 * hero wants to cut a bush or some grass.
 * Returns false by default.
 *
 * \param entity The entity to check.
 * \return \c true if the sword is cutting this entity.
 */
bool Hero::is_striking_with_sword(Entity& entity) const {
  return get_state()->is_cutting_with_sword(entity);
}

/**
 * \brief Snaps the hero to the entity he is facing.
 *
 * The hero is snapped if there is no collision and if he is not too far.
 */
void Hero::try_snap_to_facing_entity() {

  Rectangle collision_box = get_bounding_box();
  const Point& center = collision_box.get_center();
  const Entity* facing_entity = get_facing_entity();
  const Point& facing_entity_center = facing_entity->get_center_point();

  if (get_animation_direction() % 2 == 0) {
    if (abs(center.y - facing_entity_center.y) <= 5) {
      collision_box.set_center(center.x, facing_entity_center.y);
    }
  }
  else {
    if (abs(center.x - facing_entity_center.x) <= 5) {
      collision_box.set_center(facing_entity_center.x, center.y);
    }
  }

  if (!get_map().test_collision_with_obstacles(get_layer(), collision_box, *this)) {
    set_bounding_box(collision_box);
    notify_position_changed();
  }
}

/**
 * \copydoc Entity::notify_attacked_enemy
 */
void Hero::notify_attacked_enemy(
    EnemyAttack attack,
    Enemy& victim,
    Sprite* victim_sprite,
    const EnemyReaction::Reaction& result,
    bool killed) {

  get_state()->notify_attacked_enemy(attack, victim, victim_sprite, result, killed);
}

/**
 * \brief Returns the damage power of the sword for the current attack.
 *
 * The value returned takes into account the power of the current sword
 * and the fact that a spin attack is more powerful than other attacks.
 *
 * \return the current damage factor of the sword
 */
int Hero::get_sword_damage_factor() const {
  return get_state()->get_sword_damage_factor();
}

/**
 * \brief Returns whether the hero is currently invincible.
 * \return \c true if the hero is currently invincible.
 */
bool Hero::is_invincible() const {
  return invincible;
}

/**
 * \brief Sets the hero temporarily invincible or stops the invincibility.
 * \param invincible \c true to make the hero invincible, \c false to stop.
 * \param duration How long to stay invincible in milliseconds.
 * 0 means infinite. No effect if \c invincible is \c false.
 */
void Hero::set_invincible(bool invincible, uint32_t duration) {

  this->invincible = invincible;
  this->end_invincible_date = 0;
  if (invincible) {
    this->end_invincible_date = (duration == 0) ? 0 : System::now() + duration;
  }
}

/**
 * \brief Checks whether the invincibility should end.
 */
void Hero::update_invincibility() {

  if (is_invincible() &&
      end_invincible_date != 0 &&
      System::now() >= end_invincible_date) {
    set_invincible(false, 0);
  }
}

/**
 * \brief Returns whether the hero can be hurt currently.
 * \param attacker An attacker that is trying to hurt the hero
 * (or nullptr if the source of the attack is not an entity).
 * \return \c true if the hero can be hurt.
 */
bool Hero::can_be_hurt(Entity* attacker) const {
  return !is_invincible() && get_state()->get_can_be_hurt(attacker);
}

/**
 * \brief Hurts the hero, an entity being the source of the attack.
 * \param source An entity that hurts the hero (usually an enemy).
 * \param source_sprite Sprite of the source entity that is hurting the hero
 * or nullptr.
 * \param damage Number of life points to remove
 * (this number may be reduced later by the tunic on by hero:on_taking_damage()).
 */
void Hero::hurt(
    Entity& source,
    Sprite* source_sprite,
    int damage
) {
  Point source_xy = source.get_xy();
  if (source_sprite != nullptr) {
    // Add the offset of the sprite if any.
    source_xy += source_sprite->get_xy();
  }
  set_state(std::make_shared<HurtState>(*this, &source_xy, damage));
}

/**
 * \brief Hurts the hero, a point being the source of the attack.
 * \param source_xy Coordinates of whatever hurts the hero.
 * \param damage Number of life points to remove
 * (this number may be reduced later by the tunic on by hero:on_taking_damage()).
 */
void Hero::hurt(const Point& source_xy, int damage) {

  set_state(std::make_shared<HurtState>(*this, &source_xy, damage));
}

/**
 * \brief Hurts the hero, with no located source of the attack.
 * \param damage Number of life points to remove
 * (this number may be reduced later by the tunic on by hero:on_taking_damage()).
 */
void Hero::hurt(int damage) {

  set_state(std::make_shared<HurtState>(*this, nullptr, damage));
}

/**
 * \brief Draws a grass sprite below the hero and makes him walk slower.
 */
void Hero::start_grass() {

  // Display a special sprite below the hero.
  sprites->create_ground(Ground::GRASS);

  uint32_t now = System::now();
  next_ground_date = std::max(next_ground_date, now);

  set_walking_speed(normal_walking_speed * 4 / 5);
}

/**
 * \brief Draws a shallow water sprite below the hero and makes him walk
 * slower.
 */
void Hero::start_shallow_water() {

  // Display a special sprite below the hero.
  sprites->create_ground(Ground::SHALLOW_WATER);

  uint32_t now = System::now();
  next_ground_date = std::max(next_ground_date, now);

  set_walking_speed(normal_walking_speed * 4 / 5);
}

/**
 * \brief This function is called when the hero was dying but the game-over
 * sequence was canceled.
 */
void Hero::notify_game_over_finished() {

  if (is_on_map()) {
    sprites->blink(2000);
    start_state_from_ground();
  }
}

/**
 * \brief Makes the hero drown or swim.
 */
void Hero::start_deep_water() {

  const bool can_swim = get_equipment().has_ability(Ability::SWIM);
  const bool can_jump_over_water = get_equipment().has_ability(Ability::JUMP_OVER_WATER);

  if (!get_state()->is_touching_ground()) {
    // Entering water from above the ground
    // (e.g. after a jump).
    set_state(std::make_shared<PlungingState>(*this));
  }
  else {
    // Entering water normally (e.g. by walking).
    if (can_swim) {
      set_state(std::make_shared<SwimmingState>(*this));
    }
    else if (can_jump_over_water) {
      int direction8 = get_wanted_movement_direction8();
      if (direction8 == -1) {
        direction8 = get_animation_direction() * 2;
      }
      start_jumping(direction8, 32, false, true);
    }
    else {
      set_state(std::make_shared<PlungingState>(*this));
    }
  }
}

/**
 * \brief Makes the hero move towards a hole of fall into it.
 */
void Hero::start_hole() {

  if (!can_control_movement()) {
    // the player has no control (e.g. he is running or being hurt):
    // fall immediately
    set_state(std::make_shared<FallingState>(*this));
  }
  else {
    // otherwise, push the hero towards the hole

    next_ground_date = System::now();

    // Don't calculate the attraction direction based on the wanted movement
    // because the wanted movement may be different from the real one

    if (last_solid_ground_coords.x == -1 ||
        (last_solid_ground_coords == get_xy())) {
      // fall immediately because the hero was not moving but directly placed on the hole
      set_state(std::make_shared<FallingState>(*this));
    }
    else {

      ground_dxy = { 0, 0 };

      if (get_x() > last_solid_ground_coords.x) {
        ground_dxy.x = 1;
      }
      else if (get_x() < last_solid_ground_coords.x) {
        ground_dxy.x = -1;
      }

      if (get_y() > last_solid_ground_coords.y) {
        ground_dxy.y = 1;
      }
      else if (get_y() < last_solid_ground_coords.y) {
        ground_dxy.y = -1;
      }
      set_walking_speed(normal_walking_speed / 3);
    }
  }
}

/**
 * \brief Makes the hero slide on ice ground.
 */
void Hero::start_ice() {

  next_ground_date = System::now();
  next_ice_date = System::now();

  ice_movement_direction8 = get_wanted_movement_direction8();
  if (ice_movement_direction8 == -1) {
    ground_dxy = { 0, 0 };
  }
  else {
    // Exagerate the movement.
    ground_dxy = direction_to_xy_move(ice_movement_direction8);
  }
}

/**
 * \brief Makes the hero drown into lava.
 */
void Hero::start_lava() {

  // plunge into the lava
  set_state(std::make_shared<PlungingState>(*this));
}

/**
 * \brief Makes the hero being hurt by prickles.
 * \param delay delay before returning control to the player
 */
void Hero::start_prickle(uint32_t delay) {

  Sound::play("hero_hurt");
  get_equipment().remove_life(2);
  start_back_to_solid_ground(true, delay, false);
}

/**
 * \brief Returns whether the hero can walk normally and interact with entities.
 * \return true if the hero can walk normally
 */
bool Hero::is_free() const {

  return get_state()->is_free();
}

/**
 * \brief Returns whether the hero is currently using an equipment item.
 * \return true if the hero is using an equipment item.
 */
bool Hero::is_using_item() const {

  return get_state()->is_using_item();
}

/**
 * \brief When the hero is using an equipment item, returns that item.
 * \return The current equipment item.
 */
EquipmentItemUsage& Hero::get_item_being_used() {

  return get_state()->get_item_being_used();
}

/**
 * \brief Returns whether the hero is grabbing and moving an entity in its current state.
 *
 * If he is not grabbing any entity, false is returned.
 *
 * \return true if the hero is grabbing and moving an entity
 */
bool Hero::is_moving_grabbed_entity() const {

  return get_state()->is_moving_grabbed_entity();
}

/**
 * \brief Returns whether the hero is brandishing a treasure.
 * \return \c true if the hero is brandishing a treasure.
 */
bool Hero::is_brandishing_treasure() const {

  return get_state()->is_brandishing_treasure();
}

/**
 * \brief Returns whether the hero is grabbing or pulling an entity.
 * \return true if the hero is grabbing or pulling an entity
 */
bool Hero::is_grabbing_or_pulling() const {

  return get_state()->is_grabbing_or_pulling();
}

/**
 * \brief Lets the hero walk normally.
 */
void Hero::start_free() {

  set_state(std::make_shared<FreeState>(*this));
}

/**
 * \brief Lets the hero walk.
 *
 * If the hero is carrying item in the current state, the item is preserved.
 * If the hero is loading his sword in the current state, this continues.
 * If the hero is running in the current state, he continues to run.
 */
void Hero::start_free_carrying_loading_or_running() {

  if (get_state()->get_name() == "sword loading") {
    // Nothing to do: just keep the sword loaded.
    return;
  }

  if (get_state()->get_name() == "running" && get_state()->is_touching_ground()) {
    // Nothing to do: just keep running.
    return;
  }

  if (get_state()->is_carrying_item()) {
    set_state(std::make_shared<CarryingState>(*this, get_state()->get_carried_object()));
  }
  else {
    set_state(std::make_shared<FreeState>(*this));
  }
}

/**
 * \brief Makes the hero brandish a treasure.
 * \param treasure The treasure to give him. It must be obtainable.
 * \param callback_ref Lua ref to a function to call when the
 * treasure's dialog finishes (possibly an empty ref).
 */
void Hero::start_treasure(
    const Treasure& treasure,
    const ScopedLuaRef& callback_ref
) {
  set_state(std::make_shared<TreasureState>(*this, treasure, callback_ref));
}

/**
 * \brief Makes the hero walk with a predetermined path.
 *
 * The player does not control him anymore.
 *
 * \param path the succession of basic moves
 * composing this movement (each character represents
 * a direction between '0' and '7'), as in PathMovement
 * \param loop true to make the movement return to the beginning
 * once finished
 * \param ignore_obstacles true to make the movement ignore obstacles
 */
void Hero::start_forced_walking(const std::string& path, bool loop, bool ignore_obstacles) {
  set_state(std::make_shared<ForcedWalkingState>(*this, path, loop, ignore_obstacles));
}

/**
 * \brief Makes the hero jump into a direction.
 *
 * While he is jumping, the player does not control him anymore.
 *
 * \param direction8 direction of the jump (0 to 7)
 * \param distance distance of the jump in pixels
 * \param ignore_obstacles true make the movement ignore obstacles
 * \param with_sound true to play the "jump" sound
 */
void Hero::start_jumping(
    int direction8,
    int distance,
    bool ignore_obstacles,
    bool with_sound) {

  set_state(std::make_shared<JumpingState>(
      *this,
      direction8,
      distance,
      ignore_obstacles,
      with_sound));
}

/**
 * \brief Makes the hero brandish his sword meaning a victory.
 * \param callback_ref Lua ref to a function to call when the
 * victory sequence finishes (possibly an empty ref).
 */
void Hero::start_victory(const ScopedLuaRef& callback_ref) {
  set_state(std::make_shared<VictoryState>(*this, callback_ref));
}

/**
 * \brief Freezes the hero.
 *
 * When the hero is frozen, he cannot move.
 * The current animation of the hero's sprites is stopped and the "stopped" animation is played.
 * You can call start_free() to unfreeze him.
 */
void Hero::start_frozen() {
  set_state(std::make_shared<FrozenState>(*this));
}

/**
 * \brief Makes the hero lift a destructible item.
 * \param item_to_lift The item to lift.
 */
void Hero::start_lifting(const std::shared_ptr<CarriedObject>& item_to_lift) {
  set_state(std::make_shared<LiftingState>(*this, item_to_lift));
}

/**
 * \brief Starts running with the speed shoes.
 */
void Hero::start_running() {

  // The running state may be triggered by the action command or an
  // item command.
  GameCommand command;
  if (is_free()) {
    command = GameCommand::ACTION;
  }
  else {
    command = get_commands().is_command_pressed(GameCommand::ITEM_1) ?
        GameCommand::ITEM_1 : GameCommand::ITEM_2;
  }
  set_state(std::make_shared<RunningState>(*this, command));
}

/**
 * \brief Starts pushing an obstacle.
 */
void Hero::start_pushing() {

  get_equipment().notify_ability_used(Ability::PUSH);
  set_state(std::make_shared<PushingState>(*this));
}

/**
 * \brief Starts grabbing an obstacle.
 */
void Hero::start_grabbing() {

  get_equipment().notify_ability_used(Ability::GRAB);
  set_state(std::make_shared<GrabbingState>(*this));
}

/**
 * \brief Starts pulling an obstacle.
 */
void Hero::start_pulling() {

  get_equipment().notify_ability_used(Ability::PULL);
  set_state(std::make_shared<PullingState>(*this));
}

/**
 * \brief Returns whether the hero can pick a treasure in his current state.
 * \param item The equipment item to pick.
 * \return true if this equipment item can currently be picked.
 */
bool Hero::can_pick_treasure(EquipmentItem& item) {

  return get_state()->get_can_pick_treasure(item);
}

/**
 * \brief Returns whether the hero currently ignores the effect of a teletransporter.
 * \param teletransporter A candidate teletransporter.
 * \return \c true if the hero currently ignores this teletransporter.
 */
bool Hero::can_avoid_teletransporter(const Teletransporter& teletransporter) const {

  if (teletransporter.is_on_map_side()) {
    // Never ignore this kind of teletransporter.
    return false;
  }

  if (has_stream_action()) {
    // Ignore teletransporters until the stream is finished.
    return true;
  }

  return get_state()->can_avoid_teletransporter();
}

/**
 * \brief Returns whether the hero can currently start running.
 * \return \c true if the hero can run.
 */
bool Hero::can_run() const {

  if (!get_equipment().has_ability(Ability::RUN)) {
    return false;
  }

  if (
      has_stream_action() &&
      !get_stream_action()->get_stream().get_allow_movement()
  ) {
    // Don't run on a blocking stream.
    return false;
  }

  return is_free();
}

/**
 * \brief Returns whether the hero can currently push an obstacle.
 * \return \c true if the hero can push.
 */
bool Hero::can_push() const {

  return get_equipment().has_ability(Ability::PUSH);
}

/**
 * \brief Returns whether the hero can currently grab an obstacle.
 * \return \c true if the hero can grab.
 */
bool Hero::can_grab() const {

  return get_equipment().has_ability(Ability::GRAB);
}

/**
 * \brief Returns whether the hero can currently pull an obstacle.
 * \return \c true if the hero can pull.
 */
bool Hero::can_pull() const {

  return get_equipment().has_ability(Ability::PULL);
}

/**
 * \brief Returns whether the hero can interact with the given NPC.
 * \param npc A non-playing character.
 * \return \c true if the hero can interact with this NPC.
 */
bool Hero::can_interact_with_npc(Npc& npc) {
  return get_state()->get_can_interact_with_npc(npc);
}

/**
 * \brief Returns whether the hero can stop attacks with a shield in his
 * current state.
 * \return \c true if the shield is active is this state.
 */
bool Hero::can_use_shield() const {

  return get_state()->get_can_use_shield();
}


/**
 * \brief Returns whether the hero can currently use his sword.
 * \return \c true if the sword can be used now.
 */
bool Hero::can_start_sword() const {

  if (has_stream_action() &&
      !get_stream_action()->get_stream().get_allow_attack()) {
    // A stream prevents from using the sword.
    return false;
  }

  return get_state()->get_can_start_sword();
}

/**
 * \brief Starts using the sword.
 */
void Hero::start_sword() {
  set_state(std::make_shared<SwordSwingingState>(*this));
}

/**
 * \brief Starts loading the sword.
 * \param spin_attack_delay Delay before allowing the spin attack (-1 means never).
 */
void Hero::start_sword_loading(int spin_attack_delay) {
  set_state(std::make_shared<SwordLoadingState>(*this, spin_attack_delay));
}

/**
 * \brief Returns whether the hero can starts using an equipment item.
 * \param item The equipment item to use.
 * \return true if this equipment item can currently be used.
 */
bool Hero::can_start_item(EquipmentItem& item) {

  if (!item.is_saved()) {
    // This item has no possession state, it cannot be used.
    return false;
  }

  if (!item.is_assignable()) {
    // This item cannot be used explicitly.
    return false;
  }

  if (item.get_variant() == 0) {
    // The player does not have this item.
    return false;
  }

  if (has_stream_action() &&
      !get_stream_action()->get_stream().get_allow_item()) {
    // A stream prevents from using items.
    return false;
  }

  return get_state()->get_can_start_item(item);
}

/**
 * \brief Starts using an equipment item.
 * \param item The equipment item to use.
 */
void Hero::start_item(EquipmentItem& item) {
  Debug::check_assertion(can_start_item(item),
      std::string("The hero cannot start using item '")
      + item.get_name() + "' now");
  set_state(std::make_shared<UsingItemState>(*this, item));
}

/**
 * \brief Starts shooting a boomerang.
 * \param max_distance maximum distance of the movement in pixels
 * \param speed speed of the movement in pixels per second
 * \param tunic_preparing_animation animation name of the hero's tunic sprite
 * when preparing the boomerang
 * \param sprite_name animation set id that represents the boomerang
 */
void Hero::start_boomerang(int max_distance, int speed,
    const std::string& tunic_preparing_animation,
    const std::string& sprite_name) {

  set_state(std::make_shared<BoomerangState>(*this, max_distance, speed,
      tunic_preparing_animation, sprite_name));
}

/**
 * \brief Starts shooting an arrow with a bow.
 */
void Hero::start_bow() {
  set_state(std::make_shared<BowState>(*this));
}

/**
 * \brief Starts shooting the hookshot.
 */
void Hero::start_hookshot() {
  set_state(std::make_shared<HookshotState>(*this));
}

/**
 * \brief Makes the hero return to his last solid ground position.
 * \param use_specified_position true to get back to the place previously specified (if any),
 * false to get back to the last coordinates with solid ground
 * \param end_delay a delay to add at the end before returning control to the hero (default 0)
 * \param with_sound true to play a sound when returning to solid ground (default true)
 */
void Hero::start_back_to_solid_ground(bool use_specified_position,
    uint32_t end_delay, bool with_sound) {

  set_state(std::make_shared<BackToSolidGroundState>(
              *this, use_specified_position, end_delay, with_sound));
}

/**
 * \brief Activates immediately the state corresponding to the current ground.
 *
 * Only the state is changed here.
 * Some other functions like start_deep_water() and start_hole()
 * are triggered when the ground changes (for example,
 * going from normal ground to deep water ground) and make more
 * complex transitions.
 * This function is supposed to called when the ground was ignored
 * and you want to apply its effect now (no matter whether it has changed or not).
 * This function is typically called at the end of a state that ignores
 * the ground (like JumpingState) to choose the
 * correct next state depending on the ground the hero lands on.
 */
void Hero::start_state_from_ground() {

  update_ground_below();  // Make sure the ground is up-to-date.

  switch (get_ground_below()) {

  case Ground::DEEP_WATER:
    if (get_state()->is_touching_ground()
        && get_equipment().has_ability(Ability::SWIM)) {
      set_state(std::make_shared<SwimmingState>(*this));
    }
    else {
      set_state(std::make_shared<PlungingState>(*this));
    }
    break;

  case Ground::HOLE:
    set_state(std::make_shared<FallingState>(*this));
    break;

  case Ground::LAVA:
    set_state(std::make_shared<PlungingState>(*this));
    break;

  case Ground::PRICKLE:
    // There is no specific state for prickles (yet?).
    set_state(std::make_shared<FreeState>(*this));
    start_prickle(0);
    break;

  case Ground::SHALLOW_WATER:
    start_shallow_water();
    start_free_carrying_loading_or_running();
    break;

  case Ground::GRASS:
    start_grass();
    start_free_carrying_loading_or_running();
    break;

  case Ground::TRAVERSABLE:
  case Ground::EMPTY:
  case Ground::LADDER:
  case Ground::ICE:
    start_free_carrying_loading_or_running();
    break;

  case Ground::WALL:
  case Ground::LOW_WALL:
  case Ground::WALL_TOP_RIGHT:
  case Ground::WALL_TOP_LEFT:
  case Ground::WALL_BOTTOM_LEFT:
  case Ground::WALL_BOTTOM_RIGHT:
  case Ground::WALL_TOP_RIGHT_WATER:
  case Ground::WALL_TOP_LEFT_WATER:
  case Ground::WALL_BOTTOM_LEFT_WATER:
  case Ground::WALL_BOTTOM_RIGHT_WATER:
    // The hero is stuck in a wall,
    // possibly because a teletransporter sent him here.
    // It is the fault of the quest maker and there is not much we can do.
    start_free_carrying_loading_or_running();
    break;
  }
}

/**
 * @brief Starts the given custom Lua state.
 * @param custom_state The Lua state object.
 */
void Hero::start_custom_state(const std::shared_ptr<CustomState>& custom_state) {

  custom_state->set_entity(*this);
  set_state(custom_state);
}

}

