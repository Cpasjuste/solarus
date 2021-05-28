-- Lua script of map multiplayer/friendly_fire.
-- This script is executed every time the hero enters this map.

-- Feel free to modify the code below.
-- You can add more events and remove the ones you don't need.

-- See the Solarus Lua API documentation:
-- http://www.solarus-games.org/doc/latest

local map = ...
local game = map:get_game()
local hero = map:get_hero()

local alter_hero
local alter_controls
local controls = hero:get_controls()

-- Event called at initialization time, as soon as this map is loaded.
function map:on_started()

  -- You can initialize the movement and sprites of various
  -- map entities here.
  local x,y,l = alter_destination:get_position()
  alter_hero = map:create_hero{
    x = x,
    y = y,
    layer = l,
  }
  alter_hero:set_ability("sword", 1)
  alter_controls = alter_hero:get_controls()
end

local function simulate_attack(controls)
  controls:simulate_pressed('attack')
  sol.timer.start(map, 30, function()
      controls:simulate_released('attack')
  end)
end

-- Event called after the opening transition effect of the map,
-- that is, when the player takes control of the hero.
function map:on_opening_transition_finished()
  simulate_attack(alter_controls)
  
  sol.timer.start(2000, function()
      error("Timout... hero was not touched")
      end)
end

function hero:on_taking_damage(num)
  sol.main.exit()
end
