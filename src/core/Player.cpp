#include "solarus/core/Player.h"
#include "solarus/lua/LuaContext.h"
#include "solarus/core/Game.h"

namespace Solarus {

Player::Player(Game& game, const std::string& id) :
  game(game),
  save(game.get_savegame()),
  id(id)
{

}

const std::string& Player::get_lua_type_name() const {
  return LuaContext::player_module_name;
}

}

