local map = ...
local game = map:get_game()

function map:on_opening_transition_finished()

  local local_block = map:get_entity("block")
  local local_block_2 = map:get_entity("block_2")

  assert(block ~= nil)
  assert_equal(block:get_name(), "block")
  assert(local_block ~= nil)
  assert_equal(local_block:get_name(), "block")

  assert(block_2 ~= nil)
  assert_equal(block_2:get_name(), "block_2")
  assert(local_block_2 ~= nil)
  assert_equal(local_block_2:get_name(), "block_2")

  -- Remove the name.
  local_block:set_name(nil)
  assert_equal(local_block:get_name(), nil)
  assert_equal(block, nil)

  -- Restore the same name.
  local_block:set_name("block")
  assert_equal(local_block:get_name(), "block")
  assert(block ~= nil)
  assert_equal(block:get_name(), "block")
  assert_equal(local_block, block)

  -- Rename to an already used name.
  local_block:set_name("block_2")
  assert_equal(block, nil)
  assert(block_2 ~= nil)
  assert_equal(block_2:get_name(), "block_2")
  assert(local_block_2 ~= nil)
  assert_equal(local_block_2:get_name(), "block_2")
  assert_equal(local_block:get_name(), "block_3")
  assert(block_3 ~= nil)
  assert_equal(block_3:get_name(), "block_3")
  assert_equal(local_block, block_3)

  -- Rename to the same.
  local_block:set_name("block_3")
  assert_equal(local_block:get_name(), "block_3")
  assert(block_3 ~= nil)
  assert_equal(block_3:get_name(), "block_3")
  assert_equal(local_block, block_3)

  -- Rename the second one to the old name of the first one.
  local_block_2:set_name("block")
  assert_equal(local_block_2:get_name(), "block")
  assert(block ~= nil)
  assert_equal(block:get_name(), "block")
  assert_equal(local_block_2, block)
  assert(block_2 == nil)

  -- Rename the first one to the already used name again.
  local_block:set_name("block")
  assert_equal(local_block:get_name(), "block_2")
  assert(block_2 ~= nil)
  assert_equal(local_block, block_2)
  assert(block ~= nil)
  assert_equal(local_block_2:get_name(), "block")
  assert_equal(local_block_2, block)

  sol.main.exit()
end
