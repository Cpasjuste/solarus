-- Lua script of map first_map.
-- This script is executed every time the hero enters this map.

-- Feel free to modify the code below.
-- You can add more events and remove the ones you don't need.

-- See the Solarus Lua API documentation:
-- http://www.solarus-games.org/doc/latest

local map = ...
local game = map:get_game()

local cam1, cam2

-- Event called at initialization time, as soon as this map becomes is loaded.
function map:on_started()

  -- You can initialize the movement and sprites of various
  -- map entities here.
  cam1 = map:get_camera()
  cam2 = map:create_camera{
    name  = "camera2",
    layer = 0,
    x = 320,
    y = 0
  }
  
  cam1:set_size(320/2-10, 240-10)
  cam1:set_position_on_screen(5,5)
  cam2:set_size(320/2-10, 240-10)
  cam2:set_position_on_screen(320/2+5,5)
  
  cam1:set_viewport(0,0,0.25,1)
  cam2:set_viewport(0.25,0,0.75,1)
  
  cam2:start_tracking(richard)
  

  
  local mov = sol.movement.create"random"
  mov:start(richard)
end

function map:on_draw(dst)
  local x,y = cam1:get_position()
  local w,h = cam1:get_size()
  dst:fill_color({255,0,0,128},x,y,w,h)
end

-- Event called after the opening transition effect of the map,
-- that is, when the player takes control of the hero.
function map:on_opening_transition_finished()
end
