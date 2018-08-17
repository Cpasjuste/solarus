local map = ...
local game = map:get_game()
local hero = map:get_hero()

local test_maps = {
  { "teletransportation_tests/start_in_hole" },
  { "teletransportation_tests/start_in_deep_water_drown" },
  { "teletransportation_tests/start_in_deep_water_swim" },
  { "teletransportation_tests/start_in_shallow_water" },
  { "teletransportation_tests/start_in_lava" },
  { "teletransportation_tests/start_in_stairs" },
  { "teletransportation_tests/start_same_point", "_same" },
}

function hero:assert_position_equal(other)

  local x_1, y_1, layer_1 = hero:get_position()
  local x_2, y_2, layer_2 = other:get_position()
  assert(x_1 == x_2)
  assert(y_1 == y_2)
  assert(layer_1 == layer_2)
end

function hero:assert_state_ground_animation(state, ground, animation)

  assert_equal(hero:get_state(), state)

  local map = hero:get_map()
  local hero_x, hero_y, hero_layer = hero:get_ground_position()
  assert_equal(map:get_ground(hero_x, hero_y, hero_layer), ground)
  assert_equal(hero:get_ground_below(), ground)

  assert_equal(hero:get_sprite("tunic"):get_animation(), animation)
end

function map:on_started()

  sol.timer.start(game, 5000, function()
    if #test_maps > 0 then
      -- Next map in the list.
      local next_map = test_maps[1]
      table.remove(test_maps, 1)
      hero:teleport(unpack(next_map))
      return true  -- Repeat the timer.
    end
  end)
end
