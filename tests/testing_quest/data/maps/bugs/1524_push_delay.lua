local map = ...
local game = map:get_game()

function map:on_started()

end

function map:on_opening_transition_finished()
  local hero = game:get_hero()
  hero:set_push_delay(300)
  assert_equal(hero:get_push_delay(), 300)
  game:simulate_command_pressed("right")
  sol.timer.start(hero, 350, function()
    assert_equal(hero:get_state(), "pushing")
  end)
  sol.main.exit()
end
