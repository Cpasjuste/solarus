/*
 * Copyright (C) 2006-2019 Christopho, Solarus - http://www.solarus-games.org
 *
 * Solarus is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Solarus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "solarus/core/Debug.h"
#include "solarus/core/Equipment.h"
#include "solarus/core/Game.h"
#include "solarus/core/Controls.h"
#include "solarus/core/MainLoop.h"
#include "solarus/core/Savegame.h"
#include "solarus/entities/Hero.h"
#include "solarus/lua/LuaContext.h"
#include <lua.hpp>
#include <sstream>

namespace Solarus {

bool Controls::analog_commands_enabled = false;

const std::string Controls::direction_names[4] = {
  "right",
  "up",
  "left",
  "down"
};

const uint16_t Controls::direction_masks[4] = {
  0x0001,
  0x0002,
  0x0004,
  0x0008
};

const int Controls::masks_to_directions8[] = {
  -1,  // none: stop
  0,  // right
  2,  // up
  1,  // right + up
  4,  // left
  -1,  // left + right: stop
  3,  // left + up
  -1,  // left + right + up: stop
  6,  // down
  7,  // down + right
  -1,  // down + up: stop
  -1,  // down + right + up: stop
  5,  // down + left
  -1,  // down + left + right: stop
  -1,  // down + left + up: stop
  -1,  // down + left + right + up: stop
};


/**
 * \brief Default binding Constructor.
 * \param main_loop, the main_loop of the game
 */
Controls::Controls(MainLoop &main_loop):
  main_loop(main_loop),
  customizing(false),
  command_to_customize(CommandId::NONE),
  customize_callback_ref() {
}

Controls::Controls(MainLoop& main_loop, Game& game):
  main_loop(main_loop),
  customizing(false),
  command_to_customize(CommandId::NONE),
  customize_callback_ref()
{
  load_default_joypad_bindings();
  load_default_keyboard_bindings();

  const Savegame& save = game.get_savegame();
  // Load the commands from the savegame.
  for (const auto& kvp : EnumInfoTraits<CommandId>::names) {

    CommandId command = kvp.first;
    if (command == CommandId::NONE) {
      continue;
    }

    // Keyboard.
    InputEvent::KeyboardKey keyboard_key = get_saved_keyboard_binding(command, save);
    keyboard_mapping[keyboard_key] = command;

    // Joypad.
    const JoypadBinding& joypad_binding = get_saved_joypad_binding(command, save);
    joypad_mapping[joypad_binding] = command;
  }

  //Replicate binding of keyboard on default command axes :
  keyboard_axis_mapping[get_saved_keyboard_binding(CommandId::UP, save)] = ControlAxisBinding{AxisId::Y, AxisDirection::MINUS};
  keyboard_axis_mapping[get_saved_keyboard_binding(CommandId::DOWN, save)] = ControlAxisBinding{AxisId::Y, AxisDirection::PLUS};
  keyboard_axis_mapping[get_saved_keyboard_binding(CommandId::LEFT, save)] = ControlAxisBinding{AxisId::X, AxisDirection::MINUS};
  keyboard_axis_mapping[get_saved_keyboard_binding(CommandId::RIGHT, save)] = ControlAxisBinding{AxisId::X, AxisDirection::PLUS};

  //Add default joypad if auto mapping is enabled
  if(InputEvent::is_legacy_joypad_enabled()) {
    set_joypad(InputEvent::other_joypad(nullptr));
  }
}

Controls::~Controls() {
  ControlsDispatcher::get().remove_commands(this);
}

/**
 * \brief Returns whether the specified game command is pressed.
 *
 * The command can be activated from the keyboard or the joypad.
 *
 * \param command A game command.
 * \return true if this game command is currently pressed.
 */
bool Controls::is_command_pressed(const Command&command) const {
  return commands_pressed.find(command) != commands_pressed.end();
}

/**
 * @brief Returns the state of any command axis
 *
 * The state is 0 as long as the axis as not been moved or do not exist
 *
 * @param axis
 * @return
 */
double Controls::get_axis_state(const Axis &axis) const {
  const auto& it = command_axes_state.find(axis);
  if(it != command_axes_state.end()) {
    return it->second;
  } else {
    return 0.0;
  }
}

/**
 * \brief Returns the direction corresponding to the directional commands
 * currently pressed by the player.
 * \return The direction (0 to 7), or -1 if no directional command is pressed
 * or if the combination of directional command is not a valid direction.
 */
int Controls::get_wanted_direction8() const {
  uint16_t direction_mask = 0x0000;

  if(are_analog_commands_enabled()) {
    double x = get_axis_state(AxisId::X);
    double y = get_axis_state(AxisId::Y);

    if(x > 0) {
      direction_mask |= direction_masks[0];
    }
    if (y < 0) {
      direction_mask |= direction_masks[1];
    }
    if (x < 0) {
      direction_mask |= direction_masks[2];
    }
    if (y > 0) {
      direction_mask |= direction_masks[3];
    }
  } else {
    if (is_command_pressed(CommandId::RIGHT)) {
      direction_mask |= direction_masks[0];
    }
    if (is_command_pressed(CommandId::UP)) {
      direction_mask |= direction_masks[1];
    }
    if (is_command_pressed(CommandId::LEFT)) {
      direction_mask |= direction_masks[2];
    }
    if (is_command_pressed(CommandId::DOWN)) {
      direction_mask |= direction_masks[3];
    }
  }

  return masks_to_directions8[direction_mask];
}

std::pair<double, double> Controls::get_wanted_polar() const {
  double x = 0.0;
  double y = 0.0;
  if(are_analog_commands_enabled()) {
    x = get_axis_state(AxisId::X);
    y = get_axis_state(AxisId::Y);
  } else {
    if (is_command_pressed(CommandId::RIGHT)) {
      x += 1.0;
    }
    if (is_command_pressed(CommandId::UP)) {
      y -= 1.0;
    }
    if (is_command_pressed(CommandId::LEFT)) {
      x -= 1.0;
    }
    if (is_command_pressed(CommandId::DOWN)) {
      y += 1.0;
    }
  }
  double angle = atan2(-y, x);
  double norm = std::min(sqrt(x*x+y*y), 1.0); //Clamp the norm to 1
  return {norm, angle};
}

/**
 * \brief This function is called by the game when a low-level event occurs.
 * \param event An input event.
 */
void Controls::notify_input(const InputEvent& event) {

  // If no game command is being customized, we look for a binding
  // for this input event and we ignore the event if no binding is found.
  // If a command is being customized, we consider instead this event as
  // the new binding for this game command.

  if (event.is_keyboard_key_pressed()) {
    keyboard_key_pressed(event.get_keyboard_key());
  }
  else if (event.is_keyboard_key_released()) {
    keyboard_key_released(event.get_keyboard_key());
  } else if(event.is_joypad_event() && event.get_joypad() == joypad) {
    //Only handle events of the selected joypad
    if (event.is_joypad_button_released()) {
      joypad_button_released(event.get_joypad_button());
    }
    else if (event.is_joypad_button_pressed()) {
      joypad_button_pressed(event.get_joypad_button());
    }
    else if (event.is_joypad_axis_moved()) {
      joypad_axis_moved(event.get_joypad_axis(), event.get_joypad_axis_state());
    }
    else if (event.is_joypad_hat_moved()) {
      joypad_hat_moved(event.get_joypad_hat(), event.get_joypad_hat_direction());
    }
  }
}

/**
 * \brief This function is called when a low-level keyboard key is pressed.
 * \param keyboard_key_pressed The key pressed.
 */
void Controls::keyboard_key_pressed(InputEvent::KeyboardKey keyboard_key_pressed) {

  // Retrieve the game command (if any) corresponding to this keyboard key.
  const Command command = get_command_from_keyboard(keyboard_key_pressed);

  if (!customizing) {
    // If the key is mapped, notify the game.
    if (command != Command(CommandId::NONE)) {
      command_pressed(command);
    }

    ControlAxisBinding cab = get_axis_from_keyboard(keyboard_key_pressed);

    if(cab.axis != Axis(AxisId::NONE)) {
      command_axis_moved(cab.axis, get_axis_state(cab.axis)+(cab.direction == AxisDirection::PLUS ? 1.0 : -1.0));
    }
  }
  else {
    customizing = false;

    if (command != command_to_customize) {
      // Consider this keyboard key as the new mapping for the game command being customized.
      set_keyboard_binding(command_to_customize, keyboard_key_pressed);
      commands_pressed.insert(command_to_customize);
    }
    do_customization_callback();
  }
}

/**
 * \brief This function is called when a low-level keyboard key is released.
 * \param keyboard_control_released The key released.
 */
void Controls::keyboard_key_released(InputEvent::KeyboardKey keyboard_key_released) {

  // Retrieve the game command (if any) corresponding to this keyboard key.
  Command command = get_command_from_keyboard(keyboard_key_released);

  // If the keyboard key is mapped, notify the game.
  if (command != Command(CommandId::NONE)) {
    command_released(command);
  }

  ControlAxisBinding cab = get_axis_from_keyboard(keyboard_key_released);

  if(cab.axis != Axis(AxisId::NONE)) {
    command_axis_moved(cab.axis, get_axis_state(cab.axis)+(cab.direction == AxisDirection::PLUS ? -1.0 : 1.0));
  }
}

/**
 * \brief This function is called when a joypad button is pressed.
 * \param button The button pressed.
 */
void Controls::joypad_button_pressed(JoyPadButton button) {

  // Retrieve the game command (if any) corresponding to this joypad button.
  auto binding = JoypadBinding(button);
  Command command = get_command_from_joypad(binding);

  if (!customizing) {
    // If the joypad button is mapped, notify the game.
    if (command != Command(CommandId::NONE)) {
      command_pressed(command);
    }
  }
  else {
    customizing = false;

    if (command != command_to_customize) {
      // Consider this button as the new mapping for the game command being customized.
      set_joypad_binding(command_to_customize, binding);
      commands_pressed.insert(command_to_customize);
    }
    do_customization_callback();
  }
}

/**
 * \brief This function is called when a joypad button is released.
 * \param button The button released.
 */
void Controls::joypad_button_released(JoyPadButton button) {
  auto binding = JoypadBinding(button);
  Command command = get_command_from_joypad(binding);

  // If the key is mapped, notify the game.
  if (command != Command(CommandId::NONE)) {
    command_released(command);
  }
}

/**
 * \brief This function is called when a joypad axis is moved.
 * \param axis The axis moved.
 * \param state The new axis direction (-1: left or up, 0: centered, 1: right or down).
 */
void Controls::joypad_axis_moved(JoyPadAxis axis, double state) {
  if (std::abs(state) < 1e-5) {
    // Axis in centered position : Test both positive and negative binding for release
    Command command = get_command_from_joypad(JoypadBinding(axis, AxisDirection::PLUS));
    if (command != Command(CommandId::NONE)) {
      command_released(command);
    }
    command = get_command_from_joypad(JoypadBinding(axis, AxisDirection::MINUS));
    if (command != Command(CommandId::NONE)) {
      command_released(command);
    }
  }
  else {
    // Axis not centered.
    auto binding = JoypadBinding(axis, state > 0 ? AxisDirection::PLUS : AxisDirection::MINUS);
    Command command = get_command_from_joypad(binding);
    Command inverse_command_pressed = get_command_from_joypad(JoypadBinding(axis, -state > 0 ? AxisDirection::PLUS : AxisDirection::MINUS));

    if (!customizing) {

      // If the command is mapped, notify the game.
      if (command != Command(CommandId::NONE)) {
        if (is_command_pressed(inverse_command_pressed)) {
          command_released(inverse_command_pressed);
        }
        command_pressed(command);
      }
    }
    else {
      customizing = false;

      if (command != command_to_customize) {
        // Consider this axis movement as the new mapping for the game command being customized.
        set_joypad_binding(command_to_customize, binding);
        commands_pressed.insert(command_to_customize);
      }
      do_customization_callback();
    }
  }

  //Handle command axes
  const ControlAxisBinding& ab = get_axis_from_joypad(axis);
  if(ab.axis != Axis(AxisId::NONE)) {
    //Axis event !
    command_axis_moved(ab.axis, ab.direction == AxisDirection::PLUS ? state : -state);
  }
}

/**
 * \brief This function is called when a joypad hat is moved.
 * \param hat The hat moved.
 * \param direction The new hat position (-1: centered, 0 to 7: a direction).
 */
void Controls::joypad_hat_moved(int hat, int value) {
  //TODO remove completly
  (void)hat; (void)value;
}

/**
 * \brief This function is called when a game command is pressed.
 *
 * This event may come from the keyboard or the joypad.
 *
 * \param command The game command pressed.
 */
void Controls::command_pressed(const Command& command) {
  commands_pressed.insert(command);
  main_loop.notify_control(ControlEvent::make_pressed(command, shared_from_this_cast<Controls>()));
}

/**
 * \brief This function is called when a game command is pressed.
 *
 * This event may come from the keyboard or the joypad.
 *
 * \param command The game command released.
 */
void Controls::command_released(const Command &command) {
  commands_pressed.erase(command);
  main_loop.notify_control(ControlEvent::make_released(command, shared_from_this_cast<Controls>()));
}

void Controls::command_axis_moved(const Axis& axis, double state) {
  command_axes_state[axis] = state;
  main_loop.notify_control(ControlEvent::make_moved(axis,state, shared_from_this_cast<Controls>()));
}

/**
 * \brief Returns the low-level keyboard key where the specified game command
 * is currently mapped.
 * \param command A game command.
 * \return The keyboard key mapped to this game command, or InputEvent::KEY_GameCommand::NONE
 * if the command is not mapped to a keyboard key.
 */
InputEvent::KeyboardKey Controls::get_keyboard_binding(const Command &command) const {

  for (const auto& kvp: keyboard_mapping) {

    if (kvp.second == command) {
      return kvp.first;
    }
  }

  return InputEvent::KeyboardKey::NONE;
}

/**
 * \brief Maps the specified keyboard key to a new game command.
 *
 * If this key was already mapped to a command, both commands are switched.
 * (The old command gets the previous key of the new command.)
 *
 * \param command A game command.
 * \param key The keyboard key to map to this game command, or InputEvent::KEY_NONE
 * to unmap the command.
 */
void Controls::set_keyboard_binding(const Command &command, InputEvent::KeyboardKey key) {

  InputEvent::KeyboardKey previous_key = get_keyboard_binding(command);
  Command previous_command = get_command_from_keyboard(key);

  if (previous_key != InputEvent::KeyboardKey::NONE) {
    // The command was already assigned.
    if (previous_command != Command(CommandId::NONE)) {
      // This key is already mapped to a command.
      keyboard_mapping[previous_key] = previous_command;
      //set_saved_keyboard_binding(previous_command, previous_key);
    }
    else {
      keyboard_mapping.erase(previous_key);
    }
  }

  if (key != InputEvent::KeyboardKey::NONE) {
    keyboard_mapping[key] = command;
  }
  //set_saved_keyboard_binding(command, key);
}

/**
 * \brief Returns a string representing the joypad action where the specified
 * game command is currently mapped.
 * \param command A game command.
 * \return The joypad action mapped to this game command, or an empty string if
 * this game command is not mapped to a joypad action.
 */
std::optional<Controls::JoypadBinding> Controls::get_joypad_binding(const Command &command) const {

  for (const auto& kvp: joypad_mapping) {

    if (kvp.second == command) {
      return kvp.first;
    }
  }
  return {};
}

/**
 * \brief Maps the specified joypad action to a new game command.
 *
 * If this joypad action was already mapped to a command, both commands are switched.
 * (The old command gets the previous joypad action of the new command.)
 *
 * \param command A game command.
 * \param joypad_string A string describing the joypad action to map to this
 * game command, or an empty string to unmap the command.
 */
void Controls::set_joypad_binding(const Command &command, const JoypadBinding& joypad_binding) {

  std::optional<JoypadBinding> previous_joypad_binding = get_joypad_binding(command);
  Command previous_command = get_command_from_joypad(joypad_binding);

  if (previous_joypad_binding) {
    // The command was already assigned.
    if (previous_command != Command(CommandId::NONE)) {
      // This joypad action is already mapped to a command.
      joypad_mapping[*previous_joypad_binding] = previous_command;
      //set_saved_joypad_binding(previous_command, previous_joypad_string);
    }
    else {
      joypad_mapping.erase(*previous_joypad_binding);
    }
  }

  joypad_mapping[joypad_binding] = command;

  //set_saved_joypad_binding(command, joypad_string);
}

/**
 * @brief Get the keys triggering change of the given command axis
 * @param command_axis an axis
 * @return two key for minus and plus moves
 */
std::pair<InputEvent::KeyboardKey, InputEvent::KeyboardKey> Controls::get_keyboard_axis_binding(const Axis& command_axis) const {
  InputEvent::KeyboardKey plus = InputEvent::KeyboardKey::NONE, minus = InputEvent::KeyboardKey::NONE;
  for(const auto& [key, binding] : keyboard_axis_mapping) {
    if(binding.axis == command_axis) {
      (binding.direction == AxisDirection::PLUS ? plus : minus) = key;
    }
  }
  return {minus, plus};
}

/**
 * @brief Maps the specified two keys to axis motion
 *
 * If the keys were already mapped to axises, bindings are switched
 *
 * @param command_axis the axis to move
 * @param minus key trigering negative state
 * @param plus key triggering positive state
 */
void Controls::set_keyboard_axis_binding(const Axis& command_axis, InputEvent::KeyboardKey minus, InputEvent::KeyboardKey plus) {
  auto [pm,pp] = get_keyboard_axis_binding(command_axis);
  auto prev_min = get_axis_from_keyboard(minus);
  auto prev_plus = get_axis_from_keyboard(plus);
  if(pm != InputEvent::KeyboardKey::NONE or pp != InputEvent::KeyboardKey::NONE){
    if(prev_min.axis != Axis(AxisId::NONE) || prev_plus.axis != Axis(AxisId::NONE)) {
      keyboard_axis_mapping[pm] = prev_min;
      keyboard_axis_mapping[pp] = prev_plus;
    } else {
      keyboard_axis_mapping.erase(pm);
      keyboard_axis_mapping.erase(pp);
    }
  }

  keyboard_axis_mapping[minus] = ControlAxisBinding{command_axis, AxisDirection::MINUS};
  keyboard_axis_mapping[plus] = ControlAxisBinding{command_axis, AxisDirection::PLUS};
}

/**
 * @brief Get the joypad axis bound to a given command axis
 *
 * Invalid joypad axis is returned if none match
 *
 * @param command_axis the axis
 * @return
 */
JoyPadAxis Controls::get_joypad_axis_binding(const Axis& command_axis) const {
  JoyPadAxis jaxis = JoyPadAxis::INVALID;
  for(const auto& [axis, binding] : joypad_axis_mapping) {
    if(binding.axis == command_axis){
      jaxis = axis;
    }
  }
  return jaxis;
}

/**
 * \brief Maps the specified joypad axis to a command axis
 *
 * If this joypad axis was already mapped to a command axis, both caxis are switched.
 *
 * \param command_axis A game command.
 * \param axis A joypad axis
 * game command, or an empty string to unmap the command.
 */
void Controls::set_joypad_axis_binding(const Axis& command_axis, JoyPadAxis axis) {
  auto previous_binding = get_joypad_axis_binding(command_axis);
  auto previous_command_axis = get_axis_from_joypad(axis);

  if(previous_binding != JoyPadAxis::INVALID){
    if(previous_command_axis.axis != Axis(AxisId::NONE)){
      joypad_axis_mapping[previous_binding] = previous_command_axis;
    } else {
      joypad_axis_mapping.erase(previous_binding);
    }
  }

  joypad_axis_mapping[axis] = ControlAxisBinding{command_axis, AxisDirection::PLUS};
}

  /**
 * \brief Returns the name of the savegame variable that stores the keyboard
 * mapping of a game command.
 * \param command A game command.
 * \return The savegame variable that stores the keyboard key mapped to this
 * game command, or an empty string if this command is GameCommand::NONE.
 */
  std::string Controls::get_keyboard_binding_savegame_variable(
        const Command& command) const {

    static const std::map<CommandId, std::string> savegame_variables = {
      { CommandId::NONE, "" },
      { CommandId::ACTION, Savegame::KEY_KEYBOARD_ACTION },
      { CommandId::ATTACK, Savegame::KEY_KEYBOARD_ATTACK },
      { CommandId::ITEM_1, Savegame::KEY_KEYBOARD_ITEM_1 },
      { CommandId::ITEM_2, Savegame::KEY_KEYBOARD_ITEM_2 },
      { CommandId::PAUSE, Savegame::KEY_KEYBOARD_PAUSE },
      { CommandId::RIGHT, Savegame::KEY_KEYBOARD_RIGHT },
      { CommandId::UP, Savegame::KEY_KEYBOARD_UP },
      { CommandId::LEFT, Savegame::KEY_KEYBOARD_LEFT },
      { CommandId::DOWN, Savegame::KEY_KEYBOARD_DOWN }
    };

    if(std::holds_alternative<CommandId>(command)){
      return savegame_variables.find(std::get<CommandId>(command))->second;
    } else if (std::holds_alternative<CustomId>(command)){
      return "_command_key_" + std::get<CustomId>(command).id;
    }

    return "";
  }

  void Controls::save(Savegame& /*savegame*/) const {
    //TODO !!
  }

  /**
 * \brief Returns the name of the savegame variable that stores the joypad
 * mapping of a game command.
 * \param command A game command.
 * \return The savegame variable that stores the joypad action mapped to this
 * game command, or an empty string if this command is GameCommand::NONE.
 */
  std::string Controls::get_joypad_binding_savegame_variable(
        Command command) const {

    static const std::map<CommandId, std::string> savegame_variables = {
      { CommandId::NONE, "" },
      { CommandId::ACTION, Savegame::KEY_JOYPAD_ACTION },
      { CommandId::ATTACK, Savegame::KEY_JOYPAD_ATTACK },
      { CommandId::ITEM_1, Savegame::KEY_JOYPAD_ITEM_1 },
      { CommandId::ITEM_2, Savegame::KEY_JOYPAD_ITEM_2 },
      { CommandId::PAUSE, Savegame::KEY_JOYPAD_PAUSE },
      { CommandId::RIGHT, Savegame::KEY_JOYPAD_RIGHT },
      { CommandId::UP, Savegame::KEY_JOYPAD_UP },
      { CommandId::LEFT, Savegame::KEY_JOYPAD_LEFT },
      { CommandId::DOWN, Savegame::KEY_JOYPAD_DOWN }
    };

    if(std::holds_alternative<CommandId>(command)){
      return savegame_variables.find(std::get<CommandId>(command))->second;
    } else if (std::holds_alternative<CustomId>(command)){
      return "_command_joy_" + std::get<CustomId>(command).id;
    }

    return "";
  }

  /**
 * \brief Determines from the savegame the low-level keyboard key where the
 * specified game command is mapped.
 * \param command A game command.
 * \return The keyboard key mapped to this game command in the savegame.
 */
  InputEvent::KeyboardKey Controls::get_saved_keyboard_binding(
        Command command, const Savegame &save) const {

    const std::string& savegame_variable = get_keyboard_binding_savegame_variable(command);
    const std::string& keyboard_key_name = save.get_string(savegame_variable);
    return name_to_enum(keyboard_key_name, InputEvent::KeyboardKey::NONE);
  }

  /**
 * \brief Saves the low-level keyboard command where the specified game key is
 * mapped.
 * \param command A game command.
 * \param keyboard_key The keyboard key to map to this game command in the
 * savegame.
 */
  void Controls::set_saved_keyboard_binding(
        Command command, InputEvent::KeyboardKey keyboard_key, Savegame& save) {

    const std::string& savegame_variable = get_keyboard_binding_savegame_variable(command);
    const std::string& keyboard_key_name = enum_to_name(keyboard_key);
    save.set_string(savegame_variable, keyboard_key_name);
  }

  /**
 * \brief Returns the game command (if any) associated to the specified
 * keyboard key.
 * \param key A keyboard key.
 * \return The game command mapped to that key or GameCommand::NONE.
 */
  Command Controls::get_command_from_keyboard(
        InputEvent::KeyboardKey key) const {

    const auto& it = keyboard_mapping.find(key);
    if (it != keyboard_mapping.end()) {
      return it->second;
    }

    return CommandId::NONE;
  }

  /**
 * @brief Get axis binding from keyboard key
 * @param key a keyboard key
 * @return axis binding or invalid axis binding if no mapping found
 */
  Controls::ControlAxisBinding Controls::get_axis_from_keyboard(InputEvent::KeyboardKey key) const {
    const auto& it = keyboard_axis_mapping.find(key);
    if(it != keyboard_axis_mapping.end()){
      return it->second;
    }

    return {};
  }

  /**
 * \brief Determines from the savegame the low-level joypad action where the
 * specified game command is mapped.
 * \param command A game command.
 * \return The joypad action mapped to this game command in the savegame.
 */
  Controls::JoypadBinding Controls::get_saved_joypad_binding(
        Command command, const Savegame& save) const {

    const std::string& savegame_variable = get_joypad_binding_savegame_variable(command);
    return JoypadBinding(save.get_string(savegame_variable));
  }

  /**
 * \brief Saves the low-level joypad action where the specified game command
 * is mapped.
 * \param command A game command.
 * \return The joypad action to map to this game command in the savegame.
 */
  void Controls::set_saved_joypad_binding(Command command, const JoypadBinding &joypad_binding, Savegame &save) {

    const std::string& savegame_variable = get_joypad_binding_savegame_variable(command);
    save.set_string(savegame_variable, joypad_binding.to_string());
  }

  void Controls::set_joypad(const JoypadPtr& joypad) {
    this->joypad = joypad;
  }

  const JoypadPtr& Controls::get_joypad() {
    return joypad;
  }

  void Controls::load_default_joypad_bindings() {
    joypad_mapping[JoypadBinding(JoyPadAxis::LEFT_X, AxisDirection::PLUS)] = CommandId::RIGHT;
    joypad_mapping[JoypadBinding(JoyPadAxis::LEFT_X, AxisDirection::MINUS)] = CommandId::LEFT;
    joypad_mapping[JoypadBinding(JoyPadAxis::LEFT_Y, AxisDirection::PLUS)] = CommandId::DOWN;
    joypad_mapping[JoypadBinding(JoyPadAxis::LEFT_Y, AxisDirection::MINUS)] = CommandId::UP;

    joypad_mapping[JoypadBinding(JoyPadButton::A)] = CommandId::ATTACK;
    joypad_mapping[JoypadBinding(JoyPadButton::X)] = CommandId::ITEM_1;
    joypad_mapping[JoypadBinding(JoyPadButton::B)] = CommandId::ACTION;
    joypad_mapping[JoypadBinding(JoyPadButton::Y)] = CommandId::ITEM_2;
    joypad_mapping[JoypadBinding(JoyPadButton::START)] = CommandId::PAUSE;

    joypad_axis_mapping[JoyPadAxis::LEFT_X] = ControlAxisBinding{AxisId::X, AxisDirection::PLUS};
    joypad_axis_mapping[JoyPadAxis::LEFT_Y] = ControlAxisBinding{AxisId::Y, AxisDirection::PLUS};
  }

  void Controls::load_default_keyboard_bindings() {
    keyboard_mapping[InputEvent::KeyboardKey::UP] = CommandId::UP;
    keyboard_mapping[InputEvent::KeyboardKey::DOWN] = CommandId::DOWN;
    keyboard_mapping[InputEvent::KeyboardKey::LEFT] = CommandId::LEFT;
    keyboard_mapping[InputEvent::KeyboardKey::RIGHT] = CommandId::RIGHT;

    keyboard_mapping[InputEvent::KeyboardKey::c] = CommandId::ATTACK;
    keyboard_mapping[InputEvent::KeyboardKey::x] = CommandId::ITEM_1;
    keyboard_mapping[InputEvent::KeyboardKey::SPACE] = CommandId::ACTION;
    keyboard_mapping[InputEvent::KeyboardKey::v] = CommandId::ITEM_2;
    keyboard_mapping[InputEvent::KeyboardKey::d] = CommandId::PAUSE;

    keyboard_axis_mapping[InputEvent::KeyboardKey::UP] = ControlAxisBinding{AxisId::Y, AxisDirection::MINUS};
    keyboard_axis_mapping[InputEvent::KeyboardKey::DOWN] = ControlAxisBinding{AxisId::Y, AxisDirection::PLUS};
    keyboard_axis_mapping[InputEvent::KeyboardKey::LEFT] = ControlAxisBinding{AxisId::X, AxisDirection::PLUS};
    keyboard_axis_mapping[InputEvent::KeyboardKey::RIGHT] = ControlAxisBinding{AxisId::X, AxisDirection::MINUS};
  }

  /**
 * \brief Returns the game command (if any) associated to the specified
 * joypad action.
 * \param joypad_string A joypad action.
 * \return The game command mapped to that joypad action or GameCommand::NONE.
 */
  Command Controls::get_command_from_joypad(
        const JoypadBinding& joypad_binding) const {

    const auto& it = joypad_mapping.find(joypad_binding);
    if (it != joypad_mapping.end()) {
      return it->second;
    }

    return CommandId::NONE;
  }

  Controls::ControlAxisBinding Controls::get_axis_from_joypad(JoyPadAxis joypad_axis) const {
    const auto& it = joypad_axis_mapping.find(joypad_axis);
    if(it != joypad_axis_mapping.end()) {
      return it->second;
    }

    return {};
  }

  // customization

  /**
 * \brief Sets the specified command to be customized.
 *
 * After this function is called, the next keyboard or joypad event received will
 * not be treated normally; it will be considered as the new keyboard or joypad
 * binding for this game key. Then, keyboard and joypad events will be treated
 * normally again.
 *
 * \param command The command to customize.
 * \param callback_ref Lua ref to a function to call when the customization
 * finishes, or an empty ref.
 */
  void Controls::customize(const Command &command,
                           const ScopedLuaRef& callback_ref
                           ) {
    this->customizing = true;
    this->command_to_customize = command;
    this->customize_callback_ref = callback_ref;
  }

  /**
 * \brief Returns whether the player is currently customizing a command.
 * \return true if the player is currently customizing a command.
 */
  bool Controls::is_customizing() const {
    return customizing;
  }

  /**
 * \brief When the player is customizing a command, returns the command that
 * is being customized.
 * \return The command being customized.
 */
  Command Controls::get_command_to_customize() const {

    Debug::check_assertion(is_customizing(),
                           "The player is not customizing a command");
    return command_to_customize;
  }

  /**
 * \brief Calls the Lua function that was registered to be called after a
 * command customization phase.
 */
  void Controls::do_customization_callback() {

    customize_callback_ref.clear_and_call("capture command callback");
  }

  /**
 * \brief Returns whether a string describes a valid joypad action.
 *
 * The string should have one of the following forms:
 * - "button X" where X is the index of a joypad button.
 * - "axis X +" where X is the index of a joypad axis.
 * - "axis X -" where X is the index of a joypad axis.
 * - "hat X Y" where X is the index of a joypad hat and Y is a direction (0 to 7).
 *
 * \param joypad_string The string to check.
 * \return true if this string is a valid joypad action.
 */
  bool Controls::is_joypad_string_valid(const std::string& /* joypad_string */) {

    // TODO
    return true;
  }

  /**
 * \brief Returns the name of a game command.
 * \param command a game command.
 * \return The name of this command, or an empty string if the command is GameCommand::NONE.
 */
  std::string Controls::get_command_name(const Command& command) {
    return std::visit(overloaded{
                        [](const CommandId& cid) {
                          return enum_to_name(cid);
                        },
                        [](const CustomId& cid){
                          return cid.id;
                        }
                      }, command);
  }

  /**
 * \brief Returns the name of a game command.
 * \param command a game command.
 * \return The name of this command, or an empty string if the command is GameCommand::NONE.
 */
  std::string Controls::get_axis_name(const Axis& command) {
    return std::visit(overloaded{
                        [](const AxisId& cid) {
                          return enum_to_name(cid);
                        },
                        [](const CustomId& cid){
                          return cid.id;
                        }
                      }, command);
  }

  /**
 * \brief Returns a game command given its Lua name.
 * \param command_name Lua name of a game command.
 * \return The corresponding game command, or GameCommand::NONE if the name is invalid.
 */
  Command Controls::get_command_by_name(
        const std::string& command_name) {

    CommandId id = name_to_enum<CommandId>(command_name, CommandId::NONE);
    return id != CommandId::NONE ? Command(id) : CustomId{command_name};
  }

  /**
 * \brief Returns a game command axis given its Lua name.
 * \param command_name Lua name of a game axis command.
 * \return The corresponding game command, or GameCommand::NONE if the name is invalid.
 */
  Axis Controls::get_axis_by_name(
        const std::string& command_name) {

    AxisId id = name_to_enum<AxisId>(command_name, AxisId::NONE);
    return id != AxisId::NONE ? Axis(id) : CustomId{command_name};
  }

  void Controls::set_analog_commands_enabled(bool enabled) {
    analog_commands_enabled = enabled;
  }

  bool Controls::are_analog_commands_enabled() {
    return analog_commands_enabled;
  }

  /**
 * @brief Gets the effects of these commands, const version
 * @return
 */
  const CommandsEffects& Controls::get_effects() const {
    return effects;
  }

  /**
 * @brief Gets the effects of these commands, non-const version
 * @return
 */
  CommandsEffects& Controls::get_effects() {
    return effects;
  }

  /**
 * \brief Returns the name identifying this type in Lua.
 * \return The name identifying this type in Lua.
 */
  const std::string& Controls::get_lua_type_name() const {
    return LuaContext::controls_module_name;
  }

  /**
 * @brief Parses a joypad binding from a string
 *
 * If the string is not a valid joypad binding, an invalid binding is constructed
 * and can be tested with `is_invalid`.
 * @param str a binding string
 */
  Controls::JoypadBinding::JoypadBinding(const std::string& str) {
    //Unserialize the binding
    size_t spos = str.find(' ');
    if(spos != std::string::npos) {
      //There is a space ! Its an axis binding
      auto axis = name_to_enum<JoyPadAxis>(str.substr(0, spos), JoyPadAxis::INVALID);
      auto sdir = str[spos+1];
      auto dir = sdir == '+' ? AxisDirection::PLUS : AxisDirection::MINUS;
      *this = JoypadAxisBinding{axis, dir};
    } else {
      //Probably a button
      *this = name_to_enum<JoyPadButton>(str, JoyPadButton::INVALID);
    }
  }

  /**
 * @brief Constructs a binding from an axis and axis state
 * @param axis axis
 * @param value state
 */
  Controls::JoypadBinding::JoypadBinding(JoyPadAxis axis, AxisDirection dir) {
    *this = JoypadAxisBinding{axis, dir};
  }

  /**
 * @brief Construct a joypad button binding
 * @param button joypad button
 */
  Controls::JoypadBinding::JoypadBinding(JoyPadButton button) {
    *this = button;
  }

  /**
 * @brief Serializes this binding into a string representation
 * @return serialized string
 */
  std::string Controls::JoypadBinding::to_string() const {
    return std::visit(overloaded{
                        [](const JoyPadButton& bt){
                          return enum_to_name(bt);
                        },
                        [](const JoypadAxisBinding& ab){
                          auto dir = ab.direction == AxisDirection::PLUS ? " +" : " -";
                          return enum_to_name(ab.axis) + dir;
                        }
                      },static_cast<const _JoypadBinding&>(*this));
  }

  bool Controls::JoypadBinding::is_invalid() const {
    return std::visit(overloaded{
                        [](const JoyPadButton& button){
                          return button == JoyPadButton::INVALID;
                        },
                        [](const JoypadAxisBinding& ab) {
                          return ab.axis == JoyPadAxis::INVALID;
                        }
                      },
                      static_cast<const _JoypadBinding&>(*this));
  }

}
