#include "solarus/core/Joypad.h"
#include "solarus/lua/LuaContext.h"

namespace Solarus {

const std::string EnumInfoTraits<JoyPadAxis>::pretty_name = "joypad axis";
const std::string EnumInfoTraits<JoyPadButton>::pretty_name = "joypad button";
const EnumInfo<JoyPadButton>::names_type EnumInfoTraits<JoyPadButton>::names = {
  {JoyPadButton::A, "A"},
  {JoyPadButton::B, "B"},
  {JoyPadButton::X, "X"},
  {JoyPadButton::Y, "Y"},
  {JoyPadButton::BACK, "back"},
  {JoyPadButton::GUIDE, "guide"},
  {JoyPadButton::START, "start"},
  {JoyPadButton::LEFT_STICK, "left_stick"},
  {JoyPadButton::RIGHT_STICK, "right_stick"},
  {JoyPadButton::LEFT_SHOULDER, "left_shoulder"},
  {JoyPadButton::RIGHT_SHOULDER, "right_shoulder"},
  {JoyPadButton::DPAD_UP, "dpad_up"},
  {JoyPadButton::DPAD_DOWN, "dpad_down"},
  {JoyPadButton::DPAD_LEFT, "dpad_left"},
  {JoyPadButton::DPAD_RIGHT, "dpad_right"},
};

const EnumInfo<JoyPadAxis>::names_type EnumInfoTraits<JoyPadAxis>::names = {
  {JoyPadAxis::LEFT_X, "left_x"},
  {JoyPadAxis::LEFT_Y, "left_y"},
  {JoyPadAxis::RIGHT_X, "right_x"},
  {JoyPadAxis::RIGHT_Y, "right_y"},
  {JoyPadAxis::TRIGGER_LEFT, "trigger_left"},
  {JoyPadAxis::TRIGGER_RIGHT, "trigger_left"},
};

Joypad::Joypad(SDL_GameController *sdl_gc, SDL_Joystick *sdl_js) :
  controller(sdl_gc), joystick(sdl_js)
{
  haptic.reset(SDL_HapticOpenFromJoystick(sdl_js));
  if(haptic) {
    if(SDL_HapticRumbleInit(haptic.get()) != 0) {
      haptic = nullptr;
    }
  }
}

bool Joypad::is_button_pressed(JoyPadButton button) const {
  return SDL_GameControllerGetButton(controller.get(),(SDL_GameControllerButton)button);
}

double Joypad::get_axis(JoyPadAxis axis) const {
  return SDL_GameControllerGetAxis(controller.get(),
                                   (SDL_GameControllerAxis)axis);
}

std::string Joypad::get_name() const {
  return std::string(SDL_GameControllerName(controller.get()));
}

void Joypad::rumble(float intensity, uint32_t time) {
  if(haptic) {
    SDL_HapticRumblePlay(haptic.get(),
                         intensity,
                         time);
  }
}

bool Joypad::has_rumble() {
  return static_cast<bool>(haptic);
}

bool Joypad::is_attached() {
  return static_cast<bool>(controller);
}

void Joypad::reset() {
  controller.reset();
  joystick.reset();
  haptic.reset();
}

const std::string& Joypad::get_lua_type_name() const {
  return LuaContext::joypad_module_name;
}

double Joypad::computeAxisVal(int16_t axis) {
  constexpr int16_t deadzone = 8000;
  constexpr double factor = 1.0 / 32767.0;

  if(std::abs(axis) < deadzone) {
    return 0.0;
  }

  return axis*factor;
}

}
