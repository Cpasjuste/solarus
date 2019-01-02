-- This is the main Lua script of your project.
-- You will probably make a title screen and then start a game.
-- See the Lua API! http://www.solarus-games.org/doc/latest

local function joypad_listen(jpad)
  local i = jpad:get_name()
  print("registred callbacks for", i)
  function jpad:on_button_pressed(button)
      print(i,"pressed",button)
  end
  function jpad:on_button_released(button)
    print(i,"released",button)
  end
  function jpad:on_axis_moved(axis, val)
    print(i,axis,val)
  end
  function jpad:on_removed()
    print(i,"detached")
  end
end

-- This function is called when Solarus starts.
function sol.main:on_started()
  local jcount = sol.input.get_joypad_count()
  local joypads = sol.input.get_joypads()
  assert(jcount == #joypads)
  print(jcount, "Joypads")
  for _, jpad in ipairs(joypads) do
    joypad_listen(jpad)
  end
end

function sol.input:on_joypad_connected(joypad)
  print(joypad:get_name(), "attached")
  joypad_listen(joypad)
end