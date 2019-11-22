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
#ifndef SOLARUS_GAME_COMMAND_H
#define SOLARUS_GAME_COMMAND_H

#include "solarus/core/EnumInfo.h"
#include "solarus/core/Common.h"
#include "solarus/core/CommandsPtr.h"

namespace Solarus {

/**
 * \brief The built-in commands recognized by the engine during a game.
 *
 * These high-level commands can be mapped onto the keyboard and the joypad.
 */
enum class Command {
  NONE = -1,
  ACTION,
  ATTACK,
  ITEM_1,
  ITEM_2,
  PAUSE,
  RIGHT,
  UP,
  LEFT,
  DOWN
};


struct CommandEvent {
  enum class Type{
    PRESSED,
    RELEASED
  };

  /**
   * @brief tells if this is a press event
   * @return
   */
  inline bool is_pressed() const {
    return type == Type::PRESSED;
  }

  inline const char* event_name() const {
    return is_pressed() ?
          "on_command_pressed" :
          "on_command_released";
  }

  static CommandEvent make_pressed(Command cmd, const CommandsPtr& emitter) {
    return CommandEvent(Type::PRESSED, cmd, emitter);
  }

  static CommandEvent make_released(Command cmd, const CommandsPtr& emitter) {
    return CommandEvent(Type::RELEASED, cmd, emitter);
  }

  inline bool is_from(const CommandsPtr& other) const {
    return emitter == other;
  }

  CommandEvent(Type type, Command cmd, const CommandsPtr& emitter) :
    type(type),
    name(cmd),
    emitter(emitter)
  {}

  Type type;
  Command name;
  CommandsPtr emitter;
};

template <>
struct SOLARUS_API EnumInfoTraits<Command> {
  static const std::string pretty_name;
  static const EnumInfo<Command>::names_type names;
};

}

#endif
