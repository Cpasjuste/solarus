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
#include "solarus/audio/Music.h"
#include "solarus/core/CommandsEffects.h"
#include "solarus/core/CurrentQuest.h"
#include "solarus/core/Debug.h"
#include "solarus/core/Equipment.h"
#include "solarus/core/Map.h"
#include "solarus/core/Game.h"
#include "solarus/core/MainLoop.h"
#include "solarus/core/Savegame.h"
#include "solarus/core/Treasure.h"
#include "solarus/entities/Destination.h"
#include "solarus/entities/Entities.h"
#include "solarus/entities/EntityState.h"
#include "solarus/entities/Hero.h"
#include "solarus/entities/NonAnimatedRegions.h"
#include "solarus/entities/StartingLocationMode.h"
#include "solarus/entities/TilePattern.h"
#include "solarus/entities/Tileset.h"
#include "solarus/graphics/Color.h"
#include "solarus/graphics/Surface.h"
#include "solarus/graphics/TransitionFade.h"
#include "solarus/graphics/Video.h"
#include "solarus/lua/LuaContext.h"
#include <map>

namespace Solarus {

/**
 * \brief Creates a game.
 * \param main_loop The Solarus root object.
 * \param savegame The saved data of this game.
 */
Game::Game(MainLoop& main_loop, const SavegamePtr& savegame):
  main_loop(main_loop),
  savegame(savegame),
  pause_allowed(true),
  paused(false),
  dialog_box(*this),
  showing_game_over(false),
  suspended_by_script(false),
  started(false),
  restarting(false),
  crystal_state(false) {

  // notify objects
  savegame->set_game(this);

  // initialize members
  commands = CommandsDispatcher::get().create_commands_from_game(*this); //TODO differentiate commands from hero to hero

  default_hero = std::make_shared<Hero>(savegame->get_default_equipment(), "hero");
  CameraPtr default_camera = create_camera("main_camera");

  default_hero->set_commands(commands);
  default_hero->start_free();
  default_hero->set_linked_camera(default_camera);

  // Maybe we are restarting after a game-over sequence.
  if (get_equipment().get_life() <= 0) {
    get_equipment().restore_all_life();
  }

  // Launch the starting map.
  std::string starting_map_id = savegame->get_string(Savegame::KEY_STARTING_MAP);
  std::string starting_destination_name = savegame->get_string(Savegame::KEY_STARTING_POINT);

  bool valid_map_saved = false;
  if (!starting_map_id.empty()) {

    if (CurrentQuest::resource_exists(ResourceType::MAP, starting_map_id)) {
      // We are okay: the savegame file refers to an existing map.
      valid_map_saved = true;
    }
    else {
      // The savegame refers to a map that no longer exists.
      // Maybe the quest is in an intermediate development phase.
      // Show an error and fallback to the default map.
      Debug::error(std::string("The savegame refers to a non-existing map: '") + starting_map_id + "'");
    }
  }

  if (!valid_map_saved) {
    // When no valid starting map is set, use the first one declared in the
    // resource list file.
    const std::map<std::string, std::string>& maps =
        CurrentQuest::get_resources(ResourceType::MAP);
    if (maps.empty()) {
      Debug::die("This quest has no map");
    }
    starting_map_id = maps.begin()->first;
    starting_destination_name = "";  // Default destination.
  }


  teleport_hero(get_hero(), starting_map_id, starting_destination_name, Transition::Style::FADE);
}

/**
 * \brief Starts this game.
 *
 * Does nothing if the game is already started.
 */
void Game::start() {

  if (started) {
    return;
  }

  started = true;
  get_hero()->get_equipment().notify_game_started();
  get_lua_context().game_on_started(*this);
}

/**
 * \brief Ends this game.
 *
 * Does nothing if the game is not started.
 */
void Game::stop() {

  if (!started) {
    return;
  }

  for_each_hero([](Hero& hero){
    if(hero.is_on_map()) {
      hero.notify_being_removed();
    }
    hero.get_equipment().notify_game_finished();
  });

  // Unload each map
  for(const MapPtr& current_map : current_maps) {

    if (current_map->is_started()) {
      current_map->leave();
    }

    current_map->unload();
  }

  get_lua_context().game_on_finished(*this);
  get_savegame().set_game(nullptr);

  Music::stop_playing();

  started = false;
}

/**
 * \brief Returns the Solarus main loop.
 * \return The main loop object.
 */
MainLoop& Game::get_main_loop() {
  return main_loop;
}

/**
 * \brief Returns the Lua context of this game.
 * \return The Lua context.
 */
LuaContext& Game::get_lua_context() {
  return main_loop.get_lua_context();
}

/**
 * \brief Returns the resource provider of the quest.
 * \return The resource provider.
 */
ResourceProvider& Game::get_resource_provider() {
  return main_loop.get_resource_provider();
}

/**
 * \brief Returns the hero.
 * \return the hero
 */
const HeroPtr& Game::get_hero() const {
  return default_hero;
}

/**
 * \brief Returns the game commands mapped to the keyboard and the joypad.
 * \return The game commands.
 */
Commands& Game::get_commands() {
  return *commands;
}

/**
 * \brief Returns the game commands mapped to the keyboard and the joypad.
 * \return The game commands.
 */
const Commands& Game::get_commands() const {
  return *commands;
}

/**
 * \brief Returns the current effect of the main keys (action, sword, pause, etc.).
 * \return the current effect of the main keys
 */
CommandsEffects& Game::get_commands_effects() {
  return get_commands().get_effects();
}

/**
 * \brief Returns the saved data associated to this game.
 * \return The saved data.
 */
Savegame& Game::get_savegame() {
  return *savegame;
}

/**
 * \brief Returns the saved data associated to this game.
 * \return The saved data.
 */
const Savegame& Game::get_savegame() const {
  return *savegame;
}

/**
 * \brief Returns the equipment of the player.
 *
 * It is equivalent to get_savegame().get_equipment().
 *
 * \return The equipment.
 */
Equipment& Game::get_equipment() {
  return get_hero()->get_equipment();
}

/**
 * \brief Returns the equipment of the player.
 *
 * It is equivalent to get_savegame().get_equipment().
 *
 * \return The equipment.
 */
const Equipment& Game::get_equipment() const {
  return get_hero()->get_equipment();
}

/**
 * \brief This function is called when a low-level input event occurs during the game.
 * \param event The event to handle.
 * \return \c true if the event was handled and should stop being propagated.
 */
bool Game::notify_input(const InputEvent& event) {

  if(current_maps.empty()) {
    // no maps, game is not started yet

    return true; // pretend we handled the event
  }

  // propagate input to each map

  //TODO check if at least one map must be loaded (is_loaded)
  bool handled = get_lua_context().game_on_input(*this, event);

  if(!handled) {
    for(const MapPtr& current_map : current_maps) {
      if (current_map != nullptr && current_map->is_loaded()) {
        handled |= current_map->notify_input(event);
      }
    }
  }

  return handled;
}

/**
 * \brief This function is called when a game commend event is raised.
 * \param command A game command.
 */
void Game::notify_command(const CommandEvent& command) {

  bool is_pressed = command.type == CommandEvent::Type::PRESSED;
  // Is a built-in dialog box being shown?
  if (is_dialog_enabled() and is_pressed) {
    if (dialog_box.notify_command_pressed(command.name)) {
      return;
    }
  }

  // See if the game script handles the command.
  if (get_lua_context().game_on_command(*this, command)) {
    return;
  }

  // See if the map scripts handled the command.
  for(const MapPtr& map : current_maps) {
    if(map->notify_command(command)) {
      return;
    }
  }

  // Lua scripts did not override the command: do the built-in behavior.
  if (command.name == Command::PAUSE and command.is_pressed()) {
    if (is_paused()) {
      if (can_unpause()) {
        set_paused(false);
      }
    }
    else {
      if (can_pause()) {
        set_paused(true);
      }
    }
  }
}

/**
 * \brief Updates the game elements.
 *
 * Updates the map, the equipment, the HUD, etc.
 */
void Game::update() {
  // Update the transitions between maps.
  // Use the side effect of remove_if to elegantly remove the finished teleportations
  for(auto& ct : cameras_teleportations) {
    ct.removed = update_teleportation(ct);
  }

  cameras_teleportations.erase(
        std::remove_if(cameras_teleportations.begin(), cameras_teleportations.end(),
                 [](CameraTeleportation& ht){
          return ht.removed;
        }),
        cameras_teleportations.end()
  );

  if (restarting && cameras_teleportations.empty()) { //All transitions finished ! Restart !
    stop();
    MainLoop& main_loop = get_main_loop();
    main_loop.set_game(new Game(main_loop, savegame));
    this->savegame = nullptr;
    return;
  }

  //If we are not doing any teleportation
  if(cameras_teleportations.empty()) {
    //Remove map that must be unloaded
    for(const MapPtr& map : maps_to_unload) {
      map->leave(); // Leave the map
      map->unload();

      current_maps.erase(std::remove(current_maps.begin(),
                                     current_maps.end(),
                                     map),
                         current_maps.end());
    }
    maps_to_unload.clear(); //Done removing maps
  }

  if(restarting or not started) {
    return;
  }

  // Update the map.
  update_tilesets();

  for(const MapPtr& current_map : current_maps) {
    if(current_map->is_started()) {
      current_map->update();
    }
  }

  // Call game:on_update() in Lua.
  get_lua_context().game_on_update(*this);

  // Update the equipment and HUD.
  get_equipment().update(); //TODO move equipement update were equipement is located
  //update_commands_effects();
}

/**
 * \brief Updates all tilesets currently loaded.
 */
void Game::update_tilesets() {

  // Need to update at least all tilesets used by the current map.
  const std::map<std::string, std::shared_ptr<Tileset>>& tilesets = get_resource_provider().get_loaded_tilesets();
  for (const auto& kvp : tilesets) {
    kvp.second->update();
  }
}

/**
 * \brief Handles the transitions.
 *
 * This functions changes the map when needed and plays the
 * transitions between the two maps. This function is called
 * by the update() function.
 * Note that the two maps can actually be the same.
 * \returns true if the transition has succeeded and needs to be removed
 */
bool Game::update_teleportation(CameraTeleportation& tp) {
  if(tp.removed) {
    return true; //Set this tp to be really removed
  }

  MapPtr& next_map = tp.next_map;
  const auto& camera = tp.camera;
  auto& transition = camera->get_transition();

  if (transition != nullptr) {
    transition->update();
  } else {
    return true; //Transition has died
  }

  // if a transition was playing and has just been finished
  if (transition != nullptr && transition->is_finished()) {

    Transition::Direction transition_direction = transition->get_direction();
    transition = nullptr;

    if (restarting) {
      return true; //We are done closing let's restart or kill the camera!
    }
    else if(next_map == nullptr) {
        //Camera leave the map before dying...
        leave_map(camera, tp.current_map);

        //Transition to nowhere, camera is to be removed
        cameras.erase(std::remove(
                          cameras.begin(),
                          cameras.end(),
                          camera), cameras.end());
        return true;
    }
    else if (transition_direction == Transition::Direction::CLOSING) {
      // The closing transition has just finished.
      teleportation_change_map(tp);
    }
    else { //The opening transition has finished
      next_map->notify_opening_transition_finished(tp.destination_name, tp.opt_hero);
      return true; // This teleportation has come to an end
    }
  }

  return false; //This transition is not yet finished
}

/**
 * @brief Called when in middle of the teleportation
 * @param tp the teleportation struct
 */
void Game::teleportation_change_map(CameraTeleportation &tp) {
  MapPtr& current_map = tp.current_map;
  MapPtr& next_map = tp.next_map;

  auto& transition_style = tp.transition_style;
  const auto& camera = tp.camera;
  auto& transition = camera->get_transition();
  //Create opening transition
  transition = std::unique_ptr<Transition>(Transition::create(
      transition_style,
      Transition::Direction::OPENING
  ));
  bool needs_previous_surface = transition->needs_previous_surface();

  // before closing the map, draw it on a backup surface for transition effects
  // that want to display both maps at the same time
  if (needs_previous_surface) {
    const SurfacePtr& previous_map_surface = Surface::create(
        camera->get_size()
    );
    current_map->draw();
    camera->get_surface()->draw(previous_map_surface);
    transition->set_previous_surface(previous_map_surface);
  }

  transition->set_destination_side(next_map->get_destination_side(tp.destination_name));
  transition->start(); //Start opening transition

  if (next_map != current_map) {
    if(current_map) {
      leave_map(camera, current_map);
    }

    //Go to the new map
    camera->place_on_map(*next_map);
  }

  if(tp.opt_hero) {
      on_hero_map_prepare(tp.opt_hero, tp);
  } else {
      EntityPtr destination = next_map->get_entities().find_entity(tp.destination_name);

      if(destination) {
        camera->track_position(destination->get_center_point());
      }
  }

  notify_map_changed(*next_map, *camera);
  //All entities should be there, start the map if necessary
  if(!next_map->is_started()) {
    Debug::check_assertion(next_map->is_loaded(), "This map is not loaded");
    next_map->start(tp.destination_name);
  }

  if(tp.opt_hero) {
      on_hero_map_change(tp.opt_hero, tp);
  } //Call the on_map_changed after map start
}

/**
 * \brief Draws the game.
 * \param dst_surface The surface where the game will be drawn.
 */
void Game::draw(const SurfacePtr& dst_surface) {
  /** Draw maps to their camera */
  for(const MapPtr& current_map : current_maps) {
    current_map->draw();
  }

  //Draw cameras to screen
  for(const CameraPtr& camera : cameras) {
    camera->draw(dst_surface);
  }

  if(current_maps.size() > 0) {
    // Draw the built-in dialog box if any.
    if (is_dialog_enabled()) {
      dialog_box.draw(dst_surface);
    }

    //TODO check if this is at the right place
    get_lua_context().game_on_draw(*this, dst_surface);
  }
}


/**
 * @brief Called by the main loop when the window size changed
 *
 * Propagate window size change events
 *
 * @param size the new window size
 */
void Game::notify_window_size_changed(const Size& size) {
  for(const MapPtr& current_map : current_maps) {
     current_map->notify_window_size_changed(size);
  }
}

/**
 * @brief get the first map in this game
 *
 * Serve for retro compatibility
 *
 * @return
 */
Map& Game::get_default_map() {
  return *current_maps.front();
}

/**
 * \brief Returns whether there is a current map in this game.
 *
 * This function always returns true except when the game is being created
 * and no map is loaded yet.
 *
 * \return true if there is a map
 */
bool Game::has_current_map() const {
  return current_maps.size() > 0;
}

/**
 * \brief Returns the current map.
 * \return the current map
 */
Map& Game::get_current_map(const HeroPtr &hero) {
  return hero->get_map();
}

void Game::on_hero_map_change(const HeroPtr& hero, const CameraTeleportation& tp) {
    const auto& current_map = tp.current_map;
    const auto& next_map = tp.next_map;
    const auto& a_destination_name = tp.destination_name;

    std::string previous_world;
    if (current_map != nullptr) {
      previous_world = current_map->get_world();
    }

    bool world_changed = current_map && next_map != current_map &&
        (!next_map->has_world() || next_map->get_world() != previous_world);

    if (world_changed) {
      // Reset the crystal blocks.
      crystal_state = false;
    }

    // Determine the destination to use and its name.
    std::string destination_name = a_destination_name;
    bool special_destination = destination_name == "_same" ||
        destination_name.substr(0,5) == "_side";
    StartingLocationMode starting_location_mode = StartingLocationMode::NO;
    if (!special_destination) {
      EntityPtr destination;
      if (destination_name.empty()) {
        std::shared_ptr<Destination> default_destination = next_map->get_entities().get_default_destination();
        if (default_destination != nullptr) {
          destination = default_destination;
          destination_name = destination->get_name();
        }
      }
      else {
        destination = next_map->get_entities().find_entity(destination_name);
      }
      if (destination != nullptr && destination->get_type() == EntityType::DESTINATION) {
        starting_location_mode =
            std::static_pointer_cast<Destination>(destination)->get_starting_location_mode();
        get_lua_context().destination_on_activated(static_cast<Destination&>(*destination), *hero);
      }
    }

    bool save_starting_location = false;
    switch (starting_location_mode) {

    case StartingLocationMode::YES:
      save_starting_location = true;
      break;

    case StartingLocationMode::NO:
      save_starting_location = false;
      break;

    case StartingLocationMode::WHEN_WORLD_CHANGES:
      save_starting_location = world_changed;
      break;
    }

    // Save the location if needed, except if this is a special destination.
    if (save_starting_location && !special_destination) {
      get_savegame().set_string(Savegame::KEY_STARTING_MAP, next_map->get_id());
      get_savegame().set_string(Savegame::KEY_STARTING_POINT, destination_name);
    }

    std::string new_world = next_map->get_world();
    if (previous_world.empty() || new_world.empty() || new_world != previous_world) {
     get_lua_context().game_on_world_changed(*this, previous_world, new_world);
    }
}

void Game::on_hero_map_prepare(const HeroPtr& hero, const CameraTeleportation &tp) {
    const auto& current_map = tp.current_map;
    const auto& next_map = tp.next_map;
    const auto& a_destination_name = tp.destination_name;

    Rectangle previous_map_location;
    if (current_map != nullptr) {
      previous_map_location = current_map->get_location();
    }

    if(current_map and current_map != next_map) {
      leave_map(hero, current_map);
    }

    hero->place_on_destination(*next_map, previous_map_location, a_destination_name);
    hero->get_linked_camera()->start_tracking(hero); //TODO verify if necessary
}

/**
 * \brief Changes the current map.
 *
 * Call this function when you want the hero to go to another map.
 *
 * \param map_id Id of the map to launch. It must exist.
 * \param destination_name Name of the destination point you want to use.
 * An empty string means the default one.
 * You can also use "_same" to keep the hero's coordinates, or
 * "_side0", "_side1", "_side2" or "_side3"
 * to place the hero on a side of the map.
 * \param transition_style Type of transition between the two maps.
 */
void Game::teleport_hero(
    const HeroPtr& hero,
    const std::string& map_id,
    const std::string& a_destination_name,
    Transition::Style transition_style) {

  if(hero->get_linked_camera()) {
    teleport_camera(hero->get_linked_camera(),
                    map_id,
                    a_destination_name,
                    transition_style,
                    hero);
  } else {
    if(is_map_loaded(map_id)) {
        MapPtr current_map = hero->get_map().shared_from_this_cast<Map>();

        MapPtr next_map = prepare_map(map_id);
        hero->place_on_destination(*next_map, current_map->get_location(), a_destination_name);
    }
  }
}

/**
 * @brief Tells if the map with this id is loaded
 * @param map_id
 * @return
 */
bool Game::is_map_loaded(const std::string& map_id) const {
    auto it = std::find_if(current_maps.begin(), current_maps.end(), [&](const MapPtr& map) {
        return map->get_id() == map_id && map->is_loaded();
    });
    return it != current_maps.end();
}

/**
 * @brief Teleports camera to a new map
 * @param camera
 * @param map_id
 * @param destination_name
 * @param transition_style
 */
void Game::teleport_camera(const CameraPtr& camera,
                     const std::string map_id,
                     const std::string& destination_name,
                     Transition::Style transition_style,
                     const HeroPtr &opt_hero) {

  //Verify if there is already a teleportation for this camera
  auto it = std::find_if(cameras_teleportations.begin(),
                         cameras_teleportations.end(),
                         [&](const CameraTeleportation& ct){
    return ct.camera == camera;
  });

  // Set the old tp to be removed
  if(it != cameras_teleportations.end()) {
    it->removed = true;
  }

  CameraTeleportation ct;

  ct.camera = camera;
  ct.opt_hero = opt_hero;

  if(camera->is_on_map()) {
    ct.current_map = camera->get_map().shared_from_this_cast<Map>();
  }

  // prepare the next map
  if(map_id.empty()) {
    ct.next_map = nullptr;
  } else {
    ct.next_map = prepare_map(map_id);
  }

  if (ct.current_map != nullptr) {
    ct.current_map->check_suspended();
  }

  ct.destination_name = destination_name;
  ct.transition_style = transition_style;

  //Setup the transition
  auto transition = std::unique_ptr<Transition>(Transition::create(
                                                  transition_style,
                                                  Transition::Direction::CLOSING
                                              ));

  transition->start();
  ct.camera->set_transition(std::move(transition));

  //Camera teleported without hero, stop tracking
  if(!opt_hero) {
      camera->start_manual();
  }

  // Add the teleportation details to the list of current teleportations
  cameras_teleportations.emplace_back(std::move(ct));
}

/**
 * @brief Create a camera object with the given id
 * @param id
 * @return
 */
CameraPtr Game::create_camera(const std::string& id) {
    CameraPtr camera = std::make_shared<Camera>(id);
    cameras.push_back(camera);
    return camera;
}

/**
 * @brief remove a camera from the running game
 *
 * Removing a camera is equivalent to teleporting it to nowhere
 *
 * @param camera
 * @param transition_style
 */
void Game::remove_camera(const CameraPtr& camera, Transition::Style transition_style)
{
    //Make it transition to nowhere
    teleport_camera(camera, "", "", transition_style, nullptr);
}

/**
 * @brief Get the living cameras of this game
 * @return
 */
const std::vector<CameraPtr>& Game::get_cameras() const {
    return cameras;
}

/**
 * @brief Tells if the given map is current in this game
 * @param map
 */
bool Game::is_current_map(Map& map) const {
  return std::find(current_maps.begin(),
                   current_maps.end(),
                   map.shared_from_this()) != current_maps.end();
}

/**
 * @brief Tells if this game is running multiple maps
 * @return true if the games is multi-map
 */
bool Game::has_multiple_maps() const {
  return current_maps.size() > 1;
}

const MapPtr& Game::prepare_map(const std::string& map_id) {
  const auto it = std::find_if(current_maps.begin(), current_maps.end(),
                               [&](const MapPtr& map){
    return map->get_id() == map_id;
  });

  //If map is already loaded, return it immediatly
  if(it != current_maps.end()) {
    return *it;
  }

  // Load map and add it to the list
  MapPtr map = std::make_shared<Map>(map_id);
  map->load(*this);
  map->check_suspended();

  auto emp_it = current_maps.emplace(current_maps.begin(), map); //Emplace front to have default map being the new one
  return *emp_it;
}


/**
 * @brief Remove an entity from a map and ensure the map is unloaded if necessary
 * @param leaving the entity leaving the map
 * @param map the map
 */
void Game::leave_map(const EntityPtr &leaving, const MapPtr& map) {

  //Remove the hero from the map
  map->get_entities().remove_entity(*leaving);

  //check if map is empty
  if(!map->has_cameras()) {
    // unload the map on next tick
    maps_to_unload.insert(map);
  }


  /*next_map->set_destination(destination_name);
  this->current_transition_style = transition_style;
*/
}

/**
 * @brief Get the maps running in this game
 * @return a collection of maps
 */
const std::vector<MapPtr>& Game::get_maps() const {
    return current_maps;
}

/**
 * \brief Notifies the game objects that the another map has just become active.
 */
void Game::notify_map_changed(Map &map, Camera& camera) {

  // Call game:on_map_changed() in Lua.
  get_lua_context().game_on_map_changed(*this, map, camera);

  // Notify the equipment.
  get_equipment().notify_map_changed(map, camera);
}

/**
 * \brief Returns the transition style to use by default.
 * \return The default transition style.
 */
Transition::Style Game::get_default_transition_style() const {
  return get_savegame().get_default_transition_style();
}

/**
 * \param Sets the transition style to use by default.
 * \param default_transition_style The default transition style.
 */
void Game::set_default_transition_style(Transition::Style default_transition_style) {
  get_savegame().set_default_transition_style(default_transition_style);
}

/**
 * \brief Returns the state of the crystal blocks.
 *
 * Returns false if the orange blocks are lowered or true if the blue blocks are lowered.
 *
 * \return the state of the crystals or this world
 */
bool Game::get_crystal_state() const {
  return crystal_state;
}

/**
 * \brief Changes the state of the crystal blocks.
 */
void Game::change_crystal_state() {
  crystal_state = !crystal_state;
}

/**
 * \brief Returns whether the game is paused.
 * \return true if the game is paused
 */
bool Game::is_paused() const {
  return paused;
}

/**
 * \brief Returns whether we are playing a transition between two maps.
 * \return true if there is a transition
 */
bool Game::is_playing_transition() const {
  //return cameras_teleportations.size() > 0;

  return std::find_if(cameras_teleportations.begin(),
                      cameras_teleportations.end(),
                      [](const CameraTeleportation& tp){
    return !tp.is_finished();
  }) != cameras_teleportations.end();
}

/**
 * \brief Returns whether the game is suspended.
 *
 * This is true in the following cases:
 * - the game is paused,
 * - or a dialog a being dispayed,
 * - or a transition between two maps is playing,
 * - or the game over sequence is active,
 * - or the camera is moving,
 * - or a script explicitly suspended the game.
 *
 * \return true if the game is suspended
 */
bool Game::is_suspended() const {

  return !has_current_map() ||
      is_paused() ||
      is_dialog_enabled() ||
      is_playing_transition() ||
      is_showing_game_over() ||
      is_suspended_by_camera() ||
      is_suspended_by_script();
}

/**
 * \brief Returns whether a dialog is currently active.
 * \return true if a dialog box is being shown
 */
bool Game::is_dialog_enabled() const {
  return dialog_box.is_enabled();
}

/**
 * \brief Starts to show a dialog.
 *
 * No other dialog should be already running.
 * If the dialog does not exist, a non-fatal error message is printed
 * and no dialog is shown.
 *
 * \param dialog_id Id of the dialog to show.
 * \param info_ref Lua ref to an optional info parameter to pass to the
 * dialog box, or LUA_REFNIL.
 * \param callback_ref Lua ref to a function to call when the dialog finishes,
 * or an empty ref.
 */
void Game::start_dialog(
    const std::string& dialog_id,
    const ScopedLuaRef& info_ref,
    const ScopedLuaRef& callback_ref
) {
  if (!CurrentQuest::dialog_exists(dialog_id)) {
    Debug::error(std::string("No such dialog: '") + dialog_id + "'");
  }
  else {
    dialog_box.open(dialog_id, info_ref, callback_ref);
  }
}

/**
 * \brief Stops the dialog currently running if any.
 * \param status_ref Lua ref to a status value to return to the start_dialog
 * callback, or an empty ref. "skipped" means that the dialog was canceled by
 * the user.
 */
void Game::stop_dialog(const ScopedLuaRef& status_ref) {

  dialog_box.close(status_ref);
}

/**
 * \brief Returns whether the player can currently pause the game.
 *
 * He can pause the game if the pause command is enabled
 * and if his life is greater than zero.
 *
 * \return \c true if the player can currently pause the game.
 */
bool Game::can_pause() const {
  return !is_suspended()
      && is_pause_allowed()               // see if the map currently allows the pause command
      && get_equipment().get_life() > 0;  // don't allow to pause the game if the gameover sequence is about to start
}

/**
 * \brief Returns whether the player can currently unpause the game.
 * \return \c true if the player can currently unpause the game.
 */
bool Game::can_unpause() const {
  return is_paused()
      && is_pause_allowed()
      && !is_dialog_enabled();
}

/**
 * \brief Returns whether the pause command is available.
 *
 * Even when the pause command is available, the player may still
 * be unauthorized to pause the game, depending on the result of can_pause().
 *
 * \return \c true if the pause command is available.
 */
bool Game::is_pause_allowed() const {
  return pause_allowed;
}

/**
 * \brief Sets whether the pause command is available.
 *
 * Even when the pause command is available, the player may still
 * be unauthorized to pause the game, depending on the result of can_pause().
 *
 * \param pause_allowed \c true to make the pause command available.
 */
void Game::set_pause_allowed(bool pause_allowed) {

  this->pause_allowed = pause_allowed;
  get_commands_effects().set_pause_key_enabled(pause_allowed);
}

/**
 * \brief Pauses or resumes the game.
 * \param paused true to pause the game, false to resume it.
 */
void Game::set_paused(bool paused) {

  if (paused != is_paused()) {

    this->paused = paused;
    if (paused) {
      get_commands_effects().save_action_key_effect();
      get_commands_effects().set_action_key_effect(CommandsEffects::ACTION_KEY_NONE);
      get_commands_effects().save_sword_key_effect();
      get_commands_effects().set_sword_key_effect(CommandsEffects::ATTACK_KEY_NONE);
      get_commands_effects().set_pause_key_effect(CommandsEffects::PAUSE_KEY_RETURN);
      get_lua_context().game_on_paused(*this);
    }
    else {
      get_lua_context().game_on_unpaused(*this);
      get_commands_effects().restore_action_key_effect();
      get_commands_effects().restore_sword_key_effect();
      get_commands_effects().set_pause_key_effect(CommandsEffects::PAUSE_KEY_PAUSE);
    }
  }
}

/**
 * \brief Returns whether this game is currently suspended by a camera sequence.
 * \return \c true if a camera is suspending the game.
 */
bool Game::is_suspended_by_camera() const {

  if (!has_current_map()) {
    return false;
  }

  // The game is suspended when any camera is scrolling on a separator. //TODO avoid this
  for(const MapPtr& map : current_maps) {
    for(const CameraPtr& cam : map->get_entities().get_cameras()) {
      if(cam->is_traversing_separator()) {
        return true;
      }
    }
  }
  return false;
}

/**
 * \brief Returns whether this game is currently suspended by a script.
 * \return \c true if a script is suspending the game.
 */
bool Game::is_suspended_by_script() const {
  return suspended_by_script;
}

/**
 * \brief Sets whether this game is currently suspended by a script.
 * \param \c true to suspend the game, \c false to resume it
 * if nothing else is suspending it.
 */
void Game::set_suspended_by_script(bool suspended) {
  this->suspended_by_script = suspended;
}

/**
 * \brief Restarts the game with the current savegame state.
 */
void Game::restart() {
  //Transition each hero out of their map
  for(const CameraPtr& camera : cameras) {
    if(camera->is_on_map()) {
      CameraTeleportation ht;
      ht.camera = camera;
      ht.current_map = camera->get_map().shared_from_this_cast<Map>();
      auto transition = std::unique_ptr<Transition>(Transition::create(
                                                  Transition::Style::FADE,
                                                  Transition::Direction::CLOSING
                                              ));
      transition->start();
      camera->set_transition(std::move(transition));
      cameras_teleportations.emplace_back(std::move(ht));
    }
  }

  //TODO !

  restarting = true;
}

/**
 * \brief Returns whether the gameover sequence is being shown.
 * \return true if the gameover sequence is being shown
 */
bool Game::is_showing_game_over() const {
  return showing_game_over;
}

/**
 * \brief Launches the game-over sequence.
 */
void Game::start_game_over() {

  Debug::check_assertion(!is_showing_game_over(),
      "The game-over sequence is already active");

  showing_game_over = true;

  if (!get_lua_context().game_on_game_over_started(*this)) {
    // The script does not define a game-over sequence:
    // then, the built-in behavior is to restart the game.
    restart();
    stop_game_over();
  }
}

/**
 * \brief Cancels the current game-over sequence.
 */
void Game::stop_game_over() {

  Debug::check_assertion(is_showing_game_over(),
      "The game-over sequence is not running");

  get_lua_context().game_on_game_over_finished(*this);
  showing_game_over = false;
  if (!restarting && !get_main_loop().is_resetting()) {
    // The heroes gets back to life.
    for(const MapPtr& current_map : current_maps) {
      current_map->check_suspended();  // To unsuspend the heroes before making them blink.
      for(const HeroPtr& hero : current_map->get_entities().get_heroes()) {
        hero->notify_game_over_finished();
      }
    }
  }
}

/**
 * \brief Simulates pressing a game command.
 * \param command The command to simulate.
 */
void Game::simulate_command_pressed(Command command){
  commands->command_pressed(command);
}

/**
 * \brief Simulates releasing a game command.
 * \param command The command to simulate.
 */
void Game::simulate_command_released(Command command){
  commands->command_released(command);
}

bool Game::CameraTeleportation::is_finished() const {
  const auto& tr = camera->get_transition();
  return
      tr == nullptr or
      (tr->is_finished() and tr->get_direction() == Transition::Direction::OPENING);
}

}
