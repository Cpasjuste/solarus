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
#include "solarus/core/System.h"
#include "solarus/entities/TimeScrollingTilePattern.h"
#include "solarus/entities/Tileset.h"
#include "solarus/graphics/Surface.h"

namespace Solarus {

int TimeScrollingTilePattern::shift = 0;
uint32_t TimeScrollingTilePattern::next_shift_date = 0;

/**
 * \brief Creates a tile pattern with scrolling.
 * \param ground Kind of ground of the tile pattern.
 * \param xy Coordinates of the tile pattern in the tileset.
 * \param size Size of the tile pattern in the tileset.
 */
TimeScrollingTilePattern::TimeScrollingTilePattern(Ground ground,
    const Point& xy, const Size& size):
  SimpleTilePattern(ground, xy, size) {

}

/**
 * \brief Initializes the time-scrolling tile pattern system.
 */
void TimeScrollingTilePattern::initialize() {
  shift = System::now();
  next_shift_date = System::now();
}

/**
 * \brief Cleans the animated time-scrolling tile pattern system.
 */
void TimeScrollingTilePattern::quit() {
  shift = 0;
  next_shift_date = 0;
}

/**
 * \brief Updates all scrolling tiles patterns.
 *
 * This function is called repeatedly by the map.
 */
void TimeScrollingTilePattern::update() {

  const uint32_t now = System::now();

  while (now >= next_shift_date) {
    shift++;
    next_shift_date += 50;
  }
}

/**
 * \brief Draws the tile image on a surface.
 * \param dst_surface the surface to draw
 * \param dst_position position where tile pattern should be drawn on dst_surface
 * \param tileset the tileset of this tile
 * \param viewport coordinates of the top-left corner of dst_surface relative
 * to the map (may be used for scrolling tiles)
 */
void TimeScrollingTilePattern::draw(
    const SurfacePtr& dst_surface,
    const Point& dst_position,
    const Tileset& tileset,
    const Point& /* viewport */
) const {
  Rectangle src = position_in_tileset;
  Point dst = dst_position;

  Point offset; // draw the tile with an offset that depends on the time

  offset.x = src.get_width() - (shift % src.get_width());
  offset.y = shift % src.get_height();

  src.add_x(offset.x);
  src.add_width(-offset.x);
  src.add_y(offset.y);
  src.add_height(-offset.y);
  tileset.get_tiles_image()->draw_region(src, dst_surface, dst);

  src = position_in_tileset;
  dst = dst_position;
  src.add_y(offset.y);
  src.add_height(-offset.y);
  dst.x += src.get_width() - offset.x;
  src.set_width(offset.x);
  tileset.get_tiles_image()->draw_region(src, dst_surface, dst);

  src = position_in_tileset;
  dst = dst_position;
  src.add_x(offset.x);
  src.add_width(-offset.x);
  dst.y += src.get_height() - offset.y;
  src.set_height(offset.y);
  tileset.get_tiles_image()->draw_region(src, dst_surface, dst);

  src = position_in_tileset;
  dst = dst_position;
  dst.x += src.get_width() - offset.x;
  src.set_width(offset.x);
  dst.y += src.get_height() - offset.y;
  src.set_height(offset.y);
  tileset.get_tiles_image()->draw_region(src, dst_surface, dst);
}

/**
 * \brief Returns whether this tile pattern is animated, i.e. not always displayed
 * the same way.
 *
 * Non-animated tiles may be rendered faster by using intermediate surfaces
 * that are drawn only once.
 *
 * \return true if this tile pattern is animated
 */
bool TimeScrollingTilePattern::is_animated() const {
  return true;
}

}

