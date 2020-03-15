#include "solarus/lua/LuaContext.h"
#include "solarus/lua/LuaTools.h"

#include "solarus/core/ControlsDispatcher.h"

namespace Solarus {

const std::string LuaContext::controls_module_name = "sol.controls";

void LuaContext::register_controls_module() {

  // Functions of sol.commands
  const std::vector<luaL_Reg> functions = {
    { "create_from_keyboard", controls_api_create_from_keyboard},
    { "create_from_joypad", controls_api_create_from_joypad},
    { "set_analog_commands_enabled", controls_api_set_analog_commands_enabled},
    { "are_analog_commands_enabled", controls_api_are_analog_commands_enabled}
  };

  // Methods of the commands type.
  const std::vector<luaL_Reg> methods = {
    { "is_pressed", controls_api_is_pressed},
    { "get_axis_state", controls_api_get_axis_state},
    { "get_direction", controls_api_get_direction},
    { "set_keyboard_binding", controls_api_set_keyboard_binding},
    { "get_keyboard_binding", controls_api_get_keyboard_binding},
    { "set_joypad_binding", controls_api_set_joypad_binding},
    { "get_joypad_binding", controls_api_get_joypad_binding},
    { "get_keyboard_axis_binding", controls_api_get_keyboard_axis_binding},
    { "set_keyboard_axis_binding", controls_api_set_keyboard_axis_binding},
    { "set_joypad_axis_binding", controls_api_set_joypad_axis_binding},
    { "get_joypad_axis_binding", controls_api_get_joypad_axis_binding},
    { "capture_bindings", controls_api_capture_bindings},
    { "simulate_pressed", controls_api_simulate_pressed},
    { "simulate_released", controls_api_simulate_released},
    { "simulate_axis_moved", controls_api_simulate_axis_moved}
  };

  // Metamethods of the commands type
  const std::vector<luaL_Reg> metamethods = {
    { "__gc", userdata_meta_gc },
    { "__newindex", userdata_meta_newindex_as_table },
    { "__index", userdata_meta_index_as_table },
  };

  register_type(controls_module_name, functions, methods, metamethods);
}

/**
 * @brief push a commands userdata on the lua stack
 * @param current_l lua state
 * @param joypad
 */
void LuaContext::push_controls(lua_State* current_l, Controls& commands) {
  push_userdata(current_l, commands);
}

/**
 * @brief check wheter a value on the stack is a joypad
 * @param current_l lua state
 * @param index index on the stack
 * @return
 */
bool LuaContext::is_controls(lua_State* current_l, int index) {
  return is_userdata(current_l,index, controls_module_name);
}

/**
 * @brief return a Shared Joypad from the lua stack
 * @param current_l lua state
 * @param index index on the stack
 * @return a joypad if index point to a true joypad
 */
std::shared_ptr<Controls> LuaContext::check_controls(lua_State* current_l, int index) {
  return std::static_pointer_cast<Controls>(check_userdata(
                                              current_l,index,controls_module_name)
                                            );
}

/**
 * \brief Implementation of sol.commands.create().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::controls_api_create_from_keyboard(lua_State* l) {
  return state_boundary_handle(l, [&]{
    ControlsPtr cmds = CommandsDispatcher::get().create_commands_from_keyboard();

    push_controls(l, *cmds);
    return 1;
  });
}

/**
 * \brief Implementation of sol.commands.create().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::controls_api_create_from_joypad(lua_State* l) {
  return state_boundary_handle(l, [&]{
    JoypadPtr joypad = check_joypad(l, 1);

    ControlsPtr cmds = CommandsDispatcher::get().create_commands_from_joypad(joypad);

    push_controls(l, *cmds);
    return 1;
  });
}

/**
 * \brief Implementation of sol.commands.set_analog_commands_enabled().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::controls_api_set_analog_commands_enabled(lua_State* l) {
  return state_boundary_handle(l, [&]{
    bool enabled = LuaTools::check_boolean(l, 1);
    Controls::set_analog_commands_enabled(enabled);
    return 0;
  });
}

/**
 * \brief Implementation of sol.commands.get_analog_commands_enabled().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::controls_api_are_analog_commands_enabled(lua_State* l) {
  return state_boundary_handle(l, [&]{
    bool enabled = Controls::are_analog_commands_enabled();
    lua_pushboolean(l, enabled);
    return 1;
  });
}

/**
 * \brief Implementation of command:is_pressed
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::controls_api_is_pressed(lua_State* l) {
  return state_boundary_handle(l, [&]{
    Controls& cmds = *check_controls(l, 1);
    Command cmd = check_command(l, 2);

    lua_pushboolean(l, cmds.is_command_pressed(cmd));
    return 1;
  });
}

/**
 * \brief Implementation of command:get_axis_state
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::controls_api_get_axis_state(lua_State* l) {
  return state_boundary_handle(l, [&]{
    Controls& cmds = *check_controls(l, 1);
    Axis cmda = check_axis(l, 2);

    lua_pushnumber(l, cmds.get_axis_state(cmda));
    return 1;
  });
}

/**
 * \brief Implementation of command:get_direction
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::controls_api_get_direction(lua_State* l) {
  return state_boundary_handle(l, [&]{
    Controls& cmds = *check_controls(l, 1);

    int wanted_direction8 = cmds.get_wanted_direction8();
    if (wanted_direction8 == -1) {
      lua_pushnil(l);
    }
    else {
      lua_pushinteger(l, wanted_direction8);
    }

    return 1;
  });
}

/**
 * \brief Implementation of command:set_keyboard_binding
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::controls_api_set_keyboard_binding(lua_State* l) {
  return state_boundary_handle(l, [&]{
    Controls& cmds = *check_controls(l, 1);
    Command cmd = check_command(l, 2);

    InputEvent::KeyboardKey key = LuaTools::check_enum<InputEvent::KeyboardKey>(l, 3);

    cmds.set_keyboard_binding(cmd, key);

    return 0;
  });
}

/**
 * \brief Implementation of commands:get_keyboard_binding
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::controls_api_get_keyboard_binding(lua_State* l) {
  return state_boundary_handle(l, [&]{
    Controls& cmds = *check_controls(l, 1);
    Command command = check_command(l, 2);

    InputEvent::KeyboardKey key = cmds.get_keyboard_binding(command);
    const std::string& key_name = enum_to_name(key);

    if (key_name.empty()) {
      lua_pushnil(l);
    } else {
      push_string(l, key_name);
    }
    return 1;
  });
}

/**
 * \brief Implementation of command:set_joypad_binding
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::controls_api_set_joypad_binding(lua_State* l) {
  return state_boundary_handle(l, [&]{
    Controls& cmds = *check_controls(l, 1);
    Command cmd = check_command(l, 2);

    const std::string& key_name = LuaTools::opt_string(l, 3, "");

    Controls::JoypadBinding binding(key_name);

    if(!binding.is_invalid()) {
      cmds.set_joypad_binding(cmd, binding);
    } else {
      LuaTools::error(l, "invalid joypad binding : " + key_name);
    }

    return 0;
  });
}

/**
 * \brief Implementation of commands:get_joypad_binding
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::controls_api_get_joypad_binding(lua_State* l) {
  return state_boundary_handle(l, [&]{
    Controls& cmds = *check_controls(l, 1);
    Command command = check_command(l, 2);

    auto binding = cmds.get_joypad_binding(command);

    if (binding) {
      push_string(l, binding->to_string());
    } else {
      lua_pushnil(l);
    }
    return 1;
  });
}

/**
 * \brief Implementation of command:set_joypad_axis_binding
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::controls_api_set_joypad_axis_binding(lua_State* l) {
  return state_boundary_handle(l, [&]{
    Controls& cmds = *check_controls(l, 1);
    Axis cmd = check_axis(l, 2);

    JoyPadAxis axis = LuaTools::check_enum<JoyPadAxis>(l, 3);

    cmds.set_joypad_axis_binding(cmd, axis);

    return 0;
  });
}

/**
 * \brief Implementation of commands:get_joypad_axis_binding
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::controls_api_get_joypad_axis_binding(lua_State* l) {
  return state_boundary_handle(l, [&]{
    Controls& cmds = *check_controls(l, 1);
    Axis command = check_axis(l, 2);

    auto binding = cmds.get_joypad_axis_binding(command);

    if(binding != JoyPadAxis::INVALID) {
      push_string(l, enum_to_name(binding));
    } else {
      lua_pushnil(l);
    }
    return 1;
  });
}

/**
 * \brief Implementation of command:set_keyboard_axis_binding
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::controls_api_set_keyboard_axis_binding(lua_State* l) {
  return state_boundary_handle(l, [&]{
    Controls& cmds = *check_controls(l, 1);
    Axis cmd = check_axis(l, 2);

    InputEvent::KeyboardKey mkey = LuaTools::check_enum<InputEvent::KeyboardKey>(l, 3);
    InputEvent::KeyboardKey pkey = LuaTools::check_enum<InputEvent::KeyboardKey>(l,4);

    cmds.set_keyboard_axis_binding(cmd, mkey, pkey);

    return 0;
  });
}

/**
 * \brief Implementation of commands:get_keyboard_axis_binding
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::controls_api_get_keyboard_axis_binding(lua_State* l) {
  return state_boundary_handle(l, [&]{
    Controls& cmds = *check_controls(l, 1);
    Axis command = check_axis(l, 2);

    auto [mkey, pkey] = cmds.get_keyboard_axis_binding(command);

    if(mkey != InputEvent::KeyboardKey::NONE){
      push_string(l, enum_to_name(mkey));
    } else {
      lua_pushnil(l);
    }

    if(pkey != InputEvent::KeyboardKey::NONE) {
      push_string(l, enum_to_name(pkey));
    } else {
      lua_pushnil(l);
    }

    return 2;
  });
}

/**
 * \brief Implementation of commands:capture_bindings
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::controls_api_capture_bindings(lua_State* l) {
  return state_boundary_handle(l, [&]{
    Controls& cmds = *check_controls(l, 1);
    Command command = check_command(l, 2);
    const ScopedLuaRef& callback_ref = LuaTools::opt_function(l, 3);

    cmds.customize(command, callback_ref);

    return 0;
  });
}

/**
 * \brief Implementation of commands:simulated_pressed.
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::controls_api_simulate_pressed(lua_State* l) {
  return state_boundary_handle(l, [&]{
    Controls& cmds = *check_controls(l, 1);
    Command command = check_command(l, 2);

    cmds.command_pressed(command);
    return 0;
  });
}

/**
 * \brief Implementation of commands:simulate_released.
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::controls_api_simulate_released(lua_State* l) {
  return state_boundary_handle(l, [&]{
    Controls& cmds = *check_controls(l, 1);
    Command command = check_command(l, 2);

    cmds.command_released(command);
    return 0;
  });
}

/**
 * \brief Implementation of commands:simulate_axis_moved.
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::controls_api_simulate_axis_moved(lua_State* l) {
  return state_boundary_handle(l, [&]{
    Controls& cmds = *check_controls(l, 1);
    Axis command = check_axis(l, 2);
    double state = LuaTools::check_number(l, 3);

    cmds.command_axis_moved(command, state);
    return 0;
  });
}



/**
 * @brief Push a command, custom or not, on the lua stack
 * @param l the lua state
 * @param command the command to push
 */
void LuaContext::push_command(lua_State* l, const Command& command) {
  push_string(l, Controls::get_command_name(command));
}

/**
 * @brief Check a command string on the stack
 * @param l the lua state
 * @param index index of the object on the stack
 * @return command sum type
 */
Command LuaContext::check_command(lua_State* l, int index) {
  std::string name = LuaTools::check_string(l, index);

  return Controls::get_command_by_name(name);
}

/**
 * @brief Push a command axis, custom or not, on the lua stack
 * @param l the lua state
 * @param command the command to push
 */
void LuaContext::push_axis(lua_State* l, const Axis& command) {
  push_string(l, Controls::get_axis_name(command));
}

/**
 * @brief Check a command axis string on the stack
 * @param l the lua state
 * @param index index of the object on the stack
 * @return command sum type
 */
Axis LuaContext::check_axis(lua_State* l, int index) {
  std::string name = LuaTools::check_string(l, index);

  return Controls::get_axis_by_name(name);
}

} //Solarus

