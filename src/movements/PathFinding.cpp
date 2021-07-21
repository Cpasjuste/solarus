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
#include "solarus/core/Debug.h"
#include "solarus/core/Geometry.h"
#include "solarus/core/Map.h"
#include "solarus/entities/Entity.h"
#include "solarus/movements/PathFinding.h"
#include <limits>

namespace Solarus {

const Point PathFinding::neighbours_locations[] = {
  {  8,  0 },
  {  8, -8 },
  {  0, -8 },
  { -8, -8 },
  { -8,  0 },
  { -8,  8 },
  {  0,  8 },
  {  8,  8 }
};

const Rectangle PathFinding::transition_collision_boxes[] = {
  Rectangle(16,  0,  8, 16 ),
  Rectangle( 0, -8, 24, 24 ),
  Rectangle( 0, -8, 16,  8 ),
  Rectangle(-8, -8, 24, 24 ),
  Rectangle(-8,  0,  8, 16 ),
  Rectangle(-8,  0, 24, 24 ),
  Rectangle( 0, 16, 16,  8 ),
  Rectangle( 0,  0, 24, 24 )
};

/**
 * \brief Constructor.
 * \param map the map
 * \param source_entity the entity that will move from the starting point to the target
 * (its position must be aligned on the map grid)
 * \param target_entity the target entity (its size must be 16*16)
 */
PathFinding::PathFinding(
    Map& map,
    Entity& source_entity,
    Entity& target_entity):
  map(map),
  source_entity(source_entity),
  target_entity(target_entity) {

  SOLARUS_ASSERT(source_entity.is_aligned_to_grid(),
      "The source must be aligned on the map grid");
}

/**
 * \brief Tries to find a path between the source point and the target point.
 * \return the path found, or an empty string if no path was found
 * (because there is no path or the target is too far)
 */
std::string PathFinding::compute_path() {

  if (!target_entity.is_obstacle_for(source_entity)) {
    // No offset needed.
    return compute_path(Point());
  }

  // The target is not traversable: then try to compute a path to somewhere close.
  const std::vector<Point> offsets = {
      Point(target_entity.get_width(), 0),
      Point(0, -target_entity.get_height()),
      Point(-target_entity.get_width(), 0),
      Point(0, target_entity.get_height())
  };

  std::string best_path;
  size_t minimum_steps = std::numeric_limits<int>::max();
  for (const Point& offset : offsets) {
    std::string path = compute_path(offset);
    if (!path.empty() && path.size() < minimum_steps) {
      best_path = path;
      minimum_steps = path.size();
    }
  }

  return best_path;
}

/**
 * \brief Tries to find a path from the source point to the target point
 * plus an offset.
 * \param offset Translation to add to the target.
 * \return the path found, or an empty string if no path was found
 * (because there is no path or the target is too far)
 */
std::string PathFinding::compute_path(const Point& offset) {

  Point source = source_entity.get_bounding_box().get_xy();
  Point target = target_entity.get_bounding_box().get_xy() + offset;

  // std::cout << "will compute a path from " << source << " to " << target << std::endl;

  target.x += 4;
  target.x += -target.x % 8;
  target.y += 4;
  target.y += -target.y % 8;
  const int target_index = get_square_index(target);

  SOLARUS_ASSERT(target.x % 8 == 0 && target.y % 8 == 0,
      "Could not snap the target to the map grid");

  const int total_mdistance = Geometry::get_manhattan_distance(source, target);
  if (total_mdistance > 200 || target_entity.get_layer() != source_entity.get_layer()) {
    //std::cout << "too far, not computing a path\n";
    return ""; // too far to compute a path
  }

  std::string path = "";

  Node starting_node;
  const int index = get_square_index(source);
  starting_node.location = source;
  starting_node.index = index;
  starting_node.previous_cost = 0;
  starting_node.heuristic = total_mdistance;
  starting_node.total_cost = total_mdistance;
  starting_node.direction = ' ';
  starting_node.parent_index = -1;

  open_list.clear();
  closed_list.clear();
  open_list_indices.clear();

  open_list[index] = starting_node;
  open_list_indices.push_front(index);

  bool finished = false;
  while (!finished) {

    // pick the node with the lowest total cost in the open list
    const int index = open_list_indices.front();
    Node* current_node = &open_list[index];
    open_list_indices.pop_front();
    closed_list[index] = *current_node;
    open_list.erase(index);
    current_node = &closed_list[index];

    //std::cout << System::now() << " picking the lowest cost node in the open list ("
    //  << (open_list_indices.size() + 1) << " elements): "
    //  << current_node->location << ", index = " << current_node->index
    //  << ", cost = " << current_node->previous_cost << " + " << current_node->heuristic << "\n";

    if (index == target_index) {
      //std::cout << "target node was added to the closed list\n";
      finished = true;
      path = rebuild_path(current_node);
      //std::cout << "rebuild done\n";
    }
    else {
      // look at the accessible nodes from it
      //std::cout << System::now() << " looking for accessible states\n";
      for (int i = 0; i < 8; i++) {

        Node new_node;
        const int immediate_cost = (i & 1) ? 11 : 8;
        new_node.previous_cost = current_node->previous_cost + immediate_cost;
        new_node.location = current_node->location;
        new_node.location += neighbours_locations[i];
        new_node.index = get_square_index(new_node.location);
        //std::cout << "  node in direction " << i << ": index = " << new_node.index << std::endl;

        const bool in_closed_list = (closed_list.find(new_node.index) != closed_list.end());
        if (!in_closed_list && Geometry::get_manhattan_distance(new_node.location, target) < 200
            && is_node_transition_valid(*current_node, i)) {
          //std::cout << "  node in direction " << i << " is not in the closed list\n";
          // not in the closed list: look in the open list

          const bool in_open_list = open_list.find(new_node.index) != open_list.end();

          if (!in_open_list) {
            // not in the open list: add it
            new_node.heuristic = Geometry::get_manhattan_distance(new_node.location, target);
            new_node.total_cost = new_node.previous_cost + new_node.heuristic;
            new_node.parent_index = index;
            new_node.direction = '0' + i;
            open_list[new_node.index] = new_node;
            add_index_sorted(&open_list[new_node.index]);
            //std::cout << "  node in direction " << i << " is not in the open list, adding it with cost "
            //  << new_node.previous_cost << " (" << current_node->previous_cost << " + "
            //  << immediate_cost << ") + " << new_node.heuristic << " and parent " << new_node.parent_index << "\n";
          }
          else {
            //std::cout << "  node in direction " << i << " is already in the open list\n";
            Node* existing_node = &open_list[new_node.index];
            // already in the open list: see if the current path is better
            if (new_node.previous_cost < existing_node->previous_cost) {
              existing_node->previous_cost = new_node.previous_cost;
              existing_node->total_cost = existing_node->previous_cost + existing_node->heuristic;
              existing_node->parent_index = index;
              open_list_indices.sort();
            }
          }
        }
        else {
          //std::cout << "skipping node in direction " << i << ": already in closed list = "
          //  << in_closed_list << ", too far from target = "
          //  << (get_manhattan_distance(new_node.location, target) >= 200) << ", invalid transition = "
          //  << (!is_node_transition_valid(*current_node, i)) << std::endl;
        }
      }
      if (open_list_indices.empty()) {
        finished = true;
      }
    }
  }

  //std::cout << "path found: " << path << ", open nodes: " << open_list.size() << ", closed nodes: " << closed_list.size() << std::endl;
  return path;
}

/**
 * \brief Returns the index of the 8*8 square in the map
 * corresponding to the specified location.
 * \param location location of a node on the map
 * \return index of the square corresponding to the top-left part of the location
 */
int PathFinding::get_square_index(const Point& location) const {

  const int x8 = location.x / 8;
  const int y8 = location.y / 8;
  return y8 * map.get_width8() + x8;
}

/**
 * \brief Compares two nodes according to their total estimated cost.
 * \param other the other node
 */
bool PathFinding::Node::operator<(const Node& other) const {
  return total_cost < other.total_cost;
}

/**
 * \brief Adds the index of a node to the sorted list of indices of the open
 * list, making sure that the list remains sorted.
 * \param node The node.
 */
void PathFinding::add_index_sorted(Node* node) {

  bool inserted = false;
  for (auto it = open_list_indices.begin();
      it != open_list_indices.end() && !inserted;
      ++it) {
    const int index = *it;
    const Node* current_node = &open_list[index];
    if (current_node->total_cost >= node->total_cost) {
      open_list_indices.insert(it, node->index);
      inserted = true;
    }
  }

  if (!inserted) {
    open_list_indices.push_back(node->index);
  }
}

/**
 * \brief Builds the string representation of the path found by the algorithm.
 * \param closed_list The closed list of A*.
 * \return final_node The final node of the path.
 */
std::string PathFinding::rebuild_path(const Node* final_node) {

  const Node* current_node = final_node;
  std::string path = "";
  while (current_node->direction != ' ') {
    path = current_node->direction + path;
    current_node = &closed_list[current_node->parent_index];
    //std::cout << "current_node: " << current_node->index << ", path = " << path << std::endl;
  }
  return path;
}

/**
 * \brief Returns whether a transition between two nodes is valid, i.e.
 * whether there is no collision with the map.
 * \param initial_node the first node
 * \param direction the direction to take (0 to 7)
 * \return true if there is no collision for this transition
 */
bool PathFinding::is_node_transition_valid(
    const Node& initial_node, int direction) const {

  Rectangle collision_box = transition_collision_boxes[direction];
  collision_box.add_xy(initial_node.location);

  return !map.test_collision_with_obstacles(source_entity.get_layer(), collision_box, source_entity);
}

}

