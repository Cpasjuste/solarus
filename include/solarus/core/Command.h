/*
 * Copyright (C) 2018-2019 Christopho, Solarus - http://www.solarus-games.org
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
#include "solarus/core/ControlsPtr.h"

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
struct CustomId{
    std::string id;

    inline bool operator!=(const CustomId& other) const {
        return id != other.id;
    }

    inline bool operator==(const CustomId& other) const {
        return id == other.id;
    }

    inline bool operator<(const CustomId& other) const {
        return id < other.id;
    }
};

using Command = std::variant<CommandId, CustomId>;

enum class AxisId{
    NONE = -1,
    X,
    Y
};

using Axis = std::variant<AxisId, CustomId>;

struct CommandButton{
    Command command;
};

struct CommandPressed : public CommandButton {};
struct CommandReleased : public CommandButton {};

struct AxisMoved{
    Axis axis;
    double state;
};

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

struct ControlEvent {
  using Data = std::variant<CommandPressed, CommandReleased, AxisMoved>;
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
      return std::holds_alternative<AxisMoved>(data);
  }

  static CommandId command_to_id(const Command& cmd) {
      return std::visit(overloaded{
            [&](const CommandId& id) {
                return id;
            },
            [&](const CustomId&) {
                return CommandId::NONE;
            }
        }, cmd);
  }

  inline Command get_command() const {
      return std::visit(overloaded{
                [&](const CommandButton& cp) {
                    return cp.command;
                },
                [&](const AxisMoved&) {
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
            return "on_axis_moved";
    }
  }

  std::string get_command_or_axis_name() const;

  inline double get_axis_state() const {
      return std::visit(overloaded{
            [](auto) {
                return 0.0;
            },
            [](const AxisMoved& am) {
                return am.state;
            }
        }, data);
  }

  static ControlEvent make_pressed(const Command& cmd, const ControlsPtr& emitter) {
      return ControlEvent(CommandPressed{{cmd}}, emitter);
  }

  static ControlEvent make_released(const Command& cmd, const ControlsPtr& emitter) {
      return ControlEvent(CommandReleased{{cmd}}, emitter);
  }

  static ControlEvent make_moved(const Axis& axis, double state, const ControlsPtr& emitter) {
      return ControlEvent(AxisMoved{axis, state}, emitter);
  }

  inline bool is_from(const ControlsPtr& other) const {
    return emitter == other;
  }

  ControlEvent(const Data& data, const ControlsPtr& emitter) :
      data(data),
      emitter(emitter)
  {}

  Data data;
  ControlsPtr emitter;
};

template <>
struct SOLARUS_API EnumInfoTraits<CommandId> {
  static const std::string pretty_name;
  static const EnumInfo<CommandId>::names_type names;
};

template <>
struct SOLARUS_API EnumInfoTraits<AxisId> {
  static const std::string pretty_name;
  static const EnumInfo<AxisId>::names_type names;
};

}

#endif
