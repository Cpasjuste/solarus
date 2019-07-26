#pragma once

#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include "solarus/core/Rectangle.h"

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
  View(const glm::vec2& size);
  View(const Rectangle& rect);
  View(const glm::vec2& center, const glm::vec2& size);

  glm::vec2 get_center() const;
  glm::vec2 get_size() const;
  float get_rotation() const;

  void set_center(const glm::vec2& center);
  void set_size(const glm::vec2& size);
  void set_rotation(float rotation);

  void move(const glm::vec2& delta);
  void zoom(const glm::vec2& factor);
  void rotate(float rotation);

  void reset(const Rectangle& rect);

  const glm::mat4& get_transform() const;
  const glm::mat4& get_inverse_transform() const;

  void set_transform(const glm::mat4& transform);
private:
  inline void invalidate() {
    transform_dirty = true;
    inv_transform_dirty = true;
  }
  glm::vec2 center;                         /**< Center at which the view looks */
  glm::vec2 size;                           /**< Size the view covers */
  float rotation = 0;                       /**< Rotation of the view around the center */
  mutable glm::mat4 transform;              /**< Cached transform from the viewed space to target space */
  mutable glm::mat4 inv_transform;          /**< Cached transform from the target space to viewed space */
  mutable bool transform_dirty = true;      /**< Is the tranform up to date ? */
  mutable bool inv_transform_dirty = true;  /**< Is the inverse transform up to date */
};

}

