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
#include "solarus/audio/Sound.h"
#include "solarus/core/Debug.h"
#include "solarus/core/Game.h"
#include "solarus/core/Map.h"
#include "solarus/core/QuestFiles.h"
#include "solarus/entities/DynamicTile.h"
#include "solarus/entities/Entities.h"
#include "solarus/entities/Stairs.h"
#include <list>

namespace Solarus {

/**
 * \brief Creates a new stairs entity.
 * \param name Name of the entity to create.
 * \param layer Layer of the entity to create on the map.
 * \param xy Coordinates of the entity to create.
 * \param direction Direction of the stairs (0 to 3).
 * \param subtype The subtype of stairs.
 */
Stairs::Stairs(
    const std::string& name,
    int layer,
    const Point& xy,
    int direction,
    Subtype subtype):
  Entity(name, direction, layer, xy, Size(16, 16)),
  subtype(subtype) {

  set_collision_modes(COLLISION_TOUCHING | COLLISION_OVERLAPPING);

  if (is_inside_floor()) {
    set_layer_independent_collisions(true);
  }
  else {
    set_size(16, 8);
    if (direction == 3) {  // Down.
      set_origin(0, -8);
    }
  }
}

/**
 * \brief Returns the type of entity.
 * \return the type of entity
 */
EntityType Stairs::get_type() const {
  return ThisType;
}

/**
 * \brief Returns whether entities of this type can be drawn.
 * \return \c true if this type of entity can be drawn.
 */
bool Stairs::can_be_drawn() const {
  return false;
}

/**
 * \brief Returns whether entities of this type can override the ground
 * of where they are placed.
 * \return \c true if this type of entity can change the ground.
 */
bool Stairs::can_change_ground() const {

  // To allow the hero to stay on the highest of both layers of the stairs.
  return true;
}

/**
 * \brief When can_change_ground() is \c true, returns the ground defined
 * by this entity.
 *
 * Entities overlapping it should take it into account.
 *
 * \return The ground defined by this entity.
 */
Ground Stairs::get_ground() const {
  return Ground::TRAVERSABLE;
}

/**
 * \copydoc Entity::notify_creating
 */
void Stairs::notify_creating() {

  Entity::notify_creating();
  update_dynamic_tiles();
}

/**
 * \brief Returns whether the subtype of these stairs is INSIDE_FLOOR.
 * \return \c true if the subtype if INSIDE_FLOOR.
 */
bool Stairs::is_inside_floor() const {
  return subtype == INSIDE_FLOOR;
}

/**
 * \brief Returns true if this entity does not react to the sword.
 *
 * If true is returned, nothing will happen when the hero taps this entity
 * with the sword.
 *
 * \return \c true if the sword is ignored.
 */
bool Stairs::is_sword_ignored() const {
  return true;
}

/**
 * \brief Returns whether this entity is an obstacle for another one
 * when it is enabled.
 * \param other another entity
 * \return true if this entity is an obstacle for the other one
 */
bool Stairs::is_obstacle_for(Entity& other) {

  return other.is_stairs_obstacle(*this);
}

/**
 * \brief This function is called when another entity overlaps this entity.
 * \param entity_overlapping The other entity.
 * \param collision_mode The collision mode that detected the collision.
 */
void Stairs::notify_collision(
    Entity& entity_overlapping, CollisionMode collision_mode) {

  if (is_enabled()) {
    entity_overlapping.notify_collision_with_stairs(*this, collision_mode);
  }
}

/**
 * \brief Returns the direction of the movement an entity would take when
 * activating these stairs.
 * \param way The way you intend to take these stairs.
 * \return The movement direction an entity should take on these stairs
 * (0 to 7).
 */
int Stairs::get_movement_direction(Way way) const {

  int movement_direction = get_direction() * 2;
  if (way == REVERSE_WAY) {
    movement_direction = (movement_direction + 4) % 8;
  }

  return movement_direction;
}

/**
 * \brief Returns the direction of the animation an entity should take when
 * walking these stairs.
 *
 * For spiral stairs, the direction returned is diagonal.
 *
 * \param way The way you are taking these stairs.
 * \return The direction of animation (0 to 7).
 */
int Stairs::get_animation_direction(Way way) const {

  int basic_direction = get_direction() * 2;
  int result = basic_direction;

  if (subtype == SPIRAL_UPSTAIRS) {
    result = (basic_direction == 2) ? 1 : 5;
  }
  else if (subtype == SPIRAL_DOWNSTAIRS) {
    result = (basic_direction == 2) ? 3 : 7;
  }

  if (way == REVERSE_WAY) {
    result = (result + 4) % 8;
  }

  return result;
}

/**
 * \brief Plays a sound corresponding to these stairs.
 *
 * When an entity collides with the stairs (usually the hero),
 * it can call this function to play the appropriate stairs sound.
 *
 * \param way The way you are taking these stairs.
 */
void Stairs::play_sound(Way way) const {

  std::string sound_id;
  if (is_inside_floor()) {
    // Choose the sound depending on whether we are going
    // upstairs or downstairs.
    sound_id = (way == NORMAL_WAY) ? "stairs_up_end" : "stairs_down_end";
  }
  else {
    // Choose the sound depending on whether we are on the old floor or the
    // new floor, and going upstairs or downstairs.
    if (subtype == SPIRAL_UPSTAIRS || subtype == STRAIGHT_UPSTAIRS) {
      sound_id = (way == NORMAL_WAY) ? "stairs_up_start" : "stairs_down_end";
    }
    else {
      sound_id = (way == NORMAL_WAY) ? "stairs_down_start" : "stairs_up_end";
    }
  }

  if (Sound::exists(sound_id)) {
    Sound::play(sound_id);
  }
}

/**
 * \brief Returns the path an entity should follow to take these stairs.
 *
 * When an entity collides with the stairs (usually the hero),
 * it can call this function to know the path it should take to make the
 * appropriate movement.
 *
 * \param way The way you are taking these stairs.
 * \return The corresponding path to make.
 */
std::string Stairs::get_path(Way way) const {

  // Determine the movement direction.
  int initial_direction = get_direction() * 2;
  std::string path = "";
  int nb_steps;
  if (is_inside_floor()) {
    nb_steps = 5;
  }
  else {
    nb_steps = (get_direction() == 1) ? 1 : 2;
  }
  for (int i = 0; i < nb_steps; i++) {
    path += '0' + initial_direction;
  }

  if (!is_inside_floor()) {

    // Second direction to take for each subtype of stairs
    // (assuming the direction is north).
    static const int second_directions[] = {
      0, 4, 2, 2
    };
    int second_direction = second_directions[subtype];
    if (get_direction() == 3) {  // Direction south.
      second_direction = (second_direction + 4) % 8;
    }
    char c = '0' + second_direction;
    path = path + c;
    if (subtype == SPIRAL_UPSTAIRS || subtype == SPIRAL_DOWNSTAIRS) {
      path = path + c;
    }
  }

  if (way == REVERSE_WAY) {
    std::string inverse_path = "";
    std::string::reverse_iterator it;
    for (it = path.rbegin(); it != path.rend(); ++it) {
      int direction = *it - '0';
      direction = (direction + 4) % 8;
      inverse_path += '0' + direction;
    }
    path = inverse_path;
  }

  return path;
}

/**
 * \brief Returns the subarea in which an entity tooking these stairs can be
 * displayed.
 * \param way The way you are taking these stairs.
 * \return The subarea of the map where the entity displaying should be
 * restricted to.
 */
Rectangle Stairs::get_clipping_rectangle(Way /* way */) const {

  if (subtype == INSIDE_FLOOR || subtype == STRAIGHT_UPSTAIRS) {
    return Rectangle(0, 0, 0, 0); // no restriction
  }

  Rectangle clipping_rectangle(0, 0, get_map().get_width(), get_map().get_height());
  bool north = get_direction() == 1; // north or south

  // Hide the hero after the north of south edge.
  if (north) {
    clipping_rectangle.set_y(get_top_left_y() - 8);
    clipping_rectangle.set_height(48);
  }
  else { // south
    clipping_rectangle.set_y(get_top_left_y() - 32);
    clipping_rectangle.set_height(48);
  }

  // Spiral staircase: also hide a side.
  if ((subtype == SPIRAL_DOWNSTAIRS && north)
      || (subtype == SPIRAL_UPSTAIRS && !north)) {
     // north downstairs or south upstairs: hide the west side
    clipping_rectangle.set_x(get_top_left_x());
    clipping_rectangle.set_width(32);
  }
  else if ((subtype == SPIRAL_UPSTAIRS && north)
      || (subtype == SPIRAL_DOWNSTAIRS && !north)) {
     // north downstairs or south upstairs: hide the east side
    clipping_rectangle.set_x(get_top_left_x() - 16);
    clipping_rectangle.set_width(32);
  }

  return clipping_rectangle;
}

/**
 * \brief Notifies this entity that it was just enabled or disabled.
 * \param enabled \c true if the entity is now enabled.
 *
 * All dynamic tiles whose prefix is "stairsname_enabled"
 * and "stairsame_disabled" will be updated depending on the stairs state
 * (where stairsname is the name of the stairs).
 */
void Stairs::notify_enabled(bool enabled) {

  Entity::notify_enabled(enabled);

  update_dynamic_tiles();
}

/**
 * \brief Enables or disables the dynamic tiles related to these stairs.
 *
 * The dynamic tiles impacted by this function are the ones whose prefix is
 * the stairs's name
 * followed by "_enabled" or "_disabled", depending on the stairs state.
 */
void Stairs::update_dynamic_tiles() {

  std::vector<EntityPtr> tiles = get_entities().get_entities_with_prefix(
      EntityType::DYNAMIC_TILE, get_name() + "_enabled");
  for (const EntityPtr& tile: tiles) {
    tile->set_enabled(is_enabled());
  }

  tiles = get_entities().get_entities_with_prefix(
      EntityType::DYNAMIC_TILE, get_name() + "_disabled");
  for (const EntityPtr& tile: tiles) {
    tile->set_enabled(!is_enabled());
  }
}

}

