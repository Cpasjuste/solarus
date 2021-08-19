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
#include "solarus/entities/Block.h"
#include "solarus/entities/Chest.h"
#include "solarus/entities/Crystal.h"
#include "solarus/entities/Destructible.h"
#include "solarus/entities/Enemy.h"
#include "solarus/entities/Hero.h"
#include "solarus/entities/Hookshot.h"
#include "solarus/entities/Stairs.h"
#include "solarus/entities/Switch.h"
#include "solarus/movements/PathMovement.h"
#include "solarus/movements/TargetMovement.h"
#include <memory>
#include <string>

namespace Solarus {

/**
 * \brief Creates a hookshot.
 * \param hero the hero
 */
Hookshot::Hookshot(Hero& hero):
    Entity("", 0, hero.get_layer(), Point(0, 0), Size(0, 0)),
    next_sound_date(System::now()),
    has_to_go_back(false),
    going_back(false),
    entity_reached(nullptr),
    link_sprite(std::make_shared<Sprite>("entities/hookshot")),
    hero(hero)
{

  // initialize the entity
  int direction = hero.get_animation_direction();
  const SpritePtr& sprite = create_sprite("entities/hookshot");
  sprite->enable_pixel_collisions();
  sprite->set_current_direction(direction);
  link_sprite->set_current_animation("link");

  set_size(16, 16);
  set_origin(8, 13);
  set_drawn_in_y_order(true);
  set_xy(hero.get_xy());

  std::string path = " ";
  path[0] = '0' + (direction * 2);
  set_movement(std::make_shared<PathMovement>(path, 192, true, false, false));
}

/**
 * \brief Returns the type of entity.
 * \return the type of entity
 */
EntityType Hookshot::get_type() const {
  return ThisType;
}

/**
 * \brief Returns whether a teletransporter is currently considered as an obstacle for this entity.
 * \param teletransporter a teletransporter
 * \return true if the teletransporter is currently an obstacle for this entity
 */
bool Hookshot::is_teletransporter_obstacle(Teletransporter& /* teletransporter */) {
  return false;
}

/**
 * \copydoc Entity::is_stream_obstacle
 */
bool Hookshot::is_stream_obstacle(Stream& /* stream */) {
  return false;
}

/**
 * \brief Returns whether some stairs are currently considered as an obstacle for this entity.
 * \param stairs an stairs entity
 * \return true if the stairs are currently an obstacle for this entity
 */
bool Hookshot::is_stairs_obstacle(Stairs& stairs) {
  return stairs.is_inside_floor() && get_layer() == stairs.get_layer();
}

/**
 * \brief Returns whether a deep water tile is currently considered as an obstacle for this entity.
 * \return true if the deep water tiles are currently an obstacle for this entity
 */
bool Hookshot::is_deep_water_obstacle() const {
  return false;
}

/**
 * \brief Returns whether a hole is currently considered as an obstacle for this entity.
 * \return true if the holes are currently an obstacle for this entity
 */
bool Hookshot::is_hole_obstacle() const {
  return false;
}

/**
 * \brief Returns whether lava is currently considered as an obstacle for this entity.
 * \return true if lava is currently an obstacle for this entity
 */
bool Hookshot::is_lava_obstacle() const {
  return false;
}

/**
 * \brief Returns whether prickles are currently considered as an obstacle for this entity.
 * \return true if prickles are currently an obstacle for this entity
 */
bool Hookshot::is_prickle_obstacle() const {
  return false;
}

/**
 * \brief Returns whether a ladder is currently considered as an obstacle for this entity.
 * \return true if the ladders are currently an obstacle for this entity
 */
bool Hookshot::is_ladder_obstacle() const {
  return false;
}

/**
 * \brief Returns whether a switch is currently considered as an obstacle by this entity.
 * \param sw a switch
 * \return true if the switch is currently an obstacle for this entity
 */
bool Hookshot::is_switch_obstacle(Switch& /* sw */) {
  return false;
}

/**
 * \brief Returns whether a crystal is currently considered as an obstacle for this entity.
 * \param crystal a crystal
 * \return true if the crystal is currently an obstacle for this entity
 */
bool Hookshot::is_crystal_obstacle(Crystal& /* crystal */) {
  return false;
}

/**
 * \copydoc Entity::is_jumper_obstacle
 */
bool Hookshot::is_jumper_obstacle(Jumper& /* jumper */, const Rectangle& /* candidate_position */) {
  return false;
}

/**
 * \brief Updates this entity.
 */
void Hookshot::update() {

  Entity::update();

  if (is_suspended()) {
    return;
  }

  uint32_t now = System::now();
  if (now >= next_sound_date) {
    Sound::play("hookshot", get_game().get_resource_provider());
    next_sound_date = now + 150;
  }

  if (entity_reached == nullptr) {
    if (!going_back) {

      if (has_to_go_back) {
        going_back = true;
        std::shared_ptr<Movement> movement = std::make_shared<TargetMovement>(
            hero.shared_from_this_cast<Hero>(),
            0,
            0,
            192,
            true
        );
        clear_movement();
        set_movement(movement);
      }
      else if (get_distance(hero) >= 120) {
        go_back();
      }
    }
    else if (get_distance(hero) == 0 ||
        (get_movement() != nullptr && get_movement()->is_finished())) {
      remove_from_map();
      hero.start_state_from_ground();
    }
  }
}

/**
 * \copydoc Entity::built_in_draw
 */
void Hookshot::built_in_draw(Camera& camera) {

  static constexpr int nb_links = 7;
  static constexpr Point dxy[] = {
    {  16,  -5 },
    {   0, -13 },
    { -16,  -5 },
    {   0,   7 }
  };

  Entity::built_in_draw(camera);

  // also draw the links
  const SpritePtr& sprite = get_sprite();
  if (sprite == nullptr) {
    return;
  }
  int direction = sprite->get_current_direction();
  if (direction > 4) {
    return;
  }
  int x1 = hero.get_x() + dxy[direction].x;
  int y1 = hero.get_y() + dxy[direction].y;
  int x2 = get_x();
  int y2 = get_y() - 5;

  Point link_xy;
  for (int i = 0; i < nb_links; i++) {
    link_xy.x = x1 + (x2 - x1) * i / nb_links;
    link_xy.y = y1 + (y2 - y1) * i / nb_links;
    get_map().draw_visual(*link_sprite, link_xy);
  }
}

/**
 * \brief Returns whether the hookshot is currently flying.
 * \return true if the hookshot was shot, is not going back and has not reached any target yet
 */
bool Hookshot::is_flying() const {
  return !is_going_back() && entity_reached == nullptr;
}

/**
 * \brief Returns whether the hookshot is going back towards the hero, i.e. if go_back() has been called.
 * \return true if the hookshot is going back
 */
bool Hookshot::is_going_back() const {
  return has_to_go_back || going_back;
}

/**
 * \brief Makes the hookshot go back towards the hero.
 */
void Hookshot::go_back() {

  SOLARUS_ASSERT(!is_going_back(), "The hookshot is already going back");

  has_to_go_back = true;
}

/**
 * \brief Attachs the hookshot to an entity and makes the hero move towards this entity.
 * \param entity_reached the entity to attach the hookshot to
 */
void Hookshot::attach_to(Entity& entity_reached) {

  SOLARUS_ASSERT(this->entity_reached == nullptr,
      "The hookshot is already attached to an entity");

  this->entity_reached = &entity_reached;
  clear_movement();
  const SpritePtr& sprite = get_sprite();
  if (sprite == nullptr) {
    return;
  }
  int direction = sprite->get_current_direction();
  if (direction > 4) {
    return;
  }
  std::string path = " ";
  path[0] = '0' + (direction * 2);
  hero.set_movement(std::make_shared<PathMovement>(
      path, 192, true, false, false
  ));
}

/**
 * \brief Notifies this entity that it has just failed to change its position
 * because of obstacles.
 */
void Hookshot::notify_obstacle_reached() {

  if (is_flying()) {
    if (!get_map().test_collision_with_border(
        get_movement()->get_last_collision_box_on_obstacle())) {
      // play a sound unless the obstacle is the map border
      Sound::play("sword_tapping", get_game().get_resource_provider());
    }
    go_back();
  }
}

/**
 * \copydoc Entity::notify_collision_with_enemy(Enemy&, Sprite&, Sprite&)
 */
void Hookshot::notify_collision_with_enemy(
    Enemy& enemy, Sprite& /* this_sprite */, Sprite& enemy_sprite) {

  if (!overlaps(hero)) {
    enemy.try_hurt(EnemyAttack::HOOKSHOT, *this, &enemy_sprite);
  }
}

/**
 * \copydoc Entity::notify_attacked_enemy
 */
void Hookshot::notify_attacked_enemy(
    EnemyAttack /* attack */,
    Enemy& /* victim */,
    Sprite* /* victim_sprite */,
    const EnemyReaction::Reaction& result,
    bool /* killed */) {

  if (result.type != EnemyReaction::ReactionType::IGNORED && !is_going_back()) {
    go_back();
  }
}

/**
 * \brief This function is called when a chest detects a collision with this entity.
 * \param chest the chest
 */
void Hookshot::notify_collision_with_chest(Chest& chest) {

  if (is_flying()) {
    attach_to(chest);
  }
}

/**
 * \brief This function is called when a destructible item detects a non-pixel precise collision with this entity.
 * \param destructible the destructible item
 * \param collision_mode the collision mode that detected the event
 */
void Hookshot::notify_collision_with_destructible(Destructible& destructible, CollisionMode /* collision_mode */) {

  if (destructible.is_obstacle_for(*this) && is_flying()) {

    if (destructible.get_can_explode()) {
      destructible.explode();
      go_back();
    }
    else {
      attach_to(destructible);
    }
  }
}

/**
 * \brief This function is called when a block detects a collision with this entity.
 * \param block the block
 */
void Hookshot::notify_collision_with_block(Block& block) {

  if (is_flying()) {
    attach_to(block);
  }
}

/**
 * \brief This function is called when a switch detects a collision with this entity.
 * \param sw the switch
 * \param collision_mode the collision mode that detected the event
 */
void Hookshot::notify_collision_with_switch(Switch& sw, CollisionMode collision_mode) {

  if (is_flying() && collision_mode == COLLISION_OVERLAPPING) {

    sw.try_activate();
    if (!is_going_back()) {
      go_back();
      Sound::play("sword_tapping", get_game().get_resource_provider());
    }
  }
}

/**
 * \brief This function is called when a crystal detects a collision with this entity.
 * \param crystal the crystal
 * \param collision_mode the collision mode that detected the event
 */
void Hookshot::notify_collision_with_crystal(Crystal& crystal, CollisionMode /* collision_mode */) {

  if (is_flying()) {

    crystal.activate(*this);
    if (!is_going_back()) {
      go_back();
    }
  }
}

}

