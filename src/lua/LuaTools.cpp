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
#include "solarus/core/Map.h"
#include "solarus/graphics/Color.h"
#include "solarus/lua/LuaException.h"
#include "solarus/lua/LuaTools.h"
#include "solarus/lua/LuaContext.h"
#include "solarus/lua/ScopedLuaRef.h"
#include <cctype>
#include <sstream>

namespace Solarus {
namespace LuaTools {

/**
 * \brief For an index in the Lua stack, returns an equivalent positive index.
 *
 * Pseudo-indexes are unchanged.
 *
 * \param l A Lua state.
 * \param index An index in the stack.
 * \return The corresponding positive index.
 */
int get_positive_index(lua_State* l, int index) {

  int positive_index = index;
  if (index < 0 && index >= -lua_gettop(l)) {
    positive_index = lua_gettop(l) + index + 1;
  }

  return positive_index;
}

/**
 * \brief Returns whether the specified name is a valid Lua identifier.
 * \param name The name to check.
 * \return \c true if the name only contains alphanumeric characters or '_' and
 * does not start with a digit.
 */
bool is_valid_lua_identifier(const std::string& name) {

  if (name.empty() || std::isdigit(name[0])) {
    return false;
  }

  for (char character: name) {
    if (!std::isalnum(character) && character != '_') {
      return false;
    }
  }
  return true;
}

/**
 * \brief Returns the type name of a value.
 * Similar to the standard Lua function type(), except that for userdata
 * known by Solarus, it returns the exact Solarus type name.
 * \param l A Lua state.
 * \param index An index in the stack.
 * \return The type name.
 */
std::string get_type_name(lua_State*l, int index) {

  std::string module_name;
  if (!LuaContext::is_solarus_userdata(l, index, module_name)) {
    // Return the same thing as the usual Lua type() function.
    return luaL_typename(l, index);
  }

  // This is a Solarus type.
  // Remove the "sol." prefix.
  return module_name.substr(4);
}

/**
 * \brief Creates a reference to the Lua value on top of the stack and pops
 * this value.
 * \param l A Lua state.
 * \return The reference created, wrapped in an object that manages its
 * lifetime.
 */
ScopedLuaRef create_ref(lua_State* l) {
  lua_State* main = LuaContext::get().get_main_state();
  //Cross move values to main state to avoid  coroutines GC to invalidate them
  if(l != main) {
    lua_xmove(l,main,1);
  }
  return ScopedLuaRef(main, luaL_ref(main, LUA_REGISTRYINDEX));
}

/**
 * \brief Creates a reference to the Lua value at the specified index.
 * \param l A Lua state.
 * \param index An index in the Lua stack.
 * \return The reference created, wrapped in an object that manages its
 * lifetime.
 */
ScopedLuaRef create_ref(lua_State* l, int index) {
  lua_pushvalue(l, index);
  return create_ref(l);
}

/**
 * \brief Calls the Lua function with its arguments on top of the stack.
 *
 * This function is like lua_pcall, except that it additionally handles the
 * error message if an error occurs in the Lua code (the error is printed).
 * This function leaves the results on the stack if there is no error,
 * and leaves nothing on the stack in case of error.
 *
 * \param l A Lua state.
 * \param nb_arguments Number of arguments placed on the Lua stack above the
 * function to call.
 * \param nb_results Number of results expected (you get them on the stack if
 * there is no error).
 * \param function_name A name describing the Lua function (only used to print
 * the error message if any).
 * This is not a <tt>const std::string&</tt> but a <tt>const char*</tt> on
 * purpose to avoid costly conversions as this function is called very often.
 * \return \c true in case of success.
 */
bool call_function(
    lua_State* l,
    int nb_arguments,
    int nb_results,
    const char* function_name
) {
  Debug::check_assertion(lua_gettop(l) > nb_arguments, "Missing arguments");
  int base = lua_gettop(l) - nb_arguments;
  lua_pushcfunction(l, &LuaContext::l_backtrace);
  lua_insert(l, base);
  int status = lua_pcall(l, nb_arguments, nb_results, base);
  lua_remove(l, base);
  if (status != 0) {
    Debug::check_assertion(lua_isstring(l, -1), "Missing error message");
    Debug::error(std::string("In ") + function_name + ": "
        + lua_tostring(l, -1)
    );
    lua_pop(l, 1);
    return false;
  }
  return true;
}

/**
 * \brief Similar to luaL_error() but throws a LuaException.
 *
 * This function never returns.
 *
 * \param l A Lua state.
 * \param message Error message.
 */
[[noreturn]] void error(lua_State* l, const std::string& message) {
  throw LuaException(l, message);
}

/**
 * \brief Similar to luaL_argerror() but throws a LuaException.
 *
 * This function never returns.
 *
 * \param l A Lua state.
 * \param arg_index Index of an argument in the stack.
 * \param message Error message.
 */
[[noreturn]] void arg_error(lua_State* l, int arg_index, const std::string& message) {

  // The code below is very similar to luaL_argerror(), but ends with
  // an exception instead of a luaL_error() call.

  std::ostringstream oss;
  lua_Debug info;
  if (!lua_getstack(l, 0, &info)) {
    // No stack frame.
    oss << "bad argument #" << arg_index << " (" << message << ")";
    error(l, oss.str());
  }

  lua_getinfo(l, "n", &info);
  if (std::string(info.namewhat) == "method") {
     arg_index--;  // Do not count self.
     if (arg_index == 0) {
       // Error is in the self argument itself.
       oss << "calling " << info.name << " on bad self (" << message << ")";
       error(l, oss.str());
     }
  }

  if (info.name == nullptr) {
    info.name = "?";
  }
  oss << "bad argument #" << arg_index << " to " << info.name << " (" << message << ")";
  error(l, oss.str());
}

/**
 * \brief Similar to luaL_typerror() but throws a LuaException.
 *
 * This function never returns.
 *
 * \param l A Lua state.
 * \param arg_index Index of an argument in the stack.
 * \param expected_type_name A name describing the type that was expected.
 */
[[noreturn]] void type_error(
    lua_State* l,
    int arg_index,
    const std::string& expected_type_name
) {
  arg_error(l, arg_index, std::string(expected_type_name) +
      " expected, got " + get_type_name(l, arg_index));
}

/**
 * \brief Like luaL_checktype() but throws a LuaException in case of error.
 * \param l A Lua state.
 * \param arg_index Index of an argument in the stack.
 * \param expected_type A Lua type value.
 */
void check_type(
    lua_State* l,
    int arg_index,
    int expected_type
) {
  if (lua_type(l, arg_index) != expected_type) {
    arg_error(l, arg_index, std::string(lua_typename(l, expected_type)) +
        " expected, got " + get_type_name(l, arg_index));
  }
}

/**
 * \brief Like luaL_checkany() but throws a LuaException in case of error.
 * \param l A Lua state.
 * \param arg_index Index of an argument in the stack.
 */
void check_any(
    lua_State* l,
    int arg_index
) {
  if (lua_type(l, arg_index) == LUA_TNONE) {
    arg_error(l, arg_index, "value expected");
  }
}

/**
 * \brief Checks that a value is a number and returns it as an integer.
 *
 * This function acts like luaL_checkint() except that it throws a LuaException
 * in case of error.
 *
 * \param l A Lua state.
 * \param index Index of a table in the stack.
 * \return The value as an integer.
 */
int check_int(
    lua_State* l,
    int index
) {
  if (!lua_isnumber(l, index)) {
    arg_error(l, index,
        std::string("integer expected, got ")
            + get_type_name(l, index) + ")"
    );
  }

  return (int) lua_tointeger(l, index);
}

/**
 * \brief Checks that a table field is a number and returns it as an integer.
 *
 * This function acts like lua_getfield() followed by LuaTools::check_int().
 *
 * \param l A Lua state.
 * \param table_index Index of a table in the stack.
 * \param key Key of the field to get in that table.
 * \return The wanted field as an integer.
 */
int check_int_field(
    lua_State* l,
    int table_index,
    const std::string& key
) {
  lua_getfield(l, table_index, key.c_str());
  if (!lua_isnumber(l, -1)) {
    arg_error(l, table_index,
        std::string("Bad field '") + key + "' (integer expected, got "
        + get_type_name(l, -1) + ")"
    );
  }

  int value = (int) lua_tointeger(l, -1);
  lua_pop(l, 1);
  return value;
}

/**
 * \brief Like LuaTools::check_int() but with a default value.
 *
 * This function acts like luaL_optint() except that it throws a LuaException
 * in case of error.
 *
 * \param l A Lua state.
 * \param index Index of a value in the stack.
 * \param default_value The default value to return if the value is \c nil.
 * \return The wanted value as an integer.
 */
int opt_int(
    lua_State* l,
    int index,
    int default_value
) {
  if (lua_isnoneornil(l, index)) {
    return default_value;
  }
  return check_int(l, index);
}

/**
 * \brief Like LuaTools::check_int_field() but with a default value.
 *
 * This function acts like lua_getfield() followed by luaL_optint().
 *
 * \param l A Lua state.
 * \param table_index Index of a table in the stack.
 * \param key Key of the field to get in that table.
 * \param default_value The default value to return if the field is \c nil.
 * \return The wanted field as an integer.
 */
int opt_int_field(
    lua_State* l,
    int table_index,
    const std::string& key,
    int default_value
) {
  lua_getfield(l, table_index, key.c_str());
  if (lua_isnil(l, -1)) {
    lua_pop(l, 1);
    return default_value;
  }

  if (!lua_isnumber(l, -1)) {
    arg_error(l, table_index,
        std::string("Bad field '") + key + "' (integer expected, got "
        + get_type_name(l, -1) + ")"
    );
  }
  int value = (int) lua_tointeger(l, -1);
  lua_pop(l, 1);
  return value;
}

/**
 * \brief Checks that a value is a number and returns it as a double.
 *
 * This function acts like luaL_checknumber() except that it throws a
 * LuaException in case of error.
 *
 * \param l A Lua state.
 * \param index Index of a value in the stack.
 * \return The number value.
 */
double check_number(
    lua_State* l,
    int index
) {
  if (!lua_isnumber(l, index)) {
    arg_error(l, index,
        std::string("number expected, got ")
            + get_type_name(l, index) + ")"
    );
  }

  return lua_tonumber(l, index);
}

/**
 * \brief Checks that a table field is a number and returns it as a double.
 *
 * This function acts like lua_getfield() followed by LuaTools::check_number().
 *
 * \param l A Lua state.
 * \param table_index Index of a table in the stack.
 * \param key Key of the field to get in that table.
 * \return The wanted field as a double.
 */
double check_number_field(
    lua_State* l,
    int table_index,
    const std::string& key
) {
  lua_getfield(l, table_index, key.c_str());
  if (!lua_isnumber(l, -1)) {
    arg_error(l, table_index,
        std::string("Bad field '") + key + "' (number expected, got "
        + get_type_name(l, -1) + ")"
    );
  }

  double value = lua_tonumber(l, -1);
  lua_pop(l, 1);
  return value;
}

/**
 * \brief Like LuaTools::check_number() but with a default value.
 *
 * This function acts like luaL_optnumber() except that it throws a
 * LuaException in case of error.
 *
 * \param l A Lua state.
 * \param index Index of a value in the stack.
 * \param default_value The default value to return if the value is \c nil.
 * \return The wanted value as a double.
 */
double opt_number(
    lua_State* l,
    int index,
    double default_value
) {
  if (lua_isnoneornil(l, index)) {
    return default_value;
  }
  return check_number(l, index);
}

/**
 * \brief Like LuaTools::check_number_field() but with a default value.
 *
 * This function acts like lua_getfield() followed by LuaTools::opt_number().
 *
 * \param l A Lua state.
 * \param table_index Index of a table in the stack.
 * \param key Key of the field to get in that table.
 * \param default_value The default value to return if the field is \c nil.
 * \return The wanted field as a double.
 */
double opt_number_field(
    lua_State* l,
    int table_index,
    const std::string& key,
    double default_value
) {
  lua_getfield(l, table_index, key.c_str());
  if (lua_isnil(l, -1)) {
    lua_pop(l, 1);
    return default_value;
  }

  if (!lua_isnumber(l, -1)) {
    arg_error(l, table_index,
        std::string("Bad field '") + key + "' (number expected, got "
        + luaL_typename(l, -1) + ")"
    );
  }
  double value = lua_tonumber(l, -1);
  lua_pop(l, 1);
  return value;
}

/**
 * \brief Checks that a value is a string and returns it.
 *
 * This function acts like luaL_checkstring() except that it throws a
 * LuaException in case of error.
 *
 * \param l A Lua state.
 * \param index Index of a value in the stack.
 * \return The string value.
 */
std::string check_string(
    lua_State* l,
    int index
) {
  if (!lua_isstring(l, index)) {
    arg_error(l, index,
        std::string("string expected, got ")
            + get_type_name(l, index) + ")"
    );
  }
  size_t size = 0;
  const char* data = lua_tolstring(l, index, &size);
  return {data, size};
}

/**
 * \brief Checks that a table field is a string and returns it.
 *
 * This function acts like lua_getfield() followed by LuaTools::check_string().
 *
 * \param l A Lua state.
 * \param table_index Index of a table in the stack.
 * \param key Key of the field to get in that table.
 * \return The wanted field as a string.
 */
std::string check_string_field(
    lua_State* l,
    int table_index,
    const std::string& key
) {
  lua_getfield(l, table_index, key.c_str());
  if (!lua_isstring(l, -1)) {
    arg_error(l, table_index,
        std::string("Bad field '") + key + "' (string expected, got "
        + get_type_name(l, -1) + ")"
    );
  }

  size_t size = 0;
  const char* data = lua_tolstring(l, -1, &size);
  const std::string value = {data, size};
  lua_pop(l, 1);
  return value;
}

/**
 * \brief Like LuaTools::check_string() but with a default value.
 *
 * This function acts like luaL_optstring() except that it throws a
 * LuaException in case of error.
 *
 * \param l A Lua state.
 * \param index Index of a value in the stack.
 * \param default_value The default value to return if the value is \c nil.
 * \return The wanted value as a string.
 */
std::string opt_string(
    lua_State* l,
    int index,
    const std::string& default_value
) {
  if (lua_isnoneornil(l, index)) {
    return default_value;
  }
  return check_string(l, index);
}

/**
 * \brief Like LuaTools::check_string_field() but with a default value.
 *
 * This function acts like lua_getfield() followed by LuaTools::opt_string().
 *
 * \param l A Lua state.
 * \param table_index Index of a table in the stack.
 * \param key Key of the field to get in that table.
 * \param default_value The default value to return if the field is \c nil.
 * \return The wanted field as a string.
 */
std::string opt_string_field(
    lua_State* l,
    int table_index,
    const std::string& key,
    const std::string& default_value
) {
  lua_getfield(l, table_index, key.c_str());
  if (lua_isnil(l, -1)) {
    lua_pop(l, 1);
    return default_value;
  }

  if (!lua_isstring(l, -1)) {
    arg_error(l, table_index,
        std::string("Bad field '") + key + "' (string expected, got "
        + get_type_name(l, -1) + ")"
    );
  }
  size_t size = 0;
  const char* data = lua_tolstring(l, -1, &size);
  const std::string value = {data, size};
  lua_pop(l, 1);
  return value;
}

/**
 * \brief Checks that a value is a boolean and returns it.
 *
 * This function throws a LuaException in case of error.
 *
 * \param l A Lua state.
 * \param index Index of a value in the stack.
 * \return The boolean value.
 */
bool check_boolean(
    lua_State* l,
    int index
) {
  if (!lua_isboolean(l, index)) {
    arg_error(l, index,
        std::string("boolean expected, got ")
            + get_type_name(l, index) + ")"
    );
  }
  return lua_toboolean(l, index);
}

/**
 * \brief Checks that a table field is a boolean and returns it.
* \param l A Lua state.
 * \param table_index Index of a table in the stack.
 * \param key Key of the field to get in that table.
 * \return The wanted field as a boolean.
 */
bool check_boolean_field(
    lua_State* l,
    int table_index,
    const std::string& key
) {
  lua_getfield(l, table_index, key.c_str());
  if (lua_type(l, -1) != LUA_TBOOLEAN) {
    arg_error(l, table_index,
        std::string("Bad field '") + key + "' (boolean expected, got "
        + get_type_name(l, -1) + ")"
    );
  }

  bool value = lua_toboolean(l, -1);
  lua_pop(l, 1);
  return value;
}

/**
 * \brief Like LuaTools::check_boolean() but with a default value.
 * \param l A Lua state.
 * \param index Index of a value in the stack.
 * \param default_value The default value to return if the value is \c nil.
 * \return The wanted value as a boolean.
 */
bool opt_boolean(
    lua_State* l,
    int index,
    bool default_value
) {
  if (lua_isnoneornil(l, index)) {
    return default_value;
  }
  return check_boolean(l, index);
}

/**
 * \brief Like check_boolean_field() but with a default value.
 *
 * This function acts like lua_getfield() followed by lua_toboolean().
 *
 * \param l A Lua state.
 * \param table_index Index of a table in the stack.
 * \param key Key of the field to get in that table.
 * \param default_value The default value to return if the field is \c nil.
 * \return The wanted field as a string.
 */
bool opt_boolean_field(
    lua_State* l,
    int table_index,
    const std::string& key,
    bool default_value
) {
  lua_getfield(l, table_index, key.c_str());
  if (lua_isnil(l, -1)) {
    lua_pop(l, 1);
    return default_value;
  }

  if (lua_type(l, -1) != LUA_TBOOLEAN) {
    arg_error(l, table_index,
        std::string("Bad field '") + key + "' (boolean expected, got "
        + get_type_name(l, -1) + ")"
    );
  }
  return lua_toboolean(l, -1);
}

/**
 * \brief Checks that a value is a function and returns a ref to it.
 *
 * This function throws a LuaException in case of error.
 *
 * \param l A Lua state.
 * \param index Index of a value in the stack.
 * \return The wanted value as a Lua ref to the function.
 */
ScopedLuaRef check_function(
    lua_State* l,
    int index
) {
  check_type(l, index, LUA_TFUNCTION);
  return create_ref(l, index);  // Leave the function in the stack.
}

/**
 * \brief Checks that a table field is a function and returns a ref to it.
 * \param l A Lua state.
 * \param table_index Index of a table in the stack.
 * \param key Key of the field to get in that table.
 * \return The wanted field as a Lua ref to the function.
 */
ScopedLuaRef check_function_field(
    lua_State* l,
    int table_index,
    const std::string& key
) {
  lua_getfield(l, table_index, key.c_str());
  if (!lua_isfunction(l, -1)) {
    arg_error(l, table_index,
        std::string("Bad field '") + key + "' (function expected, got "
        + get_type_name(l, -1) + ")"
    );
  }

  return create_ref(l);  // This also pops the function from the stack.
}

/**
 * \brief Like LuaTools::check_function() but the value is optional.
 * \param l A Lua state.
 * \param index Index of a value in the stack.
 * \return The wanted value as a Lua ref to the function, or an empty ref.
 */
ScopedLuaRef opt_function(
    lua_State* l,
    int index
) {
  if (lua_isnoneornil(l, index)) {
    return ScopedLuaRef();
  }
  return check_function(l, index);
}

/**
 * \brief Like LuaTools::check_function_field() but the field is optional.
 * \param l A Lua state.
 * \param table_index Index of a table in the stack.
 * \param key Key of the field to get in that table.
 * \return The wanted field as a Lua ref to the function, or an empty ref.
 */
ScopedLuaRef opt_function_field(
    lua_State* l,
    int table_index,
    const std::string& key
) {
  lua_getfield(l, table_index, key.c_str());
  if (lua_isnil(l, -1)) {
    lua_pop(l, 1);
    return ScopedLuaRef();
  }

  if (!lua_isfunction(l, -1)) {
    arg_error(l, table_index,
        std::string("Bad field '") + key + "' (function expected, got "
        + get_type_name(l, -1) + ")"
    );
  }
  return create_ref(l);  // This also pops the function from the stack.
}
/**
 * \brief Returns whether a value is a layer.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \param map The map whose number of layer matters.
 * \return \c true if the value is a valid layer for the map.
 */
bool is_layer(
    lua_State* l,
    int index,
    const Map& map
) {
  if (!lua_isnumber(l, index)) {
    return false;
  }
  int layer = check_int(l, index);
  return map.is_valid_layer(layer);
}

/**
 * \brief Checks that the value at the given index is a valid layer and returns it.
 * \param l A Lua state.
 * \param index An index in the Lua stack.
 * \param map The map whose number of layer matters.
 * \return The layer at this index.
 */
int check_layer(
    lua_State* l,
    int index,
    const Map& map
) {
  if (!lua_isnumber(l, index)) {
    type_error(l, index, "number");
  }

  if (!is_layer(l, index, map)) {
    std::ostringstream oss;
    oss << "Invalid layer: " << lua_tonumber(l, index);
    arg_error(l, index, oss.str());
  }

  return lua_tointeger(l, index);
}

/**
 * \brief Checks that a table field is a valid layer and returns it.
 *
 * This function acts like lua_getfield() followed by LuaTools::check_layer().
 *
 * \param l A Lua state.
 * \param table_index Index of a table in the stack.
 * \param key Key of the field to get in that table.
 * \param map The map whose number of layer matters.
 * \return The wanted field as a layer.
 */
int check_layer_field(
    lua_State* l,
    int table_index,
    const std::string& key,
    const Map& map
) {
  lua_getfield(l, table_index, key.c_str());
  if (!is_layer(l, -1, map)) {
    arg_error(l, table_index,
        std::string("Bad field '") + key + "' (layer expected, got "
        + get_type_name(l, -1) + ")"
    );
  }

  int value = lua_tointeger(l, -1);
  lua_pop(l, 1);
  return value;
}

/**
 * \brief Like LuaTools::check_layer() but with a default value.
 * \param l A Lua state.
 * \param index Index of a value in the stack.
 * \param map The map whose number of layer matters.
 * \param default_value The default value to return if the value is \c nil.
 * \return The wanted value as a layer.
 */
int opt_layer(
    lua_State* l,
    int index,
    const Map& map,
    int default_value
) {
  if (lua_isnoneornil(l, index)) {
    return default_value;
  }
  return check_layer(l, index, map);
}

/**
 * \brief Like LuaTools::check_layer_field() but with a default value.
 * \param l A Lua state.
 * \param table_index Index of a table in the stack.
 * \param key Key of the field to get in that table.
 * \param map The map whose number of layer matters.
 * \param default_value The default value to return if the field is \c nil.
 * \return The wanted field as a layer.
 */
int opt_layer_field(
    lua_State* l,
    int table_index,
    const std::string& key,
    const Map& map,
    int default_value
) {
  lua_getfield(l, table_index, key.c_str());
  if (lua_isnil(l, -1)) {
    lua_pop(l, 1);
    return default_value;
  }

  if (!is_layer(l, -1, map)) {
    arg_error(l, table_index,
        std::string("Bad field '") + key + "' (layer expected, got "
        + get_type_name(l, -1) + ")"
    );
  }
  int value = lua_tointeger(l, -1);
  lua_pop(l, 1);
  return value;
}

/**
 * \brief Returns whether a value is a color.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return true if the value is a color, that is, an array with three integers.
 */
bool is_color(lua_State* l, int index) {

  index = get_positive_index(l, index);

  if (lua_type(l, index) != LUA_TTABLE) {
    return false;
  }

  lua_rawgeti(l, index, 1);
  lua_rawgeti(l, index, 2);
  lua_rawgeti(l, index, 3);
  lua_rawgeti(l, index, 4);
  bool result = lua_isnumber(l, -4) &&
      lua_isnumber(l, -3) &&
      lua_isnumber(l, -2) &&
      (lua_isnumber(l, -1) || lua_isnil(l, -1));
  lua_pop(l, 4);
  return result;
}

/**
 * \brief Checks that the value at the given index is a color and returns it.
 *
 * Set opaque by default if alpha channel is not specified.
 *
 * \param l a Lua state
 * \param index an index in the Lua stack
 * \return the color at this index
 */
Color check_color(lua_State* l, int index) {

  index = get_positive_index(l, index);

  check_type(l, index, LUA_TTABLE);
  lua_rawgeti(l, index, 1);
  lua_rawgeti(l, index, 2);
  lua_rawgeti(l, index, 3);
  lua_rawgeti(l, index, 4);
  Color color(
      check_int(l, -4),
      check_int(l, -3),
      check_int(l, -2),
      opt_int(l, -1, 255)
  );
  lua_pop(l, 4);

  return color;
}

/**
 * \brief Checks that a table field is a color and returns it.
 *
 * This function acts like lua_getfield() followed by LuaTools::check_color().
 *
 * \param l A Lua state.
 * \param table_index Index of a table in the stack.
 * \param key Key of the field to get in that table.
 * \return The wanted field as a layer.
 */
Color check_color_field(
    lua_State* l,
    int table_index,
    const std::string& key
) {
  lua_getfield(l, table_index, key.c_str());
  if (!is_color(l, -1)) {
    arg_error(l, table_index,
        std::string("Bad field '") + key + "' (color table expected, got "
        + get_type_name(l, -1) + ")"
    );
  }

  const Color& value = check_color(l, -1);
  lua_pop(l, 1);
  return value;
}

/**
 * \brief Like LuaTools::check_color() but with a default value.
 * \param l A Lua state.
 * \param index Index of a value in the stack.
 * \param default_value The default value to return if the value is \c nil.
 * \return The wanted value as a color.
 */
Color opt_color(
    lua_State* l,
    int index,
    const Color& default_value
) {
  if (lua_isnoneornil(l, index)) {
    return default_value;
  }
  return check_color(l, index);
}

/**
 * \brief Like LuaTools::check_color_field() but with a default value.
 * \param l A Lua state.
 * \param table_index Index of a table in the stack.
 * \param key Key of the field to get in that table.
 * \param default_value The default value to return if the field is \c nil.
 * \return The wanted field as a color.
 */
Color opt_color_field(
    lua_State* l,
    int table_index,
    const std::string& key,
    const Color& default_value
) {
  lua_getfield(l, table_index, key.c_str());
  if (lua_isnil(l, -1)) {
    lua_pop(l, 1);
    return default_value;
  }

  if (!is_color(l, -1)) {
    arg_error(l, table_index,
        std::string("Bad field '") + key + "' (color expected, got "
        + get_type_name(l, -1) + ")"
    );
  }
  const Color& color = check_color(l, -1);
  lua_pop(l, 1);
  return color;
}

}  // namespace LuaTools

}  // namespace Solarus
