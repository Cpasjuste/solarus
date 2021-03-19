local map = ...
local game = map:get_game()

function map:on_opening_transition_finished()
  sol.game.delete("save_initial.dat")
  game:set_command_keyboard_binding("attack", [[\]])
  game:save()

  local game_2 = sol.game.load("save_initial.dat")
  sol.game.delete("save_initial.dat")
  sol.main.exit()
end
