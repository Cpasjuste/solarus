-- Lua script of map bugs/1424_rawget_rawset.

local map = ...
local game = map:get_game()

local teletransporter_meta = sol.main.get_metatable("teletransporter")
local function meta_on_activated(self) end
local function unique_on_activated(self) end
assert(meta_on_activated ~= unique_on_activated)

function map:on_started()
  self:test_on_userdata()

  -- Make sure that the behaviour on tables matches Lua's rawget and rawset.
  self:test_on_table(rawget, rawset)
  self:test_on_table(sol.main.rawget, sol.main.rawset)

  -- No delays, it is all about code state.
  sol.main.exit()
end

function map:test_on_userdata()
  -- Set up visible outside of the script.
  teletransporter_meta.on_activated = meta_on_activated
  tele2.on_activated = unique_on_activated

  -- Testing sol.main.rawget on userdata.
  assert_equal(tele1.on_activated, meta_on_activated, "__index: tele1")
  assert_equal(tele2.on_activated, unique_on_activated, "__index: tele2")
  assert_equal(sol.main.rawget(tele1, "on_activated"), nil, "rawget: tele1")
  assert_equal(sol.main.rawget(tele2, "on_activated"), unique_on_activated, "rawget: tele2")
end

function map:test_on_table(get, set)
  local data = {a = 1, b = 2, c = 3}

  -- Check the formal cases:
  assert_equal(get(data, "a"), 1, "1 not found at a")
  assert_equal(get(data, "z"), nil, "nil not found at z")
  set(data, "d", 4)
  assert_equal(data.d, 4, "4 is not found at d")

  -- Edge cases that might not be documented:
  assert_equal(get(data, nil), nil, "nil not found at nil")
  assert_equal(get(data, "a", "b", "c"), 1, "extra argument test get failed")
  set(data, "a", 5, 6)
  assert_equal(data.a, 5, "extra argument test set failed")

  -- Ensure all error cases are errors.
  assert_error{get, data, label="get with 1 argument worked"}
  assert_error{set, data, label="set with 1 argument worked"}
  assert_error{set, data, "a", label="set with 2 arguments worked"}
  assert_error{set, data, nil, 9, label="set index nil worked"}
end
