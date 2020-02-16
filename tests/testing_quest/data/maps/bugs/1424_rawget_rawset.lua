-- Lua script of map bugs/1424_rawget_rawset.

local map = ...
local game = map:get_game()

local teletransporter_meta = sol.main.get_metatable("teletransporter")
local function meta_on_activated(self) end
local function unique_on_activated(self) end
assert(meta_on_activated ~= unique_on_activated)

function map:on_started()
  -- Set up visible outside of the script.
  teletransporter_meta.on_activated = meta_on_activated
  tele2.on_activated = unique_on_activated

  -- Testing sol.main.rawget on userdata.
  assert(tele1.on_activated == meta_on_activated, "__index: tele1")
  assert(tele2.on_activated == unique_on_activated, "__index: tele2")
  assert(sol.main.rawget(tele1, "on_activated") == nil, "rawget: tele1")
  assert(sol.main.rawget(tele2, "on_activated") == unique_on_activated, "rawget: tele2")

  -- No delays, it is all about code state.
  sol.main.exit()
end
