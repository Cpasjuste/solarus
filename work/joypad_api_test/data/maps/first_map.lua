-- Lua script of map first_map.
-- This script is executed every time the hero enters this map.

-- Feel free to modify the code below.
-- You can add more events and remove the ones you don't need.

-- See the Solarus Lua API documentation:
-- http://www.solarus-games.org/doc/latest

local map = ...
local game = map:get_game()


print("caca ?")

-- Event called at initialization time, as soon as this map becomes is loaded.
function map:on_started()
  -- get the first joypad
  local joypad = sol.input.get_joypads()[1]
  
  -- create default bindings from the joypad
  local alter_commands = sol.controls.create_from_joypad(joypad)
  --local alter_commands = sol.commands.create_from_keyboard()
  
  --create an other hero on a destination
  local x,y,l = alter_destination:get_position()
  local alter_hero = map:create_hero{
    x = x,
    y = y,
    layer = l,
  }
  
  -- give the other hero a sword
  alter_hero:set_ability("sword", 1)
  
  -- override hero's commands with the joypad one
  alter_hero:set_controls(alter_commands)
  
  alter_commands:set_joypad_axis_binding('look_x', 'right_x')
  alter_commands:set_keyboard_axis_binding('X', 'h', 'l')
  alter_commands:simulate_axis_moved('test', 0.5)
  print("test axis", alter_commands:get_axis_state('test'))
end

-- Event called after the opening transition effect of the map,
-- that is, when the player takes control of the hero.
function map:on_opening_transition_finished()
end
