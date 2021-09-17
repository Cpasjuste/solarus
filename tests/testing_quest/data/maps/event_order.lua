-- Lua script of map event_order.
-- This script is executed every time the hero enters this map.

-- Feel free to modify the code below.
-- You can add more events and remove the ones you don't need.

-- See the Solarus Lua API documentation:
-- http://www.solarus-games.org/doc/latest

local map = ...
local game = map:get_game()

local just_log = true

local event_log = {
}

local i = 1
local function check_event(target, name)
  table.insert(event_log, {target=target, name=name})
  target[name] = function(this, ...)
    -- just log
    print(sol.main.get_type(this), name)
    local expected = event_log[i]
    i = i + 1
    if expected and not just_log then
      -- check event name and args
      assert_equal(this, expected.target,  "Wrong self argument for event")
      assert_equal(name, expected.name, "wrong event name")
    end
  end
end

--order matter here !
check_event(game, "on_started")
check_event(map, "on_started")
check_event(game, "on_map_changed")
check_event(map, "on_opening_transition_finished")



sol.timer.start(2000, function() error("Timeout!!") end)