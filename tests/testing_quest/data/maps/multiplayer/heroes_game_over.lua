-- Lua script of map multiplayer/heroes_transitions.
-- This script is executed every time the hero enters this map.

-- Feel free to modify the code below.
-- You can add more events and remove the ones you don't need.

-- See the Solarus Lua API documentation:
-- http://www.solarus-games.org/doc/latest

local map = ...
local game = map:get_game()
local hero = map:get_hero()

local heroes = {}

-- Event called at initialization time, as soon as this map is loaded.
function map:on_started()

  -- You can initialize the movement and sprites of various
  -- map entities here.
  local x,y,l = alter_destination:get_position()
  local alter_hero = map:create_hero{
    x = x,
    y = y,
    layer = l,
  }
  
  alter_hero.is_alter = true
  
  local alter_controls = alter_hero:get_controls()
  
  local x,y,l = center_destination:get_position()
  local center_hero = map:create_hero{
    x = x,
    y = y,
    layer = l,
  }
  
  alter_hero.is_alter = true
  
  local center_controls = center_hero:get_controls()
  local controls = hero:get_controls()
  
  sol.timer.start(map, 2000, function()
    alter_controls:simulate_pressed('down')
    center_controls:simulate_pressed('down')
    controls:simulate_pressed('down')
  end)
  
  
  local alter_camera = game:create_camera("alter_camera", map:get_id())
  
  
  alter_camera:set_size(320/2, 240)
  alter_camera:set_position_on_screen(320/2, 0)
  
  --alter_camera:teleport(map:get_id())
  alter_camera:start_tracking(alter_hero)
  
  local camera = map:get_camera()
  camera:set_size(320/2, 240)
  
  heroes = {center_hero, alter_hero, hero}
end

function hero_sensor:on_activated(ahero)
  assert_equal(ahero, hero)
  sol.main.exit()
end

local index = 1
function game:on_game_over_finished(ahero)
  assert_equal(ahero, heroes[index])
  index = index + 1
end

-- Event called after the opening transition effect of the map,
-- that is, when the player takes control of the hero.
function map:on_opening_transition_finished()

end
