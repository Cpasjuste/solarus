/*
 * Copyright (C) 2018-2020 std::gregwar, Solarus - http://www.solarus-games.org
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
#pragma once

#include "solarus/core/InputEvent.h"
#include "solarus/core/ControlsPtr.h"
#include "solarus/core/Command.h"

namespace Solarus {

class MainLoop;
class Game;

/**
 * @brief Holds all of the current Commands object and dispatches the input into them
 */
class CommandsDispatcher
{
  friend class Controls;
public:
  CommandsDispatcher(MainLoop& main);
  static CommandsDispatcher& get();
  void notify_input(const InputEvent& event);
  ControlsPtr create_commands_from_game(Game& game);
  ControlsPtr create_commands_from_keyboard();
  ControlsPtr create_commands_from_joypad(const JoypadPtr& joypad);
private:
  static CommandsDispatcher* instance;
  using WeakCommands = std::weak_ptr<Controls>;

  void add_commands(const WeakCommands& cmds);
  void remove_commands(const Controls *cmds);

  std::vector<WeakCommands> commands;
  MainLoop& main_loop;
};

}

