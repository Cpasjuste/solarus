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
#include "solarus/core/Game.h"
#include "solarus/core/Map.h"
#include "solarus/core/System.h"
#include "solarus/entities/Bomb.h"
#include "solarus/entities/CarriedObject.h"
#include "solarus/entities/Entities.h"
#include "solarus/entities/Explosion.h"
#include "solarus/entities/Hero.h"
#include "solarus/entities/Stream.h"
#include "solarus/graphics/Sprite.h"
#include "solarus/movements/PathMovement.h"
#include <memory>

namespace Solarus {

/**
 * \brief Constructor.
 *
 * Creates a bomb.
 *
 * \param name Name identifying the entity on the map or an empty string.
 * \param layer layer of the entity to create
 * \param xy Coordinates of the entity to create.
 */
Bomb::Bomb(const std::string& name, int layer, const Point& xy):
  Entity(name, 0, layer, xy, Size(16, 16)),
  explosion_date(System::now_ms() + 6000) {

  set_collision_modes(CollisionMode::COLLISION_FACING);
  const SpritePtr& sprite = create_sprite("entities/bomb");
  sprite->enable_pixel_collisions();
  set_size(16, 16);
  set_origin(8, 13);
  set_drawn_in_y_order(true);
  set_weight(0);
}

/**
 * \brief Returns the type of entity.
 * \return the type of entity
 */
EntityType Bomb::get_type() const {
  return ThisType;
}

/**
 * \copydoc Entity::is_stream_obstacle
 */
bool Bomb::is_stream_obstacle(Stream& /* stream */) {
  return false;
}

/**
 * \brief Returns whether a teletransporter is currently considered as an obstacle for this entity.
 * \param teletransporter a teletransporter
 * \return true if the teletransporter is currently an obstacle for this entity
 */
bool Bomb::is_teletransporter_obstacle(Teletransporter& /* teletransporter */) {
  return false;
}

/**
 * \brief Returns whether a deep water tile is currently considered as an obstacle for this entity.
 * \return true if the deep water tiles are currently an obstacle for this entity
 */
bool Bomb::is_deep_water_obstacle() const {
  return false;
}

/**
 * \brief Returns whether a hole is currently considered as an obstacle for this entity.
 * \return true if the holes are currently an obstacle for this entity
 */
bool Bomb::is_hole_obstacle() const {
  return false;
}

/**
 * \brief Returns whether lava is currently considered as an obstacle for this entity.
 * \return true if lava is currently an obstacle for this entity
 */
bool Bomb::is_lava_obstacle() const {
  return false;
}

/**
 * \brief Returns whether prickles are currently considered as an obstacle for this entity.
 * \return true if prickles are currently an obstacle for this entity
 */
bool Bomb::is_prickle_obstacle() const {
  return false;
}

/**
 * \brief Returns whether a ladder is currently considered as an obstacle for this entity.
 * \return true if the ladders are currently an obstacle for this entity
 */
bool Bomb::is_ladder_obstacle() const {
  return false;
}

/**
 * \brief This function is called by the engine when an entity overlaps this detector.
 * \param entity_overlapping the entity overlapping the detector
 * \param collision_mode the collision mode that detected the collision
 */
void Bomb::notify_collision(Entity& entity_overlapping, CollisionMode collision_mode) {

  entity_overlapping.notify_collision_with_bomb(*this, collision_mode);
}

/**
 * \brief This function is called when an explosion's sprite
 * detects a pixel-perfect collision with a sprite of this entity.
 * \param explosion the explosion
 * \param sprite_overlapping the sprite of the current entity that collides with the explosion
 */
void Bomb::notify_collision_with_explosion(Explosion& /* explosion */, Sprite& /* sprite_overlapping */) {

  if (!is_being_removed()) {
    explode();
  }
}

/**
 * \copydoc Entity::notify_collision_with_stream
 */
void Bomb::notify_collision_with_stream(Stream& stream, int /* dx */, int /* dy */) {

  if (get_movement() == nullptr) {
    // TODO use a StreamAction, since it now works with any entity and not only the hero.

    // Check that a significant part of the bomb is on the stream.
    Rectangle center(get_center_point(), Size(2, 2));
    center.add_xy(-1, -1);

    if (stream.overlaps(center)) {
      set_xy(stream.get_xy());

      std::string path = "  ";
      path[0] = path[1] = '0' + stream.get_direction();
      clear_movement();
      set_movement(std::make_shared<PathMovement>(
          path, 64, false, false, false
      ));
    }
  }
}

/**
 * \brief This function is called when the entity has just moved.
 */
void Bomb::notify_position_changed() {

  Entity::notify_position_changed();

  if (get_hero().get_facing_entity() == this
      && get_commands_effects().get_action_key_effect() == CommandsEffects::ACTION_KEY_LIFT
      && !get_hero().is_facing_point_in(get_bounding_box())) {

    get_commands_effects().set_action_key_effect(CommandsEffects::ACTION_KEY_NONE);
  }
}

/**
 * \copydoc Entity::notify_action_command_pressed
 */
bool Bomb::notify_action_command_pressed() {

  CommandsEffects::ActionKeyEffect effect = get_commands_effects().get_action_key_effect();

  if (effect == CommandsEffects::ACTION_KEY_LIFT
      && get_hero().get_facing_entity() == this
      && get_hero().is_facing_point_in(get_bounding_box())) {

    get_hero().start_lifting(std::make_shared<CarriedObject>(
        get_hero(),
        *this,
        "entities/bomb",
        "",
        0,
        explosion_date)
    );
    Sound::play("lift", get_game().get_resource_provider());
    remove_from_map();
    return true;
  }

  return false;
}

/**
 * \brief This function is called by the map when the game is suspended or resumed.
 * \param suspended true to suspend the entity, false to resume it
 */
void Bomb::set_suspended(bool suspended) {

  Entity::set_suspended(suspended); // suspend the animation and the movement

  if (!suspended && get_when_suspended() != 0) {
    // recalculate the timer
    uint32_t diff = System::now_ms() - get_when_suspended();
    explosion_date += diff;
  }
}

/**
 * \brief This function is called repeatedly.
 */
void Bomb::update() {

  Entity::update();

  if (is_suspended()) {
    return;
  }

  // check the explosion date
  uint32_t now = System::now_ms();
  if (now >= explosion_date) {
    explode();
  }
  else if (now >= explosion_date - 1500) {
    const SpritePtr& sprite = get_sprite();
    if (sprite != nullptr &&
        sprite->get_current_animation() != "stopped_explosion_soon") {
      sprite->set_current_animation("stopped_explosion_soon");
    }
  }

  // destroy the movement once finished
  if (get_movement() != nullptr && get_movement()->is_finished()) {
    clear_movement();
  }

  // check collision with explosions, streams, etc.
  check_collision_with_detectors();
}

/**
 * \brief Makes the bomb explode.
 */
void Bomb::explode() {

  get_entities().add_entity(std::make_shared<Explosion>(
      "", get_layer(), get_center_point(), true
  ));
  Sound::play("explosion", get_game().get_resource_provider());
  remove_from_map();
}

}

