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

