local map = ...
local game = map:get_game()

function map:on_started()
  assert_equal(hero:get_layer(), destination:get_layer())
end

function map:on_opening_transition_finished()
  assert_equal(hero:get_layer(), destination:get_layer())
  sol.main.exit()
end
