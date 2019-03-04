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
#include "solarus/core/EquipmentItem.h"
#include "solarus/core/FontResource.h"
#include "solarus/core/Game.h"
#include "solarus/core/Map.h"
#include "solarus/core/QuestFiles.h"
#include "solarus/core/Savegame.h"
#include "solarus/entities/Hero.h"
#include "solarus/entities/ShopTreasure.h"
#include "solarus/graphics/Sprite.h"
#include "solarus/graphics/TextSurface.h"
#include "solarus/lua/LuaContext.h"
#include <sstream>

namespace Solarus {

/**
 * \brief Creates a new shop treasure with the specified treasure and price.
 * \param name The name identifying this entity.
 * \param layer Layer of the entity to create.
 * \param xy Coordinates of the entity to create.
 * \param treasure The treasure that the hero can buy.
 * \param price The treasure's price.
 * \param font_id Id of the font to use to display the price or an empty
 * string for the default font.
 * \param dialog_id Id of the dialog describing the treasure when the hero
 * watches it.
 */
ShopTreasure::ShopTreasure(
    const std::string& name,
    int layer,
    const Point& xy,
    const Treasure& treasure,
    int price,
    const std::string& font_id,
    const std::string& dialog_id):
  Entity(name, 0, layer, xy, Size(32, 32)),
  treasure(treasure),
  price(price),
  dialog_id(dialog_id),
  treasure_sprite(treasure.create_sprite()),
  rupee_icon_sprite(std::make_shared<Sprite>("entities/rupee_icon")),
  price_digits(0, 0, TextSurface::HorizontalAlignment::LEFT, TextSurface::VerticalAlignment::TOP) {

  set_collision_modes(CollisionMode::COLLISION_FACING);

  std::ostringstream oss;
  oss << price;
  price_digits.set_text(oss.str());
  if (FontResource::exists(font_id)) {
    price_digits.set_font(font_id);
  }
}

/**
 * \brief Creates a new shop treasure with the specified treasure and price.
 * \param game The current game.
 * \param name The name identifying this entity.
 * \param layer Layer of the entity to create.
 * \param xy Coordinates of the entity to create.
 * \param treasure The treasure that the hero can buy.
 * \param price The treasure's price.
 * \param font_id Id of the font to use to display the price.
 * \param dialog_id Id of the dialog describing the treasure when the hero watches it.
 * \return The shop treasure created, or nullptr if it is already bought or if it
 * is not obtainable.
 */
std::shared_ptr<ShopTreasure> ShopTreasure::create(
    Game& /* game */,
    const std::string& name,
    int layer,
    const Point& xy,
    const Treasure& treasure,
    int price,
    const std::string& font_id,
    const std::string& dialog_id
) {
  // See if the item is not already bought and is obtainable.
  if (treasure.is_found() || !treasure.is_obtainable()) {
    return nullptr;
  }

  return std::make_shared<ShopTreasure>(
      name, layer, xy, treasure, price, font_id, dialog_id
  );
}

/**
 * \brief Returns the type of entity.
 * \return the type of entity
 */
EntityType ShopTreasure::get_type() const {
  return ThisType;
}

/**
 * \brief Returns the treasure for sale in this entity.
 * \return The treasure.
 */
const Treasure& ShopTreasure::get_treasure() const {
  return treasure;
}

/**
 * \brief Returns the price of this shop item.
 * \return The price.
 */
int ShopTreasure::get_price() const {
  return price;
}

/**
 * \brief Returns the id of the dialog describing this shop item when the
 * player watches it.
 * \return The dialog id.
 */
const std::string& ShopTreasure::get_dialog_id() const {
  return dialog_id;
}

/**
 * \brief Returns true if this entity does not react to the sword.
 *
 * If true is returned, nothing will happen when the hero taps this entity with the sword.
 *
 * \return true if the sword is ignored
 */
bool ShopTreasure::is_sword_ignored() const {
  return true;
}

/**
 * \brief Returns whether this entity is an obstacle for another one.
 * \param other another entity
 * \return true
 */
bool ShopTreasure::is_obstacle_for(Entity& /* other */) {
  return true;
}

/**
 * \brief This function is called by the engine when an entity overlaps the shop item.
 *
 * If the entity is the hero, we allow him to buy the item.
 *
 * \param entity_overlapping the entity overlapping the detector
 * \param collision_mode the collision mode that detected the collision
 */
void ShopTreasure::notify_collision(
    Entity& entity_overlapping, CollisionMode /* collision_mode */) {

  if (entity_overlapping.is_hero() && !get_game().is_suspended()) {

    Hero& hero = static_cast<Hero&>(entity_overlapping);

    if (get_commands_effects().get_action_key_effect() == CommandsEffects::ACTION_KEY_NONE
        && hero.is_free()) {

      // we show the 'look' icon
      get_commands_effects().set_action_key_effect(CommandsEffects::ACTION_KEY_LOOK);
    }
  }
}

/**
 * \copydoc Entity::notify_action_command_pressed
 */
bool ShopTreasure::notify_action_command_pressed() {

  if (get_hero().is_free()
      && get_commands_effects().get_action_key_effect() == CommandsEffects::ACTION_KEY_LOOK) {

    get_lua_context()->notify_shop_treasure_interaction(*this);
    return true;
  }

  return Entity::notify_action_command_pressed();
}

/**
 * \copydoc Entity::update
 */
void ShopTreasure::update() {

  Entity::update();

  treasure_sprite->update();
  rupee_icon_sprite->update();
}

/**
 * \copydoc Entity::built_in_draw
 */
void ShopTreasure::built_in_draw(Camera& camera) {

  const SurfacePtr& camera_surface = camera.get_surface();
  int x = get_x();
  int y = get_y();

  // draw the treasure
  treasure_sprite->draw(camera_surface,
      x + 16 - camera.get_top_left_x(),
      y + 13 - camera.get_top_left_y()
  );

  // also draw the price
  price_digits.draw(camera_surface,
      x + 12 - camera.get_top_left_x(),
      y + 21 - camera.get_top_left_y());
  rupee_icon_sprite->draw(camera_surface,
      x - camera.get_top_left_x(),
      y + 22 - camera.get_top_left_y());

  Entity::built_in_draw(camera);
}

}

