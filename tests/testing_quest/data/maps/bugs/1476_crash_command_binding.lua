local map = ...
local game = map:get_game()

function map:on_opening_transition_finished()
  local game_2 = sol.game.load("save_1476")
  local success, error_message = pcall(game_2.set_command_keyboard_binding, game_2, "action", "space")
  assert_equal(success, false)
  assert(error_message:match("is not running"))
  sol.main.exit()
end
