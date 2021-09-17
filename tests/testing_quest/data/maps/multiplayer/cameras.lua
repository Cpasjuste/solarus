-- Lua script of map multiplayer/cameras.
-- This script is executed every time the hero enters this map.

-- Feel free to modify the code below.
-- You can add more events and remove the ones you don't need.

-- See the Solarus Lua API documentation:
-- http://www.solarus-games.org/doc/latest

local map = ...
local game = map:get_game()
local camera = map:get_camera()


-- Event called at initialization time, as soon as this map is loaded.
function map:on_started()

  -- You can initialize the movement and sprites of various
  -- map entities here.
end

local function check_iter(it, exp)
  local n = 0
  for el in it do    
    assert(el == exp[n+1], "an unexpected element")
    n = n + 1
  end
  assert_equal(n, #exp)
end


-- Event called after the opening transition effect of the map,
-- that is, when the player takes control of the hero.
function map:on_opening_transition_finished()
  if second_time then
    return
  end
  
  second_time = true
  
  game:start_coroutine(function()
    local alter_camera = game:create_camera("alter_camera", "multiplayer/cameras2")
    
    alter_camera:set_size(320/2, 240)
    alter_camera:set_position_on_screen(320/2, 0)
    
    camera:set_size(320/2, 240)
    
    wait(1000)
    
    -- check there is only the default camera in map
    check_iter(map:get_cameras(), {camera})
    
    camera:teleport("multiplayer/cameras2", "on_four")
    
    wait(1000)
    
    local other_map = camera:get_map()
    local middle_sensor = other_map:get_entity('middle_sensor')
    
    -- check camera appeared at the right spot
    assert_equal_xy(camera, middle_sensor)
    
    -- check if cameras are correctly in game
    check_iter(game:get_cameras(), {camera, alter_camera})
    
    -- check if cameras are in second map
    check_iter(other_map:get_cameras(), {alter_camera, camera})
    
    -- check if maps are in game
    check_iter(game:get_maps(), {other_map, map}) 
    
    
    wait(1000)
    
    game:remove_camera(camera)
    
    wait(3000)
    
    check_iter(game:get_maps(), {other_map})
    check_iter(game:get_cameras(), {alter_camera})
    check_iter(other_map:get_cameras(), {alter_camera})
    
    sol.main.exit()
  end)
end

local cam_order = {"alter_camera", "main_camera"}
local i = 1

function game:on_map_changed(map, camera)
  local expected = cam_order[i]
  --assert_equal(camera:get_name(), expected)
  print("Changing cam",i, camera:get_name())
  i = i +1
end
