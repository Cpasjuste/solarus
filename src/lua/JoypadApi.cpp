#include "solarus/lua/LuaContext.h"
#include "solarus/lua/LuaTools.h"

namespace Solarus {

const std::string LuaContext::joypad_module_name = "sol.joypad";

void LuaContext::register_joypad_module() {

  const std::vector<luaL_Reg> methods = {
    {"get_axis", joypad_api_get_axis},
    {"is_button_pressed", joypad_api_is_button_pressed},
    {"get_name", joypad_api_get_name},
    {"rumble", joypad_api_rumble},
    {"has_rumble", joypad_api_has_rumble},
    {"is_attached", joypad_api_is_attached}
  };

  const std::vector<luaL_Reg> metamethods = {
      { "__gc", userdata_meta_gc },
      { "__newindex", userdata_meta_newindex_as_table },
      { "__index", userdata_meta_index_as_table },
  };

  register_type(joypad_module_name, {}, methods, metamethods);
}

/**
 * @brief push a joypad userdata on the lua stack
 * @param current_l lua state
 * @param joypad
 */
void LuaContext::push_joypad(lua_State* current_l, Joypad& joypad) {
  push_userdata(current_l, joypad);
}

/**
 * @brief check wheter a value on the stack is a joypad
 * @param current_l lua state
 * @param index index on the stack
 * @return
 */
bool LuaContext::is_joypad(lua_State* current_l, int index) {
  return is_userdata(current_l,index,joypad_module_name);
}

/**
 * @brief return a Shared Joypad from the lua stack
 * @param current_l lua state
 * @param index index on the stack
 * @return a joypad if index point to a true joypad
 */
std::shared_ptr<Joypad> LuaContext::check_joypad(lua_State* current_l, int index) {
  return std::static_pointer_cast<Joypad>(check_userdata(
    current_l,index,joypad_module_name)
  );
}

// Implementations

/**
 * \brief Implementation of joypad:get_axis(axis).
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::joypad_api_get_axis(lua_State* l) {
  return state_boundary_handle(l,[&](){
    auto joy = check_joypad(l,1);
    JoyPadAxis axis = name_to_enum(
          LuaTools::check_string(l,2),JoyPadAxis::INVALID);
    double val = joy->get_axis(axis);
    lua_pushnumber(l,val);
    return 1;
  });
}

/**
 * \brief Implementation of joypad:is_button_pressed(button).
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::joypad_api_is_button_pressed(lua_State* l) {
  return state_boundary_handle(l,[&](){
    auto joy = check_joypad(l,1);
    JoyPadButton button = name_to_enum(
          LuaTools::check_string(l,2), JoyPadButton::INVALID);
    lua_pushboolean(l,joy->is_button_pressed(button));
    return 1;
  });
}

/**
 * \brief Implementation of joypad:get_name().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::joypad_api_get_name(lua_State* l) {
  return state_boundary_handle(l,[&](){
    auto joy = check_joypad(l,1);
    lua_pushstring(l,joy->get_name().c_str());
    return 1;
  });
}

/**
 * \brief Implementation of joypad:rumble(intensity,duration).
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::joypad_api_rumble(lua_State* l) {
  return state_boundary_handle(l,[&](){
    auto joy = check_joypad(l,1);
    double intensity = LuaTools::check_number(l,2);
    int duration = LuaTools::check_int(l,3);
    if(duration < 0) {
      Debug::error("negative rumble duration");
      return 0;
    }
    joy->rumble(intensity, duration);
    return 0;
  });
}

/**
 * \brief Implementation of joypad:has_rumble().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::joypad_api_has_rumble(lua_State* l) {
  return state_boundary_handle(l,[&](){
    auto joy = check_joypad(l,1);
    lua_pushboolean(l,joy->has_rumble());
    return 1;
  });
}

/**
 * \brief Implementation of joypad:is_attached().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::joypad_api_is_attached(lua_State* l) {
  return state_boundary_handle(l,[&](){
    auto joy = check_joypad(l,1);
    lua_pushboolean(l,joy->is_attached());
    return 1;
  });
}

// Events
bool LuaContext::on_joypad_axis_moved(Joypad& joypad, JoyPadAxis axis, double val) {
  if(!userdata_has_field(joypad,"on_axis_moved")) {
    return false;
  }

  push_joypad(current_l, joypad);
  bool handled = false;

  if(find_method("on_axis_moved")) {
    lua_pushstring(current_l,enum_to_name(axis).c_str());
    lua_pushnumber(current_l, val);
    bool success = call_function(3, 1, "on_axis_moved");

    if (!success) {
      // Something was wrong in the script: don't propagate the input to other objects.
      handled = true;
    }
    else {
      handled = lua_toboolean(current_l, -1);
      lua_pop(current_l, 1);
    }
  }
  lua_pop(current_l, 1);
  return handled;
}

bool LuaContext::on_joypad_button_pressed(Joypad& joypad, JoyPadButton button) {
  if(!userdata_has_field(joypad,"on_button_pressed")) {
    return false;
  }

  push_joypad(current_l, joypad);
  bool handled = false;

  if(find_method("on_button_pressed")) {
    lua_pushstring(current_l, enum_to_name(button).c_str());
    bool success = call_function(2, 1, "on_button_pressed");
    if (!success) {
      // Something was wrong in the script: don't propagate the input to other objects.
      handled = true;
    }
    else {
      handled = lua_toboolean(current_l, -1);
      lua_pop(current_l, 1);
    }
  }
  lua_pop(current_l, 1);
  return handled;
}

bool LuaContext::on_joypad_button_released(Joypad& joypad, JoyPadButton button) {
  if(!userdata_has_field(joypad,"on_button_released")) {
    return false;
  }

  push_joypad(current_l, joypad);
  bool handled = false;

  if(find_method("on_button_released")) {
    lua_pushstring(current_l, enum_to_name(button).c_str());

    bool success = call_function(2, 1, "on_button_released");

    if (!success) {
      // Something was wrong in the script: don't propagate the input to other objects.
      handled = true;
    }
    else {
      handled = lua_toboolean(current_l, -1);
      lua_pop(current_l, 1);
    }
  }
  lua_pop(current_l, 1);
  return handled;
}

bool LuaContext::on_joypad_removed(Joypad& joypad) {
  if(!userdata_has_field(joypad,"on_removed")) {
    return false;
  }
  push_joypad(current_l, joypad);

  bool handled = false;
  if(find_method("on_removed")) {
    bool success = call_function(1, 1, "on_removed");

    if (!success) {
      // Something was wrong in the script: don't propagate the input to other objects.
      handled = true;
    }
    else {
      handled = lua_toboolean(current_l, -1);
      lua_pop(current_l, 1);
    }
  }
  lua_pop(current_l, 1);
  return handled;
}

} //Solarus

