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
#include "solarus/audio/Sound.h"
#include "solarus/core/CommandsEffects.h"
#include "solarus/core/Debug.h"
#include "solarus/core/Game.h"
#include "solarus/core/Map.h"
#include "solarus/core/QuestFiles.h"
#include "solarus/core/System.h"
#include "solarus/entities/Block.h"
#include "solarus/entities/Hero.h"
#include "solarus/entities/Separator.h"
#include "solarus/entities/Switch.h"
#include "solarus/graphics/Sprite.h"
#include "solarus/lua/LuaContext.h"
#include "solarus/movements/RelativeMovement.h"
#include <memory>

namespace Solarus {

/**
 * \brief Creates a block.
 * \param name name identifying this block
 * \param layer layer of the entity to create
 * \param xy Coordinate of the entity to create.
 * \param direction the only direction where the block can be moved
 * or -1 to allow it to be pushed in any direction
 * \param sprite_name animation set id of the sprite for this block
 * \param can_be_pushed true to allow the hero to push this block
 * \param can_be_pulled true to allow the hero to pull this block
 * \param max_moves indicates how many times the block can
 * be moved (-1 means infinite).
 */
Block::Block(
    const std::string& name,
    int layer,
    const Point& xy,
    int direction,
    const std::string& sprite_name,
    bool can_be_pushed,
    bool can_be_pulled,
    int max_moves
):
  Entity(name, direction, layer, xy, Size(16, 16)),
  max_moves(max_moves),
  sound_played(false),
  when_can_move(System::now()),
  last_position(xy),
  initial_position(xy),
  initial_max_moves(max_moves),
  can_be_pushed(can_be_pushed),
  can_be_pulled(can_be_pulled) {

  Debug::check_assertion(max_moves >= -1,
      "maxm_moves must be between postive, 0 or -1");

  set_collision_modes(CollisionMode::COLLISION_FACING);
  set_origin(8, 13);
  set_direction(direction);
  const SpritePtr& sprite = create_sprite(sprite_name);
  set_drawn_in_y_order(sprite->get_size().height > 16);
}

/**
 * \brief Returns the type of entity.
 * \return the type of entity
 */
EntityType Block::get_type() const {
  return ThisType;
}

/**
 * \brief Returns whether this entity is sensible to the ground below it.
 * \return \c true if this entity is sensible to its ground.
 */
bool Block::is_ground_observer() const {
  return true;  // To make blocks fall into holes, water, etc.
}

/**
 * \brief Returns whether this entity is an obstacle for another one.
 * \param other another entity
 * \return true
 */
bool Block::is_obstacle_for(Entity& other) {

  return other.is_block_obstacle(*this);
}

/**
 * \copydoc Entity::is_hole_obstacle
 */
bool Block::is_hole_obstacle() const {
  return false;
}

/**
 * \copydoc Entity::is_deep_water_obstacle
 */
bool Block::is_deep_water_obstacle() const {
  return false;
}

/**
 * \copydoc Entity::is_lava_obstacle
 */
bool Block::is_lava_obstacle() const {
  return false;
}

/**
 * \brief Returns whether a teletransporter is currently considered as an obstacle for this entity.
 * \param teletransporter a teletransporter
 * \return true if the teletransporter is currently an obstacle for this entity
 */
bool Block::is_teletransporter_obstacle(Teletransporter& /* teletransporter */) {
  // necessary to push a block into a hole having a teletransporter
  return false;
}

/**
 * \brief Returns whether the hero is currently considered as an obstacle by this entity.
 * \param hero the hero
 * \return true if the hero is an obstacle for this entity.
 */
bool Block::is_hero_obstacle(Hero& hero) {

  // The block is not an obstacle when the hero is already overlapping it,
  // which is easily possible with blocks created dynamically.
  if (hero.overlaps(*this)) {
    return false;
  }

  // When the block is moved by the hero, one pixel can overlap.
  return get_movement() == nullptr;
}

/**
 * \brief Returns whether an enemy is currently considered as an obstacle by this entity.
 * \param enemy an enemy
 * \return true if this enemy is currently considered as an obstacle by this entity.
 */
bool Block::is_enemy_obstacle(Enemy& /* enemy */) {
  return true;
}

/**
 * \brief Returns whether a destructible item is currently considered as an obstacle by this entity.
 * \param destructible a destructible item
 * \return true if the destructible item is currently an obstacle by this entity
 */
bool Block::is_destructible_obstacle(Destructible& /* destructible */) {
  return true;
}

/**
 * \copydoc Entity::is_separator_obstacle
 */
bool Block::is_separator_obstacle(Separator& separator, const Rectangle& candidate_position) {
  return separator.is_crossed_by(candidate_position);
}

/**
 * \copydoc Entity::notify_created
 */
void Block::notify_created() {

  Entity::notify_created();

  check_collision_with_detectors();
  update_ground_below();
}

/**
 * \brief This function is called by the engine when there is a collision with another entity.
 * \param entity_overlapping the entity overlapping the detector
 * \param collision_mode the collision mode that detected the collision
 */
void Block::notify_collision(Entity& entity_overlapping, CollisionMode /* collision_mode */) {

  entity_overlapping.notify_collision_with_block(*this);
}

/**
 * \brief This function is called when a switch detects a collision with this entity.
 * \param sw the switch
 * \param collision_mode the collision mode that detected the event
 */
void Block::notify_collision_with_switch(Switch& sw, CollisionMode /* collision_mode */) {

  sw.try_activate(*this);
}

/**
 * \copydoc Entity::notify_action_command_pressed
 */
bool Block::notify_action_command_pressed() {

  if (get_commands_effects().get_action_key_effect() == CommandsEffects::ACTION_KEY_GRAB &&
      get_hero().can_grab()
  ) {
    get_hero().start_grabbing();
    return true;
  }

  return Entity::notify_action_command_pressed();
}

/**
 * \brief This function is called when the player tries to push or pull this block.
 * \return true if the player is allowed to move this block
 */
bool Block::start_movement_by_hero() {

  Hero& hero = get_hero();
  bool pulling = hero.is_grabbing_or_pulling();
  int allowed_direction = get_direction();
  int hero_direction = hero.get_animation_direction();
  if (pulling) {
    // the movement direction is backwards
    hero_direction = (hero_direction + 2) % 4;
  }

  if (get_movement() != nullptr             // the block is already moving
      || max_moves == 0                     // the block cannot move anymore
      || System::now() < when_can_move      // the block cannot move for a while
      || (pulling && !can_be_pulled)        // the hero tries to pull a block that cannot be pulled
      || (!pulling && !can_be_pushed)       // the hero tries to push a block that cannot be pushed
      || (allowed_direction != -1 && hero_direction != allowed_direction)) { // incorrect direction
    return false;
  }

  int dx = get_x() - hero.get_x();
  int dy = get_y() - hero.get_y();

  set_movement(std::make_shared<RelativeMovement>(
      std::static_pointer_cast<Hero>(hero.shared_from_this()),
      dx,
      dy,
      false
  ));
  sound_played = false;

  return true;
}

/**
 * \brief Notifies the block that it has just moved.
 */
void Block::notify_position_changed() {

  Entity::notify_position_changed();

  // Now we know that the block moves at least of 1 pixel:
  // we can play the sound.
  if (get_movement() != nullptr && !sound_played) {
    Sound::play("hero_pushes");
    sound_played = true;
  }
}

/**
 * \brief Notifies this entity that it has just failed to change its position
 * because of obstacles.
 */
void Block::notify_obstacle_reached() {

  // The block is stopped by an obstacle while being pushed or pulled.
  get_hero().notify_grabbed_entity_collision();

  Entity::notify_obstacle_reached();
}

/**
 * \copydoc Entity::notify_ground_below_changed
 */
void Block::notify_ground_below_changed() {

  Ground ground = get_ground_below();
  switch (ground) {

    case Ground::HOLE:
      Sound::play("jump");
      remove_from_map();
      break;

    case Ground::LAVA:
    case Ground::DEEP_WATER:
      Sound::play("splash");
      remove_from_map();
      break;

    default:
      break;
  }
}

/**
 * \brief This function is called when this entity stops being moved by the
 * hero.
 */
void Block::stop_movement_by_hero() {

  clear_movement();
  when_can_move = System::now() + moving_delay;

  // see if the block has moved
  if (get_xy() != last_position) {

    // the block has moved
    last_position = get_xy(); // save the new position for next time

    if (max_moves > 0) {
      --max_moves;
    }
  }
}

/**
 * \copydoc Entity::notify_moving_by
 */
void Block::notify_moving_by(Entity& /* entity */) {

  get_lua_context()->block_on_moving(*this);
}

/**
 * \copydoc Entity::notify_moved_by
 */
void Block::notify_moved_by(Entity& /* entity */) {

  get_lua_context()->block_on_moved(*this);
}

/**
 * \brief Resets the block at its initial position.
 */
void Block::reset() {

  if (get_movement() != nullptr) {
    // the block was being pushed or pulled by the hero
    clear_movement();
    when_can_move = System::now() + moving_delay;
  }

  last_position = initial_position;
  this->max_moves = initial_max_moves;
  set_xy(initial_position);
  notify_position_changed();
}

/**
 * \brief Returns whether this block can be pushed.
 * \return \c true if it can be pushed, independently of the maximum moves.
 */
bool Block::is_pushable() const {
  return can_be_pushed;
}

/**
 * \brief Returns whether this block can be pushed.
 * \param pushable \c true if it can be pushed, independently of the maximum
 * moves.
 */
void Block::set_pushable(bool pushable) {
  this->can_be_pushed = pushable;
}

/**
 * \brief Returns whether this block can be pulled.
 * \return \c true if it can be pulled, independently of the maximum moves.
 */
bool Block::is_pullable() const {
  return can_be_pulled;
}

/**
 * \brief Sets whether this block can be pulled.
 * \param pullable \c true to make the block pullable, independently of the
 * maximum moves.
 */
void Block::set_pullable(bool pullable) {
  this->can_be_pulled = pullable;
}

/**
 * \brief Returns the initial maximum moves of this block.
 *
 * This value is the one passed to the constructor or to set_max_moves,
 * no matter if the block was already moved or not.
 *
 * \return How many times the block can be moved
 * (-1 means infinite).
 */
int Block::get_max_moves() const {
  return initial_max_moves;
}

/**
 * \brief Sets the maximum number of moves of this block.
 *
 * This resets the remaining allowed moves.
 *
 * \param max_moves How many times the block can be moved
 * (-1 means infinite).
 */
void Block::set_max_moves(int max_moves) {

  Debug::check_assertion(max_moves >= -1,
        "max_moves must be positive, 0 or -1");

  this->initial_max_moves = max_moves;
  this->max_moves = max_moves;
}

}

