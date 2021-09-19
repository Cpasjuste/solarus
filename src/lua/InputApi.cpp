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
#include "solarus/core/CurrentQuest.h"
#include "solarus/core/InputEvent.h"
#include "solarus/core/Rectangle.h"
#include "solarus/graphics/Video.h"
#include "solarus/lua/LuaContext.h"
#include "solarus/lua/LuaTools.h"

namespace Solarus {

/**
 * Name of the Lua table representing the input module.
 */
const std::string LuaContext::input_module_name = "sol.input";

/**
 * \brief Initializes the input features provided to Lua.
 */
void LuaContext::register_input_module() {

  std::vector<luaL_Reg> functions = {
      { "is_joypad_enabled", input_api_is_joypad_enabled },
      { "set_joypad_enabled", input_api_set_joypad_enabled },
      { "is_key_pressed", input_api_is_key_pressed },
      { "get_key_modifiers", input_api_get_key_modifiers },
      { "is_joypad_button_pressed", input_api_is_joypad_button_pressed },
      { "get_joypad_axis_state", input_api_get_joypad_axis_state },
      { "get_joypad_hat_direction", input_api_get_joypad_hat_direction },
      { "is_mouse_button_pressed", input_api_is_mouse_button_pressed },
      { "get_mouse_position", input_api_get_mouse_position },
  };
  if (CurrentQuest::is_format_at_least({ 1, 6 })) {
    functions.insert(functions.end(), {
        { "is_finger_pressed", input_api_is_finger_pressed },
        { "get_finger_position", input_api_get_finger_position },
        { "get_finger_pressure", input_api_get_finger_pressure },
        { "simulate_key_pressed", input_api_simulate_key_pressed },
        { "simulate_key_released", input_api_simulate_key_released },
                       //TODO put in 1.7 functions
        { "get_joypad_count", input_api_get_joypad_count},
        { "get_joypads", input_api_get_joypads}
    });
  }

  register_functions(input_module_name, functions);

  // ...
  lua_getglobal(current_l, "sol");
  // ... sol
  lua_getfield(current_l, -1, "input");
  // ... sol input
  lua_setfield(current_l, LUA_REGISTRYINDEX, input_module_name.c_str());
  // ... sol
  lua_pop(current_l, 1);
  // ...
}

void LuaContext::push_input(lua_State *current_l) {
  lua_getfield(current_l, LUA_REGISTRYINDEX, input_module_name.c_str());
}

/**
 * \brief Implementation of sol.input.is_joypad_enabled().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::input_api_is_joypad_enabled(lua_State* l) {

  return state_boundary_handle(l, [&] {
    lua_pushboolean(l, InputEvent::is_legacy_joypad_enabled());
    return 1;
  });
}

/**
 * \brief Implementation of sol.input.set_joypad_enabled().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::input_api_set_joypad_enabled(lua_State* l) {

  return state_boundary_handle(l, [&] {
    bool joypad_enabled = LuaTools::opt_boolean(l, 2, true);

    InputEvent::set_legacy_joypad_enabled(joypad_enabled);

    return 0;
  });
}

/**
 * \brief Implementation of sol.input.is_key_pressed().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::input_api_is_key_pressed(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const std::string& key_name = LuaTools::check_string(l, 1);
    InputEvent::KeyboardKey key = name_to_enum(key_name, InputEvent::KeyboardKey::NONE);

    if (key == InputEvent::KeyboardKey::NONE) {
      LuaTools::arg_error(l, 1, std::string(
          "Unknown keyboard key name: '") + key_name + "'");
    }

    lua_pushboolean(l, InputEvent::is_key_down(key));
    return 1;
  });
}

/**
 * \brief Implementation of sol.input.get_key_modifiers().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::input_api_get_key_modifiers(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const bool shift = InputEvent::is_shift_down();
    const bool control = InputEvent::is_control_down();
    const bool alt = InputEvent::is_alt_down();
    const bool caps_lock = InputEvent::is_caps_lock_on();
    const bool num_lock = InputEvent::is_num_lock_on();

    lua_newtable(l);
    if (shift) {
      lua_pushboolean(l, 1);
      lua_setfield(l, -2, "shift");
    }
    if (control) {
      lua_pushboolean(l, 1);
      lua_setfield(l, -2, "control");
    }
    if (alt) {
      lua_pushboolean(l, 1);
      lua_setfield(l, -2, "alt");
    }
    if (caps_lock) {
      lua_pushboolean(l, 1);
      lua_setfield(l, -2, "caps lock");
    }
    if (num_lock) {
      lua_pushboolean(l, 1);
      lua_setfield(l, -2, "num lock");
    }

    return 1;
  });
}

/**
 * \brief Implementation of sol.input.is_joypad_button_pressed().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::input_api_is_joypad_button_pressed(lua_State* l) {

  return state_boundary_handle(l, [&] {
    int button = LuaTools::check_int(l, 1);

    lua_pushboolean(l, InputEvent::is_joypad_button_down(button));
    return 1;
  });
}

/**
 * \brief Implementation of sol.input.get_joypad_axis_state().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::input_api_get_joypad_axis_state(lua_State* l) {

  return state_boundary_handle(l, [&] {
    int axis = LuaTools::check_int(l, 1);

    lua_pushinteger(l, InputEvent::get_joypad_axis_state(axis));
    return 1;
  });
}

/**
 * \brief Implementation of sol.input.get_joypad_hat_direction().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::input_api_get_joypad_hat_direction(lua_State* l) {

  return state_boundary_handle(l, [&] {
    int hat = LuaTools::check_int(l, 1);

    lua_pushinteger(l, InputEvent::get_joypad_hat_direction(hat));
    return 1;
  });
}

/**
 * \brief Implementation of sol.input.is_mouse_button_pressed().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::input_api_is_mouse_button_pressed(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const std::string& button_name = LuaTools::check_string(l, 1);
    InputEvent::MouseButton button = name_to_enum(button_name, InputEvent::MouseButton::NONE);

    if (button == InputEvent::MouseButton::NONE) {
      LuaTools::arg_error(l, 1, std::string(
          "Unknown mouse button name: '") + button_name + "'");
    }

    lua_pushboolean(l, InputEvent::is_mouse_button_down(button));
    return 1;
  });
}

/**
 * \brief Implementation of sol.input.get_mouse_position().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::input_api_get_mouse_position(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const Point mouse_xy = InputEvent::get_global_mouse_position();

    lua_pushinteger(l, mouse_xy.x);
    lua_pushinteger(l, mouse_xy.y);
    return 2;
  });
}

/**
 * \brief Implementation of sol.input.is_finger_pressed().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::input_api_is_finger_pressed(lua_State* l) {

  return state_boundary_handle(l, [&] {
    int finger_id = LuaTools::check_int(l, 1);

    lua_pushboolean(l, InputEvent::is_finger_down(finger_id));
    return 1;
  });
}

/**
 * \brief Implementation of sol.input.get_finger_position().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::input_api_get_finger_position(lua_State* l) {

  return state_boundary_handle(l, [&] {
    int finger_id = LuaTools::check_int(l, 1);
    Point finger_xy;

    if (!InputEvent::get_global_finger_position(finger_id, finger_xy)) {
      lua_pushnil(l);
      return 1;
    }

    lua_pushinteger(l, finger_xy.x);
    lua_pushinteger(l, finger_xy.y);
    return 2;
  });
}

/**
 * \brief Implementation of sol.input.get_finger_pressure().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::input_api_get_finger_pressure(lua_State* l) {

  return state_boundary_handle(l, [&] {

    int finger_id = LuaTools::check_int(l, 1);
    float finger_pressure;
    if (!InputEvent::get_global_finger_pressure(finger_id, finger_pressure)) {
      lua_pushnil(l);
      return 1;
    }

    lua_pushnumber(l, finger_pressure);
    return 1;
  });
}

/**
 * \brief Implementation of sol.input.simulate_key_pressed().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::input_api_simulate_key_pressed(lua_State* l) {

  return state_boundary_handle(l, [&] {

    const std::string& key_name = LuaTools::check_string(l, 1);
    InputEvent::KeyboardKey key = name_to_enum(key_name, InputEvent::KeyboardKey::NONE);

    if (key == InputEvent::KeyboardKey::NONE) {
      LuaTools::arg_error(l, 1, std::string(
          "Unknown keyboard key name: '") + key_name + "'");
    }

    InputEvent::simulate_key_pressed(key);
    return 0;
  });
}

/**
 * \brief Implementation of sol.input.simulate_key_released().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::input_api_simulate_key_released(lua_State* l) {

  return state_boundary_handle(l, [&] {

    const std::string& key_name = LuaTools::check_string(l, 1);
    InputEvent::KeyboardKey key = name_to_enum(key_name, InputEvent::KeyboardKey::NONE);

    if (key == InputEvent::KeyboardKey::NONE) {
      LuaTools::arg_error(l, 1, std::string(
          "Unknown keyboard key name: '") + key_name + "'");
    }

    InputEvent::simulate_key_released(key);
    return 0;
  });
}

/**
 * \brief Implementation of sol.input.get_joypad_count().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::input_api_get_joypad_count(lua_State* l) {
  return state_boundary_handle(l,[&]{
    lua_pushnumber(l,InputEvent::get_jopad_count());
    return 1;
  });
}

/**
 * \brief Implementation of sol.input.get_joypads().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::input_api_get_joypads(lua_State* l) {
  return state_boundary_handle(l,[&]{
    lua_newtable(l); // t
    int i = 1;
    for(const auto& pair : InputEvent::get_joypads()) {
      lua_pushnumber(l,i); // t i
      push_joypad(l,*pair.second); // t i j
      lua_settable(l,-3); // t
      i++;
    }
    return 1;
  });
}

// Events
void LuaContext::input_on_joypad_connected(Joypad& joy) {
  check_callback_thread();
  push_input(current_l); //push_input(current_l);
  if(find_method("on_joypad_connected")) {
    push_joypad(current_l, joy);
    call_function(2, 0, "on_joypad_connected");
  }
  lua_pop(current_l,1);
}

}

