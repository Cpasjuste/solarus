#include "solarus/core/Command.h"
#include "solarus/core/Controls.h"

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

  const std::string EnumInfoTraits<AxisId>::pretty_name = "command axis";

  const EnumInfo<AxisId>::names_type EnumInfoTraits<AxisId>::names = {
    { AxisId::NONE, "" },
    { AxisId::X, "X"},
    { AxisId::Y, "Y"},
  };


  std::string ControlEvent::get_command_or_axis_name() const {
      return std::visit(overloaded{
                [&](const CommandPressed& cp) {
                    return Controls::get_command_name(cp.command);
                },
                [&](const CommandReleased& cp) {
                    return Controls::get_command_name(cp.command);
                },
                [&](const AxisMoved& am) {
                    return Controls::get_axis_name(am.axis);
                }
            }, data);
  }
}
