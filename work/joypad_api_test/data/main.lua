-- This is the main Lua script of your project.
-- You will probably make a title screen and then start a game.
-- See the Lua API! http://www.solarus-games.org/doc/latest

local game_manager = require'scripts/game_manager'

--sol.video.set_geometry_mode('dynamic_absolute')

print("patate ?")

sol.commands.set_analog_commands_enabled(true)

local game

local function joypad_listen(jpad)
  local i = jpad:get_name()
  print("registred callbacks for", i, "rumble" , jpad:has_rumble())
  function jpad:on_button_pressed(button)
      print(i,"pressed",button)
      if button == 'a' then
        jpad:rumble(1, 200)
      end
      
      if button == 'right_shoulder' then
        game:simulate_command_pressed('custom_cmd')
      end
  end
  function jpad:on_button_released(button)
    print(i,"released",button)
    if button == 'right_shoulder' then
        game:simulate_command_released('custom_cmd')
      end
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
  
  game = game_manager:create('foo.sav')
  game:start()
  
  function game:on_command_pressed(cmd)
    print("command", cmd, "pressed")
  end
  
  function game:on_command_released(cmd)
    print("command", cmd, "released")
  end
  
  function game:on_command_axis_moved(axis, state)
    print("command axis", axis, "state", state)
  end
  
  print("prout")
end

function sol.input:on_joypad_connected(joypad)
  print(joypad:get_name(), "attached")
  joypad_listen(joypad)
end