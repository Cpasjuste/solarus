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
#include "solarus/core/Geometry.h"
#include "solarus/core/QuestFiles.h"
#include "solarus/entities/Enemy.h"
#include "solarus/hero/FreeState.h"
#include "solarus/hero/HeroSprites.h"
#include "solarus/hero/SpinAttackState.h"
#include "solarus/movements/CircleMovement.h"
#include "solarus/movements/StraightMovement.h"
#include <memory>
#include <sstream>
#include <string>

namespace Solarus {

/**
 * \brief Constructor.
 * \param hero The hero controlled by this state.
 */
Hero::SpinAttackState::SpinAttackState(Hero& hero):
  HeroState(hero, "sword spin attack"),
  being_pushed(false) {

}

/**
 * \brief Starts this state.
 * \param previous_state the previous state
 */
void Hero::SpinAttackState::start(const State* previous_state) {

  HeroState::start(previous_state);

  // play the sound
  play_spin_attack_sound();

  // start the animation
  Hero& hero = get_entity();
  if (get_equipment().has_ability(Ability::SWORD_KNOWLEDGE)) {
    get_sprites().set_animation_super_spin_attack();
    std::shared_ptr<CircleMovement> movement =
        std::make_shared<CircleMovement>();
    movement->set_center(hero.get_xy());
    movement->set_radius_speed(128);
    movement->set_radius(24);
    movement->set_angular_speed(Geometry::degrees_to_radians(1000));
    movement->set_max_rotations(3);
    movement->set_clockwise(true);
    hero.set_movement(movement);
    get_equipment().notify_ability_used(Ability::SWORD_KNOWLEDGE);
  }
  else {
    get_sprites().set_animation_spin_attack();
  }
}

/**
 * \brief Ends this state.
 * \param next_state the next state
 */
void Hero::SpinAttackState::stop(const State* next_state) {

  HeroState::stop(next_state);

  Hero& hero = get_entity();
  if (hero.get_movement() != nullptr) {
    // stop the movement of being pushed by an enemy after hitting him
    hero.clear_movement();
  }
}

/**
 * \brief Updates this state.
 */
void Hero::SpinAttackState::update() {

  // check the animation
  Hero& hero = get_entity();
  if (get_sprites().is_animation_finished()) {
    hero.set_state(std::make_shared<FreeState>(hero));
  }

  // check the movement if any
  if (hero.get_movement() != nullptr && hero.get_movement()->is_finished()) {
    hero.clear_movement();

    if (!being_pushed) {
      // end of a super spin attack
      hero.set_state(std::make_shared<FreeState>(hero));
    }
  }
}

/**
 * \brief Returns whether crystals can be activated by the sword in this state.
 * \return true if crystals can be activated by the sword in this state
 */
bool Hero::SpinAttackState::can_sword_hit_crystal() const {
  return true;
}

/**
 * \brief Returns whether the hero can pick a treasure in this state.
 * \param item The equipment item to obtain.
 * \return true if the hero can pick that treasure in this state.
 */
bool Hero::SpinAttackState::get_can_pick_treasure(EquipmentItem& /* item */) const {
  return true;
}

/**
 * \brief Returns whether the hero can be hurt in this state.
 * \return true if the hero can be hurt in this state
 * \param attacker an attacker that is trying to hurt the hero
 * (or nullptr if the source of the attack is not an enemy)
 */
bool Hero::SpinAttackState::get_can_be_hurt(Entity* /* attacker */) {
  return false;
}

/**
 * \copydoc Entity::State::is_cutting_with_sword
 */
bool Hero::SpinAttackState::is_cutting_with_sword(Destructible& /* destructible */) {

  // during a spin attack, any sprite collision can cut things
  return true;
}

/**
 * \brief Returns the damage power of the sword for the current attack.
 * \return the current damage factor of the sword
 */
int Hero::SpinAttackState::get_sword_damage_factor() const {

  // the damage are multiplied by 2
  return HeroState::get_sword_damage_factor() * 2;
}

/**
 * \brief Plays the sword loading sound.
 */
void Hero::SpinAttackState::play_spin_attack_sound() {

  std::ostringstream oss;
  oss << "sword_spin_attack_release_" << get_equipment().get_ability(Ability::SWORD);
  std::string custom_sound_name = oss.str();
  if (Sound::exists(custom_sound_name)) {
    Sound::play(custom_sound_name); // this particular sword has a spin attack sound effect
  }
  else {
    Sound::play("sword_spin_attack_release");
  }
}

/**
 * \brief Returns whether a deep water tile is considered as an obstacle in this state.
 * \return true if the deep water tiles are considered as obstacles in this state
 */
bool Hero::SpinAttackState::is_deep_water_obstacle() const {
  return !being_pushed;
}

/**
 * \brief Returns whether a hole is considered as an obstacle in this state.
 * \return true if the holes are considered as obstacles in this state
 */
bool Hero::SpinAttackState::is_hole_obstacle() const {
  return !being_pushed;
}

/**
 * \brief Returns whether lava is considered as an obstacle in this state.
 * \return true if lava is considered as obstacles in this state
 */
bool Hero::SpinAttackState::is_lava_obstacle() const {
  return !being_pushed;
}

/**
 * \brief Returns whether prickles are considered as an obstacle in this state.
 * \return true if prickles are considered as obstacles in this state
 */
bool Hero::SpinAttackState::is_prickle_obstacle() const {
  return !being_pushed;
}

/**
 * \brief Returns whether a teletransporter is considered as an obstacle in this state.
 * \param teletransporter a teletransporter
 * \return true if the teletransporter is an obstacle in this state
 */
bool Hero::SpinAttackState::is_teletransporter_obstacle(
    Teletransporter& /* teletransporter */) {

  // if the hero is pushed by an enemy or making a super spin attack,
  // don't go on a teletransporter
  return get_entity().get_movement() != nullptr;
}

/**
 * \copydoc Entity::State::is_separator_obstacle
 */
bool Hero::SpinAttackState::is_separator_obstacle(
    Separator& /* separator */) {
  return true;
}

/**
 * \brief Notifies this state that the hero has just failed to change its
 * position because of obstacles.
 */
void Hero::SpinAttackState::notify_obstacle_reached() {

  // the hero reached an obstacle while being pushed after hitting an enemy
  // or making a super spin attack
  get_entity().clear_movement();

  if (!being_pushed) {
    // obstacle while making a super spin attack: finish with a normal spin attack
    get_sprites().set_animation_spin_attack();
  }
}

/**
 * \copydoc Entity::State::notify_attacked_enemy
 */
void Hero::SpinAttackState::notify_attacked_enemy(
    EnemyAttack attack,
    Enemy& victim,
    Sprite* victim_sprite,
    const EnemyReaction::Reaction& result,
    bool /* killed */) {

  Hero& hero = get_entity();
  if (attack == EnemyAttack::SWORD &&
      victim.get_push_hero_on_sword() &&
      result.type != EnemyReaction::ReactionType::IGNORED &&
      result.type != EnemyReaction::ReactionType::LUA_CALLBACK) {

    if (hero.get_movement() != nullptr) {
      // interrupting a super spin attack: finish with a normal one
      hero.clear_movement();
      get_sprites().set_animation_spin_attack();
    }

    being_pushed = true;
    double angle = victim.get_angle(hero, victim_sprite, nullptr);
    std::shared_ptr<StraightMovement> movement =
        std::make_shared<StraightMovement>(false, true);
    movement->set_max_distance(24);
    movement->set_speed(120);
    movement->set_angle(angle);
    hero.set_movement(movement);
  }
}

}

