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
#include "solarus/core/Map.h"
#include "solarus/entities/Camera.h"
#include "solarus/entities/Entities.h"
#include "solarus/entities/NonAnimatedRegions.h"
#include "solarus/entities/Tileset.h"
#include "solarus/graphics/Surface.h"

namespace Solarus {

/**
 * \brief Constructor.
 * \param map The map. Its size must be known.
 * \param layer The layer to represent.
 */
NonAnimatedRegions::NonAnimatedRegions(Map& map, int layer):
  map(map),
  layer(layer),
  non_animated_tiles(map.get_size(), Size(512, 256)) {

}

/**
 * \brief Adds a tile to the list of tiles.
 */
void NonAnimatedRegions::add_tile(const TileInfo& tile) {

  Debug::check_assertion(are_squares_animated.empty(),
      "Tile regions are already built");
  Debug::check_assertion(tile.layer == layer, "Wrong layer for add tile");

  tiles.push_back(tile);
}

/**
 * \brief Determines which rectangles are animated to allow drawing all non-animated
 * rectangles of tiles only once.
 * \param[out] rejected_tiles The list of tiles that are in animated regions.
 * They include all animated tiles plus the static tiles overlapping them.
 * You will have to redraw these tiles at each frame.
 */
void NonAnimatedRegions::build(std::vector<TileInfo>& rejected_tiles) {

  Debug::check_assertion(are_squares_animated.empty(),
      "Tile regions are already built");

  const int map_width8 = map.get_width8();
  const int map_height8 = map.get_height8();

  // Initialize the are_squares_animated booleans to false.
  for (int i = 0; i < map_width8 * map_height8; ++i) {
    are_squares_animated.push_back(false);
  }

  // Mark animated 8x8 squares of the map.
  for (size_t i = 0; i < tiles.size(); ++i) {
    const TileInfo& tile = tiles[i];
    if (tile.pattern->is_animated()) {
      // Animated tile: mark its region as non-optimizable
      // (otherwise, a non-animated tile above an animated one would screw us).

      int tile_x8 = tile.box.get_x() / 8;
      int tile_y8 = tile.box.get_y() / 8;
      int tile_width8 = tile.box.get_width() / 8;
      int tile_height8 = tile.box.get_height() / 8;

      for (int i = 0; i < tile_height8; i++) {
        for (int j = 0; j < tile_width8; j++) {

          const int x8 = tile_x8 + j;
          const int y8 = tile_y8 + i;
          if (x8 >= 0 && x8 < map_width8 && y8 >= 0 && y8 < map_height8) {
            int index = y8 * map_width8 + x8;
            are_squares_animated[index] = true;
          }
        }
      }
    }
  }

  // Build the list of animated tiles and tiles overlapping them.
  for (const TileInfo& tile: tiles) {
    if (!tile.pattern->is_animated()) {
      non_animated_tiles.add(tile, tile.box);
      if (overlaps_animated_tile(tile)) {
        rejected_tiles.push_back(tile);
      }
    }
    else {
      rejected_tiles.push_back(tile);
    }
  }

  // No need to keep all tiles at this point.
  // Just keep the non-animated ones to draw them lazily.
  tiles.clear();
}

/**
 * \brief Clears previous drawings because the tileset has changed.
 */
void NonAnimatedRegions::notify_tileset_changed() {

  optimized_tiles_surfaces.clear();
  // Everything will be redrawn when necessary.
}

/**
 * \brief Returns whether a tile is overlapping an animated other tile.
 * \param tile The tile to check.
 * \return \c true if this tile is overlapping an animated tile.
 */
bool NonAnimatedRegions::overlaps_animated_tile(const TileInfo& tile) const {

  if (tile.layer != layer) {
    return false;
  }

  int tile_x8 = tile.box.get_x() / 8;
  int tile_y8 = tile.box.get_y() / 8;
  int tile_width8 = tile.box.get_width() / 8;
  int tile_height8 = tile.box.get_height() / 8;

  for (int i = 0; i < tile_height8; i++) {
    for (int j = 0; j < tile_width8; j++) {

      int x8 = tile_x8 + j;
      int y8 = tile_y8 + i;
      if (x8 >= 0 && x8 < map.get_width8() && y8 >= 0 && y8 < map.get_height8()) {

        int index = y8 * map.get_width8() + x8;
        if (are_squares_animated[index]) {
          return true;
        }
      }
    }
  }
  return false;
}

/**
 * \brief Called at each frame of the main loop.
 */
void NonAnimatedRegions::update() {

  // Limit the size of the cache to avoid growing the memory usage.
  if (optimized_tiles_surfaces.size() < 25) {
    return;
  }

  std::vector<int> indexes_to_clear;
  const CameraPtr& camera = map.get_camera();
  if (camera == nullptr) {
    return;
  }

  const Size& cell_size = non_animated_tiles.get_cell_size();
  const Rectangle& camera_position = camera->get_bounding_box();
  const int row1 = camera_position.get_y() / cell_size.height;
  const int row2 = (camera_position.get_y() + camera_position.get_height()) / cell_size.height;
  const int column1 = camera_position.get_x() / cell_size.width;
  const int column2 = (camera_position.get_x() + camera_position.get_width()) / cell_size.width;

  for (const auto& kvp : optimized_tiles_surfaces) {
    const int cell_index = kvp.first;
    const int row = cell_index / non_animated_tiles.get_num_columns();
    const int column = cell_index % non_animated_tiles.get_num_columns();
    if (column < column1 || column > column2 || row < row1 || row > row2) {
      indexes_to_clear.push_back(cell_index);
    }
  }

  for (int cell_index : indexes_to_clear) {
    optimized_tiles_surfaces.erase(cell_index);
  }
}

/**
 * \brief Draws a layer of non-animated regions of tiles on the current map.
 */
void NonAnimatedRegions::draw_on_map(const Camera &camera) {
  // Check all grid cells that overlap the camera.
  const int num_rows = non_animated_tiles.get_num_rows();
  const int num_columns = non_animated_tiles.get_num_columns();
  const Size& cell_size = non_animated_tiles.get_cell_size();
  const Rectangle& camera_position = camera.get_bounding_box();

  const int row1 = camera_position.get_y() / cell_size.height;
  const int row2 = (camera_position.get_y() + camera_position.get_height()) / cell_size.height;
  const int column1 = camera_position.get_x() / cell_size.width;
  const int column2 = (camera_position.get_x() + camera_position.get_width()) / cell_size.width;

  if (row1 > row2 || column1 > column2) {
    // No cell.
    return;
  }

  const auto& surface = camera.get_surface();

  for (int i = row1; i <= row2; ++i) {
    if (i < 0 || i >= num_rows) {
      continue;
    }

    for (int j = column1; j <= column2; ++j) {
      if (j < 0 || j >= num_columns) {
        continue;
      }

      // Make sure this cell is built.
      int cell_index = i * num_columns + j;
      if (optimized_tiles_surfaces.find(cell_index) == optimized_tiles_surfaces.end()) {
        // Lazily build the cell.
        build_cell(cell_index);
      }

      const Point cell_xy = {
          j * cell_size.width,
          i * cell_size.height
      };


      const Point dst_position = cell_xy;
      optimized_tiles_surfaces.at(cell_index)->draw(
        surface, dst_position
      );
    }
  }
}

/**
 * \brief Draws all non-animated tiles of a cell on its surface.
 * \param cell_index Index of the cell to draw.
 */
void NonAnimatedRegions::build_cell(int cell_index) {

  Debug::check_assertion(
      cell_index >= 0 && (size_t) cell_index < non_animated_tiles.get_num_cells(),
      "Wrong cell index"
  );
  Debug::check_assertion(optimized_tiles_surfaces.find(cell_index) == optimized_tiles_surfaces.end(),
      "This cell is already built"
  );

  const int row = cell_index / non_animated_tiles.get_num_columns();
  const int column = cell_index % non_animated_tiles.get_num_columns();

  // Position of this cell on the map.
  const Size cell_size = non_animated_tiles.get_cell_size();
  const Point cell_xy = {
      column * cell_size.width,
      row * cell_size.height
  };

  SurfacePtr cell_surface = Surface::create(cell_size,true);
  optimized_tiles_surfaces[cell_index] = cell_surface;

  const std::vector<TileInfo>& tiles_in_cell =
      non_animated_tiles.get_elements(cell_index);
  for (const TileInfo& tile: tiles_in_cell) {

    Rectangle dst_position(
        tile.box.get_x() - cell_xy.x,
        tile.box.get_y() - cell_xy.y,
        tile.box.get_width(),
        tile.box.get_height()
    );

    const Tileset* tileset = tile.tileset != nullptr ? tile.tileset : &map.get_tileset();
    Debug::check_assertion(tileset != nullptr, "Missing tileset");
    tile.pattern->fill_surface(
        cell_surface,
        dst_position,
        *tileset,
        cell_xy
    );
  }

  // Remember that non-animated tiles are drawn after animated ones.
  // We may have drawn too much.
  // We have to make sure we don't exceed the non-animated regions.
  // Erase 8x8 squares that contain animated tiles.
  for (int y = cell_xy.y; y < cell_xy.y + cell_size.height; y += 8) {
    if (y >= map.get_height()) {  // The last cell might exceed the map border.
      continue;
    }
    for (int x = cell_xy.x; x < cell_xy.x + cell_size.width; x += 8) {
      if (x >= map.get_width()) {
        continue;
      }

      int square_index = (y / 8) * map.get_width8() + (x / 8);

      if (are_squares_animated[square_index]) {
        Rectangle animated_square(
            x - cell_xy.x,
            y - cell_xy.y,
            8,
            8
        );
        cell_surface->clear(animated_square);
      }
    }
  }
}

}
