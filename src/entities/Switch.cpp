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
#include "solarus/entities/Arrow.h"
#include "solarus/entities/Block.h"
#include "solarus/entities/Hero.h"
#include "solarus/entities/Switch.h"
#include "solarus/core/Debug.h"
#include "solarus/core/Game.h"
#include "solarus/core/Map.h"
#include "solarus/core/QuestFiles.h"
#include "solarus/graphics/Sprite.h"
#include "solarus/lua/LuaContext.h"

namespace Solarus {

const std::map<Switch::Subtype, std::string> Switch::subtype_names = {
    { Subtype::WALKABLE, "walkable" },
    { Subtype::ARROW_TARGET, "arrow_target" },
    { Subtype::SOLID, "solid" }
};

/**
 * \brief Constructor.
 * \param name Name of the entity.
 * \param layer Layer of the entity.
 * \param xy Coordinates the entity.
 * \param subtype The subtype of switch.
 * \param sprite_name Sprite animation set id to use, or an empty string.
 * \param sound_id Sound to play when activating the switch,
 * or an empty string.
 * \param needs_block \c true if a block is required to activate this switch.
 * \param inactivate_when_leaving \c true to inactivate the switch when the
 * hero or the block leaves it.
 */
Switch::Switch(
    const std::string& name,
    int layer,
    const Point& xy,
    Subtype subtype,
    const std::string& sprite_name,
    const std::string& sound_id,
    bool needs_block,
    bool inactivate_when_leaving):
  Entity(name, 0, layer, xy, Size(16, 16)),
  subtype(subtype),
  sound_id(sound_id),
  activated(false),
  locked(false),
  needs_block(needs_block),
  inactivate_when_leaving(inactivate_when_leaving),
  entity_overlapping(nullptr),
  entity_overlapping_still_present(false) {

  // Sprite.
  if (!sprite_name.empty()) {
    const SpritePtr& sprite = create_sprite(sprite_name);
    sprite->set_current_animation("inactivated");
  }

  // Collisions.
  if (is_walkable()) {
    set_collision_modes(CollisionMode::COLLISION_CUSTOM);
  }
  else if (subtype == Subtype::ARROW_TARGET) {
    set_collision_modes(CollisionMode::COLLISION_FACING);
  }
  else if (subtype == Subtype::SOLID) {
    set_collision_modes(CollisionMode::COLLISION_SPRITE | CollisionMode::COLLISION_OVERLAPPING);
  }
}

/**
 * \brief Returns the type of entity.
 * \return the type of entity
 */
EntityType Switch::get_type() const {
  return ThisType;
}

/**
 * \brief Returns whether this entity is an obstacle for another one when
 * it is enabled.
 * \param other another entity
 * \return true if this entity is an obstacle for the other one
 */
bool Switch::is_obstacle_for(Entity& other) {
  return other.is_switch_obstacle(*this);
}

/**
 * \brief Returns whether this switch is a walkable switch.
 * \return \c true if the subtype of this switch is WALKABLE.
 */
bool Switch::is_walkable() const {
  return subtype == Subtype::WALKABLE;
}

/**
 * \brief Returns whether this switch is an arrow target.
 * \return \c true if the subtype of this switch is ARROW_TARGET.
 */
bool Switch::is_arrow_target() const {
  return subtype == Subtype::ARROW_TARGET;
}

/**
 * \brief Returns whether this switch is a solid switch.
 * \return \c true if the subtype of this switch is SOLID.
 */
bool Switch::is_solid() const {
  return subtype == Subtype::SOLID;
}

/**
 * \brief Returns whether this switch is currently activated.
 *
 * \return true if the switch is activated
 */
bool Switch::is_activated() const {
  return activated;
}

/**
 * \brief Activates the switch, playing a sound and notifying the map script.
 *
 * This function does nothing if the switch is locked or already activated.
 */
void Switch::activate(Entity* opt_entity) {

  if (!activated && !locked) {

    set_activated(true);

    if (!sound_id.empty()) {
      Sound::play(sound_id);
    }

    get_lua_context()->switch_on_activated(*this, opt_entity);
  }
}

/**
 * \brief Activates or inactivates the switch, not playing any sound.
 *
 * This function can change the switch state even if the switch is locked.
 *
 * \param activated true to make the switch on, false to make it off
 */
void Switch::set_activated(bool activated) {

  if (activated != this->activated) {
    this->activated = activated;

    const SpritePtr& sprite = get_sprite();
    if (sprite != nullptr) {
      if (activated) {
        sprite->set_current_animation("activated");
      }
      else {
        sprite->set_current_animation("inactivated");
      }
    }
  }
}

/**
 * \brief Returns whether this switch is locked its current state.
 *
 * When the switch is locked, it cannot be activated or deactivated
 * by other entities.
 * However, the state can still be changed manually by calling
 * set_activated().
 *
 * \return \c true if the switch is locked.
 */
bool Switch::is_locked() const {
  return locked;
}

/**
 * \brief Locks this switch is its current state or unlocks it.
 *
 * When the switch is locked, it cannot be activated or deactivated
 * by other entities.
 * However, the state can still be changed manually by calling
 * set_activated().
 *
 * \param locked \c true to lock the switch in its current state,
 * \c false to unlock it.
 */
void Switch::set_locked(bool locked) {
  this->locked = locked;
}

/**
 * \brief Updates this switch.
 */
void Switch::update() {

  Entity::update();

  if (is_enabled() &&
      is_walkable() &&
      entity_overlapping != nullptr) {

    // if an entity was on the switch, see if it is still there
    entity_overlapping_still_present = false;
    check_collision(*entity_overlapping);

    if (!entity_overlapping_still_present) {
      // the entity just left the switch or disappeared from the map
      // (it may even have been freed)
      Entity* old_overlap = entity_overlapping;
      entity_overlapping = nullptr;
      if (is_activated() && inactivate_when_leaving && !locked) {
        set_activated(false);
        get_lua_context()->switch_on_inactivated(*this, old_overlap);
      }
      get_lua_context()->switch_on_left(*this, *old_overlap);
    }
  }
}

/**
 * \brief Tests whether an entity's collides with this entity.
 * \param entity an entity
 * \return true if the entity's collides with this entity
 */
bool Switch::test_collision_custom(Entity& entity) {

  // This collision test is performed for walkable switches only.

  const Rectangle& entity_rectangle = entity.get_bounding_box();
  int x1 = entity_rectangle.get_x() + 4;
  int x2 = x1 + entity_rectangle.get_width() - 9;
  int y1 = entity_rectangle.get_y() + 4;
  int y2 = y1 + entity_rectangle.get_height() - 9;

  return overlaps(x1, y1) && overlaps(x2, y1) &&
    overlaps(x1, y2) && overlaps(x2, y2);
}

/**
 * \brief This function is called by the engine when an entity overlaps the switch.
 * \param entity_overlapping the entity overlapping the detector
 * \param collision_mode the collision mode that detected the collision
 */
void Switch::notify_collision(
    Entity& entity_overlapping, CollisionMode collision_mode) {

  if (&entity_overlapping == this->entity_overlapping) {
    // already overlapping
    entity_overlapping_still_present = true;
    return;
  }

  if (!locked) {
    entity_overlapping.notify_collision_with_switch(*this, collision_mode);
  }
}

/**
 * \copydoc Entity::notify_collision(Entity&, Sprite&, Sprite&)
 */
void Switch::notify_collision(
    Entity& other_entity,
    Sprite& /* this_sprite */,
    Sprite& other_sprite
) {
  if (!locked) {
    other_entity.notify_collision_with_switch(*this, other_sprite);
  }
}

/**
 * \brief This function is called when the hero overlaps this switch.
 *
 * The switch is activated if its properties allow it.
 *
 * \param hero the hero
 */
void Switch::try_activate(Hero& hero) {

  if (is_walkable() &&
      !needs_block &&
      !is_activated()) {
    // this switch allows the hero to activate it
    activate(&hero);
  }
  this->entity_overlapping = &hero;
}

/**
 * \brief This function is called when a block overlaps this switch.
 *
 * The switch is activated if its properties allow it.
 *
 * \param block the block overlapping this switch
 */
void Switch::try_activate(Block& block) {

  if (is_walkable() &&
      !is_activated()) {
    // blocks can only activate walkable, visible switches
    activate(&block);
  }
  this->entity_overlapping = &block;
}

/**
 * \brief This function is called when an arrow overlaps this switch.
 *
 * The switch is activated if its properties allow it.
 *
 * \param arrow the arrow overlapping this switch
 */
void Switch::try_activate(Arrow& arrow) {

  if ((subtype == Subtype::ARROW_TARGET || subtype == Subtype::SOLID)
      && !is_activated()) {
    // arrows can only activate arrow targets and solid switches
    activate(&arrow);
  }
}

/**
 * \brief This function is called when any entity overlaps this switch.
 *
 * Only solid switches can be activated this way.
 * The switch is activated if its properties allow it.
 */
void Switch::try_activate() {

  // arbitrary entities can activate solid switches
  if (subtype == Subtype::SOLID && !is_activated()) {
    activate(nullptr);
  }
}

}

