#include "solarus/lua/LuaContext.h"
#include "solarus/lua/LuaTools.h"

#include "solarus/core/CommandsDispatcher.h"

namespace Solarus {

const std::string LuaContext::commands_module_name = "sol.commands";

void LuaContext::register_commands_module() {

  // Functions of sol.commands
  const std::vector<luaL_Reg> functions = {
    {"create", commands_api_create}
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
int LuaContext::commands_api_create(lua_State* l) {
  return state_boundary_handle(l, [&]{
    CommandsPtr cmds = CommandsDispatcher::get().create_commands_from_default();

    push_commands(l, *cmds);
    return 1;
  });
}

/**
 * \brief Implementation of sol.commands.create().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::commands_api_is_pressed(lua_State* l) {
  return state_boundary_handle(l, [&]{

  });
}

/**
 * \brief Implementation of sol.commands.create().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::commands_api_get_direction(lua_State* l) {
  return state_boundary_handle(l, [&]{

  });
}

/**
 * \brief Implementation of sol.commands.create().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::commands_api_set_binding(lua_State* l) {
  return state_boundary_handle(l, [&]{

  });
}

/**
 * \brief Implementation of sol.commands.create().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::commands_api_get_binding(lua_State* l) {
  return state_boundary_handle(l, [&]{

  });
}

/**
 * \brief Implementation of sol.commands.create().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::commands_api_capture_bindings(lua_State* l) {
  return state_boundary_handle(l, [&]{

  });
}

/**
 * \brief Implementation of sol.commands.create().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::commands_api_simulate_pressed(lua_State* l) {
  return state_boundary_handle(l, [&]{

  });
}

/**
 * \brief Implementation of sol.commands.create().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::commands_api_simulate_released(lua_State* l) {
  return state_boundary_handle(l, [&]{

  });
}

} //Solarus

