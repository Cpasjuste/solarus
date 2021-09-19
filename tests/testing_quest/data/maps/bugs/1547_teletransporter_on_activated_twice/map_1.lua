-- Lua script of map bugs/1547_teletransporter_on_activated_twice/map_1.

local map = ...
local game = map:get_game()

function map:on_opening_transition_finished()
  game:simulate_command_pressed"down"
end

local unactivated = true

function south_exit:on_activated()
  assert(unactivated, "on_activated called twice")
  unactivated = false
end
