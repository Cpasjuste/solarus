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
#ifndef SOLARUS_GAME_H
#define SOLARUS_GAME_H

#include "solarus/core/Common.h"
#include "solarus/core/CommandsEffects.h"
#include "solarus/core/DialogBoxSystem.h"
#include "solarus/core/Controls.h"
#include "solarus/core/Point.h"
#include "solarus/entities/CameraPtr.h"
#include "solarus/entities/HeroPtr.h"
#include "solarus/core/MapPtr.h"
#include "solarus/core/Map.h"
#include "solarus/entities/NonAnimatedRegions.h"
#include "solarus/graphics/SurfacePtr.h"
#include "solarus/graphics/Transition.h"
#include "solarus/core/SavegamePtr.h"
#include <memory>
#include <functional>
#include <string>

namespace Solarus {

class CommandsEffects;
class Equipment;
class Controls;
class InputEvent;
class LuaContext;
class MainLoop;
class ResourceProvider;

/**
 * \brief Represents the game currently running.
 *
 * The game shows the current map and handles all game elements.
 */
class SOLARUS_API Game {

    using MapChangeCallback = std::function<void(const MapPtr& previous, const MapPtr& next)>;
  public:

    // creation
    Game(MainLoop& main_loop, const SavegamePtr &savegame);

    // no copy operations
    Game(const Game& game) = delete;
    Game& operator=(const Game& game) = delete;

    void start();
    void stop();
    void restart();

    // global objects
    MainLoop& get_main_loop();
    LuaContext& get_lua_context();
    ResourceProvider& get_resource_provider();
    const HeroPtr& get_hero() const;
    Controls& get_controls();
    const Controls& get_controls() const;
    CommandsEffects& get_commands_effects();
    Savegame& get_savegame();
    const Savegame& get_savegame() const;
    Equipment& get_equipment();
    const Equipment& get_equipment() const;

    // functions called by the main loop
    bool notify_input(const InputEvent& event);
    void update();

    void draw(const SurfacePtr& dst_surface);
    void notify_window_size_changed(const Size& size);

    // game controls
    void notify_control(const ControlEvent& event);
    //void notify_command_released(Command command);

    // simulate commands
    void simulate_command_pressed(Command command);
    void simulate_command_released(Command command);

    // map
    Map& get_default_map();
    bool has_current_map() const;
    Map& get_current_map(const HeroPtr& hero);
    void teleport_hero(const HeroPtr& hero, const std::string& map_id, const std::string& destination_name,
        Transition::Style transition_style);

    void teleport_camera(const CameraPtr& camera,
                         const std::string map_id,
                         const std::string& destination_name,
                         Transition::Style transition_style,
                         const HeroPtr& opt_hero);

    CameraPtr create_camera(const std::string& id);
    void remove_camera(const CameraPtr& camera, Transition::Style transition_style);
    const std::vector<CameraPtr> &get_cameras() const;

    bool is_current_map(Map& map) const;
    bool has_multiple_maps() const;
    bool is_map_loaded(const std::string& map_id) const;

    const MapPtr& prepare_map(const std::string& map_id);
    void leave_map(const EntityPtr &leaving, const MapPtr& map);
    const std::vector<MapPtr>& get_maps() const;

    Transition::Style get_default_transition_style() const;
    void set_default_transition_style(Transition::Style default_transition_style);

    // world
    bool get_crystal_state() const;
    void change_crystal_state();

    // current game state
    bool is_paused() const;
    bool is_dialog_enabled() const;
    bool is_playing_transition() const;
    bool is_showing_game_over() const;
    bool is_suspended_by_camera() const;
    bool is_suspended_by_script() const;
    bool is_suspended() const; // true if at least one of the 5 functions above returns true

    // pause
    bool can_pause() const;
    bool can_unpause() const;
    bool is_pause_allowed() const;
    void set_pause_allowed(bool pause_allowed);
    void set_paused(bool paused);

    // dialogs
    void start_dialog(
        const std::string& dialog_id,
        const ScopedLuaRef& info_ref,
        const ScopedLuaRef& callback_ref
    );
    void stop_dialog(const ScopedLuaRef& status_ref);

    // game over
    void start_game_over();
    void stop_game_over();

    // Suspend manually.
    void set_suspended_by_script(bool suspended);
private:
    struct CameraTeleportation{
      MapPtr current_map;                   /**< Map from which this teleportation is coming **/
      MapPtr next_map;                      /**< Map to which this teleportation is going **/
      std::string destination_name;         /**< Destination to which this teleportation is going **/
      CameraPtr camera;                     /**< Camera undergoing the teleportation */
      Transition::Style transition_style;   /**< Style of the teleportation transition */
      HeroPtr opt_hero;                     /**< Optional hero concerned by this teleportation */

      bool removed = false;                 /**< is this teleportation canceled ? */
      bool is_finished() const;
    };

    template<typename F>
    void for_each_hero(F&& action) {
      for(const MapPtr& map : current_maps) {
        for(const HeroPtr& hero : map->get_entities().get_heroes()) {
          action(*hero);
        }
      }
    }

    void on_hero_map_change(const HeroPtr& hero, const CameraTeleportation &tp);

    void on_hero_map_prepare(const HeroPtr& hero, const CameraTeleportation &tp);

    // main objects
    MainLoop& main_loop;       /**< the main loop object */
    std::shared_ptr<Savegame>
        savegame;              /**< the game data saved */
    HeroPtr default_hero;
    std::vector<CameraPtr> cameras;

    // current game state (elements currently shown)
    bool pause_allowed;        /**< indicates that the player is allowed to use the pause command */
    bool paused;               /**< indicates that the game is paused */
    DialogBoxSystem
        dialog_box;            /**< The dialog box manager. */
    bool showing_game_over;    /**< Whether a game-over sequence is currently active. */
    bool suspended_by_script;  /**< Whether the game is manually suspended. */
    bool started;              /**< true if this game is running, false if it is not yet started or being closed. */
    bool restarting;           /**< true if the game will be restarted */

    // controls
    ControlsPtr
        controls;              /**< this object receives the keyboard and joypad events */


    // maps
    std::vector<MapPtr>
        current_maps;           /**< the maps currently simulated */

    std::set<MapPtr>
        maps_to_unload;        /**< Maps to unload at the next tick */

    std::list<CameraTeleportation>
        cameras_teleportations;    /**< record current displacements of heroes between maps */

    // world (i.e. the current set of maps)
    bool crystal_state;        /**< indicates that a crystal has been enabled (i.e. the orange blocks are raised) */

    // update functions
    void update_tilesets();
    bool update_teleportation(CameraTeleportation &tp);
    void teleportation_change_map(CameraTeleportation &tp);
    void update_gameover_sequence();
    void notify_map_changed(Map& map, Camera &camera);

};

}

#endif

