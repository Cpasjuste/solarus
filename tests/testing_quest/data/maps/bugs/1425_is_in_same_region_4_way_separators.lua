-- Lua script of map bugs/1425_is_in_same_region_4_way_separators.
-- This script is executed every time the hero enters this map.

local map = ...
local hero = map:get_hero()
local tiles = {tile_a = true, tile_b = false, tile_c = false, tile_d = false}

-- Event called at initialization time, as soon as this map is loaded.
function map:on_started()
    for tile, expected in pairs(tiles) do
        local entity = map:get_entity(tile)
        assert_equal(hero:is_in_same_region(entity), expected, tile)
    end
    sol.main.exit()
end
