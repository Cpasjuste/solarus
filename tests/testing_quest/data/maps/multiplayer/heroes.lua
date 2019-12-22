-- Lua script of map multiplayer/heroes.
-- This script is executed every time the hero enters this map.

-- Feel free to modify the code below.
-- You can add more events and remove the ones you don't need.

-- See the Solarus Lua API documentation:
-- http://www.solarus-games.org/doc/latest

local map = ...
local game = map:get_game()
local hero = map:get_hero()

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

  -- You can initialize the movement and sprites of various
  -- map entities here.
  local x,y,l = alter_destination:get_position()
  alter_hero = map:create_hero{
    x = x,
    y = y,
    layer = l,
  }
  
  commands = game:get_commands()
  alter_commands = alter_hero:get_commands()
  alter_hero:set_ability("sword", 1)
  
  game:set_ability('sword', 1)
  --hero:set_enabled(false)
  
  test_equipment()
end



-- Event called after the opening transition effect of the map,
-- that is, when the player takes control of the hero.
function map:on_opening_transition_finished()
  map:get_camera():start_tracking(alter_hero)
  
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

