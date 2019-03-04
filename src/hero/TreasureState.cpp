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
#include "solarus/core/EquipmentItem.h"
#include "solarus/core/Game.h"
#include "solarus/core/Map.h"
#include "solarus/graphics/Sprite.h"
#include "solarus/hero/FreeState.h"
#include "solarus/hero/HeroSprites.h"
#include "solarus/hero/TreasureState.h"
#include "solarus/lua/LuaContext.h"
#include <lua.hpp>
#include <string>

namespace Solarus {

/**
 * \brief Constructor.
 * \param hero The hero controlled by this state.
 * \param treasure The treasure to give to the hero. It must be obtainable.
 * \param callback_ref Lua ref to a function to call when the
 * treasure's dialog finishes (possibly an empty ref).
 */
Hero::TreasureState::TreasureState(
    Hero& hero,
    const Treasure& treasure,
    const ScopedLuaRef& callback_ref
):

  HeroState(hero, "treasure"),
  treasure(treasure),
  treasure_sprite(),
  callback_ref(callback_ref) {

  treasure.check_obtainable();
  treasure_sprite = treasure.create_sprite();
}

/**
 * \brief Starts this state.
 * \param previous_state the previous state
 */
void Hero::TreasureState::start(const State* previous_state) {

  HeroState::start(previous_state);

  // Show the animation.
  get_sprites().save_animation_direction();
  get_sprites().set_animation_brandish();

  // Play the sound.
  const std::string& sound_id = treasure.get_item().get_sound_when_brandished();
  if (!sound_id.empty()) {
    Sound::play(sound_id);
  }

  // Give the treasure.
  treasure.give_to_player();

  // Show a dialog (Lua does the job after this).
  ScopedLuaRef callback_ref = this->callback_ref;
  this->callback_ref.clear();
  get_lua_context().notify_hero_brandish_treasure(treasure, callback_ref);
}

/**
 * \brief Stops this state.
 * \param next_state the next state
 */
void Hero::TreasureState::stop(const State* next_state) {

  HeroState::stop(next_state);

  // Restore the sprite's direction.
  get_sprites().restore_animation_direction();
  callback_ref.clear();
}

/**
 * \copydoc Hero::HeroState::update
 */
void Hero::TreasureState::update() {

  HeroState::update();

  treasure_sprite->update();
}

/**
 * \brief Draws this state.
 */
void Hero::TreasureState::draw_on_map() {

  HeroState::draw_on_map();

  const Hero& hero = get_entity();
  int x = hero.get_x();
  int y = hero.get_y();

  const CameraPtr& camera = get_map().get_camera();
  if (camera == nullptr) {
    return;
  }
  treasure_sprite->draw(get_map().get_camera_surface(),
      x - camera->get_top_left_x(),
      y - 24 - camera->get_top_left_y());
}

/**
 * \copydoc Entity::State::get_previous_carried_object_behavior
 */
CarriedObject::Behavior Hero::TreasureState::get_previous_carried_object_behavior() const {
  return CarriedObject::Behavior::REMOVE;
}

/**
 * \brief Returns whether the hero is brandishing a treasure in this state.
 * \return \c true if the hero is brandishing a treasure.
 */
bool Hero::TreasureState::is_brandishing_treasure() const {
  return true;
}

}

