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
  return std::make_shared<Commands>(main_loop, game);
}

CommandsPtr CommandsDispatcher::create_commands_from_default() {
  return std::make_shared<Commands>(main_loop);
}

}


