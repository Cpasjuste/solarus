local map = ...
local game = map:get_game()

function map:on_started()

  assert_equal(pickable:get_layer(), 2)
end

function map:on_opening_transition_finished()

  assert_equal(pickable:get_layer(), 1)
  sol.main.exit()
end
