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
#include "solarus/core/Random.h"
#include "solarus/graphics/Surface.h"
#include <algorithm>
#include <iterator>
#include <set>

namespace Solarus {

/**
 * \brief Creates a quadtree with a default space size.
 *
 * Call initialize() later to specify the size.
 */
template<typename T, typename Comparator>
Quadtree<T, Comparator>::Quadtree() :
    Quadtree(Rectangle(0, 0, 256, 256)) {

}

/**
 * \brief Creates a quadtree and initializes with the given size.
 * \param space Rectangle representing the space to create partitions of.
 */
template<typename T, typename Comparator>
Quadtree<T, Comparator>::Quadtree(const Rectangle& space) :
  space(space)
{
    initialize(space);
}

/**
 * \brief Removes all elements of the quadtree.
 */
template<typename T, typename Comparator>
void Quadtree<T, Comparator>::clear() {
  elements_storage.clear();
  elements_nodes_storage.clear();
  nodes.clear();

  free_node = -1;
}

/**
 * \brief Clears the quadtree and initializes it with a new size.
 * \param space Rectangle representing the space to create partitions of.
 */
template<typename T, typename Comparator>
void Quadtree<T, Comparator>::initialize(const Rectangle& space) {

  clear();

  // Expand the space so that it is square.
  Rectangle square = space;
  if (space.get_width() > space.get_height()) {
    square.set_y(square.get_center().y - square.get_width() / 2);
    square.set_height(square.get_width());
  }
  else {
    square.set_x(square.get_center().x - square.get_height() / 2);
    square.set_width(square.get_height());
  }

  this->space = square;

  //Allocate root node
  nodes.resize(1);
}

/**
 * \brief Returns the space partitioned by this quadtree.
 * \return The partitioned space.
 */
template<typename T, typename Comparator>
Rectangle Quadtree<T, Comparator>::get_space() const {
    return space;
}

/**
 * \brief Adds an element to the quadtree.
 *
 * It is allowed to add it outside the space delimited by the quadtree,
 * for example if it can move inside later.
 *
 * \param element The element to add.
 * \param bounding_box Bounding box of the element.
 * \return \c true in case of success.
 */
template<typename T, typename Comparator>
bool Quadtree<T, Comparator>::add(const T& element, const Rectangle& bounding_box) {
  SOL_PBLOCK("Solarus::Quadtree::add");
  /**if (contains(element)) {
    // Element already in the quadtree.
    return false;
  }*/ //TODO take care of this later

  if (!bounding_box.overlaps(get_space())) {
    // Out of the space of the quadtree.
    return false;
  }
  else if (!root().add(*this, get_space(), element, bounding_box)) {
    // Add failed.
    return false;
  }

  return true;
}

/**
 * \brief Removes an element from the quadtree.
 * \param element The element to remove.
 * \return \c true in case of success.
 */
template<typename T, typename Comparator>
bool Quadtree<T, Comparator>::remove(const T& element, const Rectangle& previous_box) {
  SOL_PBLOCK("Solarus::Quadtree::remove");

  // Normal case.
  return root().remove(*this, get_space(), element, previous_box);
}

/**
 * \brief Moves the element in the quadtree.
 *
 * This function should be called when the position or size or the element
 * is changed.
 *
 * It is allowed for an element to go to or come from outside the space of the
 * quadtree.
 *
 * \param element The element to move. If the element is not in the quadtree,
 * does nothing and returns \c false.
 * \param bounding_box New bounding box of the element.
 * \return \c true in case of success.
 */
template<typename T, typename Comparator>
bool Quadtree<T, Comparator>::move(const T& element, const Rectangle& previous_box, const Rectangle& bounding_box) {
  SOL_PBLOCK("Solarus::Quadtree::move");

  if (!remove(element, previous_box)) {
    // Failed to remove.
    return false;
  }

  if (!add(element, bounding_box)) {
    // Failed to add.
    return false;
  }
  return true;
}

/**
 * \brief Returns the total number of elements in the quadtree.
 * \return The number of elements, including elements outside the quadtree
 * space.
 */
/*template<typename T, typename Comparator>
int Quadtree<T, Comparator>::get_num_elements() const {
  return root.get_num_elements() + elements_outside.size();
}*/

/**
 * \brief Gets the elements intersecting the given rectangle.
 * \param region The rectangle to check.
 * The rectangle should be entirely contained in the quadtree space.
 * \return A list of elements intersecting the rectangle,
 * in order of the comparator.
 * Elements outside the quadtree space are not added there.
 */
template<typename T, typename Comparator>
std::vector<T> Quadtree<T, Comparator>::get_elements(
    const Rectangle& region
) const {
  Set element_set;
  root().get_elements(*this, get_space(), region, element_set);
  return std::vector<T>(element_set.begin(), element_set.end());
}

/*template<typename T, typename Comparator>
template<typename C>
void Quadtree<T, Comparator>::raw_get_elements(
    const Rectangle& where,
    C& elements
    ) const {
  root.raw_get_elements(where, elements);
}*/

/**
 * \brief Returns whether an element is in the quadtree.
 * \param element The element to check.
 * \return \c true if it is in the quadtree, even if it is
 * outside the quadtree space.
 */
/*template<typename T, typename Comparator>
bool Quadtree<T, Comparator>::contains(const T& element) const {

  const auto& it = elements.find(element);
  return it != elements.end();
}*/

/**
 * \brief Draws the quadtree on a surface for debugging purposes.
 * \param dst_surface The destination surface.
 * \param dst_position Where to draw on that surface.
 */
template<typename T, typename Comparator>
void Quadtree<T, Comparator>::draw(const SurfacePtr& dst_surface, const Point& dst_position) {
  root().draw(*this, get_space(), dst_surface, dst_position);
}

/**
 * \brief Creates a node with default coordinates.
 *
 * Call initialize later to set coordinates.
 *
 * \param quadtree The quadtree this node belongs to.
 */
template<typename T, typename Comparator>
Quadtree<T, Comparator>::Node::Node() :
    first_child(-1), count(-1) {

}

template<typename T, typename Comparator>
template <typename F>
void Quadtree<T, Comparator>::Node::foreach_child(const Quadtree& tree, const Rectangle& cell, F fun) {
  assert(count < 0);
  int hw = cell.get_width() >> 1;
  int hh = cell.get_height() >> 1;
  int x = cell.get_left();
  int y = cell.get_top();
  fun(tree.nodes[first_child], {x,y,hw,hh});
  fun(tree.nodes[first_child+1], {x+hw, y, hw, hh});
  fun(tree.nodes[first_child+2], {x, y+hh, hw, hh});
  fun(tree.nodes[first_child+3], {x+hw, y+hh, hw, hh});
}

template<typename T, typename Comparator>
template <typename F>
void Quadtree<T, Comparator>::Node::foreach_element(const Quadtree& tree, F fun) {
  assert(is_leaf);
  int curr = first_child;
  while(curr != -1) {
    ElementNode& node = tree.elements_nodes_storage[curr];
    Element& el = tree.elements_storage[node.element];
    fun(curr, node, el);
    curr = node.next;
  }
}

/**
 * \brief Clears this node.
 *
 * Children nodes and their content are destroyed.
 */
template<typename T, typename Comparator>
void Quadtree<T, Comparator>::Node::clear(Quadtree& tree) {
  foreach_element(tree, [&](int nodeid, ElementNode& node, Element& el) {
    elements_nodes_storage.erase(nodeid);
  });
}

/**
 * \brief Adds an element to this node if its bounding box intersects it.
 *
 * Splits the node if necessary when the threshold is exceeded.
 *
 * \param element The element to add.
 * \param bounding_box Bounding box of the element.
 * \return \c true in case of success.
 */
template<typename T, typename Comparator>
bool Quadtree<T, Comparator>::Node::add(
    const T& element,
    const Rectangle& bounding_box
) {
  if (!get_cell().overlaps(bounding_box)) {
    // Nothing to do.
    return false;
  }

  bool main = is_main_cell(bounding_box);

  if (!is_split()) {

    // See if it is time to split.
    if (main) {
      // We are the main cell of this element: it counts in the total.
      if (get_num_elements() >= max_in_cell &&
          get_cell_size().width > min_cell_size &&
          get_cell_size().height > min_cell_size) {
        split();
      }
    }
  }

  // Count element if we are its main cell at this level
  if(main) {
    num_elements++;
  }

  if (!is_split()) {
    // Add it to the current node.
    elements.emplace_back(element, bounding_box);
    return true;
  }

  // Add it to children cells.
  for (const std::unique_ptr<Node>& child : children) {
    child->add(element, bounding_box);
  }
  return true;
}

/**
 * \brief Removes an element from this node if its bounding box intersects it.
 *
 * Merges nodes when necessary.
 *
 * \param element The element to remove.
 * \param bounding_box Bounding box of the element.
 * \return \c true in the element was found and removed.
 */
template<typename T, typename Comparator>
bool Quadtree<T, Comparator>::Node::remove(
    const T& element,
    const Rectangle& bounding_box
) {
  if (!get_cell().overlaps(bounding_box)) {
    // Nothing to do.
    return false;
  }

  if (!is_split()) {
    // Remove from this cell.
    const auto& it = std::find(elements.begin(), elements.end(), std::make_pair(element, bounding_box));
    if (it == elements.end()) {
      // The element was not here.
      return false;
    }
    elements.erase(it);
    if(is_main_cell(bounding_box)) {
      --num_elements;
    }
    return true;
  }

  // Remove from children cells.
  bool removed = false;
  for (const std::unique_ptr<Node>& child : children) {
    removed |= child->remove(element, bounding_box);
  }

  if(removed && is_main_cell(bounding_box)) {
    --num_elements;
  }

  if (removed &&
      !children[0]->is_split()  // We are the parent node of where the element was removed.
  ) {
    // See if it is time to merge.
    int num_elements_in_children = get_num_elements();
    if (num_elements_in_children < min_in_4_cells) {
      merge();
    }
  }
  return removed;
}

/**
 * \brief Returns whether this node is split or is a leaf cell.
 * \return \c true if the node is split.
 */
template<typename T, typename Comparator>
bool Quadtree<T, Comparator>::Node::is_split() const {

  return children[0] != nullptr;
}

/**
 * \brief Splits this cell in four parts and moves its elements to them.
 */
template<typename T, typename Comparator>
void Quadtree<T, Comparator>::Node::split() {
  Debug::check_assertion(!is_split(), "Quadtree node already split");

  // Create 4 children cells.
  const Rectangle& cell = get_cell();
  const Point& center = cell.get_center();
  children[0] = std::unique_ptr<Node>(
      new Node(quadtree, Rectangle(cell.get_top_left(), center))
  );
  children[1] = std::unique_ptr<Node>(
      new Node(quadtree, Rectangle(Point(center.x, cell.get_top()), Point(cell.get_right(), center.y)))
  );
  children[2] = std::unique_ptr<Node>(
      new Node(quadtree, Rectangle(Point(cell.get_left(), center.y), Point(center.x, cell.get_bottom())))
  );
  children[3] = std::unique_ptr<Node>(
      new Node(quadtree, Rectangle(center, cell.get_bottom_right()))
  );

  // Move existing elements into them.
  for (const std::pair<T, Rectangle>& pair : elements) {
    for (const std::unique_ptr<Node>& child : children) {
      child->add(pair.first, pair.second);
    }
  }
  elements.clear();

  Debug::check_assertion(is_split(), "Quadtree node split failed");
}

/**
 * \brief Merges the four children cell into this one and destroys them.
 *
 * The children must already be leaves.
 */
template<typename T, typename Comparator>
void Quadtree<T, Comparator>::Node::merge() {
  Debug::check_assertion(is_split(), "Quadtree node already merged");

  // We want to avoid duplicates while preserving a deterministic order.
  Set merged_elements;
  for (const std::unique_ptr<Node>& child : children) {
    Debug::check_assertion(!child->is_split(), "Quadtree node child is not a leaf");
    for (const std::pair<T, Rectangle>& pair : child->elements) {
      const T& element = pair.first;
      if (merged_elements.insert(element).second) {
        elements.push_back(pair);
      }
    }
  }

  std::fill(std::begin(children), std::end(children), nullptr);

  Debug::check_assertion(!is_split(), "Quadtree node merge failed");
}

/**
 * \brief Returns whether this cell contains a box and is also its main cell.
 *
 * The main cell is used to ensure uniqueness, for example when counting
 * elements.
 */
template<typename T, typename Comparator>
bool Quadtree<T, Comparator>::Node::is_main_cell(const Rectangle& bounding_box) const {

  if (!get_cell().overlaps(bounding_box)) {
    // Not overlapping this cell.
    return false;
  }

  // The bounding box is in this cell. See if this is the main cell.
  Point center = bounding_box.get_center();

  // Clamp the center to the quadtree space,
  // in case the center it actually outside.
  const Rectangle& quadtree_space = quadtree.get_space();
  center = {
      std::max(quadtree_space.get_left(), std::min(quadtree_space.get_right() - 1, center.x)),
      std::max(quadtree_space.get_top(), std::min(quadtree_space.get_bottom() - 1, center.y))
  };

  Debug::check_assertion(quadtree_space.contains(center), "Wrong center position");

  return get_cell().contains(center);
}

/**
 * \brief Returns the number of elements whose center is under this node.
 * \return The number of elements under this node.
 */
template<typename T, typename Comparator>
int Quadtree<T, Comparator>::Node::get_num_elements() const {
  #ifndef NDEBUG //Do the slow check when in debug
  SOL_PFUN("Solarus::Quadtree::Node::get_num_elements");
  size_t num_elements = 0;
  if (!is_split()) {
    // Some elements can overlap several cells.
    // To avoid duplicates, we count an element if this cell is its main cell.
    // TODO This information could be stored for better performance.
    for (const std::pair<T, Rectangle>& pair : elements) {
      const Rectangle& box = pair.second;
      if (is_main_cell(box)) {
        ++num_elements;
      }
    }
  }
  else {
    // Ask children.
    for (const std::unique_ptr<Node>& child : children) {
      num_elements += child->get_num_elements();
    }
  }
  //Check our fast num_elements match the original num_elements
  Debug::check_assertion(this->num_elements == num_elements, "Incorrect quadtree num_element invariant");
  #endif
  return num_elements;
}

/**
 * \brief Gets the elements intersecting the given rectangle under this node.
 * \param[in] region The rectangle to check.
 * \param[in/out] result A set that will be filled with elements.
 */
template<typename T, typename Comparator>
void Quadtree<T, Comparator>::Node::get_elements(
    const Rectangle& region,
    Set& result
) const {

  if (!get_cell().overlaps(region)) {
    // Nothing here.
    return;
  }

  if (!is_split()) {
    for (const std::pair<T, Rectangle>& pair : elements) {
      if (pair.second.overlaps(region)) {
        result.emplace(pair.first);
      }
    }
  }
  else {
    // Get from from children cells.
    for (const std::unique_ptr<Node>& child : children) {
      child->get_elements(region, result);
    }
  }
}

/**
 * \brief Gets the elements intersecting the given rectangle under this node.
 * \param[in] region The rectangle to check.
 * \param[in/out] result A set that will be filled with elements.
 */
template<typename T, typename Comparator>
template<typename C>
void Quadtree<T, Comparator>::Node::raw_get_elements(
    const Rectangle& region,
    C& result
) const {

  if (!get_cell().overlaps(region)) {
    // Nothing here.
    return;
  }

  if (!is_split()) {
    for (const std::pair<T, Rectangle>& pair : elements) {
      if (pair.second.overlaps(region)) {
        result.emplace_back(pair.first);
      }
    }
  }
  else {
    // Get from from children cells.
    for (const std::unique_ptr<Node>& child : children) {
      child->raw_get_elements(region, result);
    }
  }
}
/**
 * \brief Draws the node on a surface for debugging purposes.
 * \param dst_surface The destination surface.
 * \param dst_position Where to draw on that surface.
 */
template<typename T, typename Comparator>
void Quadtree<T, Comparator>::Node::draw(const SurfacePtr& dst_surface, const Point& dst_position) {

  if (!is_split()) {
    // Draw the rectangle of the node.
    draw_rectangle(get_cell(), color, dst_surface, dst_position);

    // Draw bounding boxes of elements.
    for (const std::pair<T, Rectangle>& pair : elements) {
      const Rectangle& bounding_box = pair.second;
      if (is_main_cell(bounding_box)) {
        draw_rectangle(bounding_box, color, dst_surface, dst_position);
      }
    }
  }
  else {
    // Draw children nodes.
    for (const std::unique_ptr<Node>& child : children) {
      child->draw(dst_surface, dst_position);
    }
  }
}

/**
 * \brief Draws the border of a rectangle on a surface for debugging purposes.
 * \param rectangle The rectangle to draw.
 * \param line_color The color to use.
 * \param dst_surface The destination surface.
 * \param dst_position Where to draw on that surface.
 */
template<typename T, typename Comparator>
void Quadtree<T, Comparator>::Node::draw_rectangle(
    const Rectangle& rectangle,
    const Color& line_color,
    const SurfacePtr& dst_surface,
    const Point& dst_position
) {
  // TODO remove this function when the draw line API is available

  Rectangle where = rectangle;
  where.set_xy(where.get_xy() + dst_position);
  dst_surface->fill_with_color(line_color, Rectangle(
      where.get_top_left(), Size(where.get_width(), 1)
  ));
  dst_surface->fill_with_color(line_color, Rectangle(
      where.get_bottom_left() + Point(0, -1), Size(where.get_width(), 1)
  ));
  dst_surface->fill_with_color(line_color, Rectangle(
      where.get_top_left(), Size(1, where.get_height())
  ));
  dst_surface->fill_with_color(line_color, Rectangle(
      where.get_top_right() + Point(-1, 0), Size(1, where.get_height())
  ));
}

}  // namespace Solarus
