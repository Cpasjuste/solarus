#include "solarus/core/Command.h"
#include "solarus/core/Commands.h"

namespace Solarus {
  const std::string EnumInfoTraits<CommandId>::pretty_name = "command";

  const EnumInfo<CommandId>::names_type EnumInfoTraits<CommandId>::names = {
    { CommandId::NONE, "" },
    { CommandId::ACTION, "action" },
    { CommandId::ATTACK, "attack" },
    { CommandId::ITEM_1, "item_1" },
    { CommandId::ITEM_2, "item_2" },
    { CommandId::PAUSE, "pause" },
    { CommandId::RIGHT, "right" },
    { CommandId::UP, "up" },
    { CommandId::LEFT, "left" },
    { CommandId::DOWN, "down" }
  };

  const std::string EnumInfoTraits<CommandAxisId>::pretty_name = "command";

  const EnumInfo<CommandAxisId>::names_type EnumInfoTraits<CommandAxisId>::names = {
    { CommandAxisId::NONE, "" },
    { CommandAxisId::X, "X"},
    { CommandAxisId::Y, "Y"},
  };


  std::string CommandEvent::get_command_or_axis_name() const {
      return std::visit(overloaded{
                [&](const CommandPressed& cp) {
                    return Commands::get_command_name(cp.command);
                },
                [&](const CommandReleased& cp) {
                    return Commands::get_command_name(cp.command);
                },
                [&](const CommandAxisMoved& am) {
                    return Commands::get_axis_name(am.axis);
                }
            }, data);
  }
}
