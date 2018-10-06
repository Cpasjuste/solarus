local map = ...
local game = map:get_game()

function map:on_opening_transition_finished()

  local state = sol.state.create()
  assert_equal(state:get_previous_carried_object_behavior(), "throw")
  state:set_previous_carried_object_behavior("keep")
  assert_equal(state:get_previous_carried_object_behavior(), "keep")

  game:simulate_command_pressed("action")
  sol.timer.start(map, 1000, function()
    assert_equal(hero:get_state(), "carrying")
    hero:start_state(state)
    assert_equal(hero:get_state(), "custom")
    assert_equal(hero:get_state_object(), state)
    assert(hero:get_carried_object() ~= nil)
    sol.main.exit()
  end)
end
