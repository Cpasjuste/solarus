#include "solarus/lua/LuaContext.h"
#include "solarus/lua/LuaTools.h"
#include "solarus/core/Player.h"

namespace Solarus {

const std::string LuaContext::player_module_name = "sol.player";

void LuaContext::register_player_module() {

  const std::vector<luaL_Reg> methods = {
    //TODO
  };

  const std::vector<luaL_Reg> metamethods = {
      { "__gc", userdata_meta_gc },
      { "__newindex", userdata_meta_newindex_as_table },
      { "__index", userdata_meta_index_as_table },
  };

  register_type(player_module_name, {}, methods, metamethods);
}

/**
 * @brief push a commands userdata on the lua stack
 * @param current_l lua state
 * @param joypad
 */
void LuaContext::push_player(lua_State* current_l, Player& player) {
  push_userdata(current_l, player);
}

/**
 * @brief check wheter a value on the stack is a joypad
 * @param current_l lua state
 * @param index index on the stack
 * @return
 */
bool LuaContext::is_player(lua_State* current_l, int index) {
  return is_userdata(current_l,index, player_module_name);
}

/**
 * @brief return a Shared Joypad from the lua stack
 * @param current_l lua state
 * @param index index on the stack
 * @return a joypad if index point to a true joypad
 */
std::shared_ptr<Player> LuaContext::check_player(lua_State* current_l, int index) {
  return std::static_pointer_cast<Player>(check_userdata(
    current_l,index, player_module_name)
  );
}

// Events
/*
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
}*/

} //Solarus

