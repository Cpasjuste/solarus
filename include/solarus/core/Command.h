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

#include <variant>

namespace Solarus {

/**
 * \brief The built-in commands recognized by the engine during a game.
 *
 * These high-level commands can be mapped onto the keyboard and the joypad.
 */
enum class CommandId {
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

/**
 * @brief Struct holding custom command name
 */
struct CustomCommandId{
    std::string id;

    inline bool operator!=(const CustomCommandId& other) const {
        return id != other.id;
    }

    inline bool operator==(const CustomCommandId& other) const {
        return id == other.id;
    }

    inline bool operator<(const CustomCommandId& other) const {
        return id < other.id;
    }
};

using Command = std::variant<CommandId, CustomCommandId>;

enum class CommandAxisId{
    NONE = -1,
    X,
    Y
};

using CommandAxis = std::variant<CommandAxisId, CustomCommandId>;

struct CommandButton{
    Command command;
};

struct CommandPressed : public CommandButton {};
struct CommandReleased : public CommandButton {};

struct CommandAxisMoved{
    CommandAxis axis;
    double state;
};

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

struct CommandEvent {
  using Data = std::variant<CommandPressed, CommandReleased, CommandAxisMoved>;
  /**
   * @brief tells if this is a press event
   * @return
   */
  inline bool is_pressed() const {
    return std::holds_alternative<CommandPressed>(data);
  }

  inline bool is_released() const {
      return std::holds_alternative<CommandReleased>(data);
  }

  inline bool is_moved() const {
      return std::holds_alternative<CommandAxisMoved>(data);
  }

  static CommandId command_to_id(const Command& cmd) {
      return std::visit(overloaded{
            [&](const CommandId& id) {
                return id;
            },
            [&](const CustomCommandId&) {
                return CommandId::NONE;
            }
        }, cmd);
  }

  inline Command get_command() const {
      return std::visit(overloaded{
                [&](const CommandButton& cp) {
                    return cp.command;
                },
                [&](const CommandAxisMoved&) {
                    return Command(CommandId::NONE);
                }
            }, data);
  }

  inline CommandId get_command_id() const {
      return command_to_id(get_command());
  }

  inline Command get_pressed_command() const {
      return std::get<CommandPressed>(data).command;
  }

  inline Command get_released_command() const {
      return std::get<CommandReleased>(data).command;
  }

  inline const char* event_name() const {
    switch(data.index()) {
        case 0:
            return "on_command_pressed";
        case 1:
            return "on_command_released";
        default:
            return "on_command_moved";
    }
  }

  std::string get_command_or_axis_name() const;

  inline double get_axis_state() const {
      return std::visit(overloaded{
            [](auto) {
                return 0.0;
            },
            [](const CommandAxisMoved& am) {
                return am.state;
            }
        }, data);
  }

  static CommandEvent make_pressed(const Command& cmd, const CommandsPtr& emitter) {
      return CommandEvent(CommandPressed{cmd}, emitter);
  }

  static CommandEvent make_released(const Command& cmd, const CommandsPtr& emitter) {
      return CommandEvent(CommandReleased{cmd}, emitter);
  }

  static CommandEvent make_moved(const CommandAxis& axis, double state, const CommandsPtr& emitter) {
      return CommandEvent(CommandAxisMoved{axis, state}, emitter);
  }

  inline bool is_from(const CommandsPtr& other) const {
    return emitter == other;
  }

  CommandEvent(const Data& data, const CommandsPtr& emitter) :
      data(data),
      emitter(emitter)
  {}

  Data data;
  CommandsPtr emitter;
};

template <>
struct SOLARUS_API EnumInfoTraits<CommandId> {
  static const std::string pretty_name;
  static const EnumInfo<CommandId>::names_type names;
};

template <>
struct SOLARUS_API EnumInfoTraits<CommandAxisId> {
  static const std::string pretty_name;
  static const EnumInfo<CommandAxisId>::names_type names;
};

}

#endif
