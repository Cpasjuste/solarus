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
#include "solarus/core/Equipment.h"
#include "solarus/core/EquipmentItem.h"
#include "solarus/core/Game.h"
#include "solarus/core/Map.h"
#include "solarus/core/QuestFiles.h"
#include "solarus/core/Savegame.h"
#include "solarus/core/System.h"
#include "solarus/entities/Chest.h"
#include "solarus/entities/Hero.h"
#include "solarus/graphics/Sprite.h"
#include "solarus/lua/LuaContext.h"
#include "solarus/lua/ScopedLuaRef.h"
#include <lua.hpp>

namespace Solarus {

/**
 * \brief Lua name of each value of the OpeningMethod enum.
 */
const std::map<Chest::OpeningMethod, std::string> Chest::opening_method_names = {
    { OpeningMethod::BY_INTERACTION, "interaction" },
    { OpeningMethod::BY_INTERACTION_IF_SAVEGAME_VARIABLE, "interaction_if_savegame_variable" },
    { OpeningMethod::BY_INTERACTION_IF_ITEM, "interaction_if_item" }
};

/**
 * \brief Creates a new chest with the specified treasure.
 * \param name Name identifying this chest.
 * \param layer Layer of the chest to create on the map.
 * \param xy Coordinates of the chest to create.
 * \param sprite_name Name of the animation set of the
 * sprite to create for the chest. It must have animations "open" and "closed".
 * \param treasure The treasure in the chest.
 */
Chest::Chest(
    const std::string& name,
    int layer,
    const Point& xy,
    const std::string& sprite_name,
    const Treasure& treasure):

  Entity(name, 0, layer, xy, Size(16, 16)),
  treasure(treasure),
  open(treasure.is_found()),
  treasure_given(open),
  treasure_date(0),
  opening_method(OpeningMethod::BY_INTERACTION),
  opening_condition_consumed(false) {

  set_collision_modes(CollisionMode::COLLISION_FACING);

  // Create the sprite.
  const SpritePtr& sprite = create_sprite(sprite_name);
  std::string animation = is_open() ? "open" : "closed";
  sprite->set_current_animation(animation);

  set_origin(get_width() / 2, get_height() - 3);

  // TODO set this as the default drawn_in_y_order for Entity
  set_drawn_in_y_order(sprite->get_max_size().height > get_height());
}

/**
 * \brief Returns the type of entity.
 * \return the type of entity
 */
EntityType Chest::get_type() const {
  return ThisType;
}

/**
 * \brief Notifies this entity that it was just enabled or disabled.
 * \param enabled \c true if the entity is now enabled.
 */
void Chest::notify_enabled(bool enabled) {

  Entity::notify_enabled(enabled);

  // Make sure the chest does not appear on the heroes
  for(const HeroPtr& hero: get_heroes()) {
    if (enabled && overlaps(*hero)) {
      hero->avoid_collision(*this, 3);
    }
  }
}

/**
 * \brief Returns the treasure of this chest.
 *
 * The treasure is returned even if the chest is open.
 *
 * \return The treasure. It may be empty or non obtainable.
 */
const Treasure& Chest::get_treasure() const {
  return treasure;
}

/**
 * \brief Sets the treasure of this chest.
 *
 * The treasure can be set even if the chest is open.
 * It will be placed inside the chest if the chest is closed again.
 *
 * \param treasure The treasure to set. It may be empty or non obtainable.
 */
void Chest::set_treasure(const Treasure& treasure) {
    this->treasure = treasure;
}

/**
 * \brief Returns whether the player has found the treasure in this chest.
 * \return true if the chest is open
 */
bool Chest::is_open() const {
  return open;
}

/**
 * \brief Sets whether the chest is open.
 *
 * If you don't change the chest state, this function has no effect.
 * If you make the chest open, its sprite is updated but this function does
 * not give any treasure to the player.
 * If you close the chest, its sprite is updated and the chest will contain
 * its initial treasure again.
 *
 * \param open true to open the chest, false to close it
 */
void Chest::set_open(bool open) {

  if (open != this->open) {

    this->open = open;

    const SpritePtr& sprite = get_sprite();
    if (open) {
      // open the chest
      if (sprite != nullptr) {
        sprite->set_current_animation("open");
      }
    }
    else {
      // close the chest
      if (sprite != nullptr) {
        sprite->set_current_animation("closed");
      }
      treasure_given = false;
    }
  }
}

/**
 * \brief Returns whether the player is able to open this chest now.
 * \return \c true if this the player can open the chest.
 */
bool Chest::can_open(Hero& hero) {

  switch (get_opening_method()) {

    case OpeningMethod::BY_INTERACTION:
      // No condition: the hero can always open the chest.
      return true;

    case OpeningMethod::BY_INTERACTION_IF_SAVEGAME_VARIABLE:
    {
      // The hero can open the chest if a savegame variable is set.
      const std::string& required_savegame_variable = get_opening_condition();
      if (required_savegame_variable.empty()) {
        return false;
      }

      Savegame& savegame = get_savegame();
      if (savegame.is_boolean(required_savegame_variable)) {
        return savegame.get_boolean(required_savegame_variable);
      }

      if (savegame.is_integer(required_savegame_variable)) {
        return savegame.get_integer(required_savegame_variable) > 0;
      }

      if (savegame.is_string(required_savegame_variable)) {
        return !savegame.get_string(required_savegame_variable).empty();
      }

      return false;
    }

    case OpeningMethod::BY_INTERACTION_IF_ITEM:
    {
      // The hero can open the chest if he has an item.
      const std::string& required_item_name = get_opening_condition();
      if (required_item_name.empty()) {
        return false;
      }
      const EquipmentItem& item = hero.get_equipment().get_item(required_item_name);
      return item.is_saved()
        && item.get_variant() > 0
        && (!item.has_amount() || item.get_amount() > 0);
    }

    default:
      return false;
  }
}

/**
 * \brief Returns the opening method of this chest.
 * \return How this chest can be opened.
 */
Chest::OpeningMethod Chest::get_opening_method() const {
  return opening_method;
}

/**
 * \brief Sets the opening method of this chest.
 * \param opening_method How this chest should be opened.
 */
void Chest::set_opening_method(OpeningMethod opening_method) {
  this->opening_method = opening_method;
}

/**
 * \brief Returns the savegame variable or the equipment item name required to
 * open this chest.
 *
 * A savegame variable is returned if the opening mode is
 * OPENING_BY_INTERACTION_IF_SAVEGAME_VARIABLE.
 * The hero is allowed to open the chest if this saved value is either
 * \c true, an integer greater than zero or a non-empty string.
 *
 * An equipment item's name is returned if the opening mode is
 * OPENING_BY_INTERACTION_IF_ITEM.
 * The hero is allowed to open the chest if he has that item and,
 * for items with an amount, if the amount is greater than zero.
 *
 * For the other opening methods, this setting has no effect.
 *
 * \return The savegame variable or the equipment item name required.
 */
const std::string& Chest::get_opening_condition() const {
  return opening_condition;
}

/**
 * \brief Sets the savegame variable or the equipment item name required to
 * open this chest.
 *
 * You must set a savegame variable if the opening mode is
 * OPENING_BY_INTERACTION_IF_SAVEGAME_VARIABLE.
 * The hero will be allowed to open the chest if this saved value is either
 * \c true, an integer greater than zero or a non-empty string.
 *
 * You must set an equipment item's name if the opening mode is
 * OPENING_BY_INTERACTION_IF_ITEM.
 * The hero will be allowed to open the chest if he has that item and,
 * for items with an amount, if the amount is greater than zero.
 *
 * For the other opening methods, this setting has no effect.
 *
 * \param opening_condition The savegame variable or the equipment item name
 * required.
 */
void Chest::set_opening_condition(const std::string& opening_condition) {
  this->opening_condition = opening_condition;
}

/**
 * \brief Returns whether opening this chest consumes the condition that
 * was required.
 *
 * If this setting is \c true, here is the behavior when the hero opens
 * the chest:
 * - If the opening method is OPENING_BY_INTERACTION_IF_SAVEGAME_VARIABLE,
 *   then the required savegame variable is either:
 *   - set to \c false if it is a boolean,
 *   - decremented if it is an integer,
 *   - set to an empty string if it is a string.
 * - If the opening method is OPENING_BY_INTERACTION_IF_ITEM, then:
 *   - if the required item has an amount, the amount is decremented.
 *   - if the required item has no amount, its possession state is set to zero.
 * - With other opening methods, this setting has no effect.
 *
 * \return \c true if opening this chest consumes the condition that was
 * required.
 */
bool Chest::is_opening_condition_consumed() const {
  return opening_condition_consumed;
}

/**
 * \brief Sets whether opening this chest should consume the condition that
 * was required.
 *
 * If this setting is \c true, here is the behavior when the hero opens
 * the chest:
 * - If the opening method is OPENING_BY_INTERACTION_IF_SAVEGAME_VARIABLE,
 *   then the required savegame variable is either:
 *   - set to \c false if it is a boolean,
 *   - decremented if it is an integer,
 *   - set to an empty string if it is a string.
 * - If the opening method is OPENING_BY_INTERACTION_IF_ITEM, then:
 *   - if the required item has an amount, the amount is decremented.
 *   - if the required item has no amount, its possession state is set to zero.
 * - With other opening methods, this setting has no effect.
 *
 * \param opening_condition_consumed \c true if opening this chest should
 * consume the condition that was required.
 */
void Chest::set_opening_condition_consumed(bool opening_condition_consumed) {
   this->opening_condition_consumed = opening_condition_consumed;
}

/**
 * \brief Returns the id of the dialog to show when the player presses the action
 * command on the chest but cannot open it (i.e. if can_open() is false).
 *
 * \return The id of the "cannot open" dialog for this chest
 * (an empty string means no dialog).
 */
const std::string& Chest::get_cannot_open_dialog_id() const {
  return cannot_open_dialog_id;
}

/**
 * \brief Sets the id of the dialog to show when the player presses the action
 * command on the chest but cannot open it (i.e. if can_open() is false).
 * \param cannot_open_dialog_id The id of the "cannot open" dialog for this chest
 * (an empty string means no dialog).
 */
void Chest::set_cannot_open_dialog_id(const std::string& cannot_open_dialog_id) {
  this->cannot_open_dialog_id = cannot_open_dialog_id;
}

/**
 * \brief Returns whether this entity is an obstacle for another one when
 * it is enabled.
 * \param other Another entity.
 * \return \c true if this entity is an obstacle for the other one.
 */
bool Chest::is_obstacle_for(Entity& /* other */) {
  return true;
}

/**
 * \brief This function is called by the engine when an entity overlaps the chest.
 * \param entity_overlapping the entity overlapping the detector
 * \param collision_mode the collision mode that detected the collision
 */
void Chest::notify_collision(Entity& entity_overlapping, CollisionMode /* collision_mode */) {

  if (!is_suspended()) {
    entity_overlapping.notify_collision_with_chest(*this);
  }
}

/**
 * \brief Updates the chest.
 *
 * This function is called repeatedly by the map.
 * This is a redefinition of Entity::update()
 * the handle the chest opening.
 */
void Chest::update() {

  if (is_open() && !is_suspended()) {

    if (!treasure_given && treasure_date != 0 && System::now_ms() >= treasure_date) {

      treasure_date = 0;
      treasure_given = true;

      if (treasure.is_saved()) {
        // Mark the chest as open in the savegame.
        get_savegame().set_boolean(treasure.get_savegame_variable(), true);
      }

      // Notify scripts.
      bool done = get_lua_context()->chest_on_opened(*this, treasure);
      if (!done) {
        if (treasure.is_empty() ||
            !treasure.is_obtainable()
        ) {
          // No treasure and the script does not define any behavior:
          // unfreeze the hero.
          opening_hero->start_free();
        }
        else {
          // Give the treasure to the player.
          opening_hero->start_treasure(treasure, ScopedLuaRef());
        }
      }
      opening_hero.reset();
    }
  }

  Entity::update();
}

/**
 * \copydoc Entity::notify_action_command_pressed
 */
bool Chest::notify_action_command_pressed(Hero &hero) {
  if (is_enabled() &&
      hero.is_free() &&
      hero.get_commands_effects().get_action_key_effect() != CommandsEffects::ACTION_KEY_NONE
  ) {

    if (can_open(hero)) {
      Sound::play("chest_open", get_game().get_resource_provider());

      set_open(true);
      treasure_date = System::now_ms() + 300;

      hero.get_commands_effects().set_action_key_effect(CommandsEffects::ACTION_KEY_NONE);
      hero.start_frozen();
      opening_hero = hero.shared_from_this_cast<Hero>();
    }
    else if (!get_cannot_open_dialog_id().empty()) {
      Sound::play("wrong", get_game().get_resource_provider());
      get_game().start_dialog(get_cannot_open_dialog_id(), ScopedLuaRef(), ScopedLuaRef());
    }

    return true;
  }

  return Entity::notify_action_command_pressed(hero);
}

/**
 * \brief This function is called by the map when the game is suspended or resumed.
 *
 * This is a redefinition of Entity::set_suspended() to suspend the timer
 * of the chest being opened.
 *
 * \param suspended true to suspend the entity, false to resume it
 */
void Chest::set_suspended(bool suspended) {

  Entity::set_suspended(suspended);

  if (!suspended && treasure_date != 0) {
    // restore the timer
    treasure_date = System::now_ms() + (treasure_date - get_when_suspended());
  }
}

}

