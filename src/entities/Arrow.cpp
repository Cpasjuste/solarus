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
#include "solarus/core/Debug.h"
#include "solarus/core/Game.h"
#include "solarus/core/Map.h"
#include "solarus/core/System.h"
#include "solarus/entities/Arrow.h"
#include "solarus/entities/Crystal.h"
#include "solarus/entities/Destructible.h"
#include "solarus/entities/Enemy.h"
#include "solarus/entities/Hero.h"
#include "solarus/entities/Npc.h"
#include "solarus/entities/Stairs.h"
#include "solarus/entities/Switch.h"
#include "solarus/graphics/Sprite.h"
#include "solarus/movements/PathMovement.h"
#include "solarus/movements/RelativeMovement.h"
#include <memory>
#include <string>

namespace Solarus {

/**
 * \brief Creates an arrow.
 * \param hero The hero.
 */
Arrow::Arrow(const Hero& hero):
  Entity("", 0, hero.get_layer(), Point(0, 0), Size(0, 0)),
  hero(hero) {

  // initialize the entity
  int direction = hero.get_animation_direction();
  const SpritePtr& sprite = create_sprite("entities/arrow");
  sprite->enable_pixel_collisions();
  sprite->set_current_direction(direction);
  set_drawn_in_y_order(true);

  if (direction % 2 == 0) {
    // Horizontal.
    set_size(16, 8);
    set_origin(8, 4);
  }
  else {
    // Vertical.
    set_size(8, 16);
    set_origin(4, 8);
  }

  set_xy(hero.get_center_point());
  notify_position_changed();

  std::string path = " ";
  path[0] = '0' + (direction * 2);
  set_movement(std::make_shared<PathMovement>(
      path, 192, true, false, false
  ));

  disappear_date = System::now_ms() + 10000;
  stop_now = false;
  entity_reached = nullptr;
}

/**
 * \brief Returns the type of entity.
 * \return the type of entity
 */
EntityType Arrow::get_type() const {
  return ThisType;
}

/**
 * \brief Returns whether a teletransporter is currently considered as an obstacle for this entity.
 * \param teletransporter a teletransporter
 * \return true if the teletransporter is currently an obstacle for this entity
 */
bool Arrow::is_teletransporter_obstacle(Teletransporter& /* teletransporter */) {
  return false;
}

/**
 * \copydoc Entity::is_stream_obstacle
 */
bool Arrow::is_stream_obstacle(Stream& /* stream */) {
  return false;
}

/**
 * \brief Returns whether some stairs are currently considered as an obstacle for this entity.
 * \param stairs an stairs entity
 * \return true if the stairs are currently an obstacle for this entity
 */
bool Arrow::is_stairs_obstacle(Stairs& stairs) {
  return stairs.is_inside_floor() && get_layer() == stairs.get_layer();
}

/**
 * \brief Returns whether a low wall is currently considered as an obstacle
 * by this entity.
 * \return \c true if low walls are currently obstacle for this entity.
 */
bool Arrow::is_low_wall_obstacle() const {
  return false;
}

/**
 * \brief Returns whether a deep water tile is currently considered as an obstacle for this entity.
 * \return true if the deep water tiles are currently an obstacle for this entity
 */
bool Arrow::is_deep_water_obstacle() const {
  return false;
}

/**
 * \brief Returns whether a hole is currently considered as an obstacle for this entity.
 * \return true if the holes are currently an obstacle for this entity
 */
bool Arrow::is_hole_obstacle() const {
  return false;
}

/**
 * \brief Returns whether lava is currently considered as an obstacle for this entity.
 * \return true if lava is currently an obstacle for this entity
 */
bool Arrow::is_lava_obstacle() const {
  return false;
}

/**
 * \brief Returns whether prickles are currently considered as an obstacle for this entity.
 * \return true if prickles are currently an obstacle for this entity
 */
bool Arrow::is_prickle_obstacle() const {
  return false;
}

/**
 * \brief Returns whether a ladder is currently considered as an obstacle for this entity.
 * \return true if the ladders are currently an obstacle for this entity
 */
bool Arrow::is_ladder_obstacle() const {
  return false;
}

/**
 * \brief Returns whether a switch is currently considered as an obstacle by this entity.
 * \param sw a switch
 * \return true if the switch is currently an obstacle for this entity
 */
bool Arrow::is_switch_obstacle(Switch& /* sw */) {
  return false;
}

/**
 * \brief Returns whether a raised crystal block is currently considered as an obstacle for this entity.
 * \param raised_block a crystal block raised
 * \return false
 */
bool Arrow::is_raised_block_obstacle(CrystalBlock& /* raised_block */) {
  // arrows can traverse the crystal blocks
  return false;
}

/**
 * \brief Returns whether a crystal is currently considered as an obstacle for this entity.
 * \param crystal a crystal
 * \return true if the crystal is currently an obstacle for this entity
 */
bool Arrow::is_crystal_obstacle(Crystal& /* crystal */) {
  return false;
}

/**
 * \brief Returns whether a non-playing character is currently considered as an obstacle for this entity.
 * \param npc a non-playing character
 * \return true if the NPC is currently an obstacle for this entity
 */
bool Arrow::is_npc_obstacle(Npc& npc) {
  return npc.is_solid();
}

/**
 * \copydoc Entity::is_jumper_obstacle
 */
bool Arrow::is_jumper_obstacle(Jumper& /* jumper */, const Rectangle& /* candidate_position */) {
  return false;
}

/**
 * \brief Updates this entity.
 */
void Arrow::update() {

  Entity::update();

  if (is_suspended()) {
    return;
  }

  uint32_t now = System::now_ms();

  // stop the movement if necessary (i.e. stop() was called)
  if (stop_now) {
    clear_movement();
    stop_now = false;

    if (entity_reached != nullptr) {
      // the arrow just hit an entity (typically an enemy) and this entity may have a movement
      Point dxy = get_xy() - entity_reached->get_xy();
      set_movement(std::make_shared<RelativeMovement>(
          entity_reached, dxy.x, dxy.y, true
      ));
    }
  }

  if (entity_reached != nullptr) {

    // see if the entity reached is still valid
    if (is_stopped()) {
      // the arrow is stopped because the entity that was reached just disappeared
      disappear_date = now;
    }
    else if (entity_reached->get_type() == EntityType::DESTRUCTIBLE && !entity_reached->is_obstacle_for(*this)) {
      disappear_date = now;
    }
    else if (entity_reached->get_type() == EntityType::ENEMY &&
        (std::static_pointer_cast<Enemy>(entity_reached)->is_dying())) {
      // the enemy is dying
      disappear_date = now;
    }
  }

  // see if the arrow just hit a wall or an entity
  bool reached_obstacle = false;

  const SpritePtr& sprite = get_sprite();
  if (sprite != nullptr && sprite->get_current_animation() != "reached_obstacle") {

    if (entity_reached != nullptr) {
      // the arrow was just attached to an entity
      reached_obstacle = true;
    }
    else if (is_stopped()) {

      if (has_reached_map_border()) {
        // the map border was reached: destroy the arrow
        disappear_date = now;
      }
      else {
        // the arrow has just hit another obstacle
        reached_obstacle = true;
      }
    }
  }

  if (reached_obstacle) {
    // an obstacle or an entity was just reached
    disappear_date = now + 1500;
    if (sprite != nullptr) {
      sprite->set_current_animation("reached_obstacle");
    }
    Sound::play("arrow_hit", get_game().get_resource_provider());

    if (entity_reached == nullptr) {
      clear_movement();
    }
    check_collision_with_detectors();
  }

  // destroy the arrow when disappear_date is reached
  if (now >= disappear_date) {
    remove_from_map();
  }
}

/**
 * \brief This function is called by the map when the game is suspended or resumed.
 * \param suspended true to suspend the entity, false to resume it
 */
void Arrow::set_suspended(bool suspended) {

  Entity::set_suspended(suspended); // suspend the movement

  if (!suspended) {
    // recalculate the timer
    disappear_date += System::now_ms() - get_when_suspended();
  }
}

/**
 * \brief Stops the arrow movement.
 */
void Arrow::stop() {
  stop_now = true;
}

/**
 * \brief Returns whether the arrow is stopped.
 * \return true if the arrow is stopped
 */
bool Arrow::is_stopped() const {
  return get_movement() == nullptr || get_movement()->is_finished();
}

/**
 * \brief Returns whether the arrow is currently flying.
 * \return true if the arrow was shot and has not reached a target yet
 */
bool Arrow::is_flying() const {
  return !is_stopped() && entity_reached == nullptr;
}

/**
 * \brief Stops the arrow movement and attaches the arrow to an entity that was just reached.
 * \param entity_reached the entity that was reached
 */
void Arrow::attach_to(Entity& entity_reached) {

  Debug::check_assertion(this->entity_reached == nullptr,
      "This arrow is already attached to an entity");

  this->entity_reached = std::static_pointer_cast<Entity>(
      entity_reached.shared_from_this()
  );
  stop_now = true;
}

/**
 * \brief This function is called when a switch detects a collision with this entity.
 * \param sw the switch
 * \param collision_mode the collision mode that detected the event
 */
void Arrow::notify_collision_with_switch(Switch& sw, CollisionMode /* collision_mode */) {

  if (entity_reached != nullptr) {
    return;
  }

  if (sw.is_arrow_target() && is_stopped()) {
    sw.try_activate(*this);
    attach_to(sw);
  }
  else if (sw.is_solid() && is_flying()) {
    sw.try_activate();
    attach_to(sw);
  }
}

/**
 * \brief This function is called when a crystal detects a collision with this entity.
 * \param crystal the crystal
 * \param collision_mode the collision mode that detected the event
 */
void Arrow::notify_collision_with_crystal(Crystal& crystal, CollisionMode collision_mode) {

  if (collision_mode == COLLISION_OVERLAPPING && is_flying()) {

    crystal.activate(*this);
    attach_to(crystal);
  }
}

/**
 * \brief This function is called when a destructible item detects a non-pixel perfect collision with this entity.
 * \param destructible the destructible item
 * \param collision_mode the collision mode that detected the event
 */
void Arrow::notify_collision_with_destructible(
    Destructible& destructible, CollisionMode /* collision_mode */) {

  if (destructible.is_obstacle_for(*this) && is_flying()) {

    if (destructible.get_can_explode()) {
      destructible.explode();
      remove_from_map();
    }
    else {
      attach_to(destructible);
    }
  }
}

/**
 * \copydoc Entity::notify_collision_with_enemy(Enemy&, Sprite&, Sprite&)
 */
void Arrow::notify_collision_with_enemy(
    Enemy& enemy, Sprite& /* this_sprite */, Sprite& enemy_sprite) {

  if (!overlaps(hero) && is_flying()) {
    enemy.try_hurt(EnemyAttack::ARROW, *this, &enemy_sprite);
  }
}

/**
 * \copydoc Entity::notify_attacked_enemy
 */
void Arrow::notify_attacked_enemy(
    EnemyAttack /* attack */,
    Enemy& victim,
    Sprite* /* victim_sprite */,
    const EnemyReaction::Reaction& result,
    bool killed) {

  if (result.type == EnemyReaction::ReactionType::PROTECTED) {
    stop();
    attach_to(victim);
  }
  else if (result.type != EnemyReaction::ReactionType::IGNORED) {
    if (killed) {
      remove_from_map();
    }
    else {
      attach_to(victim);
    }
  }
}

/**
 * \brief Returns whether the arrow has just hit the map border.
 * \return true if the arrow has just hit the map border
 */
bool Arrow::has_reached_map_border() {

  const SpritePtr& sprite = get_sprite();
  if (sprite != nullptr && sprite->get_current_animation() != "flying") {
    return false;
  }

  if (get_movement() == nullptr) {
    return false;
  }

  return get_map().test_collision_with_border(get_movement()->get_last_collision_box_on_obstacle());
}

}

