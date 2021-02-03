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
#ifndef SOLARUS_QUADTREE_H
#define SOLARUS_QUADTREE_H

#include "solarus/core/Common.h"
#include "solarus/core/Rectangle.h"
#include "solarus/core/Size.h"
#include "solarus/core/Profiler.h"
#include "solarus/graphics/Color.h"
#include "solarus/graphics/SurfacePtr.h"
#include "FreeList.h"
#include <array>
#include <map>
#include <memory>
#include <set>
#include <vector>

namespace Solarus {

class Color;

/**
 * \brief A collection of objects spatially located in an adaptable 2D grid.
 *
 * The main goal of this container is to get objects in a given rectangle as
 * quickly as possible.
 *
 * \param T Type of objects.
 */
template <typename T, typename Comparator = std::less<T>>
class Quadtree {

  public:

    using Set = std::set<T, Comparator>;

    Quadtree();
    explicit Quadtree(const Rectangle& space);

    void clear();
    void initialize(const Rectangle& space);

    Rectangle get_space() const;

    bool add(const T& element, const Rectangle& bounding_box);
    bool remove(const T& element, const Rectangle& previous_box);
    bool move(const T& element, const Rectangle& previous_box, const Rectangle& bounding_box);

    std::vector<T> get_elements(
        const Rectangle& where
    ) const;

    /*template<typename C>
    void raw_get_elements(
        const Rectangle& where,
        C& elements
    ) const;*/

    //int get_num_elements() const;
    bool contains(const T& element) const;

    void draw(const SurfacePtr& dst_surface, const Point& dst_position);

    static constexpr int
        min_cell_size = 16;  /**< Don't split more if a cell is smaller than
                              * this size. */
    static constexpr int
        max_in_cell = 8;     /**< A cell is split if it exceeds this number
                              * when adding an element, unless the cell is
                              * too small. */
    static constexpr int
        min_in_4_cells = 4;  /**< 4 sibling cells are merged if their total
                              * is below this number when removing an element. */

    static constexpr bool debug_quadtrees = false;

  private:

    class Node {

      public:
        explicit Node(const Quadtree& quadtree);

        void clear();
        void initialize(const Rectangle& cell);

        //Rectangle get_cell() const;
        //Size get_cell_size() const;

        bool add(
            Quadtree& quadtree,
            const Rectangle& cell_rect,
            const T& element,
            const Rectangle& bounding_box
        );
        bool remove(
            Quadtree& quadtree,
            const Rectangle& cell_rect,
            const T& element,
            const Rectangle& bounding_box
        );

        void get_elements(
            const Quadtree& quadtree,
            const Rectangle& cell_rect,
            const Rectangle& region,
            Set& result
        ) const;

        /*template<typename C>
        void raw_get_elements(
            const QuadTree&
            const Rectangle& region,
            C& result
        ) const;*/

        //int get_num_elements() const;

        void draw(
            const Quadtree& quadtree,
            const Rectangle& cell_rect,
            const SurfacePtr& dst_surface, const Point& dst_position
        );
        void draw_rectangle(
            const Rectangle& rectangle,
            const Color& line_color,
            const SurfacePtr& dst_surface,
            const Point& dst_position
        );

      private:

        bool is_split() const;
        void split();
        void merge();
        bool is_main_cell(const Rectangle& bounding_box) const;

        /**const Quadtree& quadtree;
        std::vector<std::pair<T, Rectangle>> elements;
        std::array<std::unique_ptr<Node>, 4> children;
        size_t num_elements;
        Rectangle cell;
        Point center;
        Color color;*/
        int32_t first_child = -1; /**< first child of the node, or first element if it is a leaf */
        int32_t count = 0; /**< count of elements in the leaf or -1 if a branch */
    };

    int allocate_node();
    void add_element_to_list(int head);
    Node& root();
    const Node& root() const;

    /**
     * @brief
     */
    struct Element{
        T data; /**< Data held by the quadtree */
        Rectangle rect; /**< Extent of this element */
    };

    /**
     * @brief The QuadElementNode struct
     */
    struct ElementNode {
        int next;
        int element;
    };

    struct ElementInfo {
        Rectangle bounding_box;
    };

    FreeList<Element> elements_storage; /**< contiguous storage for elements */
    FreeList<ElementNode> elements_nodes_storage; /**< contiguous storage for element nodes */

    std::vector<Node> nodes;

    int free_node = -1; /**< first free group of four node or -1 if no free nodes */
    Rectangle space;
};

template<typename T>
using QuadtreeNode = typename Quadtree<T>::Node;

}

#include "solarus/containers/Quadtree.inl"

#endif

