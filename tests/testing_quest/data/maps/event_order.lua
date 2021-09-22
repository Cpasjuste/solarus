-- Lua script of map event_order.
-- This script is executed every time the hero enters this map.

-- Feel free to modify the code below.
-- You can add more events and remove the ones you don't need.

-- See the Solarus Lua API documentation:
-- http://www.solarus-games.org/doc/latest

local map = ...
local game = map:get_game()
local hero = game:get_hero()

local just_log = false

local event_log = {
}

local i = 1
local function check_event(target, name, ...)
  table.insert(event_log, {target=target, name=name, args={...}})
  target[name] = function(this, ...)
    -- just log
    print(sol.main.get_type(this), name, ...)
    local expected = event_log[i]
    i = i + 1
    if expected and not just_log then
      -- check event name and args
      assert_equal(this, expected.target,  "Wrong self argument for event")
      assert_equal(name, expected.name, "wrong event name")
      for j = 1,#expected.args do
        assert_equal(select(j, ...), expected.args[j], "wrong event argument")
      end
    end
    if i == #event_log then
      sol.main.exit() -- succeed if we've seen all events in order
    end
  end
end

--order matter here !
--check_event(game, "on_started")

function game:on_started()
  error("game:on_started called after map loading")
end


-- test events that should be raised on first map load
check_event(map, "on_started")
check_event(game, "on_map_changed")
check_event(destination, "on_activated") -- new in 1.7
check_event(map, "on_suspended", false)
check_event(map, "on_opening_transition_finished")

-- when pausing
check_event(game, "on_paused")
check_event(map, "on_suspended", true)
check_event(game, "on_unpaused")
check_event(map, "on_suspended", false)

-- on teleporting to same map
check_event(map, "on_suspended", true)
check_event(destination, "on_activated")
check_event(map, "on_suspended", false)
check_event(map, "on_opening_transition_finished")

-- pause and unpause the game
sol.timer.start(sol.main, 5000, function()
    game:simulate_command_pressed("pause")
    game:simulate_command_released("pause")
    sol.timer.start(sol.main, 1000, function()
        game:simulate_command_pressed("pause")
        game:simulate_command_released("pause")
  end)
end)

sol.timer.start(8000, function()
  hero:teleport("event_order", "destination")
end)

-- timeout after 20s
sol.timer.start(20000, function() error("Timeout!!") end)