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
#include "solarus/core/QuestFiles.h"
#include "solarus/lua/LuaContext.h"
#include "solarus/lua/LuaTools.h"
#include <cerrno>
#include <cstdio>
#include <cstring>
#if defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>
#endif

namespace Solarus {

/**
 * Name of the Lua table representing the file module.
 */
const std::string LuaContext::file_module_name = "sol.file";

/**
 * \brief Initializes the file features provided to Lua.
 */
void LuaContext::register_file_module() {

  std::vector<luaL_Reg> functions = {
      { "open", file_api_open },
      { "exists", file_api_exists },
      { "remove", file_api_remove },
      { "mkdir", file_api_mkdir },
  };
  if (CurrentQuest::is_format_at_least({ 1, 6 })) {
    functions.insert(functions.end(), {
        { "is_dir", file_api_is_dir },
        { "list_dir", file_api_list_dir },
    });
  }
  register_functions(file_module_name, functions);

  // Our sol.file.open needs the same environment as io.open
  // to make file:close() work.
                                  // --
  lua_getglobal(current_l, "io");
                                  // io
  lua_getfield(current_l, -1, "open");
                                  // io io.open
  Debug::check_assertion(lua_isfunction(current_l, -1), "Could not find io.open");
  lua_getglobal(current_l, "sol");
                                  // io io.open sol
  lua_getfield(current_l, -1, "file");
                                  // io io.open sol sol.file
  lua_getfield(current_l, -1, "open");
                                  // io io.open sol sol.file sol.file.open
  lua_getfenv(current_l, -4);
                                  // io io.open sol sol.file sol.file.open env
  lua_setfenv(current_l, -2);
                                  // io io.open sol sol.file
  lua_pop(current_l, 4);
                                  // --
}

/**
 * \brief Implementation of sol.file.open().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::file_api_open(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const std::string& file_name = LuaTools::check_string(l, 1);
    const std::string& mode = LuaTools::opt_string(l, 2, "r");

    const bool writing = not mode.rfind("r", 0) == 0;

    // file_name is relative to the data directory, the data archive or the
    // quest write directory.
    // Let's determine the full file path to use when opening the file.
    std::string file_path;
    if (writing) {
      // Writing a file.
      if (QuestFiles::get_quest_write_dir().empty()) {
        LuaTools::error(l,
            "Cannot open file for writing: no write directory was specified in quest.dat");
      }

      file_path = QuestFiles::get_full_quest_write_dir() + "/" + file_name;
    }
    else {
      // Reading a file.
      QuestFiles::DataFileLocation location = QuestFiles::data_file_get_location(file_name);

      switch (location) {

      case QuestFiles::DataFileLocation::LOCATION_NONE:
        // Not found.
        lua_pushnil(l);
        push_string(l, std::string("Cannot find file '") + file_name
            + "' in the quest write directory, in data/, data.solarus or in data.solarus.zip");
        return 2;

      case QuestFiles::DataFileLocation::LOCATION_WRITE_DIRECTORY:
        // Found in the quest write directory.
        file_path = QuestFiles::get_full_quest_write_dir() + "/" + file_name;
        break;

      case QuestFiles::DataFileLocation::LOCATION_DATA_DIRECTORY:
        // Found in the data directory.
        file_path = QuestFiles::get_quest_path() + "/data/" + file_name;
        break;

      case QuestFiles::DataFileLocation::LOCATION_DATA_ARCHIVE:
      {
        // Found in the data archive.
        // To open the file, we need it to be a regular file, so let's create
        // a temporary one.
        const std::string& buffer = QuestFiles::data_file_read(file_name);
        file_path = QuestFiles::create_temporary_file(buffer);
        break;
      }
      }
    }

    // Create a new file handle in the Lua stack to return
    FILE **fh = (FILE **)lua_newuserdata(l, sizeof(FILE *));
    *fh = nullptr;  /* file handle is currently 'closed' at this point */
    luaL_getmetatable(l, LUA_FILEHANDLE);
    lua_setmetatable(l, -2);

    int _errno;  // Used to independently store 'errno' below.
#if defined(_WIN32) || defined(__CYGWIN__)
    // In windows, open the file using _wfopen_s() for Unicode filenames support.
    int wfp_len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        &file_path[0], file_path.length(), nullptr, 0);
    int wmode_len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        &mode[0], mode.length(), nullptr, 0);
    if (wfp_len > 0 && wmode_len > 0) {
      wchar_t* wfp = static_cast<wchar_t*>(alloca(sizeof(wchar_t) * (wfp_len + 1)));
      wchar_t* wmode = static_cast<wchar_t*>(alloca(sizeof(wchar_t) * (wmode_len + 1)));
      wfp_len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
          &file_path[0], file_path.length(), wfp, wfp_len);
      wmode_len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
          &mode[0], mode.length(), wmode, wmode_len);
      if (wfp_len > 0 && wmode_len > 0) {
        wfp[wfp_len] = 0;
        wmode[wmode_len] = 0;
        _errno = _wfopen_s(fh, wfp, wmode);
      }
    }
#else
    // In other platforms, open the file using fopen().
    *fh = fopen(file_path.c_str(), mode.c_str());
    _errno = errno;
#endif
    if (*fh == nullptr) {
      lua_pushnil(l);
      push_string(l, file_name + ": " + strerror(_errno));
      lua_pushinteger(l, _errno);
      return 3;
    }

    return 1;
  });
}

/**
 * \brief Implementation of sol.file.exists().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::file_api_exists(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const std::string& file_name = LuaTools::check_string(l, 1);

    lua_pushboolean(l, QuestFiles::data_file_exists(file_name, false));

    return 1;
  });
}

/**
 * \brief Implementation of sol.file.remove().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::file_api_remove(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const std::string& file_name = LuaTools::check_string(l, 1);

    bool success = QuestFiles::data_file_delete(file_name);

    if (!success) {
      lua_pushnil(l);
      push_string(l, std::string("Failed to delete file '") + file_name + "'");
      return 2;
    }

    lua_pushboolean(l, true);
    return 1;
  });
}

/**
 * \brief Implementation of sol.file.mkdir().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::file_api_mkdir(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const std::string& dir_name = LuaTools::check_string(l, 1);

    bool success = QuestFiles::data_file_mkdir(dir_name);

    if (!success) {
      lua_pushnil(l);
      push_string(l, std::string("Failed to create directory '") + dir_name + "'");
      return 2;
    }

    lua_pushboolean(l, true);
    return 1;
  });
}

/**
 * \brief Implementation of sol.file.is_dir().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::file_api_is_dir(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const std::string& file_name = LuaTools::check_string(l, 1);

    lua_pushboolean(l, QuestFiles::data_file_is_dir(file_name));

    return 1;
  });
}

/**
 * \brief Implementation of sol.file.list_dir().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::file_api_list_dir(lua_State* l) {

  return state_boundary_handle(l, [&] {

    const std::string& dir_name = LuaTools::check_string(l, 1);
    if (!QuestFiles::data_file_is_dir(dir_name)) {
      lua_pushnil(l);
      return 1;
    }

    const std::vector<std::string>& files = QuestFiles::data_file_list_dir(dir_name);

    lua_createtable(l, files.size(), 0);
    int i = 1;
    for (const std::string& file : files) {
      push_string(l, file);
      lua_rawseti(l, -2, i);
      ++i;
    }

    return 1;
  });
}

}
