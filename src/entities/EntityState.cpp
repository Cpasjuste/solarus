/*
 * Copyright (C) 2006-2018 Christopho, Solarus - http://www.solarus-games.org
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
#include "solarus/core/CommandsEffects.h"
#include "solarus/core/Debug.h"
#include "solarus/core/Equipment.h"
#include "solarus/core/EquipmentItem.h"
#include "solarus/core/Game.h"
#include "solarus/core/Map.h"
#include "solarus/core/System.h"
#include "solarus/entities/Door.h"
#include "solarus/entities/EntityState.h"
#include "solarus/entities/Hero.h"
#include "solarus/entities/Jumper.h"
#include "solarus/entities/Npc.h"
#include "solarus/entities/Stairs.h"
#include "solarus/entities/Switch.h"
#include "solarus/graphics/Sprite.h"
#include "solarus/hero/HeroSprites.h"
#include "solarus/lua/LuaContext.h"

namespace Solarus {

/**
 * \brief Creates a state without specifying the entity to control yet.
 *
 * Call set_entity() later before starting the state.
 *
 * \param state_name A name describing this state.
 */
Entity::State::State(const std::string& state_name):
  suspended(false),
  when_suspended(0),
  name(state_name),
  stopping(false) {

}

/**
 * \brief Destructor.
 *
 * The state is destroyed once it is not the current state of the entity anymore.
 */
Entity::State::~State() {
}

/**
 * \brief Returns a name describing this state.
 * \return A name describing this state.
 */
const std::string& Entity::State::get_name() const {
  return name;
}

/**
 * \brief Returns whether this state is the current state.
 * \return \c true if this state is the current state.
 */
bool Entity::State::is_current_state() const {

  if (!has_entity()) {
    return false;
  }

  return entity->get_state().get() == this && !entity->get_state()->is_stopping();
}

/**
 * \brief Returns whether this state is being stopped.
 * \return \c true if this state is being stopped.
 */
bool Entity::State::is_stopping() const {
  return stopping;
}

/**
 * \brief Returns the entity of this state.
 * \return The entity.
 */
Entity& Entity::State::get_entity() {
  return *entity;
}

/**
 * \brief Returns the entity of this state.
 * \return The entity.
 */
const Entity& Entity::State::get_entity() const {
  return *entity;
}

/**
 * \brief Sets the entity to control with this state.
 * \param entity The entity to control with this state.
 */
void Entity::State::set_entity(Entity& entity) {

  this->entity = std::static_pointer_cast<Entity>(entity.shared_from_this());
  if (entity.is_on_map()) {
    this->map = std::static_pointer_cast<Map>(entity.get_map().shared_from_this());
  }
}

/**
 * \brief Returns whether this state is already associated to an entity.
 * \return \c true if an entity was set.
 */
bool Entity::State::has_entity() const {
  return this->entity != nullptr;
}

/**
 * \brief Returns the current map.
 * \return the map
 */
Map& Entity::State::get_map() {
  return *map;
}

/**
 * \brief Returns the entities of the current map.
 * \return the entities
 */
Entities& Entity::State::get_entities() {
  return map->get_entities();
}

/**
 * \brief Returns the shared Lua context.
 * \return The Lua context where all scripts are run.
 */
LuaContext& Entity::State::get_lua_context() {
  return *get_entity().get_lua_context();
}

/**
 * \brief Returns the current game.
 * \return the game
 */
Game& Entity::State::get_game() {
  return map->get_game();
}

/**
 * \brief Returns the current game.
 * \return the game
 */
const Game& Entity::State::get_game() const {
  return map->get_game();
}

/**
 * \brief Returns the current equipment.
 * \return the equipment
 */
Equipment& Entity::State::get_equipment() {
  return get_game().get_equipment();
}

/**
 * \brief Returns the current equipment.
 * \return the equipment
 */
const Equipment& Entity::State::get_equipment() const {
  return get_game().get_equipment();
}

/**
 * \brief Returns the keys effect manager.
 * \return the keys effect
 */
CommandsEffects& Entity::State::get_commands_effects() {
  return get_game().get_commands_effects();
}

/**
 * \brief Returns the game commands.
 * \return The commands.
 */
GameCommands& Entity::State::get_commands() {
  return get_game().get_commands();
}

/**
 * \brief Returns the game commands.
 * \return The commands.
 */
const GameCommands& Entity::State::get_commands() const {
  return get_game().get_commands();
}

/**
 * \brief Starts this state.
 *
 * This function is called automatically when this state becomes the active
 * state of the entity.
 * The initializations should be done here rather than in the constructor.
 *
 * \param previous_state The previous state or nullptr if this is the first state
 * (for information).
 */
void Entity::State::start(const State* /* previous_state */) {

  Debug::check_assertion(entity != nullptr, "No entity specified");

  set_suspended(entity->is_suspended());

  // Notify Lua.
  if (entity->is_on_map()) {
    get_lua_context().entity_on_state_changed(*entity, get_name());
  }
}

/**
 * \brief Ends this state.
 *
 * This function is called automatically when this state stops being the
 * active one.
 * You should here close everything the start() function has opened.
 *
 * \param next_state The next state (for information).
 */
void Entity::State::stop(const State* next_state) {

  Debug::check_assertion(!is_stopping(),
      std::string("This state is already stopping: ") + get_name());

  // Notify Lua.
  if (entity->is_on_map()) {
    std::string next_state_name;
    if (next_state != nullptr) {
      next_state_name = next_state->get_name();
    }
    get_lua_context().entity_on_state_changing(*entity, get_name(), next_state_name);
  }

  this->stopping = true;
}

/**
 * \brief Updates this state.
 *
 * This function is called repeatedly while this state is the active state.
 */
void Entity::State::update() {

}

/**
 * \brief Draws this state.
 *
 * This function draws this entity in its current state.
 * The default implement does nothing.
 * If your state needs to draw additional elements, you can redefine this function.
 */
void Entity::State::draw_on_map() {

}

/**
 * \brief Returns whether this state is suspended.
 * \return \c true if this state is suspended.
 */
bool Entity::State::is_suspended() const {
  return suspended;
}

/**
 * \brief Notifies this state that the game was just suspended or resumed.
 * \param suspended true if the game is suspended
 */
void Entity::State::set_suspended(bool suspended) {

  if (suspended != this->suspended) {

    this->suspended = suspended;

    // remember the date if the state is being suspended
    if (suspended) {
      when_suspended = System::now();
    }
  }
}

/**
 * \brief Returns the date when this state was suspended.
 * \return The date when this state was suspended.
 */
uint32_t Entity::State::get_when_suspended() const {
  return when_suspended;
}

/**
 * \brief This function is called when a game command is pressed and the game
 * is not suspended.
 * \param command The command pressed.
 */
void Entity::State::notify_command_pressed(GameCommand command) {

  switch (command) {

    // action key
  case GameCommand::ACTION:
    notify_action_command_pressed();
    break;

    // sword key
  case GameCommand::ATTACK:
    notify_attack_command_pressed();
    break;

    // move the entity
  case GameCommand::RIGHT:
    notify_direction_command_pressed(0);
    break;

  case GameCommand::UP:
    notify_direction_command_pressed(1);
    break;

  case GameCommand::LEFT:
    notify_direction_command_pressed(2);
    break;

  case GameCommand::DOWN:
    notify_direction_command_pressed(3);
    break;

    // use an equipment item
  case GameCommand::ITEM_1:
    notify_item_command_pressed(1);
    break;

  case GameCommand::ITEM_2:
    notify_item_command_pressed(2);
    break;

  default:
    break;
  }
}

/**
 * \brief This function is called when a command is released if the game is
 * not suspended.
 * \param command The command released.
 */
void Entity::State::notify_command_released(GameCommand command) {

  switch (command) {

  case GameCommand::ACTION:
    notify_action_command_released();
    break;

  case GameCommand::ATTACK:
    notify_attack_command_released();
    break;

  case GameCommand::RIGHT:
    notify_direction_command_released(0);
    break;

  case GameCommand::UP:
    notify_direction_command_released(1);
    break;

  case GameCommand::LEFT:
    notify_direction_command_released(2);
    break;

  case GameCommand::DOWN:
    notify_direction_command_released(3);
    break;

  case GameCommand::ITEM_1:
    notify_item_command_released(0);
    break;

  case GameCommand::ITEM_2:
    notify_item_command_released(1);
    break;

  default:
    break;
  }
}

/**
 * \brief Notifies this state that the action command was just pressed.
 */
void Entity::State::notify_action_command_pressed() {
}

/**
 * \brief Notifies this state that the action command was just released.
 */
void Entity::State::notify_action_command_released() {
}

/**
 * \brief Notifies this state that the attack command was just pressed.
 */
void Entity::State::notify_attack_command_pressed() {
}

/**
 * \brief Notifies this state that the attack command was just released.
 */
void Entity::State::notify_attack_command_released() {
}

/**
 * \brief Notifies this state that a directional command was just pressed.
 * \param direction4 direction of the command (0 to 3)
 */
void Entity::State::notify_direction_command_pressed(int /* direction4 */) {
}

/**
 * \brief Notifies this state that a directional command was just released.
 * \param direction4 direction of the command (0 to 3)
 */
void Entity::State::notify_direction_command_released(int /* direction4 */) {
}

/**
 * \brief Notifies this state that an item command was just pressed.
 * \param slot The slot activated (1 or 2).
 */
void Entity::State::notify_item_command_pressed(int /* slot */) {
}

/**
 * \brief Notifies this state that an item command was just released.
 * \param slot the slot (1 or 2)
 */
void Entity::State::notify_item_command_released(int /* slot */) {
}

/**
 * \brief Changes the map.
 *
 * This function is called when the entity is about to go to another map.
 *
 * \param map the new map
 */
void Entity::State::set_map(Map& map) {
  this->map = std::static_pointer_cast<Map>(map.shared_from_this());
}

/**
 * \brief Returns whether the game over sequence can start in the current state.
 * \return true if the game over sequence can start in the current state
 */
bool Entity::State::can_start_gameover_sequence() const {
  return true;
}

/**
 * \brief Returns whether the entity is visible in the current state.
 * \return true if the entity is displayed in the current state
 */
bool Entity::State::is_visible() const {
  return true;
}

/**
 * \brief Returns whether the animation direction is locked.
 *
 * When this function returns false, which is the case most of the time,
 * it means that the animation direction is set to the movement direction.
 * When it returns true, it means that the animation direction is fixed
 * and do not depends on the movement direction anymore (this is the case
 * when the hero is loading his sword).
 *
 * \return true if the animation direction is locked
 */
bool Entity::State::is_direction_locked() const {
  return false;
}

/**
 * \brief Returns whether the player can control his movements in the current state.
 *
 * This function returns true if and only if the current movement of the entity is
 * an instance of PlayerMovement.
 *
 * \return true if the player can control his movements
 */
bool Entity::State::get_can_control_movement() const {
  return false;
}

/**
 * \brief Returns the direction of the entity's movement as defined by the controls applied by the player
 * and the movements allowed is the current state.
 *
 * If he is not moving, -1 is returned.
 * This direction may be different from the real movement direction because of obstacles.
 *
 * \return the entity's wanted direction between 0 and 7, or -1 if he is stopped
 */
int Entity::State::get_wanted_movement_direction8() const {
  return -1;
}

/**
 * \brief Notifies this state that the walking speed has changed.
 *
 * If the entity can walk in this state, the state should modify its movement
 * to set the new speed.
 */
void Entity::State::notify_walking_speed_changed() {
}

/**
 * \brief Notifies this state that the layer has changed.
 */
void Entity::State::notify_layer_changed() {
}

/**
 * \brief Notifies this state that the movement has changed.
 *
 * This function is called when the entity's movement direction changes (for instance
 * because the player pressed or released a directional key, or the entity just reached an obstacle).
 * The animations and collisions should be updated according to the new movement.
 */
void Entity::State::notify_movement_changed() {
}

/**
 * \brief Notifies this state that the movement if finished.
 */
void Entity::State::notify_movement_finished() {
}

/**
 * \brief Notifies this state that the entity has just failed to change its
 * position because of obstacles.
 */
void Entity::State::notify_obstacle_reached() {
}

/**
 * \brief Notifies this state that the entity has just changed its
 * position.
 */
void Entity::State::notify_position_changed() {
}

/**
 * \brief Returns whether the entity ignores the effect of deep water in this state.
 *
 * Returns false by default.
 *
 * \return true if the entity ignores the effect of deep water in the current state
 */
bool Entity::State::can_avoid_deep_water() const {
  return false;
}

/**
 * \brief Returns whether the entity ignores the effect of holes in this state.
 *
 * Returns false by default.
 *
 * \return true if the entity ignores the effect of holes in the current state
 */
bool Entity::State::can_avoid_hole() const {
  return false;
}

/**
 * \brief Returns whether the entity ignores the effect of ice in this state.
 *
 * Returns false by default.
 *
 * \return \c true if the entity ignores the effect of ice in the current state.
 */
bool Entity::State::can_avoid_ice() const {
  return false;
}

/**
 * \brief Returns whether the entity ignores the effect of lava in this state.
 *
 * Returns false by default.
 *
 * \return true if the entity ignores the effect of lava in the current state
 */
bool Entity::State::can_avoid_lava() const {
  return false;
}

/**
 * \brief Returns whether the entity ignores the effect of prickles in this state.
 *
 * Returns false by default.
 *
 * \return true if the entity ignores the effect of prickles in the current state
 */
bool Entity::State::can_avoid_prickle() const {
  return false;
}

/**
 * \brief Returns whether shallow water affects the entity in this state.
 *
 * Returns \c true by default.
 *
 * \return \c true if shallow water affects the entity in this state.
 */
bool Entity::State::is_affected_by_shallow_water() const {
  return true;
}

/**
 * \brief Returns whether grass affects the entity in this state.
 *
 * Returns \c true by default.
 *
 * \return \c true if grass affects the entity in this state.
 */
bool Entity::State::is_affected_by_grass() const {
  return true;
}

/**
 * \brief Returns whether ladders affects the entity in this state.
 *
 * Returns \c true by default.
 *
 * \return \c true if ladders affects the entity in this state.
 */
bool Entity::State::is_affected_by_ladder() const {
  return true;
}

/**
 * \brief Returns whether the entity is touching the ground in the current state.
 *
 * Returns true by default.
 *
 * \return true if the entity is touching the ground in the current state
 */
bool Entity::State::is_touching_ground() const {
  return true;
}

/**
 * \brief Returns whether the entity's current position can be considered
 * as a place to come back after a bad ground (hole, deep water, etc).
 *
 * Returns is_touching_ground() by default.
 *
 * \return true if the entity can come back here
 */
bool Entity::State::can_come_from_bad_ground() const {
  return is_touching_ground();
}

/**
 * \brief Notifies this state that the ground was just changed.
 */
void Entity::State::notify_ground_changed() {
}

/**
 * \brief Returns whether this state ignores the collisions with the detectors and the ground.
 * \return true if the collisions are ignored
 */
bool Entity::State::are_collisions_ignored() const {
  return false;
}

/**
 * \brief Returns whether traversable ground is considered an obstacle in this state.
 *
 * Returns \c false by default.
 *
 * \return \c true if traversable ground is considered an obstacle in this state.
 */
bool Entity::State::is_traversable_obstacle() const {
  return false;
}

/**
 * \brief Returns whether wall ground is considered an obstacle in this state.
 *
 * Returns \c true by default.
 *
 * \return \c true if wall ground is considered an obstacle in this state.
 */
bool Entity::State::is_wall_obstacle() const {
  return true;
}

/**
 * \brief Returns whether low wall ground is considered an obstacle in this state.
 *
 * Returns \c true by default.
 *
 * \return \c true if low wall ground is considered an obstacle in this state.
 */
bool Entity::State::is_low_wall_obstacle() const {
  return true;
}

/**
 * \brief Returns whether grass ground is considered an obstacle in this state.
 *
 * Returns \c true by default.
 *
 * \return \c true if grass ground is considered an obstacle in this state.
 */
bool Entity::State::is_grass_obstacle() const {
  return false;
}

/**
 * \brief Returns whether shallow water is considered as an obstacle in this state.
 *
 * Returns false by default.
 *
 * \return true if shallow water is considered as an obstacle in this state
 */
bool Entity::State::is_shallow_water_obstacle() const {
  return false;
}

/**
 * \brief Returns whether deep water tile is considered as an obstacle in this state.
 *
 * Returns false by default.
 *
 * \return true if deep water is considered as an obstacle in this state
 */
bool Entity::State::is_deep_water_obstacle() const {
  return false;
}

/**
 * \brief Returns whether a hole is considered as an obstacle in this state.
 *
 * Returns false by default.
 *
 * \return true if the holes are considered as obstacles in this state
 */
bool Entity::State::is_hole_obstacle() const {
  return false;
}

/**
 * \brief Returns whether ice ground is considered an obstacle in this state.
 *
 * Returns \c true by default.
 *
 * \return \c true if ice ground is considered an obstacle in this state.
 */
bool Entity::State::is_ice_obstacle() const {
  return false;
}

/**
 * \brief Returns whether lava is considered as an obstacle in this state.
 *
 * Returns false by default.
 *
 * \return true if lava is considered as obstacles in this state
 */
bool Entity::State::is_lava_obstacle() const {
  return false;
}

/**
 * \brief Returns whether prickles are considered as an obstacle in this state.
 *
 * Returns false by default.
 *
 * \return true if prickles are considered as obstacles in this state
 */
bool Entity::State::is_prickle_obstacle() const {
  return false;
}

/**
 * \brief Returns whether a ladder is considered as an obstacle in this state.
 *
 * Returns false by default.
 *
 * \return true if the ladders are considered as obstacles in this state
 */
bool Entity::State::is_ladder_obstacle() const {
  return false;
}

/**
 * \brief Returns whether a hero is considered as an obstacle in this state.
 *
 * Returns false by default.
 *
 * \param hero A hero.
 * \return \c true if the hero is an obstacle in this state.
 */
bool Entity::State::is_hero_obstacle(
    Hero& /* hero */) {
  return false;
}

/**
 * \brief Returns whether a block is considered as an obstacle in this state.
 *
 * Returns true by default.
 *
 * \param block A block.
 * \return \c true if the block is an obstacle in this state.
 */
bool Entity::State::is_block_obstacle(
    Block& /* block */) {
  return true;
}

/**
 * \brief Returns whether a teletransporter is considered as an obstacle in this state.
 *
 * Returns false by default.
 *
 * \param teletransporter A teletransporter.
 * \return \c true if the teletransporter is an obstacle in this state.
 */
bool Entity::State::is_teletransporter_obstacle(
    Teletransporter& /* teletransporter */) {
  return false;
}

/**
 * \brief Returns whether the entity ignores the effect of teletransporters in this state.
 *
 * Returns false by default.
 *
 * \return true if the entity ignores the effect of teletransporters in this state
 */
bool Entity::State::can_avoid_teletransporter() const {
  return false;
}

/**
 * \brief Returns whether the effect of teletransporters is delayed in this state.
 *
 * When overlapping a teletransporter, if this function returns true, the teletransporter
 * will not be activated immediately. The state then has to activate it when it is ready.
 * Returns false by default.
 *
 * \return true if the effect of teletransporters is delayed in this state
 */
bool Entity::State::is_teletransporter_delayed() const {
  return false;
}

/**
 * \brief Returns whether a stream is considered as an obstacle in this state.
 * \param stream A stream.
 * \return \c true if the stream is an obstacle in this state.
 */
bool Entity::State::is_stream_obstacle(
    Stream& /* stream */) {
  return false;
}

/**
 * \brief Returns whether the entity ignores the effect of a stream in this state.
 * \param stream A stream.
 * \return \c true if the entity ignores the effect of the stream in this state.
 */
bool Entity::State::can_avoid_stream(const Stream& /* stream */) const {
  return false;
}

/**
 * \brief Returns whether this state can continue when taking a stream.
 *
 * This function only matters if the stream is applied in this state, that is,
 * when can_avoid_stream() is \c false.
 *
 * \param stream A stream.
 * \return \c true if this state can continue, \c false if the entity should
 * get back to FreeState.
 */
bool Entity::State::can_persist_on_stream(const Stream& /* stream */) const {
  return true;
}

/**
 * \brief Returns whether the entity can take stairs in this state.
 * If false is returned, stairs have no effect (but they are obstacle for the entity).
 *
 * Returns false by default.
 *
 * \return true if the entity can take stairs in this state
 */
bool Entity::State::get_can_take_stairs() const {
  return false;
}

/**
 * \brief Returns whether some stairs are considered as obstacle in this state.
 * \param stairs Some stairs.
 * \return \c true if the stairs are obstacle in this state.
 */
bool Entity::State::is_stairs_obstacle(Stairs& stairs) {

  // The entity may overlap stairs in rare cases,
  // for example if the hero arrived by swimming over them
  // and thus did not activate them.
  // This is allowed and can be used to leave water pools for example.
  if (get_entity().overlaps(stairs)) {
    return false;
  }

  return true;
}

/**
 * \brief Returns whether a sensor is considered as an obstacle in this state.
 *
 * Returns false by default.
 *
 * \param sensor A sensor.
 * \return \c true if the sensor is an obstacle in this state.
 */
bool Entity::State::is_sensor_obstacle(Sensor& /* sensor */) {
  return false;
}

/**
 * \brief Returns whether a switch is considered as an obstacle in this state.
 *
 * By default, this function returns true for solid switches and false for other ones.
 *
 * \param sw A switch.
 * \return \c true if the switch is an obstacle in this state.
 */
bool Entity::State::is_switch_obstacle(Switch& sw) {
  return sw.is_solid();
}

/**
 * \brief Returns whether a raised crystal block is currently considered as an obstacle in this state.
 *
 * This function returns true by default.
 *
 * \param raised_block A crystal block raised.
 * \return \c true if the raised block is currently an obstacle in this state.
 */
bool Entity::State::is_raised_block_obstacle(CrystalBlock& /* raised_block */) {
  return true;
}

/**
 * \brief Returns whether a crystal is currently considered as an obstacle in this state.
 *
 * This function returns true by default.
 *
 * \param crystal A crystal.
 * \return \c true if the crystal is currently an obstacle in this state.
 */
bool Entity::State::is_crystal_obstacle(Crystal& /* crystal */) {
  return true;
}

/**
 * \brief Returns whether a non-playing character is currently considered as an obstacle in this state.
 *
 * By default, this depends on the NPC.
 *
 * \param npc A non-playing character.
 * \return \c true if the NPC is currently an obstacle in this state.
 */
bool Entity::State::is_npc_obstacle(Npc& npc) {
  return !npc.is_traversable();
}

/**
 * \brief Returns whether a door is currently considered as an obstacle in this state.
 *
 * By default, this function returns \c true unless the door is open.
 *
 * \param door A door.
 * \return \c true if the door is currently an obstacle in this state.
 */
bool Entity::State::is_door_obstacle(Door& door) {
  return !door.is_open();
}

/**
 * \brief Returns whether an enemy is currently considered as an obstacle by this entity.
 *
 * This function returns false by default.
 *
 * \param enemy An enemy/
 * \return \c true if the enemy is currently an obstacle in this state.
 */
bool Entity::State::is_enemy_obstacle(Enemy& /* enemy */) {
  return false;
}

/**
 * \brief Returns whether a jumper is considered as an obstacle in this state
 * for the entity at the specified position.
 * \param jumper A jumper.
 * \param candidate_position Position of the entity to test.
 * \return \c true if the jumper is an obstacle in this state with this
 * entity position.
 */
bool Entity::State::is_jumper_obstacle(
    Jumper& /* jumper */, const Rectangle& /* candidate_position */) {
  return true;
}

/**
 * \brief Returns whether a separator is considered as an obstacle in this
 * state.
 *
 * Returns false by default.
 *
 * \param separator A separator.
 * \return \c true if the separator is an obstacle in this state.
 */
bool Entity::State::is_separator_obstacle(Separator& /* separator */) {
  return false;
}

/**
 * \brief Returns whether a destructible is considered as an obstacle in this state.
 *
 * By default, this function returns true.
 *
 * \param destructible A destructible object.
 * \return \c true if the destructible object is currently an obstacle in this state.
 */
bool Entity::State::is_destructible_obstacle(Destructible& /* destructible */) {
  return true;
}

/**
 * \brief Returns whether the entity ignores the effect of sensors in this state.
 *
 * Returns false by default.
 *
 * \return true if the entity ignores the effect of sensors in this state
 */
bool Entity::State::can_avoid_sensor() const {
  return false;
}

/**
 * \brief Returns whether the entity ignores the effect of switches in this state.
 *
 * Returns false by default.
 *
 * \return true if the entity ignores the effect of switches in this state
 */
bool Entity::State::can_avoid_switch() const {
  return false;
}

/**
 * \brief Returns whether crystals can be activated by the sword in this state.
 *
 * Returns false by default.
 *
 * \return true if crystals can be activated by the sword in this state
 */
bool Entity::State::can_sword_hit_crystal() const {
  return false;
}

/**
 * \brief Returns whether the entity ignores the effect of explosions in this state.
 *
 * Returns false by default.
 *
 * \return true if the entity ignores the effect of explosions in this state
 */
bool Entity::State::can_avoid_explosion() const {
  return false;
}

/**
 * \brief Returns whether the entity can trigger a jumper in this state.
 *
 * If false is returned, jumpers have no effect (but they are obstacle for
 * the entity).
 *
 * Returns false by default.
 *
 * \return \c true if the entity can use jumpers in this state.
 */
bool Entity::State::get_can_take_jumper() const {
  return false;
}

/**
 * \brief Notifies this state that the entity is activating a jumper.
 *
 * Does nothing by default.
 * Redefine this function if you want to perform the jump in your state.
 *
 * \param jumper The jumper activated.
 */
void Entity::State::notify_jumper_activated(Jumper& /* jumper */) {
}

/**
 * \brief Notifies this state that the entity has just attacked an enemy.
 *
 * This function is called even if this attack was not successful.
 *
 * \param attack The attack.
 * \param victim The enemy just hurt.
 * \param victim_sprite The enemy's sprite that was touched or nullptr.
 * \param result How the enemy has reacted to the attack.
 * \param killed Whether the attack has just killed the enemy.
 */
void Entity::State::notify_attacked_enemy(
    EnemyAttack /* attack */,
    Enemy& /* victim */,
    const Sprite* /* victim_sprite */,
    EnemyReaction::Reaction& /* result */,
    bool /* killed */) {
}

/**
 * \brief Returns the damage power of the sword for the current attack.
 *
 * Redefine the function if your state changes the power of the sword
 * (typically for a spin attack).
 *
 * \return the current damage factor of the sword
 */
int Entity::State::get_sword_damage_factor() const {

  return get_equipment().get_ability(Ability::SWORD);
}

/**
 * \brief Returns whether the entity can be hurt in this state.
 *
 * Returns false by default.
 *
 * \param attacker an attacker that is trying to hurt the entity
 * (or nullptr if the source of the attack is not an entity)
 * \return true if the entity can be hurt in this state
 */
bool Entity::State::get_can_be_hurt(Entity* /* attacker */) const {
  return false;
}

/**
 * \brief Returns whether the entity can walk normally and interact with entities
 * in this state.
 *
 * Returns false by default.
 *
 * \return true if the entity can walk normally
 */
bool Entity::State::is_free() const {
  return false;
}

/**
 * \brief Returns whether the entity is using an equipment item in this state.
 *
 * Returns false by default.
 *
 * \return true if the entity is using an equipment item.
 */
bool Entity::State::is_using_item() const {
  return false;
}

/**
 * \brief When the entity is using an equipment item, returns that item.
 * \return The current equipment item.
 */
EquipmentItemUsage& Entity::State::get_item_being_used() {

  Debug::die("No item is being used in this state");
}

/**
 * \brief Returns whether the entity is brandishing a treasure in this state.
 *
 * Returns false by default.
 *
 * \return \c true if the entity is brandishing a treasure.
 */
bool Entity::State::is_brandishing_treasure() const {
  return false;
}

/**
 * \brief Returns whether the entity is grabbing or pulling an entity in this state.
 *
 * Returns false by default.
 *
 * \return true if the entity is grabbing or pulling an entity
 */
bool Entity::State::is_grabbing_or_pulling() const {
  return false;
}

/**
 * \brief Returns whether the entity is grabbing and moving an entity in this state.
 *
 * If he is not grabbing any entity, false is returned.
 * Returns false by default.
 *
 * \return true if the entity is grabbing and moving an entity
 */
bool Entity::State::is_moving_grabbed_entity() const {
  return false;
}

/**
 * \brief Notifies the entity that the entity he is pushing or pulling in this state
 * cannot move anymore because of a collision.
 */
void Entity::State::notify_grabbed_entity_collision() {
}

/**
 * \brief Tests whether the entity is cutting with his sword the specified entity
 * for which a collision was detected.
 *
 * When the sword sprite collides with an entity,
 * this function can be called to determine whether the entity is
 * really cutting this particular entity precisely.
 * This depends on the entity's state, his direction and his
 * distance to the detector.
 * This function assumes that there is already a collision
 * between the sword sprite and the detector's sprite.
 * This function should be called to check whether the
 * entity wants to cut a bush or some grass.
 * Returns false by default.
 *
 * \param entity The entity to check.
 * \return \c true if the sword is cutting this entity.
 */
bool Entity::State::is_cutting_with_sword(Entity& /* detector */) {
  return false;
}

/**
 * \brief Returns whether the entity can swing his sword in this state.
 *
 * Returns false by default.
 *
 * \return true if the entity can swing his sword in this state
 */
bool Entity::State::get_can_start_sword() const {
  return false;
}

/**
 * \brief Returns whether the entity can pick a treasure in this state.
 *
 * Returns false by default.
 *
 * \param item The equipment item to obtain.
 * \return true if the entity can pick that treasure in this state.
 */
bool Entity::State::get_can_pick_treasure(EquipmentItem& /* item */) const {
  return false;
}

/**
 * \brief Returns whether attack can be stopped with a shield in this state.
 *
 * Returns \c true by default.
 *
 * \return \c true if the shield is active is this state.
 */
bool Entity::State::get_can_use_shield() const {
  return true;
}

/**
 * \brief Returns whether the entity can use an equipment item in this state.
 *
 * Returns false by default.
 *
 * \param item The equipment item to check.
 * \return true if the entity can use an equipment item in this state.
 */
bool Entity::State::get_can_start_item(EquipmentItem& /* item */) const {
  return false;
}

/**
 * \brief Returns whether the entity is currently carrying an item in this state.
 *
 * This function returns true if get_carried_object() is not nullptr.
 * Redefine get_carried_object() if the entity is able to carry an item in this state.
 *
 * \return true if the entity is currently carrying an item in this state
 */
bool Entity::State::is_carrying_item() const {
  return get_carried_object() != nullptr;
}

/**
 * \brief Returns the item currently carried by the entity in this state, if any.
 *
 * Redefine this function to make the entity able to carry an item in this state.
 *
 * \return the item carried by the entity, or nullptr
 */
std::shared_ptr<CarriedObject> Entity::State::get_carried_object() const {
  return nullptr;
}

/**
 * \brief Returns the action to do with an item previously carried by the entity when this state starts.
 *
 * Returns CarriedObject::Behavior::THROW by default.
 *
 * \return the action to do with a previous carried object when this state starts
 */
CarriedObject::Behavior Entity::State::get_previous_carried_object_behavior() const {
  return CarriedObject::Behavior::THROW;
}

}

