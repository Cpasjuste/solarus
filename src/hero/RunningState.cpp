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
#include "solarus/core/Equipment.h"
#include "solarus/core/Game.h"
#include "solarus/core/Controls.h"
#include "solarus/core/Geometry.h"
#include "solarus/core/Map.h"
#include "solarus/core/System.h"
#include "solarus/hero/FreeState.h"
#include "solarus/hero/HeroSprites.h"
#include "solarus/hero/RunningState.h"
#include "solarus/entities/Enemy.h"
#include "solarus/entities/Jumper.h"
#include "solarus/entities/Stream.h"
#include "solarus/movements/JumpMovement.h"
#include "solarus/movements/StraightMovement.h"
#include <memory>

namespace Solarus {

/**
 * \brief Constructor.
 * \param hero The hero controlled by this state.
 * \param command The game command that triggers running.
 */
Hero::RunningState::RunningState(Hero& hero, Command command):
  HeroState(hero, "running"),
  phase(0),
  next_phase_date(0),
  next_sound_date(0),
  command(command) {

}

/**
 * \brief Starts this state.
 * \param previous_state the previous state
 */
void Hero::RunningState::start(const State* previous_state) {

  HeroState::start(previous_state);

  get_sprites().set_animation_prepare_running();

  phase = 0;

  uint32_t now = System::now();
  next_phase_date = now + 500;
  next_sound_date = now + 300;
}

/**
 * \brief Stops this state.
 */
void Hero::RunningState::stop(const State* next_state) {

  HeroState::stop(next_state);

  if (phase != 0) {
    get_entity().clear_movement();
  }
}

/**
 * \brief Updates this state.
 */
void Hero::RunningState::update() {

  HeroState::update();

  if (is_suspended()) {
    return;
  }

  uint32_t now = System::now();

  if (!is_bouncing() && now >= next_sound_date) {
    Sound::play("running");
    next_sound_date = now + 170;
  }

  Hero& hero = get_entity();
  if (phase == 0) {

    if (now >= next_phase_date) {

      double angle = Geometry::degrees_to_radians(get_sprites().get_animation_direction() * 90);
      std::shared_ptr<StraightMovement> movement =
          std::make_shared<StraightMovement>(false, true);
      movement->set_max_distance(3000);
      movement->set_speed(300);
      movement->set_angle(angle);
      hero.clear_movement();
      hero.set_movement(movement);

      get_sprites().set_animation_running();
      phase++;
    }
    else if (!is_pressing_running_key()) {
      hero.set_state(std::make_shared<FreeState>(hero));
    }
  }
  else if (hero.get_movement()->is_finished()) {
    hero.start_state_from_ground();
  }
}

/**
 * \brief Notifies this state that the game was just suspended or resumed.
 * \param suspended true if the game is suspended
 */
void Hero::RunningState::set_suspended(bool suspended) {

  HeroState::set_suspended(suspended);

  if (!suspended) {
    uint32_t diff = System::now() - get_when_suspended();
    next_phase_date += diff;
    next_sound_date += diff;
  }
}

/**
 * \brief Returns whether the hero is bouncing after he reached an obstacle during the run.
 * \return true if the hero is bouncing
 */
bool Hero::RunningState::is_bouncing() const {
  return phase == 2;
}

/**
 * \brief Returns whether the hero is still pressing the key that made him
 * start running.
 * \return true if the hero is still pressing the running key
 */
bool Hero::RunningState::is_pressing_running_key() const {

  return get_commands().is_command_pressed(command);
}

/**
 * \brief Notifies this state that a directional key was just pressed.
 * \param direction4 direction of the key (0 to 3)
 */
void Hero::RunningState::notify_direction_command_pressed(int direction4) {

  if (!is_bouncing()
      && direction4 != get_sprites().get_animation_direction()) {
    Hero& hero = get_entity();
    hero.set_state(std::make_shared<FreeState>(hero));
  }
}

/**
 * \brief Notifies this state that the hero has just failed to change its
 * position because of obstacles.
 */
void Hero::RunningState::notify_obstacle_reached() {

  HeroState::notify_obstacle_reached();

  if (phase == 1) {
    int opposite_direction = (get_sprites().get_animation_direction8() + 4) % 8;
    get_entity().set_movement(std::make_shared<JumpMovement>(
        opposite_direction, 32, 64, false
    ));
    get_sprites().set_animation_hurt();
    Sound::play("running_obstacle");
    phase++;
  }
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
int Hero::RunningState::get_wanted_movement_direction8() const {
  return get_sprites().get_animation_direction8();
}

/**
 * \brief Returns whether the hero can take stairs in this state.
 * If false is returned, stairs have no effect (but they are obstacle for the hero).
 * \return true if the hero ignores the effect of stairs in this state
 */
bool Hero::RunningState::get_can_take_stairs() const {
  return !is_bouncing();
}

/**
 * \brief Returns whether can trigger a jumper in this state.
 *
 * If false is returned, jumpers have no effect (but they are obstacle
 * for the hero).
 *
 * \return true if the hero can use jumpers in this state
 */
bool Hero::RunningState::get_can_take_jumper() const {
  return !is_bouncing();
}

/**
 * \brief Notifies this state that the hero is activating a jumper.
 * \param jumper The jumper activated.
 */
void Hero::RunningState::notify_jumper_activated(Jumper& jumper) {

  // Jump immediately.
  get_entity().start_jumping(
      jumper.get_direction(),
      jumper.get_jump_length(),
      true,
      true
  );
}

/**
 * \brief Returns whether the hero can be hurt by an attacker in this state.
 * \param attacker an attacker that is trying to hurt the hero
 * (or nullptr if the source of the attack is not an enemy)
 * \return true if the hero can be hurt in this state
 */
bool Hero::RunningState::get_can_be_hurt(Entity* attacker) {

  if (phase == 0) {
    // Preparing to run.
    return true;
  }

  if (attacker != nullptr && attacker->get_type() == EntityType::ENEMY) {
    // TODO move get_can_hurt_hero_running to Entity.
    Enemy* enemy = static_cast<Enemy*>(attacker);
    return enemy->get_can_hurt_hero_running();
  }

  return false;
}

/**
 * \brief Returns whether the hero can pick a treasure in this state.
 * \param item The equipment item to obtain.
 * \return true if the hero can pick that treasure in this state.
 */
bool Hero::RunningState::get_can_pick_treasure(EquipmentItem& /* item */) const {
  return true;
}

/**
 * \copydoc Entity::State::can_sword_hit_crystal
 */
bool Hero::RunningState::can_sword_hit_crystal() const {
  return true;
}

/**
 * \brief Returns whether the game over sequence can start in the current state.
 * \return true if the game over sequence can start in the current state
 */
bool Hero::RunningState::can_start_gameover_sequence() const {
  return !is_bouncing();
}

/**
 * \brief Returns whether the hero is touching the ground in the current state.
 * \return true if the hero is touching the ground in the current state
 */
bool Hero::RunningState::is_touching_ground() const {
  return !is_bouncing();
}

/**
 * \brief Returns whether the hero ignores the effect of deep water in this state.
 * \return true if the hero ignores the effect of deep water in the current state
 */
bool Hero::RunningState::can_avoid_deep_water() const {
  return is_bouncing();
}

/**
 * \brief Returns whether the hero ignores the effect of holes in this state.
 * \return true if the hero ignores the effect of holes in the current state
 */
bool Hero::RunningState::can_avoid_hole() const {
  return is_bouncing();
}

/**
 * \brief Returns whether the hero ignores the effect of lava in this state.
 * \return true if the hero ignores the effect of lava in the current state
 */
bool Hero::RunningState::can_avoid_lava() const {
  return is_bouncing();
}

/**
 * \brief Returns whether the hero ignores the effect of prickles in this state.
 * \return true if the hero ignores the effect of prickles in the current state
 */
bool Hero::RunningState::can_avoid_prickle() const {
  return is_bouncing();
}

/**
 * \brief Returns whether the hero ignores the effect of teletransporters in this state.
 * \return true if the hero ignores the effect of teletransporters in this state
 */
bool Hero::RunningState::can_avoid_teletransporter() const {
  return is_bouncing();
}

/**
 * \copydoc Entity::State::can_avoid_stream
 */
bool Hero::RunningState::can_avoid_stream(const Stream& /* stream */) const {
  return is_bouncing();
}

/**
 * \copydoc Entity::State::can_persist_on_stream
 */
bool Hero::RunningState::can_persist_on_stream(const Stream& stream) const {

  // Continue to run if this is a non-blocking stream.
  return stream.get_allow_movement();
}

/**
 * \brief Returns whether a sensor is considered as an obstacle in this state.
 * \param sensor a sensor
 * \return true if the sensor is an obstacle in this state
 */
bool Hero::RunningState::is_sensor_obstacle(Sensor& /* sensor */) {
  return is_bouncing();
}

/**
 * \copydoc Entity::State::is_cutting_with_sword
 */
bool Hero::RunningState::is_cutting_with_sword(Entity& entity) {

  // check the distance to the detector
  const int distance = 8;
  Point tested_point = get_entity().get_facing_point();

  switch (get_sprites().get_animation_direction()) {

    case 0: // right
      tested_point.x += distance;
      break;

    case 1: // up
      tested_point.y -= distance;
      break;

    case 2: // left
      tested_point.x -= distance;
      break;

    case 3: // down
      tested_point.y += distance;
      break;
  }

  return entity.overlaps(tested_point);
}

/**
 * \brief Returns the damage power of the sword for the current attack.
 * \return the current damage factor of the sword
 */
int Hero::RunningState::get_sword_damage_factor() const {

  // the damage are multiplied by 2
  return HeroState::get_sword_damage_factor() * 2;
}

}

