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
#include "solarus/core/CommandsEffects.h"
#include "solarus/core/Debug.h"
#include "solarus/core/Game.h"
#include "solarus/core/Logger.h"
#include "solarus/core/Map.h"
#include "solarus/core/Point.h"
#include "solarus/core/System.h"
#include "solarus/entities/Entities.h"
#include "solarus/entities/Teletransporter.h"
#include "solarus/hero/CarryingState.h"
#include "solarus/hero/FreeState.h"
#include "solarus/hero/HeroSprites.h"
#include "solarus/hero/StairsState.h"
#include "solarus/movements/PathMovement.h"
#include <string>

namespace Solarus {

/**
 * \brief Constructor.
 * \param hero The hero controlled by this state.
 * \param stairs The stairs to take.
 * \param way The way you are taking the stairs.
 */
Hero::StairsState::StairsState(
    Hero& hero,
    const std::shared_ptr<Stairs>& stairs,
    Stairs::Way way
):
  HeroState(hero, "stairs"),
  stairs(stairs),
  way(way),
  phase(0),
  next_phase_date(0),
  carried_object(nullptr) {

  if (get_previous_carried_object_behavior() == CarriedObject::Behavior::KEEP) {
    // Keep the carried object of the previous state.
    carried_object = hero.get_carried_object();
  }
}

/**
 * \brief Changes the map.
 * \param map the new map
 */
void Hero::StairsState::set_map(Map& map) {

  HeroState::set_map(map);

  // the hero may go to another map while taking stairs and carrying an item
  if (carried_object != nullptr) {
    carried_object->set_map(map);
  }
}

/**
 * \brief Starts this state.
 * \param previous_state the previous state
 */
void Hero::StairsState::start(const State* previous_state) {

  HeroState::start(previous_state);

  // movement
  int speed = stairs->is_inside_floor() ? 40 : 24;
  std::string path = stairs->get_path(way);
  std::shared_ptr<PathMovement> movement = std::make_shared<PathMovement>(
      path, speed, false, true, false
  );

  // sprites and sound
  HeroSprites& sprites = get_sprites();
  if (carried_object == nullptr) {
    sprites.set_animation_walking_normal();
  }
  else {
    sprites.set_lifted_item(carried_object);
    sprites.set_animation_walking_carrying();
  }
  sprites.set_animation_direction((path[0] - '0') / 2);
  get_commands_effects().set_action_key_effect(CommandsEffects::ACTION_KEY_NONE);

  Hero& hero = get_entity();
  if (stairs->is_inside_floor()) {
    if (way == Stairs::NORMAL_WAY) {
      // Toward a higher layer: change the layer now.
      int layer = stairs->get_layer();
      SOLARUS_ASSERT(get_map().is_valid_layer(layer), "Invalid stairs layer");
      get_entities().set_entity_layer(hero, layer + 1);
    }
  }
  else {
    sprites.set_clipping_rectangle(stairs->get_clipping_rectangle(way));
    if (way == Stairs::REVERSE_WAY) {
      Point dxy = movement->get_xy_change();
      int fix_y = 8;
      if (path[path.size() - 1] == '2') {
        fix_y *= -1;
      }
      hero.set_xy(hero.get_x() - dxy.x, hero.get_y() - dxy.y + fix_y);
    }
  }
  hero.set_movement(movement);
}

/**
 * \brief Stops this state.
 * \param next_state the next state
 */
void Hero::StairsState::stop(const State* next_state) {

  HeroState::stop(next_state);

  if (carried_object != nullptr) {

    switch (next_state->get_previous_carried_object_behavior()) {

    case CarriedObject::Behavior::THROW:
      carried_object->throw_item(get_sprites().get_animation_direction());
      get_entities().add_entity(carried_object);
      carried_object = nullptr;
      get_sprites().set_lifted_item(nullptr);
      break;

    case CarriedObject::Behavior::REMOVE:
      get_sprites().set_lifted_item(nullptr);
      break;

    case CarriedObject::Behavior::KEEP:
      break;
    }
  }
}

/**
 * \brief Updates this state.
 */
void Hero::StairsState::update() {

  HeroState::update();

  if (is_suspended()) {
    return;
  }

  // first time: we play the sound and initialize
  if (phase == 0) {
    stairs->play_sound(way);
    next_phase_date = System::now_ms() + 450;
    phase++;
  }

  // update the carried object if any
  if (carried_object != nullptr) {
    carried_object->update();
  }

  Hero& hero = get_entity();
  if (stairs->is_inside_floor()) {

    // inside a single floor: return to normal state as soon as the movement is finished
    if (hero.get_movement()->is_finished()) {

      if (way == Stairs::REVERSE_WAY) {
        get_entities().set_entity_layer(hero, stairs->get_layer());
      }
      hero.clear_movement();
      if (carried_object == nullptr) {
        hero.set_state(std::make_shared<FreeState>(hero));
      }
      else {
        hero.set_state(std::make_shared<CarryingState>(hero, carried_object));
      }
    }
  }
  else {
    // stairs between two different floors: more complicated

    HeroSprites& sprites = get_sprites();
    if (hero.get_movement()->is_finished()) {
      hero.clear_movement();

      if (way == Stairs::NORMAL_WAY) {
        // we are on the old floor:
        // there must be a teletransporter associated with these stairs,
        // otherwise the hero would get stuck into the walls
        std::shared_ptr<Teletransporter> teletransporter = hero.get_delayed_teletransporter();
        if (teletransporter == nullptr ||
            !teletransporter->is_enabled() ||
            teletransporter->is_being_removed()) {
          Logger::error("Teletransporter expected with the stairs");
        }
        else {
          teletransporter->transport_hero(hero);
        }
      }
      else {
        // we are on the new floor: everything is finished
        sprites.set_clipping_rectangle();
      }

      if (carried_object == nullptr) {
        hero.set_state(std::make_shared<FreeState>(hero));
      }
      else {
        hero.set_state(std::make_shared<CarryingState>(hero, carried_object));
      }
    }
    else { // movement not finished yet

      uint32_t now = System::now_ms();
      if (now >= next_phase_date) {
        phase++;
        next_phase_date += 350;

        // main movement direction corresponding to each animation direction while taking stairs
        static constexpr int movement_directions[] = { 0, 0, 2, 4, 4, 4, 6, 0 };

        int animation_direction = stairs->get_animation_direction(way);
        if (phase == 2) { // the first phase of the movement is finished
          if (animation_direction % 2 != 0) {
            // if the stairs are spiral, take a diagonal direction of animation
            sprites.set_animation_walking_diagonal(animation_direction);
          }
          else {
            // otherwise, take a usual direction
            sprites.set_animation_direction(animation_direction / 2);
            sprites.set_animation_walking_normal();
          }
        }
        else if (phase == 3) { // the second phase of the movement (possibly diagonal) is finished
          sprites.set_animation_walking_normal();

          if (way == Stairs::NORMAL_WAY) {
            // on the old floor, take a direction towards the next floor
            sprites.set_animation_direction(movement_directions[animation_direction] / 2);
          }
          else {
            // on the new floor, take the opposite direction from the stairs
            sprites.set_animation_direction((stairs->get_direction() + 2) % 4);
          }
        }
      }
    }
  }
}

/**
 * \brief Notifies this state that the game was just suspended or resumed.
 * \param suspended true if the game is suspended
 */
void Hero::StairsState::set_suspended(bool suspended) {

  HeroState::set_suspended(suspended);

  if (carried_object != nullptr) {
    carried_object->set_suspended(suspended);
  }

  if (!suspended) {
    next_phase_date += System::now_ms() - get_when_suspended();
  }
}

/**
 * \brief Returns whether the hero is touching the ground in the current state.
 * \return \c true if the hero is touching the ground in the current state.
 */
bool Hero::StairsState::is_touching_ground() const {
  // To avoid falling to the lower layer when going upstairs
  // with INSIDE_FLOOR stairs.
  return false;
}

/**
 * \copydoc Entity::State::get_can_come_from_bad_ground
 */
bool Hero::StairsState::get_can_come_from_bad_ground() const {
  return false;
}

/**
 * \brief Returns whether the effect of teletransporters is delayed in this state.
 *
 * When overlapping a teletransporter, if this function returns true, the teletransporter
 * will not be activated immediately. The state then has to activate it when it is ready.
 *
 * \return true if the effect of teletransporters is delayed in this state
 */
bool Hero::StairsState::is_teletransporter_delayed() const {
  return true;
}

/**
 * \brief Returns the direction of the hero's movement as defined by the controls applied by the player
 * and the movements allowed is the current state.
 *
 * If he is not moving, -1 is returned.
 * This direction may be different from the real movement direction because of obstacles.
 *
 * \return the hero's wanted direction between 0 and 7, or -1 if he is stopped
 */
int Hero::StairsState::get_wanted_movement_direction8() const {
  return get_sprites().get_animation_direction8();
}

/**
 * \brief Returns the item currently carried by the hero in this state, if any.
 * \return the item carried by the hero, or nullptr
 */
std::shared_ptr<CarriedObject> Hero::StairsState::get_carried_object() const {
  return carried_object;
}

/**
 * \copydoc Entity::State::get_previous_carried_object_behavior
 */
CarriedObject::Behavior Hero::StairsState::get_previous_carried_object_behavior() const {

  if (stairs->is_inside_floor()) {
    return CarriedObject::Behavior::KEEP;
  }
  return CarriedObject::Behavior::REMOVE;
}

/**
 * \brief Notifies this state that the layer has changed.
 */
void Hero::StairsState::notify_layer_changed() {

  if (carried_object != nullptr) {
    carried_object->set_layer(get_entity().get_layer());
  }
}

}

