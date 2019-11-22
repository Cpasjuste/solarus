#include "solarus/core/Command.h"

namespace Solarus {
  const std::string EnumInfoTraits<Command>::pretty_name = "command";

  const EnumInfo<Command>::names_type EnumInfoTraits<Command>::names = {
    { Command::NONE, "" },
    { Command::ACTION, "action" },
    { Command::ATTACK, "attack" },
    { Command::ITEM_1, "item_1" },
    { Command::ITEM_2, "item_2" },
    { Command::PAUSE, "pause" },
    { Command::RIGHT, "right" },
    { Command::UP, "up" },
    { Command::LEFT, "left" },
    { Command::DOWN, "down" }
  };
}
