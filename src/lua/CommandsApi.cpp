#include "solarus/lua/LuaContext.h"
#include "solarus/lua/LuaTools.h"

#include "solarus/core/CommandsDispatcher.h"

namespace Solarus {

const std::string LuaContext::commands_module_name = "sol.commands";

void LuaContext::register_commands_module() {

  // Functions of sol.commands
  const std::vector<luaL_Reg> functions = {
    { "create_from_keyboard", commands_api_create_from_keyboard},
    { "create_from_joypad", commands_api_create_from_joypad}
  };

  // Methods of the commands type.
  const std::vector<luaL_Reg> methods = {
    { "is_pressed", commands_api_is_pressed},
    { "get_direction", commands_api_get_direction},
    { "set_binding", commands_api_set_binding},
    { "get_binding", commands_api_get_binding},
    { "capture_bindings", commands_api_capture_bindings},
    { "simulate_pressed", commands_api_simulate_pressed},
    { "simulate_released", commands_api_simulate_released}
  };

  // Metamethods of the commands type
  const std::vector<luaL_Reg> metamethods = {
      { "__gc", userdata_meta_gc },
      { "__newindex", userdata_meta_newindex_as_table },
      { "__index", userdata_meta_index_as_table },
  };

  register_type(commands_module_name, functions, methods, metamethods);
}

/**
 * @brief push a commands userdata on the lua stack
 * @param current_l lua state
 * @param joypad
 */
void LuaContext::push_commands(lua_State* current_l, Commands& commands) {
  push_userdata(current_l, commands);
}

/**
 * @brief check wheter a value on the stack is a joypad
 * @param current_l lua state
 * @param index index on the stack
 * @return
 */
bool LuaContext::is_commands(lua_State* current_l, int index) {
  return is_userdata(current_l,index, commands_module_name);
}

/**
 * @brief return a Shared Joypad from the lua stack
 * @param current_l lua state
 * @param index index on the stack
 * @return a joypad if index point to a true joypad
 */
std::shared_ptr<Commands> LuaContext::check_commands(lua_State* current_l, int index) {
  return std::static_pointer_cast<Commands>(check_userdata(
    current_l,index,commands_module_name)
  );
}

/**
 * \brief Implementation of sol.commands.create().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::commands_api_create_from_keyboard(lua_State* l) {
  return state_boundary_handle(l, [&]{
    CommandsPtr cmds = CommandsDispatcher::get().create_commands_from_keyboard();

    push_commands(l, *cmds);
    return 1;
  });
}

/**
 * \brief Implementation of sol.commands.create().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::commands_api_create_from_joypad(lua_State* l) {
  return state_boundary_handle(l, [&]{
    JoypadPtr joypad = check_joypad(l, 1);

    CommandsPtr cmds = CommandsDispatcher::get().create_commands_from_joypad(joypad);

    push_commands(l, *cmds);
    return 1;
  });
}

/**
 * \brief Implementation of command:is_pressed
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::commands_api_is_pressed(lua_State* l) {
  return state_boundary_handle(l, [&]{
    Commands& cmds = *check_commands(l, 1);
    Command cmd = check_command(l, 2);

    lua_pushboolean(l, cmds.is_command_pressed(cmd));
    return 1;
  });
}

/**
 * \brief Implementation of command:get_direction
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::commands_api_get_direction(lua_State* l) {
  return state_boundary_handle(l, [&]{
    Commands& cmds = *check_commands(l, 1);

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
 * \brief Implementation of command:set_binding
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::commands_api_set_binding(lua_State* l) {
  return state_boundary_handle(l, [&]{
    Commands& cmds = *check_commands(l, 1);
    Command cmd = check_command(l, 2);

    const std::string& key_name = LuaTools::opt_string(l, 3, "");

    InputEvent::KeyboardKey key = name_to_enum(key_name, InputEvent::KeyboardKey::NONE);
    if (!key_name.empty() && key == InputEvent::KeyboardKey::NONE) {
      LuaTools::arg_error(l, 3,
          std::string("Invalid keyboard key name: '") + key_name + "'");
    }
    cmds.set_keyboard_binding(cmd, key);

    return 0;
  });
}

/**
 * \brief Implementation of commands:get_binding
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::commands_api_get_binding(lua_State* l) {
  return state_boundary_handle(l, [&]{
    Commands& cmds = *check_commands(l, 1);
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
 * \brief Implementation of commands:capture_bindings
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::commands_api_capture_bindings(lua_State* l) {
  return state_boundary_handle(l, [&]{
    Commands& cmds = *check_commands(l, 1);
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
int LuaContext::commands_api_simulate_pressed(lua_State* l) {
  return state_boundary_handle(l, [&]{
    Commands& cmds = *check_commands(l, 1);
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
int LuaContext::commands_api_simulate_released(lua_State* l) {
  return state_boundary_handle(l, [&]{
    Commands& cmds = *check_commands(l, 1);
    Command command = check_command(l, 2);

    cmds.command_released(command);
    return 0;
  });
}

/**
 * @brief Push a command, custom or not, on the lua stack
 * @param l the lua state
 * @param command the command to push
 */
void LuaContext::push_command(lua_State* l, const Command& command) {
    std::string id = std::visit(overloaded{
        [](const CommandId& id) {
            return enum_to_name(id);
        },
        [](const CustomCommandId& id) {
            return id.id;
        }
    }, command);

    push_string(l, id);
}

/**
 * @brief Check a command object on the stack
 * @param l the lua state
 * @param index index of the object on the stack
 * @return command sum type
 */
Command LuaContext::check_command(lua_State* l, int index) {
    std::string name = LuaTools::check_string(l, index);

    CommandId id = name_to_enum(name, CommandId::NONE);
    if(id != CommandId::NONE) {
        return id;
    } else {
        return CustomCommandId{name};
    }
}

} //Solarus

