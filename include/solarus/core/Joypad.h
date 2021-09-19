/*
 * Copyright (C) 2018-2020 std::gregwar, Solarus - http://www.solarus-games.org
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

#pragma once

#include "solarus/lua/ExportableToLua.h"
#include "solarus/core/EnumInfo.h"

#include <memory>
#include <SDL_events.h>
#include <SDL_joystick.h>
#include <SDL_haptic.h>

namespace Solarus {
struct SDL_Controller_Deleter {
    void operator()(SDL_GameController* controller) {
        SDL_GameControllerClose(controller);
    }
};

using SDL_GameControllerUniquePtr = std::unique_ptr<SDL_GameController,SDL_Controller_Deleter>;

struct SDL_Haptic_Deleter {
    void operator()(SDL_Haptic* haptic) {
        SDL_HapticClose(haptic);
    }
};

using SDL_HapticUniquePtr = std::unique_ptr<SDL_Haptic,SDL_Haptic_Deleter>;

struct SDL_Joystick_Deleter {
    void operator()(SDL_Joystick* joystick) {
        SDL_JoystickClose(joystick);
    }
};

using SDL_JoystickUniquePtr = std::unique_ptr<SDL_Joystick,SDL_Joystick_Deleter>;

enum class JoyPadAxis {
    INVALID = SDL_CONTROLLER_AXIS_INVALID,
    LEFT_X = SDL_CONTROLLER_AXIS_LEFTX,
    LEFT_Y = SDL_CONTROLLER_AXIS_LEFTY,
    RIGHT_X = SDL_CONTROLLER_AXIS_RIGHTX,
    RIGHT_Y = SDL_CONTROLLER_AXIS_RIGHTY,
    TRIGGER_LEFT = SDL_CONTROLLER_AXIS_TRIGGERLEFT,
    TRIGGER_RIGHT = SDL_CONTROLLER_AXIS_TRIGGERRIGHT,
    MAX = SDL_CONTROLLER_AXIS_MAX
};

enum class JoyPadButton {
    INVALID = SDL_CONTROLLER_BUTTON_INVALID,
    A = SDL_CONTROLLER_BUTTON_A,
    B = SDL_CONTROLLER_BUTTON_B,
    X = SDL_CONTROLLER_BUTTON_X,
    Y = SDL_CONTROLLER_BUTTON_Y,
    BACK = SDL_CONTROLLER_BUTTON_BACK,
    GUIDE = SDL_CONTROLLER_BUTTON_GUIDE,
    START = SDL_CONTROLLER_BUTTON_START,
    LEFT_STICK = SDL_CONTROLLER_BUTTON_LEFTSTICK,
    RIGHT_STICK = SDL_CONTROLLER_BUTTON_RIGHTSTICK,
    LEFT_SHOULDER = SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
    RIGHT_SHOULDER = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
    DPAD_UP = SDL_CONTROLLER_BUTTON_DPAD_UP,
    DPAD_DOWN = SDL_CONTROLLER_BUTTON_DPAD_DOWN,
    DPAD_LEFT = SDL_CONTROLLER_BUTTON_DPAD_LEFT,
    DPAD_RIGHT = SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
    MAX = SDL_CONTROLLER_BUTTON_MAX
};


class Joypad : public ExportableToLua
{
public:
  Joypad(SDL_GameController* sdl_gc, SDL_Joystick* sdl_js);
  double get_axis(JoyPadAxis axis) const;
  bool is_button_pressed(JoyPadButton button) const;
  std::string get_name() const;
  void rumble(float intensity, uint32_t time);
  bool has_rumble();
  bool is_attached();
  void reset();
  const std::string& get_lua_type_name() const override;
  static double computeAxisVal(int16_t axis);
private:
  SDL_GameControllerUniquePtr controller;
  SDL_JoystickUniquePtr joystick;
  SDL_HapticUniquePtr haptic;
};

template <>
struct SOLARUS_API EnumInfoTraits<JoyPadAxis> {
  static const std::string pretty_name;
  static const EnumInfo<JoyPadAxis>::names_type names;
};

template <>
struct SOLARUS_API EnumInfoTraits<JoyPadButton> {
  static const std::string pretty_name;
  static const EnumInfo<JoyPadButton>::names_type names;
};


using JoypadPtr = std::shared_ptr<Joypad>;

}
