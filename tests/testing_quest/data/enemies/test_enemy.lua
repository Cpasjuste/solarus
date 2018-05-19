local enemy = ...
local game = enemy:get_game()
local map = enemy:get_map()
local hero = map:get_hero()

function enemy:on_created()

  sprite = enemy:create_sprite("enemies/test_enemy")
  enemy:set_size(16, 16)
end
