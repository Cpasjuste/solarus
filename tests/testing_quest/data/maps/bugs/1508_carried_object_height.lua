local map = ...
local game = map:get_game()

function map:on_started()
  hero:set_carry_height(30)
end

function get_height(carried)
  local hx, hy = carried:get_carrier():get_position()
  local cx, cy = carried:get_position()
  return hy - cy
end

function map:on_opening_transition_finished()
  game:simulate_command_pressed("action")
  sol.timer.start(map, 1000, function()
    assert_equal(get_height(hero:get_carried_object()), 30)
    assert_equal(get_height(hero:get_carried_object()), hero:get_carry_height())
    hero:set_carry_height(45)
    sol.timer.start(map, 100, function()
      assert_equal(get_height(hero:get_carried_object()), 45)
      hero:get_carried_object():set_object_height(-20)
      sol.timer.start(map, 100, function()
        assert_equal(get_height(hero:get_carried_object()), 25)
        sol.main.exit()
      end)
    end)
  end)
end

function hero:on_state_changed(state)

end
