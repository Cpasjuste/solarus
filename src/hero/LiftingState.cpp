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
#include "solarus/core/Equipment.h"
#include "solarus/core/Game.h"
#include "solarus/core/Map.h"
#include "solarus/entities/CarriedObject.h"
#include "solarus/entities/Entities.h"
#include "solarus/hero/CarryingState.h"
#include "solarus/hero/HeroSprites.h"
#include "solarus/hero/LiftingState.h"

namespace Solarus {

/**
 * \brief Constructor.
 * \param hero The hero controlled by this state.
 * \param lifted_item The item to lift.
 */
Hero::LiftingState::LiftingState(
    Hero& hero,
    const std::shared_ptr<CarriedObject>& lifted_item
):
  HeroState(hero, "lifting"),
  lifted_item(lifted_item) {

  Debug::check_assertion(lifted_item != nullptr, "Missing lifted item");
}

/**
 * \brief Starts this state.
 * \param previous_state the previous state
 */
void Hero::LiftingState::start(const State* previous_state) {

  HeroState::start(previous_state);

  // initialize the entity that will be lifted
  lifted_item->set_map(get_map());

  get_commands_effects().set_action_key_effect(CommandsEffects::ACTION_KEY_THROW);
  get_sprites().set_animation_lifting();
  get_sprites().set_lifted_item(lifted_item);
  get_entity().set_facing_entity(nullptr);

  get_equipment().notify_ability_used(Ability::LIFT);
}

/**
 * \brief Ends this state.
 * \param next_state the next state
 */
void Hero::LiftingState::stop(const State* next_state) {

  HeroState::stop(next_state);

  if (lifted_item != nullptr) {

    get_sprites().set_lifted_item(nullptr);

    // the lifted item is still managed by this state
    switch (next_state->get_previous_carried_object_behavior()) {

    case CarriedObject::Behavior::THROW:
      throw_item();
      break;

    case CarriedObject::Behavior::REMOVE:
    case CarriedObject::Behavior::KEEP:
      lifted_item = nullptr;
      break;
    }
    get_commands_effects().set_action_key_effect(CommandsEffects::ACTION_KEY_NONE);
  }
}

/**
 * \brief Updates this state.
 */
void Hero::LiftingState::update() {

  HeroState::update();

  lifted_item->update();

  // See if the item has finished being lifted.
  Hero& hero = get_entity();
  const bool lift_finished = !lifted_item->is_being_lifted() || hero.is_animation_finished();
  if (!is_suspended() && lift_finished) {

    std::shared_ptr<CarriedObject> carried_object = lifted_item;
    lifted_item = nullptr; // we do not take care of the carried object from this state anymore
    hero.set_state(std::make_shared<CarryingState>(hero, carried_object));
  }
}

/**
 * \brief Notifies this state that the game was just suspended or resumed.
 * \param suspended true if the game is suspended
 */
void Hero::LiftingState::set_suspended(bool suspended) {

  HeroState::set_suspended(suspended);

  if (lifted_item != nullptr) {
    lifted_item->set_suspended(suspended);
  }
}

/**
 * \brief Returns whether the hero can be hurt in this state.
 * \return true if the hero can be hurt in this state
 * \param attacker an attacker that is trying to hurt the hero
 * (or nullptr if the source of the attack is not an enemy)
 */
bool Hero::LiftingState::get_can_be_hurt(Entity* /* attacker */) {
  return true;
}

/**
 * \brief Throws the item that is being lifted.
 *
 * This function is called when this state is interrupted by a new state,
 * e.g. when the hero is hurt while lifting an item.
 */
void Hero::LiftingState::throw_item() {

  lifted_item->throw_item(get_sprites().get_animation_direction());
  get_entities().add_entity(lifted_item);
  lifted_item = nullptr;
}

/**
 * \copydoc Entity::State::get_carried_object
 */
std::shared_ptr<CarriedObject> Hero::LiftingState::get_carried_object() const {
  return lifted_item;
}

}

