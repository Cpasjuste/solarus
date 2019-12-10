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
#ifndef SOLARUS_EQUIPMENT_H
#define SOLARUS_EQUIPMENT_H

#include "solarus/core/Common.h"
#include "solarus/core/Ability.h"
#include "solarus/core/SavegamePtr.h"
#include <map>
#include <memory>
#include <string>

struct lua_State;

namespace Solarus {

class EquipmentItem;
class Game;
class Savegame;
class Map;
class Hero;
class Camera;

/**
 * \brief Represents the hero's equipment.
 *
 * This class gives access to the equipment data saved and the properties of
 * items.
 * You should call this class to get information about the current equipment
 * (sword, money, items...) and to modify it.
 */
class SOLARUS_API Equipment {

    friend class Hero;

  public:

    // creation and destruction
    explicit Equipment(const SavegamePtr& savegame, const std::string& prefix);

    // new equipement initialization
    void set_initial_values();

    Savegame& get_savegame();
    Game* get_game();
    void notify_game_started();
    void notify_game_finished();
    void notify_map_changed(Map& map, Camera &camera);

    void update();
    void set_suspended(bool suspended);

    // money
    int get_max_money() const;
    void set_max_money(int max_money);

    int get_money() const;
    void set_money(int money);
    void add_money(int money_to_add);
    void remove_money(int money_to_remove);

    // life
    int get_max_life() const;
    void set_max_life(int max_life);

    int get_life() const;
    void set_life(int life);
    void add_life(int life_to_add);
    void remove_life(int life_to_remove);
    void restore_all_life();

    // magic
    int get_max_magic() const;
    void set_max_magic(int max_magic);

    int get_magic() const;
    void set_magic(int magic);
    void add_magic(int magic_to_add);
    void remove_magic(int magic_to_remove);
    void restore_all_magic();

    // equipment items
    void load_items();
    bool item_exists(const std::string& item_name) const;
    EquipmentItem& get_item(const std::string& item_name);
    const EquipmentItem& get_item(const std::string& item_name) const;
    EquipmentItem* get_item_assigned(int slot);
    const EquipmentItem* get_item_assigned(int slot) const;
    void set_item_assigned(int slot, EquipmentItem* item);
    int get_item_slot(const EquipmentItem& item) const;

    // built-in abilities
    // TODO make notify_ability_changed
    bool has_ability(Ability ability, int level = 1) const;
    int get_ability(Ability ability) const;
    void set_ability(Ability ability, int level);
    void notify_ability_used(Ability ability);
    Hero* get_hero();
  private:

    void set_hero(Hero* hero);

    int get_integer(const std::string& key) const;
    void set_integer(const std::string& key, int value);
    std::string get_string(const std::string& key) const;
    void set_string(const std::string& key, const std::string& value);

    SavegamePtr savegame;                        /**< The savegame encapsulated by this equipment object. */ //TODO rename me
    bool suspended;                              /**< Indicates that the game is suspended. */

    // items
    std::map<std::string, std::shared_ptr<EquipmentItem>>
        items;                                   /**< Each item (properties loaded from item scripts). */

    std::string get_ability_savegame_variable(Ability ability) const;

    std::string prefix;                          /**< Prefix of this equipement item to access the savegame */

    Hero* hero;                                  /**< Hero owning this equipment. Optional */
};

using EquipmentPtr = std::shared_ptr<Equipment>;

}

#endif

