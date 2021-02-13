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
#include "solarus/core/Equipment.h"
#include "solarus/core/Game.h"
#include "solarus/core/Map.h"
#include "solarus/core/QuestFiles.h"
#include "solarus/core/Treasure.h"
#include "solarus/core/System.h"
#include "solarus/entities/CarriedObject.h"
#include "solarus/entities/Destructible.h"
#include "solarus/entities/Entities.h"
#include "solarus/entities/Explosion.h"
#include "solarus/entities/Hero.h"
#include "solarus/entities/Pickable.h"
#include "solarus/graphics/Sprite.h"
#include "solarus/hero/HeroSprites.h"
#include "solarus/lua/LuaContext.h"
#include "solarus/movements/FallingHeight.h"
#include <lauxlib.h>
#include <memory>

namespace Solarus {

const std::string EnumInfoTraits<Destructible::CutMethod>::pretty_name = "cut method";

const EnumInfo<Destructible::CutMethod>::names_type EnumInfoTraits<Destructible::CutMethod>::names = {
  { Destructible::CutMethod::ALIGNED, "aligned" },
  { Destructible::CutMethod::PIXEL, "pixel" },
};

/**
 * \brief Creates a new destructible item with the specified subtype.
 * \param name Name identifying the entity on the map or an empty string.
 * \param layer Layer where to create the entity.
 * \param xy Coordinates where to create the entity.
 * \param animation_set_id Sprite animation set id for this destructible object.
 * \param treasure The treasure contained in this object.
 * \param modified_ground Ground defined by this entity (usually Ground::WALL).
 */
Destructible::Destructible(
    const std::string& name,
    int layer,
    const Point& xy,
    const std::string& animation_set_id,
    const Treasure& treasure,
    Ground modified_ground
):
  Entity(name, 0, layer, xy, Size(16, 16)),
  modified_ground(modified_ground),
  treasure(treasure),
  animation_set_id(animation_set_id),
  destruction_sound_id(),
  can_be_cut(false),
  cut_method(CutMethod::ALIGNED),
  can_explode(false),
  can_regenerate(false),
  damage_on_enemies(1),
  is_being_cut(false),
  regeneration_date(0),
  is_regenerating(false) {

  set_origin(8, 13);
  create_sprite(get_animation_set_id());
  set_weight(0);

  update_collision_modes();
}

/**
 * \brief Returns the type of entity.
 * \return the type of entity
 */
EntityType Destructible::get_type() const {
  return ThisType;
}

/**
 * \brief When is_ground_modifier() is \c true, returns the ground defined
 * by this entity.
 * \return The ground defined by this entity.
 */
Ground Destructible::get_modified_ground() const {

  if (is_waiting_for_regeneration() || is_being_cut) {
    return Ground::EMPTY;
  }

  return modified_ground;
}

/**
 * \brief Returns the pickable treasure contained by this object.
 * \return The pickable treasure.
 */
const Treasure& Destructible::get_treasure() const {
  return treasure;
}

/**
 * \brief Sets the pickable treasure contained by this object.
 * \param treasure The pickable treasure.
 */
void Destructible::set_treasure(const Treasure& treasure) {
  this->treasure = treasure;
}

/**
 * \brief Returns the animation set of this destructible object.
 * \return The animations of the sprite.
 */
const std::string& Destructible::get_animation_set_id() const {
  return animation_set_id;
}

/**
 * \brief Returns the id of the sound to play when this object is destroyed.
 * \return The destruction sound id or an empty string.
 */
const std::string& Destructible::get_destruction_sound() const {
  return destruction_sound_id;
}

/**
 * \brief Sets the id of the sound to play when this object is destroyed.
 * \param destruction_sound_id The destruction sound id or an empty string.
 */
void Destructible::set_destruction_sound(const std::string& destruction_sound_id) {
  this->destruction_sound_id = destruction_sound_id;
}

/**
 * \brief Returns whether this object can be cut with the sword.
 * \return \c true if this object can be cut with the sword.
 */
bool Destructible::get_can_be_cut() const {
  return can_be_cut;
}

/**
 * \brief Sets whether this object can be cut with the sword.
 * \param can_be_cut \c true if this object can be cut with the sword.
 */
void Destructible::set_can_be_cut(bool can_be_cut) {

  this->can_be_cut = can_be_cut;
  update_collision_modes();
}

/**
 * \brief Returns how this object can be cut by the hero's sword.
 * \return The cut method.
 */
Destructible::CutMethod Destructible::get_cut_method() const {
  return cut_method;
}

/**
 * \brief Sets how this object can be cut by the hero's sword.
 * \param cut_method The cut method.
 */
void Destructible::set_cut_method(CutMethod cut_method) {
  this->cut_method = cut_method;
}

/**
 * \brief Returns whether this object explodes after a delay when the hero
 * lifts it.
 * \return \c true if this object can explode.
 */
bool Destructible::get_can_explode() const {
  return can_explode;
}

/**
 * \brief Sets whether this object explodes after a delay when the hero
 * lifts it.
 * \param can_explode \c true if this object can explode.
 */
void Destructible::set_can_explode(bool can_explode) {

  this->can_explode = can_explode;
  update_collision_modes();
}

/**
 * \brief Returns whether this object regenerates after a delay when it is
 * destroyed.
 * \return \c true if this object can regenerate.
 */
bool Destructible::get_can_regenerate() const {
  return can_regenerate;
}

/**
 * \brief Sets whether this object regenerates after a delay when it is
 * destroyed.
 * \param can_regenerate \c true if this object can regenerate.
 */
void Destructible::set_can_regenerate(bool can_regenerate) {
  this->can_regenerate = can_regenerate;
}

/**
 * \brief Returns the damage this object causes to enemies when thrown at them.
 * \return The damage on enemies.
 */
int Destructible::get_damage_on_enemies() const {
  return damage_on_enemies;
}

/**
 * \brief Sets the damage this object causes to enemies when thrown at them.
 * \param damage_on_enemies The damage on enemies.
 */
void Destructible::set_damage_on_enemies(int damage_on_enemies) {
  this->damage_on_enemies = damage_on_enemies;
}

/**
 * \brief Returns whether this entity is an obstacle for another one.
 * \param other Another entity.
 * \return \c true if this entity is an obstacle for the other one.
 */
bool Destructible::is_obstacle_for(Entity& other) {

  return get_modified_ground() == Ground::WALL &&
      !is_being_cut &&
      !is_waiting_for_regeneration() &&
      other.is_destructible_obstacle(*this);
}

/**
 * \brief Sets the appropriate collisions modes.
 *
 * This depends on whether the object is an obstacle, can be cut
 * or can explode.
 */
void Destructible::update_collision_modes() {

  // Reset previous collision modes.
  set_collision_modes(0);

  // Sets the new ones.
  if (get_modified_ground() == Ground::WALL) {
    // The object is an obstacle.
    // Set the facing collision mode to allow the hero to look at it.
    add_collision_mode(CollisionMode::COLLISION_FACING);
  }

  if (get_can_be_cut()
      || get_can_explode()) {
    add_collision_mode(CollisionMode::COLLISION_SPRITE);
  }
}

/**
 * \brief Tests whether an entity's collides with this entity.
 *
 * This custom collision test is used for destructible items that change the ground drawn under the hero.
 *
 * \param entity an entity
 * \return true if the entity's collides with this entity
 */
bool Destructible::test_collision_custom(Entity& entity) {
  return overlaps(entity.get_x(), entity.get_y() - 2);
}

/**
 * \brief Adds to the map the pickable treasure (if any) hidden under this destructible item.
 */
void Destructible::create_treasure() {

  get_entities().add_entity(Pickable::create(
      get_game(),
      "",
      get_layer(),
      get_xy(),
      treasure,
      FALLING_MEDIUM,
      false
  ));
}

/**
 * \brief This function is called by the engine when an entity overlaps the destructible item.
 *
 * If the entity is the hero, we allow him to lift the item.
 *
 * \param entity_overlapping the entity overlapping the detector
 * \param collision_mode the collision mode that detected the collision
 */
void Destructible::notify_collision(
    Entity& entity_overlapping, CollisionMode collision_mode) {

  entity_overlapping.notify_collision_with_destructible(*this, collision_mode);
}

/**
 * \brief This function is called when this entity detects a collision with the hero.
 * \param hero the hero
 * \param collision_mode the collision mode that detected the collision
 */
void Destructible::notify_collision_with_hero(Hero& hero, CollisionMode /* collision_mode */) {

  if (get_weight() != -1
      && !is_being_cut
      && !is_waiting_for_regeneration()
      && !is_regenerating
      && get_commands_effects().get_action_key_effect() == CommandsEffects::ACTION_KEY_NONE
      && hero.is_free()) {

    if (!get_equipment().has_ability(Ability::LIFT, get_weight())) {
      get_commands_effects().set_action_key_effect(CommandsEffects::ACTION_KEY_LOOK);
    }
  }
}

/**
 * \copydoc Entity::notify_collision(Entity&, Sprite&, Sprite&)
 */
void Destructible::notify_collision(
    Entity& other_entity,
    Sprite& /* this_sprite */,
    Sprite& other_sprite
) {
  if (get_can_be_cut()
      && !is_being_cut
      && !is_waiting_for_regeneration()
      && !is_regenerating
      && other_entity.is_hero()) {

    Hero& hero = static_cast<Hero&>(other_entity);
    if (other_sprite.get_animation_set_id() == hero.get_hero_sprites().get_sword_sprite_id() &&
        hero.is_cutting_with_sword(*this)) {

      play_destroy_animation();
      hero.check_position();  // To update the ground under the hero.
      create_treasure();

      get_lua_context()->destructible_on_cut(*this);

      if (get_can_explode()) {
        explode();
      }
    }
  }

  // TODO use dynamic dispatch
  if (other_entity.get_type() == EntityType::EXPLOSION
      && get_can_explode()
      && !is_being_cut
      && !is_waiting_for_regeneration()
      && !is_regenerating) {

    play_destroy_animation();
    create_treasure();
    explode();
  }
}

/**
 * \copydoc Entity::notify_action_command_pressed
 */
bool Destructible::notify_action_command_pressed() {

  CommandsEffects::ActionKeyEffect effect = get_commands_effects().get_action_key_effect();

  if ((effect == CommandsEffects::ACTION_KEY_LIFT || effect == CommandsEffects::ACTION_KEY_LOOK)
      && get_weight() != -1
      && !is_being_cut
      && !is_waiting_for_regeneration()
      && !is_regenerating) {

    if (get_equipment().has_ability(Ability::LIFT, get_weight())) {

      uint32_t explosion_date = get_can_explode() ? System::now() + 6000 : 0;
      std::shared_ptr<CarriedObject> carried_object = std::make_shared<CarriedObject>(
          get_hero(),
          *this,
          get_animation_set_id(),
          get_destruction_sound(),
          get_damage_on_enemies(),
          explosion_date
      );
      get_hero().start_lifting(carried_object);

      // Play the sound.
      Sound::play("lift", get_game().get_resource_provider());

      // Create the pickable treasure.
      create_treasure();

      if (!get_can_regenerate()) {
        // Remove this destructible from the map.
        remove_from_map();
      }
      else {
        // The item can actually regenerate.
        play_destroy_animation();
      }

      // Notify Lua.
      get_lua_context()->entity_on_lifting(*this, get_hero(), *carried_object);
    }
    else {
      // Cannot lift the object.
      if (get_hero().can_grab()) {
        get_hero().start_grabbing();
      }
      get_lua_context()->destructible_on_looked(*this);
    }

    return true;
  }

  return false;
}

/**
 * \brief Plays the animation destroy of this item.
 */
void Destructible::play_destroy_animation() {

  is_being_cut = true;
  if (!destruction_sound_id.empty()) {
    Sound::play(destruction_sound_id, get_game().get_resource_provider());
  }
  const SpritePtr& sprite = get_sprite();
  if (sprite != nullptr) {
    sprite->set_current_animation("destroy");
  }
  if (!is_drawn_in_y_order()) {
    get_entities().bring_to_front(*this);  // Show animation destroy to front.
  }
  if (is_ground_modifier()) {
    update_ground_observers();  // The ground has just disappeared.
  }
}

/**
 * \brief Returns whether the object was lifted and will regenerate.
 * \return \c true if the item is disabled and will regenerate.
 */
bool Destructible::is_waiting_for_regeneration() const {
  return regeneration_date != 0 && !is_regenerating;
}

/**
 * \brief Creates an explosion on this object.
 */
void Destructible::explode() {

  get_entities().add_entity(std::make_shared<Explosion>(
      "", get_layer(), get_xy(), true
  ));
  Sound::play("explosion", get_game().get_resource_provider());
  get_lua_context()->destructible_on_exploded(*this);
}

/**
 * \brief This function is called by the map when the game is suspended or resumed.
 * \param suspended true to suspend the entity, false to resume it
 */
void Destructible::set_suspended(bool suspended) {

  Entity::set_suspended(suspended); // suspend the animation and the movement

  if (!suspended && regeneration_date != 0) {
    // Recompute the date.
    regeneration_date += System::now() - get_when_suspended();
  }
}

/**
 * \brief Updates this entity.
 */
void Destructible::update() {

  Entity::update();

  if (is_suspended()) {
    return;
  }

  const SpritePtr& sprite = get_sprite();

  if (is_being_cut &&
      sprite != nullptr &&
      sprite->is_animation_finished()) {

    if (!get_can_regenerate()) {
      // Remove this destructible from the map.
      remove_from_map();
    }
    else {
      is_being_cut = false;
      regeneration_date = System::now() + 10000;
    }
  }

  else if (is_waiting_for_regeneration()
      && System::now() >= regeneration_date
      && !overlaps(get_hero())) {

    if (sprite != nullptr) {
      sprite->set_current_animation("regenerating");
    }
    is_regenerating = true;
    regeneration_date = 0;
    get_lua_context()->destructible_on_regenerating(*this);
  }
  else if (is_regenerating &&
      sprite != nullptr &&
      sprite->is_animation_finished()) {

    sprite->set_current_animation("on_ground");
    is_regenerating = false;
  }
}

}

