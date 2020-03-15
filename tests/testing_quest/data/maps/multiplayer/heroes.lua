-- Lua script of map multiplayer/heroes.
-- This script is executed every time the hero enters this map.

-- Feel free to modify the code below.
-- You can add more events and remove the ones you don't need.

-- See the Solarus Lua API documentation:
-- http://www.solarus-games.org/doc/latest

local map = ...
local game = map:get_game()
local hero = map:get_hero()
local camera = map:get_camera()

local commands
local alter_commands
local alter_hero

local function test_equipment()
  
  -- get all values from main hero equipement
  local ohml = hero:get_max_life()
  local ohl = hero:get_life()
  local ohmmo = hero:get_max_money()
  local ohmo = hero:get_money()
  local ohmma = hero:get_max_magic()
  local ohma = hero:get_magic()
  
  -- verify that those data correspond to game
  local function game_correspond()
    assert_equal(ohml, game:get_max_life())
    assert_equal(ohl, game:get_life())
    assert_equal(ohmmo, game:get_max_money())
    assert_equal(ohmo, game:get_money())
    assert_equal(ohmma, game:get_max_magic())
    assert_equal(ohma, game:get_magic())
  end
  
  game_correspond()
  
  local function verify_orignal_hero()
    assert_equal(ohml, hero:get_max_life())
    assert_equal(ohl, hero:get_life())
    assert_equal(ohmmo, hero:get_max_money())
    assert_equal(ohmo, hero:get_money())
    assert_equal(ohmma, hero:get_max_magic())
    assert_equal(ohma, hero:get_magic())
  end
  
  verify_orignal_hero()
  
  local function check_property(name, val)
    alter_hero['set_' .. name](alter_hero, val)
    local gval = alter_hero['get_' .. name](alter_hero)
    assert_equal(val, gval)
    verify_orignal_hero()
    game_correspond()
  end
  
  check_property('max_life', 10)
  check_property('life', 5)
  check_property('max_money', 100)
  check_property('money', 50)
  check_property('max_magic', 10)
  check_property('magic', 5)
end

-- Event called at initialization time, as soon as this map is loaded.
function map:on_started()

  if first_time then
    return
  end
  
  -- You can initialize the movement and sprites of various
  -- map entities here.
  local x,y,l = alter_destination:get_position()
  alter_hero = map:create_hero{
    x = x,
    y = y,
    layer = l,
  }
  
  alter_hero.is_alter = true
  
  commands = game:get_controls()
  alter_commands = alter_hero:get_controls()
  alter_hero:set_ability("sword", 1)
  
  game:set_ability('sword', 1)
  --hero:set_enabled(false)
  
  test_equipment()
end


-- Event called after the opening transition effect of the map,
-- that is, when the player takes control of the hero.
function map:on_opening_transition_finished()
  --map:get_camera():start_tracking(alter_hero)
  if first_time then
    return
  end
  
  first_time = true
  
  game:start_coroutine(function()
    wait(1000)
    assert(dummy:exists())
    
    alter_commands:simulate_pressed('attack')
    alter_commands:simulate_released('attack')
    
    wait(2000)
    assert(not dummy or not dummy:exists())
    
    alter_commands:simulate_pressed('right')
  end)
end

function corner_sensor:on_activated(ahero)
  if not ahero.is_alter then return end
  
  alter_commands = ahero:get_controls()
  
  alter_commands:simulate_released('right')
  alter_commands:simulate_pressed('down')
end

local alter_camera
function to_side_sensor:on_activated(ahero)
  if ahero ~= alter_hero then
    return
  end
  
  if alter_camera then
    return
  end
  
  
  -- Setup cameras for a vertical split-screen
  alter_camera = game:create_camera("alter_camera", map:get_id())
  
  
  alter_camera:set_size(320/2, 240)
  alter_camera:set_position_on_screen(320/2, 0)
  
  --alter_camera:teleport(map:get_id())
  alter_camera:start_tracking(alter_hero)
  
  camera:set_size(320/2, 240)
  
  alter_commands:simulate_pressed('right')
  alter_commands:simulate_released('down')
  
  sol.timer.start(1000, function()
    commands:simulate_pressed('left')
  end)
end

function to_corner_tp:on_activated(ahero)
  assert_equal(ahero, alter_hero)
  
  ahero:teleport(to_corner_tp:get_destination_map(), to_corner_tp:get_destination_name())
end

function dest_inner:on_activated(hero)
  print("ALLO ?", hero)
end
