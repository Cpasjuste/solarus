#include "solarus/core/CommandsDispatcher.h"

#include "solarus/core/Commands.h"

namespace Solarus {

CommandsDispatcher* CommandsDispatcher::instance = nullptr;

CommandsDispatcher::CommandsDispatcher(MainLoop& main_loop) :
  main_loop(main_loop)
{
  instance = this;
}

CommandsDispatcher& CommandsDispatcher::get() {
  Debug::check_assertion(instance, "No current CommandsDispatcher");
  return *instance;
}

void CommandsDispatcher::notify_input(const InputEvent& event) {
  for(const WeakCommands& wptr : commands) {
    CommandsPtr ptr = wptr.lock();
    ptr->notify_input(event);
  }
}

CommandsPtr CommandsDispatcher::create_commands_from_game(Game& game) {
  CommandsPtr commands =  std::make_shared<Commands>(main_loop, game);
  add_commands(commands);
  return commands;
}

CommandsPtr CommandsDispatcher::create_commands_from_default() {
  CommandsPtr commands = std::make_shared<Commands>(main_loop);
  add_commands(commands);
  return commands;
}

void CommandsDispatcher::add_commands(const WeakCommands& cmds) {
  commands.push_back(cmds);
}

void CommandsDispatcher::remove_commands(const WeakCommands& cmds)
{
  commands.erase(
        std::remove_if(
          commands.begin(),
          commands.end(),
          [&](const WeakCommands& other){
            return other.lock() == cmds.lock();
          }),
        commands.end()
        );
}

}


