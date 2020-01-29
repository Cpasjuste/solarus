-- Lua script of map multiplayer/heroes2.
-- This script is executed every time the hero enters this map.

-- Feel free to modify the code below.
-- You can add more events and remove the ones you don't need.

-- See the Solarus Lua API documentation:
-- http://www.solarus-games.org/doc/latest

local map = ...
local game = map:get_game()

-- Event called at initialization time, as soon as this map is loaded.
function map:on_started()

  -- You can initialize the movement and sprites of various
  -- map entities here.
end

-- Event called after the opening transition effect of the map,
-- that is, when the player takes control of the hero.
function map:on_opening_transition_finished()

end

local dirnames = {[0]='right', [1]='up', [2]='left', [3]='down'}

local function make_snake_sensor(togo)
  return function(sensor, hero)
    local dirs = {right=true, left=true}

    local dir = dirnames[hero:get_direction()]
    local cmds = hero:get_commands()

    if dirs[dir] then
      hero.last_dir = dir
      
      cmds:simulate_released(dir)
      cmds:simulate_pressed(togo)
    else
      cmds:simulate_released(dir)
      cmds:simulate_pressed(hero.last_dir)
    end
  end
end

to_up_sensor.on_activated = make_snake_sensor('up')
to_down_sensor.on_activated = make_snake_sensor('down')

function win_dest:on_activated(hero)
  print("PARDON?")
  
  if hero.is_alter then
    sol.main.exit()
  end
end