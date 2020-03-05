#pragma once

#include "solarus/core/InputEvent.h"
#include "solarus/core/CommandsPtr.h"
#include "solarus/core/Command.h"

namespace Solarus {

class MainLoop;
class Game;

/**
 * @brief Holds all of the current Commands object and dispatches the input into them
 */
class CommandsDispatcher
{
  friend class Commands;
public:
  CommandsDispatcher(MainLoop& main);
  static CommandsDispatcher& get();
  void notify_input(const InputEvent& event);
  CommandsPtr create_commands_from_game(Game& game);
  CommandsPtr create_commands_from_keyboard();
  CommandsPtr create_commands_from_joypad(const JoypadPtr& joypad);
private:
  static CommandsDispatcher* instance;
  using WeakCommands = std::weak_ptr<Commands>;

  void add_commands(const WeakCommands& cmds);
  void remove_commands(const Commands *cmds);

  std::vector<WeakCommands> commands;
  MainLoop& main_loop;
};

}

