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
#include "solarus/core/Game.h"
#include "solarus/core/Map.h"
#include "solarus/core/System.h"
#include "solarus/hero/FreeState.h"
#include "solarus/hero/HeroSprites.h"
#include "solarus/hero/VictoryState.h"
#include "solarus/lua/LuaContext.h"
#include <lua.hpp>

namespace Solarus {

/**
 * \brief Constructor.
 * \param hero The hero controlled by this state.
 * \param callback_ref Lua ref to a function to call when the
 * victory sequence finishes (possibly an empty ref).
 */
Hero::VictoryState::VictoryState(Hero& hero, const ScopedLuaRef& callback_ref):
  HeroState(hero, "victory"),
  end_victory_date(0),
  finished(false),
  callback_ref(callback_ref) {

}

/**
 * \brief Starts this state.
 * \param previous_state the previous state
 */
void Hero::VictoryState::start(const State* previous_state) {

  HeroState::start(previous_state);

  get_sprites().set_animation_victory();
  get_sprites().set_ignore_suspend(true);
  Sound::play("victory", get_game().get_resource_provider());

  // compute the date when the victory state is considered as finished,
  // but the game may be currently suspended
  uint32_t start_victory_date = is_suspended() ?
      get_when_suspended() : System::now();
  end_victory_date = start_victory_date + 1500;
}

/**
 * \brief Stops this state.
 * \param next_state the next state
 */
void Hero::VictoryState::stop(const State* next_state) {

  HeroState::stop(next_state);
  get_sprites().set_ignore_suspend(false);
  callback_ref.clear();
}

/**
 * \brief Updates this state.
 */
void Hero::VictoryState::update() {

  HeroState::update();

  if (!finished && System::now() >= end_victory_date) {
    finished = true;
    if (!callback_ref.is_empty()) {
      // The behavior is defined by Lua.
      callback_ref.clear_and_call("hero victory callback");
    }
    else {
      // By default, get back to the normal state.
      Hero& hero = get_entity();
      hero.set_state(std::make_shared<FreeState>(hero));
    }
  }
}

/**
 * \brief Notifies this state that the game was just suspended or resumed.
 * \param suspended true if the game is suspended
 */
void Hero::VictoryState::set_suspended(bool suspended) {

  HeroState::set_suspended(suspended);

  if (!suspended) {
    end_victory_date += System::now() - get_when_suspended();
  }
}

}

