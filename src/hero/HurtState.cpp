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
#include "solarus/core/Geometry.h"
#include "solarus/core/Game.h"
#include "solarus/core/System.h"
#include "solarus/hero/FreeState.h"
#include "solarus/hero/HeroSprites.h"
#include "solarus/hero/HurtState.h"
#include "solarus/lua/LuaContext.h"
#include "solarus/movements/StraightMovement.h"
#include <algorithm>
#include <memory>

namespace Solarus {

/**
 * \brief Constructor.
 * \param hero The hero controlled by this state.
 * \param source_xy Coordinates of the thing (usually an enemy) that hurts
 * the hero, used to compute the trajectory of the hero.
 * If nullptr, the hero won't move.
 * \param damage Number of life poitns to remove
 * (this number may be reduced by the tunic or by hero:on_taking_damage()).
 */
Hero::HurtState::HurtState(
    Hero& hero,
    const Point* source_xy,
    int damage):
  HeroState(hero, "hurt"),
  has_source(source_xy != nullptr),
  source_xy(has_source ? *source_xy : Point()),
  damage(damage),
  end_hurt_date(0) {

}

/**
 * \brief Starts this state.
 * \param previous_state The previous state.
 */
void Hero::HurtState::start(const State* previous_state) {

  HeroState::start(previous_state);

  Equipment& equipment = get_equipment();

  Sound::play("hero_hurt", get_game().get_resource_provider());

  Hero& hero = get_entity();
  const uint32_t invincibility_duration = 2000;
  hero.set_invincible(true, invincibility_duration);
  get_sprites().set_animation_hurt();
  get_sprites().blink(invincibility_duration);

  if (has_source) {
    double angle = Geometry::get_angle(source_xy, hero.get_xy());
    std::shared_ptr<StraightMovement> movement =
        std::make_shared<StraightMovement>(false, true);
    movement->set_max_distance(24);
    movement->set_speed(120);
    movement->set_angle(angle);
    hero.set_movement(movement);
  }
  end_hurt_date = System::now_ms() + 200;

  // See if the script customizes how the hero takes damages.
  bool handled = get_lua_context().hero_on_taking_damage(hero, damage);

  if (!handled && damage != 0) {
    // No customized damage: perform the default calculation.
    // The level of the tunic reduces the damage,
    // but we remove at least 1 life point.
    int life_points = std::max(1, damage / (equipment.get_ability(Ability::TUNIC)));

    equipment.remove_life(life_points);
    if (equipment.has_ability(Ability::TUNIC)) {
      equipment.notify_ability_used(Ability::TUNIC);
    }
  }
}

/**
 * \brief Ends this state.
 * \param next_state the next state
 */
void Hero::HurtState::stop(const State* next_state) {

  HeroState::stop(next_state);

  get_entity().clear_movement();
}

/**
 * \brief Updates this state.
 */
void Hero::HurtState::update() {

  HeroState::update();

  Hero& hero = get_entity();
  if ((hero.get_movement() != nullptr && hero.get_movement()->is_finished())
      || System::now_ms() >= end_hurt_date) {
    // The movement may be finished, or the end date may be reached
    // when there is an obstacle or when there is no movement at all.

    hero.clear_movement();
    hero.start_state_from_ground();
  }
}

/**
 * \brief Notifies this state that the game was just suspended or resumed.
 * \param suspended true if the game is suspended
 */
void Hero::HurtState::set_suspended(bool suspended) {

  HeroState::set_suspended(suspended);

  if (!suspended) {
    uint32_t diff = System::now_ms() - get_when_suspended();
    end_hurt_date += diff;
  }
}

/**
 * \brief Returns whether the game over sequence can start in the current state.
 * \return true if the game over sequence can start in the current state
 */
bool Hero::HurtState::can_start_gameover_sequence() const {
  return false;
}

/**
 * \brief Returns whether the hero is touching the ground in the current state.
 * \return true if the hero is touching the ground in the current state
 */
bool Hero::HurtState::is_touching_ground() const {
  return false;
}

/**
 * \brief Returns whether a teletransporter is considered as an obstacle in this state.
 * \param teletransporter a teletransporter
 * \return true if the teletransporter is an obstacle in this state
 */
bool Hero::HurtState::is_teletransporter_obstacle(
    Teletransporter& /* teletransporter */) {
  return true;
}

/**
 * \copydoc Entity::State::is_stream_obstacle
 */
bool Hero::HurtState::is_stream_obstacle(Stream& /* stream */) {
  return true;
}

/**
 * \brief Returns whether a sensor is considered as an obstacle in this state.
 * \param sensor a sensor
 * \return true if the sensor is an obstacle in this state
 */
bool Hero::HurtState::is_sensor_obstacle(Sensor& /* sensor */) {
  return true;
}

/**
 * \copydoc Entity::State::is_separator_obstacle
 */
bool Hero::HurtState::is_separator_obstacle(Separator& /* separator */) {
  return true;
}

/**
 * \brief Returns whether the hero can be hurt in this state.
 * \param attacker an attacker that is trying to hurt the hero
 * (or nullptr if the source of the attack is not an enemy)
 * \return true if the hero can be hurt in this state
 */
bool Hero::HurtState::get_can_be_hurt(Entity* /* attacker */) {
  return false;
}

/**
 * \brief Returns whether the hero ignores the effect of switches in this state.
 * \return true if the hero ignores the effect of switches in this state
 */
bool Hero::HurtState::can_avoid_switch() const {
  return true;
}

/**
 * \brief Returns whether the hero ignores the effect of ice in this state.
 * \return \c true if the hero ignores the effect of ice in the current state.
 */
bool Hero::HurtState::can_avoid_ice() const {
  return true;
}

}

