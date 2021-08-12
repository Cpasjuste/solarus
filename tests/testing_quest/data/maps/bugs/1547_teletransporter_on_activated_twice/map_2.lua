-- Lua script of map bugs/1547_teletransporter_on_activated_twice/map_2.

local map = ...
local game = map:get_game()

function map:on_opening_transition_finished()
  sol.main.exit()
end
