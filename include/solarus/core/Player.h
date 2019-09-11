#pragma once

#include "solarus/core/CommandsPtr.h"
#include "solarus/lua/ExportableToLua.h"
#include "solarus/core/CommandsEffects.h"

namespace Solarus {

class Game;
class Savegame;


/**
 * @brief A player abstraction, allow prefixed access to the savegame
 * and does bookeeping of individual life money etc
 */
class Player : public ExportableToLua
{
public:
  Player(Game& game, const std::string& id);

  //Savegame prefixed view :
  bool is_string(const std::string& key) const;
  std::string get_string(const std::string& key) const;
  void set_string(const std::string& key, const std::string& value);
  bool is_integer(const std::string& key) const;
  int get_integer(const std::string& key) const;
  void set_integer(const std::string& key, int value);
  bool is_boolean(const std::string& key) const;
  bool get_boolean(const std::string& key) const;
  void set_boolean(const std::string& key, bool value);
  bool is_set(const std::string& key) const;
  void unset(const std::string& key);

  void set_initial_values();
  void set_default_keyboard_controls();
  void set_default_joypad_controls();
  void post_process_existing_savegame();

  const std::string& get_lua_type_name() const override;

  const CommandsEffects& get_commands_effects() const;
private:
  std::string prefixed_key(const std::string& key);

  Game& game;
  Savegame& save;
  std::string id;
  CommandsPtr commands;
  CommandsEffects
      commands_effects;      /**< current effect associated to the main game keys
                              * (represented on the HUD by the action icon, the objects icons, etc.) */
};

}
