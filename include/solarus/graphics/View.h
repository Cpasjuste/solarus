/*
 * Copyright (C) 2018-2020 std::gregwar, Solarus - http://www.solarus-games.org
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
#pragma once

#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include "solarus/core/Rectangle.h"
#include "solarus/core/FRectangle.h"

namespace Solarus {

/**
 * @brief Helper class that represent a view
 *
 * Greatly inspired by SFML View class
 * https://github.com/SFML/SFML/blob/master/include/SFML/Graphics/View.hpp
 *
 *
 * Act as a proxy for glm::mat4
 */
class View
{
public:
  View(const glm::vec2& center);
  View(const Rectangle& rectangle);

  glm::vec2 get_center() const;
  //glm::vec2 get_size() const;
  float get_rotation() const;

  void set_center(const glm::vec2& center);
  //void set_size(const glm::vec2& size);
  void set_rotation(float rotation);

  void move(const glm::vec2& delta);
  void zoom(const glm::vec2& factor);
  void rotate(float rotation);

  void reset(const Rectangle& rect);

  const glm::mat4& get_transform() const;
  const glm::mat4& get_inverse_transform() const;

  void set_transform(const glm::mat4& transform);

  void set_viewport(const FRectangle& viewport);
  const FRectangle& get_viewport() const;
private:
  inline void invalidate() {
    transform_dirty = true;
    inv_transform_dirty = true;
  }
  glm::vec2 center = glm::vec2(0.f);        /**< Center at which the view looks */
  //glm::vec2 size;                         /**< Size the view covers */
  glm::vec2 scale = glm::vec2(1.f);          /**< Scaling of the view */
  float rotation = 0;                       /**< Rotation of the view around the center */
  FRectangle viewport;                      /**< Viewport of this view, in fraction of the */
  mutable glm::mat4 transform;              /**< Cached transform from the viewed space to target space */
  mutable glm::mat4 inv_transform;          /**< Cached transform from the target space to viewed space */
  mutable bool transform_dirty = true;      /**< Is the tranform up to date ? */
  mutable bool inv_transform_dirty = true;  /**< Is the inverse transform up to date */
};

}

