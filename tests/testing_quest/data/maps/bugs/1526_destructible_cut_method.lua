local map = ...
local game = map:get_game()

function map:on_started()
  game:set_ability("sword", 1)
end

function map:on_opening_transition_finished()

  game:simulate_command_pressed("attack")
  game:simulate_command_released("attack")

  assert_equal(destructible_1:get_cut_method(), "aligned")
  assert_equal(destructible_2:get_cut_method(), "aligned")
  assert_equal(destructible_3:get_cut_method(), "aligned")
  assert_equal(destructible_4:get_cut_method(), "aligned")

  sol.timer.start(map, 2000, function()
    assert(destructible_1 ~= nil)
    assert(destructible_2 == nil)
    assert(destructible_3 ~= nil)
    assert(destructible_4 ~= nil)

    destructible_1:set_cut_method("pixel")
    destructible_3:set_cut_method("pixel")
    destructible_4:set_cut_method("pixel")

    assert_equal(destructible_1:get_cut_method(), "pixel")
    assert_equal(destructible_3:get_cut_method(), "pixel")
    assert_equal(destructible_4:get_cut_method(), "pixel")

    game:simulate_command_pressed("attack")
    game:simulate_command_released("attack")

    sol.timer.start(map, 2000, function()
      assert(destructible_1 == nil)
      assert(destructible_2 == nil)
      assert(destructible_3 == nil)
      assert(destructible_4 ~= nil)

      destructible_4:set_cut_method("aligned")
      assert_equal(destructible_4:get_cut_method(), "aligned")

      sol.main.exit()
    end)
  end)
end
