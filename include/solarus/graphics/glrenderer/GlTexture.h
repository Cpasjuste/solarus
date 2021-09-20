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

#include "solarus/core/Rectangle.h"
#include "solarus/core/Point.h"
#include "solarus/graphics/SDLPtrs.h"
#include "solarus/graphics/SurfaceImpl.h"
#include "solarus/graphics/glrenderer/GlRenderer.h"

#include <memory>

namespace Solarus {

/**
 * @brief Abstract class for internal surface pixel representation and manipulation
 */
class GlTexture : public SurfaceImpl
{
  friend class GlRenderer;
public:
  GlTexture(int width, int height, bool screen_tex = false, int margin = 0);
  GlTexture(SDL_Surface_UniquePtr surface);

  GLuint get_texture() const;
  SDL_Surface* get_surface() const override;

  GlTexture& targetable();

  /**
   * @brief upload potentially modified surface
   *
   * When modifying pixels of the Surface, we have
   * to upload it to the texture for changes to be reflected
   */
  void upload_surface() override;
private:
  bool target = false;
  void release() const;
  void set_texture_params();
  void create_surface() const;
  glm::mat3 uv_transform;
  mutable bool surface_dirty = true;
  GLuint tex_id = 0;
  GlRenderer::Fbo* fbo = nullptr;
  mutable SDL_Surface_UniquePtr surface = nullptr;
};

}
