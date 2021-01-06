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
#include "solarus/core/AbilityInfo.h"
#include "solarus/core/CommandsEffects.h"
#include "solarus/core/CurrentQuest.h"
#include "solarus/core/Debug.h"
#include "solarus/core/Equipment.h"
#include "solarus/core/EquipmentItem.h"
#include "solarus/core/Game.h"
#include "solarus/core/MainLoop.h"
#include "solarus/core/QuestFiles.h"
#include "solarus/core/Savegame.h"
#include "solarus/lua/LuaContext.h"
#include "solarus/lua/LuaTools.h"

namespace Solarus {

/**
 * Name of the Lua table representing the game module.
 */
const std::string LuaContext::game_module_name = "sol.game";

/**
 * \brief Initializes the game features provided to Lua.
 */
void LuaContext::register_game_module() {

  // Functions of sol.game.
  const std::vector<luaL_Reg> functions = {
      { "exists", game_api_exists },
      { "delete", game_api_delete },
      { "load", game_api_load }
  };

  // Methods of the game type.
  const std::vector<luaL_Reg> methods = {
      { "save", game_api_save },
      { "start", game_api_start },
      { "is_started", game_api_is_started },
      { "is_suspended", game_api_is_suspended },
      { "set_suspended", game_api_set_suspended },
      { "is_paused", game_api_is_paused },
      { "set_paused", game_api_set_paused },
      { "is_pause_allowed", game_api_is_pause_allowed },
      { "set_pause_allowed", game_api_set_pause_allowed },
      { "is_dialog_enabled", game_api_is_dialog_enabled },
      { "start_dialog", game_api_start_dialog },
      { "stop_dialog", game_api_stop_dialog },
      { "is_game_over_enabled", game_api_is_game_over_enabled },
      { "start_game_over", game_api_start_game_over },
      { "stop_game_over", game_api_stop_game_over },
      { "get_map", game_api_get_map },
      { "get_hero", game_api_get_hero },
      { "get_value", game_api_get_value },
      { "set_value", game_api_set_value },
      { "get_starting_location", game_api_get_starting_location },
      { "set_starting_location", game_api_set_starting_location },
      { "get_transition_style", game_api_get_transition_style },
      { "set_transition_style", game_api_set_transition_style },
      { "get_life", game_api_get_life },
      { "set_life", game_api_set_life },
      { "add_life", game_api_add_life },
      { "remove_life", game_api_remove_life },
      { "get_max_life", game_api_get_max_life },
      { "set_max_life", game_api_set_max_life },
      { "add_max_life", game_api_add_max_life },
      { "get_money", game_api_get_money },
      { "set_money", game_api_set_money },
      { "add_money", game_api_add_money },
      { "remove_money", game_api_remove_money },
      { "get_max_money", game_api_get_max_money },
      { "set_max_money", game_api_set_max_money },
      { "get_magic", game_api_get_magic},
      { "set_magic", game_api_set_magic},
      { "add_magic", game_api_add_magic},
      { "remove_magic", game_api_remove_magic},
      { "get_max_magic", game_api_get_max_magic},
      { "set_max_magic", game_api_set_max_magic},
      { "has_ability", game_api_has_ability },
      { "get_ability", game_api_get_ability },
      { "set_ability", game_api_set_ability },
      { "get_item", game_api_get_item },
      { "has_item", game_api_has_item },
      { "get_item_assigned", game_api_get_item_assigned },
      { "set_item_assigned", game_api_set_item_assigned },
      { "is_command_pressed", game_api_is_command_pressed },
      { "get_commands_direction", game_api_get_commands_direction },
      { "get_command_effect", game_api_get_command_effect },
      { "get_command_keyboard_binding", game_api_get_command_keyboard_binding },
      { "set_command_keyboard_binding", game_api_set_command_keyboard_binding },
      { "get_command_joypad_binding", game_api_get_command_joypad_binding },
      { "set_command_joypad_binding", game_api_set_command_joypad_binding },
      { "capture_command_binding", game_api_capture_command_binding },
      { "simulate_command_pressed", game_api_simulate_command_pressed },
      { "simulate_command_released", game_api_simulate_command_released },
      //1.7 methods
      { "get_controls", game_api_get_controls },
      { "create_camera", game_api_create_camera },
      { "remove_camera", game_api_remove_camera },
      { "get_cameras", game_api_get_cameras },
      { "get_maps", game_api_get_maps }
  };

  const std::vector<luaL_Reg> metamethods = {
      { "__gc", userdata_meta_gc },
      { "__newindex", userdata_meta_newindex_as_table },
      { "__index", userdata_meta_index_as_table },
  };

  register_type(game_module_name, functions, methods, metamethods);
}

/**
 * \brief Returns whether a value is a userdata of type game.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return true if the value at this index is a game.
 */
bool LuaContext::is_game(lua_State* l, int index) {
  return is_userdata(l, index, game_module_name);
}

/**
 * \brief Checks that the userdata at the specified index of the stack is a
 * game and returns it.
 * \param l a Lua context
 * \param index an index in the stack
 * \return the game
 */
std::shared_ptr<Savegame> LuaContext::check_game(lua_State* l, int index) {
  return std::static_pointer_cast<Savegame>(check_userdata(
      l, index, game_module_name
  ));
}

/**
 * \brief Pushes a game userdata onto the stack.
 * \param l A Lua context.
 * \param game A game.
 */
void LuaContext::push_game(lua_State* l, Savegame& game) {
  push_userdata(l, game);
}

/**
 * \brief Implementation of sol.game.exists().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_exists(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const std::string& file_name = LuaTools::check_string(l, 1);

    if (QuestFiles::get_quest_write_dir().empty()) {
      LuaTools::error(l, "Cannot check savegame: no write directory was specified in quest.dat");
    }

    bool exists = QuestFiles::data_file_exists(file_name) &&
        !QuestFiles::data_file_is_dir(file_name);

    lua_pushboolean(l, exists);
    return 1;
  });
}

/**
 * \brief Implementation of sol.game.delete().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_delete(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const std::string& file_name = LuaTools::check_string(l, 1);

    if (QuestFiles::get_quest_write_dir().empty()) {
      LuaTools::error(l, "Cannot delete savegame: no write directory was specified in quest.dat");
    }

    QuestFiles::data_file_delete(file_name);

    return 0;
  });
}

/**
 * \brief Implementation of sol.game.load().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_load(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const std::string& file_name = LuaTools::check_string(l, 1);

    if (QuestFiles::get_quest_write_dir().empty()) {
      LuaTools::error(l, "Cannot load savegame: no write directory was specified in quest.dat");
    }

    std::shared_ptr<Savegame> savegame = std::make_shared<Savegame>(
        get().get_main_loop(), file_name
    );
    savegame->initialize();

    push_game(l, *savegame);
    return 1;
  });
}

/**
 * \brief Implementation of game:save().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_save(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    if (QuestFiles::get_quest_write_dir().empty()) {
      LuaTools::error(l, "Cannot save game: no write directory was specified in quest.dat");
    }

    savegame.save();

    return 0;
  });
}

/**
 * \brief Implementation of game:start().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_start(lua_State* l) {

  return state_boundary_handle(l, [&] {
    std::shared_ptr<Savegame> savegame = check_game(l, 1);

    if (CurrentQuest::get_resources(ResourceType::MAP).empty()) {
      LuaTools::error(l, "Cannot start game: there is no map in this quest");
    }

    Game* game = savegame->get_game();
    if (game != nullptr) {
      // A game is already running with this savegame: restart it.
      game->restart();
    }
    else {
      // Create a new game to run.
      MainLoop& main_loop = savegame->get_lua_context().get_main_loop();
      if (main_loop.get_game() != nullptr) {
        // Stop any previous game.
        main_loop.get_game()->stop();
      }
      Game* game = new Game(main_loop, savegame);
      main_loop.set_game(game);
    }

    return 0;
  });
}

/**
 * \brief Implementation of game:is_started().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_is_started(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    Game* game = savegame.get_game();
    bool is_started = game != nullptr;

    lua_pushboolean(l, is_started);
    return 1;
  });
}

/**
 * \brief Implementation of game:is_suspended().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_is_suspended(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    Game* game = savegame.get_game();
    bool is_suspended = game != nullptr && game->is_suspended();

    lua_pushboolean(l, is_suspended);
    return 1;
  });
}

/**
 * \brief Implementation of game:set_suspended().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_set_suspended(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    bool suspended = LuaTools::opt_boolean(l, 2, true);

    Game* game = savegame.get_game();
    if (game != nullptr) {
      game->set_suspended_by_script(suspended);
    }

    return 0;
  });
}

/**
 * \brief Implementation of game:is_paused().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_is_paused(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    Game* game = savegame.get_game();
    bool is_paused = game != nullptr && game->is_paused();

    lua_pushboolean(l, is_paused);
    return 1;
  });
}

/**
 * \brief Implementation of game:set_paused().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_set_paused(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    bool paused = LuaTools::opt_boolean(l, 2, true);

    Game* game = savegame.get_game();
    if (game != nullptr) {
      game->set_paused(paused);
    }

    return 0;
  });
}

/**
 * \brief Implementation of game:is_pause_allowed().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_is_pause_allowed(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    Game* game = savegame.get_game();
    bool is_pause_allowed = game != nullptr && game->is_pause_allowed();

    lua_pushboolean(l, is_pause_allowed);
    return 1;
  });
}

/**
 * \brief Implementation of game:set_pause_allowed().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_set_pause_allowed(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    bool pause_allowed = LuaTools::opt_boolean(l, 2, true);

    Game* game = savegame.get_game();
    if (game != nullptr) {
      game->set_pause_allowed(pause_allowed);
    }

    return 0;
  });
}

/**
 * \brief Implementation of game:is_dialog_enabled().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_is_dialog_enabled(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    Game* game = savegame.get_game();
    if (game == nullptr) {
      lua_pushboolean(l, false);
    }
    else {
      lua_pushboolean(l, game->is_dialog_enabled());
    }
    return 1;
  });
}

/**
 * \brief Implementation of game:start_dialog().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_start_dialog(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    const std::string& dialog_id = LuaTools::check_string(l, 2);
    ScopedLuaRef info_ref;
    ScopedLuaRef callback_ref;

    const std::string& language = CurrentQuest::get_language();
    if (language.empty()) {
      LuaTools::error(l, "Cannot start dialog: no language was set");
    }

    if (!CurrentQuest::dialog_exists(dialog_id)) {
      LuaTools::arg_error(l, 2, std::string("No such dialog: '") + dialog_id + "'");
    }

    Game* game = savegame.get_game();
    if (game == nullptr) {
      LuaTools::error(l, "Cannot start dialog: this game is not running");
    }

    if (game->is_dialog_enabled()) {
      LuaTools::error(l, "Cannot start dialog: another dialog is already active");
    }

    if (lua_gettop(l) >= 3) {

      LuaContext& lua_context = get();
      int callback_index = 3;
      if (lua_type(l, 3) != LUA_TFUNCTION) {
        // There is an info parameter.
        lua_pushvalue(l, 3);
        info_ref = lua_context.create_ref();
        ++callback_index;
      }

      callback_ref = LuaTools::opt_function(l, callback_index);
    }
    game->start_dialog(dialog_id, info_ref, callback_ref);

    return 0;
  });
}

/**
 * \brief Implementation of game:stop_dialog().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_stop_dialog(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    Game* game = savegame.get_game();
    if (game == nullptr) {
      LuaTools::error(l, "Cannot stop dialog: this game is not running");
    }

    if (!game->is_dialog_enabled()) {
      LuaTools::error(l, "Cannot stop dialog: no dialog is active");
    }

    // Optional parameter: status.
    ScopedLuaRef status_ref;
    if (lua_gettop(l) >= 2) {
      lua_settop(l, 2);
      status_ref = get().create_ref();
    }

    run_on_main([game,status_ref](lua_State*){
      game->stop_dialog(status_ref);
    });

    return 0;
  });
}

/**
 * \brief Implementation of game:is_game_over_enabled().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_is_game_over_enabled(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    Game* game = savegame.get_game();
    if (game == nullptr) {
      lua_pushboolean(l, false);
    }
    else {
      lua_pushboolean(l, game->is_showing_game_over());
    }
    return 1;
  });
}

/**
 * \brief Implementation of game:start_game_over().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_start_game_over(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    HeroPtr hero;
    if(!lua_isnil(l, 2)){
      hero = check_hero(l, 2);
    }

    Game* game = savegame.get_game();
    if (game == nullptr) {
      LuaTools::error(l, "Cannot start game-over: this game is not running");
    }

    game->start_game_over(hero);

    return 0;
  });
}

/**
 * \brief Implementation of game:stop_game_over().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_stop_game_over(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    HeroPtr hero;
    if(!lua_isnil(l, 2)){
      hero = check_hero(l, 2);
    }

    Game* game = savegame.get_game();
    if (game == nullptr) {
      LuaTools::error(l, "Cannot stop game-over: this game is not running");
    }

    game->stop_game_over(hero);

    return 0;
  });
}

/**
 * \brief Implementation of game:get_map().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_get_map(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    Game* game = savegame.get_game();
    if (game == nullptr || !game->has_current_map()) {
      lua_pushnil(l);
    }
    else {
      push_map(l, game->get_default_map());
    }
    return 1;
  });
}

/**
 * \brief Implementation of game:get_hero().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_get_hero(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    Game* game = savegame.get_game();
    if (game == nullptr) {
      lua_pushnil(l);
    }
    else {
      push_hero(l, *game->get_hero());
    }
    return 1;
  });
}

/**
 * \brief Implementation of game:get_value().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_get_value(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    const std::string& key = LuaTools::check_string(l, 2);

    if (!LuaTools::is_valid_lua_identifier(key)) {
      LuaTools::arg_error(l, 3,
          std::string("Invalid savegame variable '") + key
          + "': the name should only contain alphanumeric characters or '_'"
          + " and cannot start with a digit");
    }

    if (savegame.is_boolean(key)) {
      lua_pushboolean(l, savegame.get_boolean(key));
    }
    else if (savegame.is_integer(key)) {
      lua_pushinteger(l, savegame.get_integer(key));
    }
    else if (savegame.is_string(key)) {
      lua_pushstring(l, savegame.get_string(key).c_str());
    }
    else {
      lua_pushnil(l);
    }

    return 1;
  });
}

/**
 * \brief Implementation of game:set_value().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_set_value(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    const std::string& key = LuaTools::check_string(l, 2);

    if (key[0] == '_') {
      LuaTools::arg_error(l, 3,
          std::string("Invalid savegame variable '") + key
          + "': names prefixed by '_' are reserved for built-in variables");
    }

    if (!LuaTools::is_valid_lua_identifier(key)) {
      LuaTools::arg_error(l, 3,
          std::string("Invalid savegame variable '") + key
          + "': the name should only contain alphanumeric characters or '_'"
          + " and cannot start with a digit");
    }

    switch (lua_type(l, 3)) {

    case LUA_TBOOLEAN:
      savegame.set_boolean(key, lua_toboolean(l, 3));
      break;

    case LUA_TNUMBER:
      savegame.set_integer(key, int(lua_tointeger(l, 3)));
      break;

    case LUA_TSTRING:
      savegame.set_string(key, lua_tostring(l, 3));
      break;

    case LUA_TNIL:
      savegame.unset(key);
      break;

    default:
      LuaTools::type_error(l, 3, "string, number or boolean");
    }

    return 0;
  });
}

/**
 * \brief Implementation of game:get_starting_location().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_get_starting_location(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    const std::string& starting_map = savegame.get_string(Savegame::KEY_STARTING_MAP);
    const std::string& starting_point = savegame.get_string(Savegame::KEY_STARTING_POINT);

    if (starting_map.empty()) {
      lua_pushnil(l);
    }
    else {
      push_string(l, savegame.get_string(Savegame::KEY_STARTING_MAP));
    }
    if (starting_point.empty()) {
      lua_pushnil(l);
    }
    else {
      push_string(l, savegame.get_string(Savegame::KEY_STARTING_POINT));
    }
    return 2;
  });
}

/**
 * \brief Implementation of game:set_starting_location().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_set_starting_location(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    const std::string& map_id = LuaTools::check_string(l, 2);
    const std::string& destination_name = LuaTools::opt_string(l, 3, "");

    savegame.set_string(Savegame::KEY_STARTING_MAP, map_id);
    savegame.set_string(Savegame::KEY_STARTING_POINT, destination_name);

    return 0;
  });
}

/**
 * \brief Implementation of game:get_transition_style().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_get_transition_style(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const Savegame& savegame = *check_game(l, 1);

    Transition::Style transition_style = savegame.get_default_transition_style();

    push_string(l, enum_to_name(transition_style));

    return 1;
  });
}

/**
 * \brief Implementation of game:set_transition_style().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_set_transition_style(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    Transition::Style transition_style = LuaTools::check_enum<Transition::Style>(l, 2);

    savegame.set_default_transition_style(transition_style);

    return 0;
  });
}

/**
 * \brief Implementation of game:get_life().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_get_life(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    int life = savegame.get_default_equipment()->get_life();
    lua_pushinteger(l, life);
    return 1;
  });
}

/**
 * \brief Implementation of game:set_life().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_set_life(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    int life = LuaTools::check_int(l, 2);

    savegame.get_default_equipment()->set_life(life);

    return 0;
  });
}

/**
 * \brief Implementation of game:add_life().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_add_life(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    int life = LuaTools::check_int(l, 2);

    if (life < 0) {
      LuaTools::arg_error(l, 2, "Invalid life value: must be positive or zero");
    }

    savegame.get_default_equipment()->add_life(life);

    return 0;
  });
}

/**
 * \brief Implementation of game:remove_life().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_remove_life(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    int life = LuaTools::check_int(l, 2);

    if (life < 0) {
      LuaTools::arg_error(l, 2, "Invalid life value: must be positive or zero");
    }

    savegame.get_default_equipment()->remove_life(life);

    return 0;
  });
}

/**
 * \brief Implementation of game:get_max_life().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_get_max_life(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    int life = savegame.get_default_equipment()->get_max_life();

    lua_pushinteger(l, life);
    return 1;
  });
}

/**
 * \brief Implementation of game:set_max_life().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_set_max_life(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    int life = LuaTools::check_int(l, 2);

    if (life <= 0) {
      LuaTools::arg_error(l, 2, "Invalid life value: max life must be strictly positive");
    }

    savegame.get_default_equipment()->set_max_life(life);

    return 0;
  });
}

/**
 * \brief Implementation of game:add_max_life().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_add_max_life(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    int life = LuaTools::check_int(l, 2);

    if (life < 0) {
      LuaTools::arg_error(l, 2, "Invalid life value: must be positive or zero");
    }

    const EquipmentPtr& equipment = savegame.get_default_equipment();
    equipment->set_max_life(equipment->get_max_life() + life);

    return 0;
  });
}

/**
 * \brief Implementation of game:get_money().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_get_money(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    int money = savegame.get_default_equipment()->get_money();

    lua_pushinteger(l, money);
    return 1;
  });
}

/**
 * \brief Implementation of game:set_money().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_set_money(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    int money = LuaTools::check_int(l, 2);

    savegame.get_default_equipment()->set_money(money);

    return 0;
  });
}

/**
 * \brief Implementation of game:add_money().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_add_money(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    int money = LuaTools::check_int(l, 2);

    if (money < 0) {
      LuaTools::arg_error(l, 2, "Invalid money value: must be positive or zero");
    }

    savegame.get_default_equipment()->add_money(money);

    return 0;
  });
}

/**
 * \brief Implementation of game:remove_money().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_remove_money(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    int money = LuaTools::check_int(l, 2);

    if (money < 0) {
      LuaTools::arg_error(l, 2, "Invalid money value: must be positive or zero");
    }

    savegame.get_default_equipment()->remove_money(money);

    return 0;
  });
}

/**
 * \brief Implementation of game:get_max_money().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_get_max_money(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    int money = savegame.get_default_equipment()->get_max_money();

    lua_pushinteger(l, money);
    return 1;
  });
}

/**
 * \brief Implementation of game:set_max_money().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_set_max_money(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    int money = LuaTools::check_int(l, 2);

    if (money < 0) {
      LuaTools::arg_error(l, 2, "Invalid money value: must be positive or zero");
    }

    savegame.get_default_equipment()->set_max_money(money);

    return 0;
  });
}

/**
 * \brief Implementation of game:get_magic().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_get_magic(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    int magic = savegame.get_default_equipment()->get_magic();

    lua_pushinteger(l, magic);
    return 1;
  });
}

/**
 * \brief Implementation of game:set_magic().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_set_magic(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    int magic = LuaTools::check_int(l, 2);

    savegame.get_default_equipment()->set_magic(magic);

    return 0;
  });
}

/**
 * \brief Implementation of game:add_magic().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_add_magic(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    int magic = LuaTools::check_int(l, 2);

    if (magic < 0) {
      LuaTools::arg_error(l, 2, "Invalid magic points value: must be positive or zero");
    }

    savegame.get_default_equipment()->add_magic(magic);

    return 0;
  });
}

/**
 * \brief Implementation of game:remove_magic().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_remove_magic(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    int magic = LuaTools::check_int(l, 2);

    if (magic < 0) {
      LuaTools::arg_error(l, 2, "Invalid magic points value: must be positive or zero");
    }

    savegame.get_default_equipment()->remove_magic(magic);

    return 0;
  });
}

/**
 * \brief Implementation of game:get_max_magic().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_get_max_magic(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    int magic = savegame.get_default_equipment()->get_max_magic();

    lua_pushinteger(l, magic);
    return 1;
  });
}

/**
 * \brief Implementation of game:set_max_magic().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_set_max_magic(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    int magic = LuaTools::check_int(l, 2);

    if (magic < 0) {
      LuaTools::arg_error(l, 2, "Invalid magic points value: must be positive or zero");
    }

    savegame.get_default_equipment()->set_max_magic(magic);

    return 0;
  });
}

/**
 * \brief Implementation of game:has_ability().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_has_ability(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    Ability ability = LuaTools::check_enum<Ability>(l, 2);

    bool has_ability = savegame.get_default_equipment()->has_ability(ability);

    lua_pushboolean(l, has_ability);
    return 1;
  });
}

/**
 * \brief Implementation of game:get_ability().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_get_ability(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    Ability ability = LuaTools::check_enum<Ability>(l, 2);

    int ability_level = savegame.get_default_equipment()->get_ability(ability);

    lua_pushinteger(l, ability_level);
    return 1;
  });
}

/**
 * \brief Implementation of game:set_ability().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_set_ability(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    Ability ability = LuaTools::check_enum<Ability>(l, 2);
    int level = LuaTools::check_int(l, 3);

    savegame.get_default_equipment()->set_ability(ability, level);

    return 0;
  });
}

/**
 * \brief Implementation of game:get_item().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_get_item(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    const std::string& item_name = LuaTools::check_string(l, 2);

    if (!savegame.get_default_equipment()->item_exists(item_name)) {
      LuaTools::error(l, std::string("No such item: '") + item_name + "'");
    }

    push_item(l, savegame.get_default_equipment()->get_item(item_name));
    return 1;
  });
}

/**
 * \brief Implementation of game:has_item().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_has_item(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    const std::string& item_name = LuaTools::check_string(l, 2);

    const EquipmentPtr& equipment = savegame.get_default_equipment();
    if (!equipment->item_exists(item_name)) {
      LuaTools::error(l, std::string("No such item: '") + item_name + "'");
    }

    if (!equipment->get_item(item_name).is_saved()) {
      LuaTools::error(l, std::string("Item '") + item_name + "' is not saved");
    }

    lua_pushboolean(l, equipment->get_item(item_name).get_variant() > 0);
    return 1;
  });
}

/**
 * \brief Implementation of game:get_item_assigned().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_get_item_assigned(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    int slot = LuaTools::check_int(l, 2);

    if (slot < 1 || slot > 2) {
      LuaTools::arg_error(l, 2, "The item slot should be 1 or 2");
    }

    EquipmentItem* item = savegame.get_default_equipment()->get_item_assigned(slot);

    if (item == nullptr) {
      lua_pushnil(l);
    }
    else {
      push_item(l, *item);
    }
    return 1;
  });
}

/**
 * \brief Implementation of game:set_item_assigned().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_set_item_assigned(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    int slot = LuaTools::check_int(l, 2);
    EquipmentItem* item = nullptr;
    if (!lua_isnil(l, 3)) {
      item = check_item(l, 3).get();
    }

    if (slot < 1 || slot > 2) {
      LuaTools::arg_error(l, 2, "The item slot should be 1 or 2");
    }

    savegame.get_default_equipment()->set_item_assigned(slot, item);

    return 0;
  });
}

/**
 * \brief Implementation of game:get_command_effect().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_get_command_effect(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    Command command = check_command(l, 2);

    Game* game = savegame.get_game();
    if (game == nullptr || std::holds_alternative<CustomId>(command)) {
      lua_pushnil(l);
    }
    else {
      std::string effect_name;
      switch (ControlEvent::command_to_id(command)) {

      case CommandId::ACTION:
      {
        CommandsEffects::ActionKeyEffect effect = game->get_commands_effects().get_action_key_effect();
        effect_name = enum_to_name(effect);
        break;
      }

      case CommandId::ATTACK:
      {
        CommandsEffects::AttackKeyEffect effect = game->get_commands_effects().get_sword_key_effect();
        effect_name = enum_to_name(effect);
        break;
      }

      case CommandId::ITEM_1:
      {
        effect_name = game->is_suspended() ? "" : "use_item_1";
        break;
      }

      case CommandId::ITEM_2:
      {
        effect_name = game->is_suspended() ? "" : "use_item_2";
        break;
      }

      case CommandId::PAUSE:
      {
        CommandsEffects::PauseKeyEffect effect = game->get_commands_effects().get_pause_key_effect();
        effect_name = enum_to_name(effect);
        break;
      }

      case CommandId::RIGHT:
      {
        effect_name = game->is_suspended() ? "" : "move_right";
        break;
      }

      case CommandId::UP:
      {
        effect_name = game->is_suspended() ? "" : "move_up";
        break;
      }

      case CommandId::LEFT:
      {
        effect_name = game->is_suspended() ? "" : "move_left";
        break;
      }

      case CommandId::DOWN:
      {
        effect_name = game->is_suspended() ? "" : "move_down";
        break;
      }

      default:
        Debug::die("Invalid game command");
      }

      if (effect_name.empty()) {
        lua_pushnil(l);
      }
      else {
        push_string(l, effect_name);
      }
    }

    return 1;
  });
}

/**
 * \brief Implementation of game:get_command_keyboard_binding().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_get_command_keyboard_binding(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    Command command = check_command(l, 2);

    const Game* game = savegame.get_game();
    if (game == nullptr) {
      LuaTools::error(l, "This game is not running");
    }

    const Controls& commands = game->get_controls();
    InputEvent::KeyboardKey key = commands.get_keyboard_binding(command);
    const std::string& key_name = enum_to_name(key);

    if (key_name.empty()) {
      lua_pushnil(l);
    }
    else {
      push_string(l, key_name);
    }
    return 1;
  });
}

/**
 * \brief Implementation of game:set_command_keyboard_binding().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_set_command_keyboard_binding(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    Command command = check_command(l, 2);
    if (lua_gettop(l) <= 2) {
      LuaTools::type_error(l, 3, "string or nil");
    }
    const std::string& key_name = LuaTools::opt_string(l, 3, "");

    Game* game = savegame.get_game();
    if (game == nullptr) {
      LuaTools::error(l, "This game is not running");
    }

    Controls& commands = game->get_controls();
    InputEvent::KeyboardKey key = name_to_enum(key_name, InputEvent::KeyboardKey::NONE);
    if (!key_name.empty() && key == InputEvent::KeyboardKey::NONE) {
      LuaTools::arg_error(l, 3,
          std::string("Invalid keyboard key name: '") + key_name + "'");
    }
    commands.set_keyboard_binding(command, key);

    return 0;
  });
}

/**
 * \brief Implementation of game:get_command_joypad_binding().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_get_command_joypad_binding(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    Command command = check_command(l, 2);

    const Game* game = savegame.get_game();
    if (game == nullptr) {
      LuaTools::error(l, "This game is not running");
    }
    //const GameCommands& commands = game->get_commands();
    //const std::string& joypad_string = commands.get_joypad_binding(command);

    Controls& commands = savegame.get_game()->get_controls();
    auto binding = commands.get_joypad_binding(command);
    if (!binding) {
      lua_pushnil(l);
    }
    else {
      push_string(l, binding->to_string());
    }
    return 1;
  });
}

/**
 * \brief Implementation of game:set_command_joypad_binding().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_set_command_joypad_binding(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    Command command = check_command(l, 2);
    if (lua_gettop(l) <= 2) {
      LuaTools::type_error(l, 3, "string or nil");
    }
    const std::string& joypad_string = LuaTools::opt_string(l, 3, "");

    if (!joypad_string.empty() && !Controls::is_joypad_string_valid(joypad_string)) {
      LuaTools::arg_error(l, 3,
          std::string("Invalid joypad string: '") + joypad_string + "'");
    }

    Game* game = savegame.get_game();
    if (game == nullptr) {
      LuaTools::error(l, "This game is not running");
    }

    Controls& commands = savegame.get_game()->get_controls();
    commands.set_joypad_binding(command, Controls::JoypadBinding(joypad_string));

    return 0;
  });
}

/**
 * \brief Implementation of game:capture_command_binding().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_capture_command_binding(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    Command command = check_command(l, 2);
    const ScopedLuaRef& callback_ref = LuaTools::opt_function(l, 3);

    Game* game = savegame.get_game();
    if (game == nullptr) {
      LuaTools::error(l, "This game is not running");
    }
    Controls& commands = game->get_controls();
    commands.customize(command, callback_ref);
    return 0;
  });
}

/**
 * \brief Implementation of game:is_command_pressed().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_is_command_pressed(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    Command command = check_command(l, 2);

    const Game* game = savegame.get_game();
    if (game == nullptr) {
      LuaTools::error(l, "This game is not running");
    }

    const Controls& commands = savegame.get_game()->get_controls();
    lua_pushboolean(l, commands.is_command_pressed(command));

    return 1;
  });
}

/**
 * \brief Implementation of game:get_commands_direction().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_get_commands_direction(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    const Game* game = savegame.get_game();
    if (game == nullptr) {
      return 0;
    }

    const Controls& commands = savegame.get_game()->get_controls();

    int wanted_direction8 = commands.get_wanted_direction8();
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
 * \brief Implementation of game:simulate_command_pressed().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_simulate_command_pressed(lua_State* l){

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    Command command = check_command(l, 2);

    Game* game = savegame.get_game();
    if (game == nullptr) {
      LuaTools::error(l, "This game is not running");
    }
    game->simulate_command_pressed(command);

    return 0;
  });
}

/**
 * \brief Implementation of game:simulate_command_released().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_simulate_command_released(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    Command command = check_command(l, 2);

    Game* game = savegame.get_game();
    if (game == nullptr) {
      LuaTools::error(l, "This game is not running");
    }
    game->simulate_command_released(command);

    return 0;
  });
}

/**
 * \brief Implementation of game:get_controls().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_get_controls(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    Controls& cmds = savegame.get_game()->get_controls();

    push_controls(l, cmds);

    return 1;
  });
}

/**
 * \brief Implementation of game:create_camera().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_create_camera(lua_State * l) {

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    Game* game = savegame.get_game();
    Debug::check_assertion(game, "Game is not started!");

    std::string cam_id = LuaTools::check_string(l, 2);
    std::string map_id = LuaTools::check_string(l, 3);
    std::string destination_name = LuaTools::opt_string(l, 4, "");
    Transition::Style transition_style = LuaTools::opt_enum<Transition::Style>(
        l, 5, game->get_default_transition_style());

    CameraPtr camera = game->create_camera(cam_id);

    game->teleport_camera(camera, map_id, destination_name, transition_style, nullptr);
    push_camera(l, *camera);

    return 1;
  });
}

/**
 * \brief Implementation of game:remove_camera().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_remove_camera(lua_State * l) { //TODO

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);
    CameraPtr camera = check_camera(l, 2);

    Game* game = savegame.get_game();
    Debug::check_assertion(game, "Game is not started!");

    Transition::Style transition_style = LuaTools::opt_enum<Transition::Style>(
        l, 3, game->get_default_transition_style());

    game->remove_camera(camera, transition_style);

    return 1;
  });
}

/**
 * \brief Implementation of game:remove_camera().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_get_cameras(lua_State *l) { //TODO

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    Game* game = savegame.get_game();
    Debug::check_assertion(game, "Game is not started!");

    push_userdata_iterator(l, game->get_cameras());

    return 1;
  });
}

/**
 * \brief Implementation of game:remove_camera().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::game_api_get_maps(lua_State * l) { //TODO

  return state_boundary_handle(l, [&] {
    Savegame& savegame = *check_game(l, 1);

    Game* game = savegame.get_game();
    Debug::check_assertion(game, "Game is not started!");

    push_userdata_iterator(l, game->get_maps());

    return 1;
  });
}

/**
 * \brief Calls the on_started() method of a Lua game.
 *
 * Does nothing if the method is not defined.
 *
 * \param game A game.
 */
void LuaContext::game_on_started(Game& game) {

  if (!userdata_has_field(game.get_savegame(), "on_started")) {
    return;
  }

  push_game(current_l, game.get_savegame());
  on_started();
  lua_pop(current_l, 1);
}

/**
 * \brief Calls the on_finished() method of a Lua game if it is defined.
 *
 * Also stops timers and menus associated to the game.
 *
 * \param game A game.
 */
void LuaContext::game_on_finished(Game& game) {

  push_game(current_l, game.get_savegame());
  if (userdata_has_field(game.get_savegame(), "on_finished")) {
    on_finished();
  }
  remove_timers(-1);  // Stop timers and menus associated to this game.
  remove_menus(-1);
  lua_pop(current_l, 1);
}

/**
 * \brief Calls the on_update() method of a Lua game if it is defined.
 *
 * Also calls the method on its menus.
 *
 * \param game A game.
 */
void LuaContext::game_on_update(Game& game) {

  push_game(current_l, game.get_savegame());
  // This particular method is tried so often that we want to optimize
  // the std::string construction.
  static const std::string method_name = "on_update";
  if (userdata_has_field(game.get_savegame(), method_name)) {
    on_update();
  }
  menus_on_update(-1);
  lua_pop(current_l, 1);
}

/**
 * \brief Calls the on_draw() method of a Lua game if it is defined.
 *
 * Also calls the method on its menus.
 *
 * \param game A game.
 * \param dst_surface The destination surface.
 */
void LuaContext::game_on_draw(Game& game, const SurfacePtr& dst_surface) {

  push_game(current_l, game.get_savegame());
  if (userdata_has_field(game.get_savegame(), "on_draw")) {
    on_draw(dst_surface);
  }
  menus_on_draw(-1, dst_surface);
  lua_pop(current_l, 1);
}

/**
 * \brief Calls the on_map_changed() method of a Lua game.
 *
 * Does nothing if the method is not defined.
 *
 * \param game A game.
 * \param map The new active map.
 */
void LuaContext::game_on_map_changed(Game& game, Map& map, Camera& camera) {

  if (!userdata_has_field(game.get_savegame(), "on_map_changed")) {
    return;
  }

  run_on_main([this, &game, &map, &camera](lua_State* l){
    push_game(l, game.get_savegame());
    on_map_changed(map, camera);
    lua_pop(l, 1);
  });
}

/**
 * \brief Calls the on_world_changed() method of a Lua game.
 *
 * Does nothing if the method is not defined
 * or if the quest format is lower than 1.6.
 *
 * \param game A game.
 * \param previous_world The previous world or an empty string.
 * \param new_world The new world or an empty string.
 */
void LuaContext::game_on_world_changed(
    Game& game,
    const std::string& previous_world,
    const std::string& new_world
) {

  if (!CurrentQuest::is_format_at_least({ 1, 6 })) {
    return;
  }

  if (!userdata_has_field(game.get_savegame(), "on_world_changed")) {
    return;
  }

  push_game(current_l, game.get_savegame());
  on_world_changed(previous_world, new_world);
  lua_pop(current_l, 1);
}

/**
 * \brief Calls the on_paused() method of a Lua game.
 *
 * Does nothing if the method is not defined.
 *
 * \param game A game.
 */
void LuaContext::game_on_paused(Game& game) {

  if (!userdata_has_field(game.get_savegame(), "on_paused")) {
    return;
  }

  run_on_main([this,&game](lua_State* l){
    push_game(l, game.get_savegame());
    on_paused();
    lua_pop(l, 1);
  });
}

/**
 * \brief Calls the on_unpaused() method of a Lua game.
 *
 * Does nothing if the method is not defined.
 *
 * \param game A game.
 */
void LuaContext::game_on_unpaused(Game& game) {

  if (!userdata_has_field(game.get_savegame(), "on_unpaused")) {
    return;
  }

  run_on_main([this,&game](lua_State* l){
    push_game(l, game.get_savegame());
    on_unpaused();
    lua_pop(l, 1);
  });
}

/**
 * \brief Calls the on_dialog_started() method of a Lua game.
 *
 * Does nothing if the method is not defined.
 *
 * \param game A game.
 * \param dialog The dialog just started.
 * \param info_ref Lua ref to the info parameter to pass to the method,
 * or an empty ref.
 * \return true if the game:on_dialog_started() method is defined.
 */
bool LuaContext::game_on_dialog_started(
    Game& game,
    const Dialog& dialog,
    const ScopedLuaRef& info_ref
) {
  if (!userdata_has_field(game.get_savegame(), "on_dialog_started")) {
    return false;
  }

  push_game(current_l, game.get_savegame());
  bool exists = on_dialog_started(dialog, info_ref);
  lua_pop(current_l, 1);

  return exists;
}

/**
 * \brief Calls the on_dialog_finished() method of a Lua game.
 *
 * Does nothing if the method is not defined.
 *
 * \param game A game.
 * \param dialog The dialog just finished.
 */
void LuaContext::game_on_dialog_finished(Game& game,
    const Dialog& dialog) {

  if (!userdata_has_field(game.get_savegame(), "on_dialog_finished")) {
    return;
  }

  push_game(current_l, game.get_savegame());
  on_dialog_finished(dialog);
  lua_pop(current_l, 1);
}

/**
 * \brief Calls the on_game_over_started() method of a Lua game.
 *
 * Does nothing if the method is not defined.
 *
 * \param game A game.
 * \return true if the game:on_game_over_started() method is defined.
 */
bool LuaContext::game_on_game_over_started(Game& game, const HeroPtr& hero) {

  if (!userdata_has_field(game.get_savegame(), "on_game_over_started")) {
    return false;
  }

  push_game(current_l, game.get_savegame());
  bool exists = on_game_over_started(hero);
  lua_pop(current_l, 1);

  return exists;
}

/**
 * \brief Calls the on_game_over_finished() method of a Lua game.
 *
 * Does nothing if the method is not defined.
 *
 * \param game A game.
 */
void LuaContext::game_on_game_over_finished(Game& game, const HeroPtr& hero) {

  if (!userdata_has_field(game.get_savegame(), "on_game_over_finished")) {
    return;
  }

  push_game(current_l, game.get_savegame());
  on_game_over_finished(hero);
  lua_pop(current_l, 1);
}

/**
 * \brief Notifies a Lua game that an input event has just occurred.
 *
 * The appropriate callback in the game is triggered if it exists.
 * Also notifies the menus of the game if the game itself does not handle the
 * event.
 *
 * \param game A game.
 * \param event The input event to handle.
 * \return \c true if the event was handled and should stop being propagated.
 */
bool LuaContext::game_on_input(Game& game, const InputEvent& event) {

  push_game(current_l, game.get_savegame());
  bool handled = on_input(event);
  if (!handled) {
    handled = menus_on_input(-1, event);
  }
  lua_pop(current_l, 1);
  return handled;
}

bool LuaContext::game_on_control(Game& game, const ControlEvent& event) {
  bool handled = false;
  push_game(current_l, game.get_savegame());
  if (userdata_has_field(game.get_savegame(), event.event_name())) {
    handled = on_command(event);
  }
  if (!handled) {
    handled = menus_on_command(-1, event);
  }
  lua_pop(current_l, 1);
  return handled;
}

}
