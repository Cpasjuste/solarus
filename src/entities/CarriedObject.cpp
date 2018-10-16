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
#include "solarus/core/Game.h"
#include "solarus/core/Geometry.h"
#include "solarus/core/Map.h"
#include "solarus/core/System.h"
#include "solarus/entities/CarriedObject.h"
#include "solarus/entities/Crystal.h"
#include "solarus/entities/Destructible.h"
#include "solarus/entities/Hero.h"
#include "solarus/entities/Enemy.h"
#include "solarus/entities/Entities.h"
#include "solarus/entities/Explosion.h"
#include "solarus/entities/Npc.h"
#include "solarus/entities/Sensor.h"
#include "solarus/entities/Stairs.h"
#include "solarus/entities/Switch.h"
#include "solarus/graphics/Sprite.h"
#include "solarus/movements/PixelMovement.h"
#include "solarus/movements/RelativeMovement.h"
#include "solarus/movements/StraightMovement.h"
#include <memory>

namespace Solarus {

/**
 * \brief Movement of the item when the hero is lifting it.
 */
const std::string CarriedObject::lifting_trajectories[4] = {
    "0 0  0 0  -3 -3  -5 -3  -5 -2",
    "0 0  0 0  0 -1  0 -1  0 0",
    "0 0  0 0  3 -3  5 -3  5 -2",
    "0 0  0 0  0 -10  0 -12  0 0"
};

/**
 * \brief Creates a carried object (i.e. an item carried by the hero).
 * \param hero the hero who is lifting the item to be created
 * \param original_entity the entity that will be replaced by this carried object
 * (its coordinates, size and origin will be copied)
 * \param animation_set_id name of the animation set for the sprite to create
 * \param destruction_sound_id Name of the sound to play when this item is destroyed
 * (or an empty string).
 * \param damage_on_enemies damage received by an enemy if the item is thrown on him
 * (possibly 0)
 * \param explosion_date date of the explosion if the item should explode,
 * or 0 if the item does not explode
 */
CarriedObject::CarriedObject(
    Hero& hero,
    const Entity& original_entity,
    const std::string& animation_set_id,
    const std::string& destruction_sound_id,
    int damage_on_enemies,
    uint32_t explosion_date
):
  Entity("", 0, hero.get_layer(), Point(0, 0), Size(0, 0)),
  hero(std::static_pointer_cast<Hero>(hero.shared_from_this())),
  is_lifting(true),
  is_throwing(false),
  is_breaking(false),
  break_one_layer_above(false),
  destruction_sound_id(destruction_sound_id),
  damage_on_enemies(damage_on_enemies),
  shadow_sprite(nullptr),
  throwing_direction(0),
  next_down_date(0),
  item_height(0),
  y_increment(0),
  explosion_date(explosion_date) {

  // align correctly the item with the hero
  int direction = hero.get_animation_direction();
  if (direction % 2 == 0) {
    set_xy(original_entity.get_x(), hero.get_y());
  }
  else {
    set_xy(hero.get_x(), original_entity.get_y());
  }
  set_origin(original_entity.get_origin());
  set_size(original_entity.get_size());
  set_drawn_in_y_order(true);

  // create the lift movement and the sprite
  std::shared_ptr<PixelMovement> movement = std::make_shared<PixelMovement>(
      lifting_trajectories[direction], 100, false, true
  );
  main_sprite = create_sprite(animation_set_id, "main");
  main_sprite->enable_pixel_collisions();
  if (main_sprite->has_animation("stopped")) {
    main_sprite->set_current_animation("stopped");
  }
  set_default_sprite_name("main");
  set_movement(movement);

  // create the shadow (not visible yet)
  shadow_sprite = create_sprite("entities/shadow", "shadow");
  shadow_sprite->set_current_animation("big");
  shadow_sprite->stop_animation();
}

/**
 * \brief Returns the type of entity.
 * \return the type of entity
 */
EntityType CarriedObject::get_type() const {
  return ThisType;
}

/**
 * \brief Returns whether this entity is sensible to the ground below it.
 * \return \c true if this entity is sensible to its ground.
 */
bool CarriedObject::is_ground_observer() const {

  if (is_throwing) {
    return true;  // To make the item fall into holes, water, etc.
  }
  return false;
}

/**
 * \brief Returns the entity that is carrying this object.
 * \return The carrier.
 */
EntityPtr CarriedObject::get_carrier() const {
  return hero;
}

/**
 * \brief Returns the damage this item can cause to ennemies.
 * \return the damage on enemies
 */
int CarriedObject::get_damage_on_enemies() const {
  return damage_on_enemies;
}

/**
 * \brief Sets the damage this object causes to enemies when thrown at them.
 * \param damage_on_enemies The damage on enemies.
 */
void CarriedObject::set_damage_on_enemies(int damage_on_enemies) {
  this->damage_on_enemies = damage_on_enemies;
}

/**
 * \brief Returns the id of the sound to play when this object is destroyed.
 * \return The destruction sound id or an empty string.
 */
const std::string& CarriedObject::get_destruction_sound() const {
  return destruction_sound_id;
}

/**
 * \brief Sets the id of the sound to play when this object is destroyed.
 * \param destruction_sound_id The destruction sound id or an empty string.
 */
void CarriedObject::set_destruction_sound(const std::string& destruction_sound_id) {
  this->destruction_sound_id = destruction_sound_id;
}

/**
 * \brief Makes the item sprite stop moving.
 *
 * This function is called when the hero stops walking while carrying the item.
 * The item also stops moving.
 */
void CarriedObject::set_animation_stopped() {

  if (!is_lifting && !is_throwing) {
    std::string animation = will_explode_soon() ? "stopped_explosion_soon" : "stopped";
    if (main_sprite->has_animation("animation")) {
      main_sprite->set_current_animation(animation);
    }
  }
}

/**
 * \brief Makes the item sprite move.
 *
 * This function is called when the hero starts walking while carrying the item.
 * The item moves like him.
 */
void CarriedObject::set_animation_walking() {

  if (!is_lifting && !is_throwing) {
    std::string animation = will_explode_soon() ? "walking_explosion_soon" : "walking";
    if (main_sprite->has_animation("animation")) {
      main_sprite->set_current_animation(animation);
    }
  }
}

/**
 * \brief Throws the item.
 * \param direction direction where the hero throws the item (0 to 3)
 */
void CarriedObject::throw_item(int direction) {

  this->throwing_direction = direction;
  this->is_lifting = false;
  this->is_throwing = true;

  // play the sound
  Sound::play("throw");

  // Set up sprites.
  if (main_sprite->has_animation("animation")) {
    main_sprite->set_current_animation("stopped");
  }
  shadow_sprite->start_animation();

  // set the movement of the item sprite
  set_y(hero->get_y());
  std::shared_ptr<StraightMovement> movement =
      std::make_shared<StraightMovement>(false, false);
  movement->set_speed(200);
  movement->set_angle(Geometry::degrees_to_radians(direction * 90));
  clear_movement();
  set_movement(movement);

  this->y_increment = -2;
  this->next_down_date = System::now() + 40;
  this->item_height = 18;
}

/**
 * \brief Returns whether the item is being lifted.
 * \return true if the item is being lifted
 */
bool CarriedObject::is_being_lifted() const {
  return is_lifting;
}

/**
 * \brief Returns whether the item is being thrown.
 * \return true if the item is being thrown
 */
bool CarriedObject::is_being_thrown() const {
  return is_throwing;
}

/**
 * \brief Returns whether the item is about to explode.
 * \return true if the item is about to explode
 */
bool CarriedObject::will_explode_soon()  const{
  return can_explode() && System::now() >= explosion_date - 1500;
}

/**
 * \brief Destroys the item while it is being thrown.
 */
void CarriedObject::break_item() {

  if (is_throwing && throwing_direction != 3) {
    // destroy the item where it is actually drawn
    set_y(get_y() - item_height);
  }

  get_movement()->stop();
  shadow_sprite->stop_animation();

  if (!can_explode()) {
    if (!destruction_sound_id.empty()) {
      Sound::play(destruction_sound_id);
    }
    if (main_sprite->has_animation("destroy")) {
      main_sprite->set_current_animation("destroy");
    }
    else {
      remove_from_map();
    }
  }
  else {
    get_entities().add_entity(std::make_shared<Explosion>(
        "", get_layer(), get_xy(), true
    ));
    Sound::play("explosion");
    if (is_throwing) {
      remove_from_map(); // because if the item was still carried by the hero, then the hero class will destroy it
    }
  }
  is_throwing = false;
  is_breaking = true;
}

/**
 * \brief Destroys the item after it finishes its thrown movement.
 *
 * How the item breaks depends on the ground where it lands.
 */
void CarriedObject::break_item_on_ground() {

  get_movement()->stop();

  Ground ground = get_ground_below();
  switch (ground) {

    case Ground::EMPTY:
      // Nothing here: fall one layer below.
    {
      int layer = get_layer();
      if (layer == get_map().get_min_layer()) {
        // Cannot fall lower.
        break_item();
      }
      else {
        get_entities().set_entity_layer(*this, layer - 1);
        break_item_on_ground();  // Do this again on the next layer.
      }
      break;
    }

    case Ground::HOLE:
      Sound::play("jump");
      remove_from_map();
      break;

    case Ground::DEEP_WATER:
    case Ground::LAVA:
      Sound::play("walk_on_water");
      remove_from_map();
      break;

    default:
      // Break the item normally.
      break_item();
      break;
  }

  is_throwing = false;
  is_breaking = true;
}

/**
 * \brief Returns whether the item is broken.
 * \return true if the item is broken
 */
bool CarriedObject::is_broken() const {
  return is_breaking && (main_sprite->is_animation_finished() || can_explode());
}

/**
 * \brief Returns whether the item can explode.
 * \return true if the item will explode
 */
bool CarriedObject::can_explode() const {
  return explosion_date != 0;
}

/**
 * \brief This function is called by the map when the game is suspended or resumed.
 * \param suspended true to suspend the entity, false to resume it
 */
void CarriedObject::set_suspended(bool suspended) {

  Entity::set_suspended(suspended); // suspend the animation and the movement

  if (is_throwing) {
    // suspend the shadow
    shadow_sprite->set_suspended(suspended);
  }

  if (!suspended && get_when_suspended() != 0) {
    // recalculate the timers
    uint32_t diff = System::now() - get_when_suspended();
    if (is_throwing) {
      next_down_date += diff;
    }
    if (can_explode()) {
      explosion_date += diff;
    }
  }
}

/**
 * \brief This function is called repeatedly.
 */
void CarriedObject::update() {

  // update the sprite and the position
  Entity::update();

  if (is_suspended()) {
    return;
  }

  // when the hero finishes lifting the item, start carrying it
  if (is_lifting && get_movement()->is_finished()) {
    is_lifting = false;

    // make the item follow the hero
    clear_movement();
    set_movement(std::make_shared<RelativeMovement>(
        hero,
        0,
        -18,
        true
    ));
  }

  // when the item has finished flying, destroy it
  else if (can_explode() && !is_breaking) {

    uint32_t now = System::now();

    if (now >= explosion_date) {
      break_item();
    }
    else if (will_explode_soon()) {

      std::string animation = main_sprite->get_current_animation();
      if (animation == "stopped") {
        main_sprite->set_current_animation("stopped_explosion_soon");
      }
      else if (animation == "walking") {
        main_sprite->set_current_animation("walking_explosion_soon");
      }
    }
  }

  if (is_broken()) {
    remove_from_map();
  }
  else if (is_throwing) {

    if (break_one_layer_above) {
      break_item();
      int layer = get_layer();
      if (layer != get_map().get_max_layer()) {
        get_entities().set_entity_layer(*this, layer + 1);
      }
      break_one_layer_above = false;
    }
    else if (get_movement()->is_stopped() || y_increment >= 7) {
      // Interrupt the movement.
      break_item_on_ground();
    }
    else {
      uint32_t now = System::now();
      while (now >= next_down_date) {
        next_down_date += 40;
        item_height -= y_increment;
        y_increment++;
      }
    }
  }
}

/**
 * \brief Notifies this entity that it has just failed to change its position
 * because of obstacles.
 */
void CarriedObject::notify_obstacle_reached() {

  if (is_throwing && !is_broken()) {
    break_item();
  }
}

/**
 * \copydoc Entity::built_in_draw
 *
 * This is a redefinition to draw the shadow independently of the movement.
 */
void CarriedObject::built_in_draw(Camera& camera) {

  if (!is_throwing) {
    // Draw the sprite normally.
    Entity::built_in_draw(camera);
  }
  else {
    // When the item is being thrown, draw the shadow and the item separately.
    get_map().draw_visual(*shadow_sprite, get_xy());
    get_map().draw_visual(*main_sprite, get_x(), get_y() - item_height);
  }
}

/**
 * \copydoc Entity::notify_collision_with_enemy(Enemy&, Sprite&, Sprite&)
 */
void CarriedObject::notify_collision_with_enemy(
    Enemy& enemy,
    Sprite& /* this_sprite */,
    Sprite& enemy_sprite) {

  if (is_throwing
      && !can_explode()
      && get_damage_on_enemies() > 0) {
    enemy.try_hurt(EnemyAttack::THROWN_ITEM, *this, &enemy_sprite);
  }
}

/**
 * \copydoc Entity::notify_attacked_enemy
 */
void CarriedObject::notify_attacked_enemy(
    EnemyAttack /* attack */,
    Enemy& /* victim */,
    Sprite* /* victim_sprite */,
    const EnemyReaction::Reaction& result,
    bool /* killed */) {

  if (result.type != EnemyReaction::ReactionType::IGNORED &&
      result.type != EnemyReaction::ReactionType::LUA_CALLBACK) {
    break_item();
  }
}

/**
 * \brief Returns whether a teletransporter is currently considered as an obstacle for this entity.
 * \param teletransporter a teletransporter
 * \return true if the teletransporter is currently an obstacle for this entity
 */
bool CarriedObject::is_teletransporter_obstacle(Teletransporter& /* teletransporter */) {
  return false;
}

/**
 * \copydoc Entity::is_stream_obstacle
 */
bool CarriedObject::is_stream_obstacle(Stream& /* stream */) {
  return false;
}

/**
 * \brief Returns whether some stairs are currently considered as an obstacle for this entity.
 * \param stairs an stairs entity
 * \return true if the stairs are currently an obstacle for this entity
 */
bool CarriedObject::is_stairs_obstacle(Stairs& /* stairs */) {
  return false;
}

/**
 * \brief Returns whether a low wall is currently considered as an obstacle
 * by this entity.
 * \return \c true if low walls are currently obstacle for this entity.
 */
bool CarriedObject::is_low_wall_obstacle() const {
  return false;
}

/**
 * \brief Returns whether a deep water tile is currently considered as an obstacle for this entity.
 * \return true if the deep water tiles are currently an obstacle for this entity
 */
bool CarriedObject::is_deep_water_obstacle() const {
  return false;
}

/**
 * \brief Returns whether a hole is currently considered as an obstacle for this entity.
 * \return true if the holes are currently an obstacle for this entity
 */
bool CarriedObject::is_hole_obstacle() const {
  return false;
}

/**
 * \brief Returns whether lava is currently considered as an obstacle for this entity.
 * \return true if lava is currently an obstacle for this entity
 */
bool CarriedObject::is_lava_obstacle() const {
  return false;
}

/**
 * \brief Returns whether prickles are currently considered as an obstacle for this entity.
 * \return true if prickles are currently an obstacle for this entity
 */
bool CarriedObject::is_prickle_obstacle() const {
  return false;
}

/**
 * \brief Returns whether a ladder is currently considered as an obstacle for this entity.
 * \return true if the ladders are currently an obstacle for this entity
 */
bool CarriedObject::is_ladder_obstacle() const {
  return false;
}

/**
 * \brief Returns whether a switch is currently considered as an obstacle by this entity.
 * \param sw a switch
 * \return true if the switch is currently an obstacle for this entity
 */
bool CarriedObject::is_switch_obstacle(Switch& /* sw */) {
  return !is_being_thrown();
}

/**
 * \brief Returns whether a raised crystal block is currently considered as an obstacle for this entity.
 * \param raised_block a crystal block raised
 * \return false
 */
bool CarriedObject::is_raised_block_obstacle(CrystalBlock& /* raised_block */) {
  // thrown items can traverse crystal blocks
  return false;
}

/**
 * \brief Returns whether a crystal is currently considered as an obstacle for this entity.
 * \param crystal a crystal
 * \return true if the crystal is currently an obstacle for this entity
 */
bool CarriedObject::is_crystal_obstacle(Crystal& /* crystal */) {
  return !is_being_thrown();
}

/**
 * \brief Returns whether a non-playing character is currently considered as an obstacle for this entity.
 * \param npc a non-playing character
 * \return true if the NPC is currently an obstacle for this entity
 */
bool CarriedObject::is_npc_obstacle(Npc& npc) {
  return npc.is_solid();
}

/**
 * \copydoc Entity::is_jumper_obstacle
 */
bool CarriedObject::is_jumper_obstacle(Jumper& /* jumper */, const Rectangle& /* candidate_position */) {
  return false;
}

/**
 * \brief Returns whether a sensor is currently considered as an obstacle for this entity.
 * \param sensor a sensor
 * \return true if this sensor is currently an obstacle for this entity.
 */
bool CarriedObject::is_sensor_obstacle(Sensor& /* sensor */) {
  return false;
}

/**
 * \brief Returns whether an enemy character is considered as an obstacle for this entity.
 * \param enemy an enemy
 * \return true if this enemy is considered as an obstacle for this entity.
 */
bool CarriedObject::is_enemy_obstacle(Enemy& /* enemy */) {
  // if this item explodes when reaching an obstacle, then we consider enemies as obstacles
  return can_explode();
}

/**
 * \brief This function is called when a switch detects a collision with this entity.
 * \param sw the switch
 * \param collision_mode the collision mode that detected the event
 */
void CarriedObject::notify_collision_with_switch(Switch& sw, CollisionMode collision_mode) {

  if (collision_mode == COLLISION_OVERLAPPING
      && is_being_thrown()
      && !can_explode()) {

    sw.try_activate();
    break_item();
  }
}

/**
 * \brief This function is called when a crystal detects a collision with this entity.
 * \param crystal the crystal
 * \param collision_mode the collision mode that detected the event
 */
void CarriedObject::notify_collision_with_crystal(Crystal& crystal, CollisionMode collision_mode) {

  if (collision_mode == COLLISION_OVERLAPPING
      && is_being_thrown()
      && !can_explode()) {

    crystal.activate(*this);
    break_item();
  }
}

/**
 * \brief This function is called when some stairs detect a collision with this entity.
 * \param stairs the stairs entity
 * \param collision_mode the collision mode that detected the event
 */
void CarriedObject::notify_collision_with_stairs(Stairs& stairs, CollisionMode /* collision_mode */) {

  if (is_throwing
      && !is_breaking
      && stairs.is_inside_floor()
      && get_layer() == stairs.get_layer()) {
    break_one_layer_above = true; // show the destruction animation above the stairs
  }
}

const std::string EnumInfoTraits<CarriedObject::Behavior>::pretty_name = "carried object behavior";

/**
 * \brief Lua name of each value of the CarriedObject::Behavior enum.
 */
const EnumInfo<CarriedObject::Behavior>::names_type EnumInfoTraits<CarriedObject::Behavior>::names = {
  { CarriedObject::Behavior::THROW, "throw" },
  { CarriedObject::Behavior::REMOVE, "remove" },
  { CarriedObject::Behavior::KEEP, "keep" }
};

}

