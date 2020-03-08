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
#ifndef SOLARUS_GAME_COMMANDS_H
#define SOLARUS_GAME_COMMANDS_H

#include "solarus/core/Common.h"
#include "solarus/core/Command.h"
#include "solarus/core/InputEvent.h"
#include "solarus/lua/ScopedLuaRef.h"
#include "solarus/core/CommandsEffects.h"
#include <unordered_map>
#include <set>
#include <string>
#include <variant>
#include <optional>

namespace Solarus {

class Savegame;
class MainLoop;
class Game;

/**
 * \brief Stores the mapping between the in-game high-level commands
 * and their keyboard and joypad bindings.
 *
 * This class receives the low-level keyboard and joypad events that occur
 * during the game and then notifies the appropriate objects that a built-in
 * game command was pressed or released.
 * What we call a game command is a high-level notion such as "action" or
 * "attack".
 * The corresponding low-level input event can be a keyboard event or a
 * joypad event.
 */
class Commands : public ExportableToLua {

  public:

  enum class AxisDirection{
      PLUS,
      MINUS
  };

    /**
     * @brief Represent an axis binding used as a key in joypad bindings
     */
    struct JoypadAxisBinding {


        JoyPadAxis axis;
        AxisDirection  direction;

        /**
         * @brief comparison operator being forwarded by std::variant
         * @param ib
         * @return
         */
        bool operator<(const JoypadAxisBinding& ib) const {
            return std::tie(axis, direction) < std::tie(ib.axis, ib.direction);
        }
    };

    /**
     * @brief A command axis binding
     */
    struct CommandAxisBinding{
      CommandAxis axis = CommandAxisId::NONE;
      AxisDirection direction = AxisDirection::PLUS;
    };

  private:
    /**
     * Private joypad binding variant
     */
    using _JoypadBinding = std::variant<JoyPadButton, JoypadAxisBinding>;
  public:

    /**
     * @brief JoypadBinding variant with added methods and constructors
     */
    struct JoypadBinding : public _JoypadBinding {
        explicit JoypadBinding(const std::string& str);
        JoypadBinding(JoyPadAxis axis, AxisDirection dir);
        JoypadBinding(JoyPadButton button);
        std::string to_string() const;

        using _JoypadBinding::operator=;

        bool is_invalid() const;

        /**
         * @brief mandatory comparison operator to allow type to be a key in std::map
         * @param other
         * @return
         */
        inline bool operator<(const JoypadBinding& other) const {
            return static_cast<const _JoypadBinding&>(*this) < static_cast<const _JoypadBinding&>(other);
        }
    };

    explicit Commands(MainLoop& main_loop);
    explicit Commands(MainLoop& main_loop, Game& game);

    InputEvent::KeyboardKey get_keyboard_binding(const Command& command) const;
    void set_keyboard_binding(const Command& command, InputEvent::KeyboardKey keyboard_key);
    std::optional<JoypadBinding> get_joypad_binding(const Command& command) const;
    void set_joypad_binding(const Command& command, const JoypadBinding& joypad_binding);

    std::pair<InputEvent::KeyboardKey, InputEvent::KeyboardKey> get_keyboard_axis_binding(const CommandAxis& command_axis) const;
    void set_keyboard_axis_binding(const CommandAxis& command_axis, InputEvent::KeyboardKey minus, InputEvent::KeyboardKey plus);
    JoyPadAxis get_joypad_axis_binding(const CommandAxis& command_axis) const;
    void set_joypad_axis_binding(const CommandAxis& command_axis, JoyPadAxis axis);

    void set_joypad(const JoypadPtr& joypad);
    const JoypadPtr& get_joypad();

    void load_default_joypad_bindings();
    void load_default_keyboard_bindings();

    void notify_input(const InputEvent& event);
    bool is_command_pressed(const Command& command) const;
    double get_axis_state(const CommandAxis& axis) const;
    int get_wanted_direction8() const;
    std::pair<double, double> get_wanted_polar() const;

    void customize(const Command& command, const ScopedLuaRef& callback_ref);
    bool is_customizing() const;
    Command get_command_to_customize() const;

    static bool is_joypad_string_valid(const std::string& joypad_string);
    static std::string get_command_name(const Command &command);
    static std::string get_axis_name(const CommandAxis &command);
    static Command get_command_by_name(const std::string& command_name);
    static CommandAxis get_axis_by_name(const std::string& axis_name);

    // High-level resulting commands.
    void command_pressed(const Command& command);
    void command_released(const Command& command);
    void command_axis_moved(const CommandAxis& axis, double state);

    const CommandsEffects& get_effects() const;
    CommandsEffects& get_effects();

    const std::string& get_lua_type_name() const override;

    ~Commands();
  private:


    // Keyboard mapping.
    void keyboard_key_pressed(InputEvent::KeyboardKey keyboard_key_pressed);
    void keyboard_key_released(InputEvent::KeyboardKey keyboard_key_released);
    std::string get_keyboard_binding_savegame_variable(const Command &command) const;
    InputEvent::KeyboardKey get_saved_keyboard_binding(Command command, const Savegame &save) const;
    void set_saved_keyboard_binding(Command command, InputEvent::KeyboardKey key, Savegame& save);
    Command get_command_from_keyboard(InputEvent::KeyboardKey key) const;
    CommandAxisBinding get_axis_from_keyboard(InputEvent::KeyboardKey key) const;

    // Joypad mapping.
    void joypad_button_pressed(JoyPadButton button);
    void joypad_button_released(JoyPadButton button);
    void joypad_axis_moved(JoyPadAxis axis, double direction);
    void joypad_hat_moved(int hat, int direction);
    std::string get_joypad_binding_savegame_variable(Command command) const;
    JoypadBinding get_saved_joypad_binding(Command command, const Savegame& save) const;
    void set_saved_joypad_binding(Command command, const JoypadBinding &joypad_binding, Savegame& save);
    Command get_command_from_joypad(const JoypadBinding &joypad_binding) const;
    CommandAxisBinding get_axis_from_joypad(JoyPadAxis joypad_axis) const;

    void save(Savegame& savegame) const;

    void do_customization_callback();

    MainLoop& main_loop;                          /**< The game we are controlling. */
    std::map<InputEvent::KeyboardKey, Command>
        keyboard_mapping;                /**< Associates each game command to the
                                          * keyboard key that triggers it. */
    std::map<JoypadBinding, Command>
        joypad_mapping;                  /**< Associates each game command to the
                                          * joypad action that triggers it. */
    std::map<JoyPadAxis, CommandAxisBinding>
        joypad_axis_mapping;             /**< Associates command axises to the joypad axis
                                          * that move it. */
    std::map<InputEvent::KeyboardKey, CommandAxisBinding>
        keyboard_axis_mapping;           /**< Associates command axises to the keyboad keys
                                          * that move it. */
    std::set<Command>
        commands_pressed;                /**< Memorizes the state of each game command. */

    std::map<CommandAxis, double>
        command_axes_state;              /**< Memorizes the state of each command axes */

    bool customizing;                    /**< Indicates that the next keyboard or
                                          * joypad event will be considered as the
                                          * new binding for a game command. */
    Command command_to_customize;       /**< The game command being customized
                                          * when customizing is true. */

    CommandsEffects effects;             /**< Effect of each of those commands */
    ScopedLuaRef customize_callback_ref; /**< Lua ref to a function to call
                                          * when the customization finishes. */

    static const uint16_t
        direction_masks[4];              /**< Bit mask associated to each direction:
                                          * this allows to store any combination of
                                          * the four directions into an integer. */
    static const int
        masks_to_directions8[16];        /**< Associates to each combination of
                                          * directional command a direction between 0
                                          * and 7 (-1 means none). */
    static const std::string
        direction_names[4];              /**< English name of each arrow direction,
                                          * used to save a joypad action as a string. */

    JoypadPtr joypad;                    /** optional joypad those commands are listening to */
};

}
#endif

