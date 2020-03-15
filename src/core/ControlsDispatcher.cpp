#include "solarus/core/ControlsDispatcher.h"

#include "solarus/core/Controls.h"

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
    ControlsPtr ptr = wptr.lock();
    ptr->notify_input(event);
  }
}

ControlsPtr CommandsDispatcher::create_commands_from_game(Game& game) {
  ControlsPtr commands =  std::make_shared<Controls>(main_loop, game);
  add_commands(commands);
  return commands;
}

ControlsPtr CommandsDispatcher::create_commands_from_keyboard() {
    ControlsPtr commands = std::make_shared<Controls>(main_loop);
    add_commands(commands);

    commands->load_default_keyboard_bindings();

    return commands;
}

ControlsPtr CommandsDispatcher::create_commands_from_joypad(const JoypadPtr& joypad) {
    ControlsPtr commands = std::make_shared<Controls>(main_loop);
    add_commands(commands);

    commands->load_default_joypad_bindings();
    commands->set_joypad(joypad);

    return commands;
}

void CommandsDispatcher::add_commands(const WeakCommands& cmds) {
  commands.push_back(cmds);
}

void CommandsDispatcher::remove_commands(const Controls* cmds)
{
  commands.erase(
        std::remove_if(
          commands.begin(),
          commands.end(),
          [&](const WeakCommands& other){
            return other.lock().get() == cmds;
          }),
        commands.end()
        );
}

}


