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
#include "solarus/graphics/glrenderer/GlTexture.h"
#include "solarus/graphics/glrenderer/GlRenderer.h"
#include "solarus/core/Debug.h"
#include "solarus/graphics/Video.h"

#include <glm/gtx/matrix_transform_2d.hpp>
#include <SDL_render.h>

namespace Solarus {

inline glm::mat3 uv_view(int width, int height) {
  using namespace glm;
    return scale(mat3(1.f),vec2(1.f/width,1.f/height));
}

GlTexture::GlTexture(int width, int height, bool screen_tex)
  : SurfaceImpl({width, height}),
    target(true),
    uv_transform(uv_view(width,height)),
    fbo(GlRenderer::get().get_fbo(width,height,screen_tex)) {
  glGenTextures(1,&tex_id);
  glBindTexture(GL_TEXTURE_2D,tex_id);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);

  set_texture_params();
  GlRenderer::get().rebind_texture();
}

GlTexture::GlTexture(SDL_Surface_UniquePtr a_surface)
  : SurfaceImpl({a_surface->w, a_surface->h}),
    target(false),
    uv_transform(uv_view(a_surface->w,a_surface->h)),
    surface(std::move(a_surface)) {
  int width = surface->w;
  int height = surface->h;
  glGenTextures(1,&tex_id);

  glBindTexture(GL_TEXTURE_2D,tex_id);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,surface->pixels);
  set_texture_params();
  GlRenderer::get().rebind_texture();
}

void GlTexture::set_texture_params() {
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

/**
 * @brief upload potentially modified surface
 *
 * When modifying pixels of the Surface, we have
 * to upload it to the texture for changes to be reflected
 */
void GlTexture::upload_surface() {
  SDL_Surface* surface = get_surface();
  GlRenderer::get().put_pixels(this,surface->pixels);
}

/**
 * \copydoc SurfaceImpl::get_texture
 */
GLuint GlTexture::get_texture() const {
  return tex_id;
}

/**
 * \copydoc SurfaceImpl::get_surface
 */
SDL_Surface* GlTexture::get_surface() const {
  if(!surface) {
    create_surface();
  }
  if (target and surface_dirty) {
    GlRenderer::get().read_pixels(const_cast<GlTexture*>(this),surface->pixels);
    surface_dirty = false;
  }
  return surface.get();
}

/**
 * @brief create_surface
 */
void GlTexture::create_surface() const {
  SDL_PixelFormat* format = Video::get_pixel_format();
  SDL_Surface* surf_ptr = SDL_CreateRGBSurface(
        0,
        get_size().width,
        get_size().height,
        32,
        format->Rmask,
        format->Gmask,
        format->Bmask,
        format->Amask);
  SOLARUS_ASSERT(surf_ptr != nullptr,
      std::string("Failed to create backup surface ") + SDL_GetError());
  surface.reset(surf_ptr);
}

GlTexture& GlTexture::targetable()  {
  surface_dirty = true; //Just tag the surface as outdated
  if(!fbo)
    fbo = GlRenderer::get().get_fbo(get_width(),get_height());
  return *this;
}

void GlTexture::release() const {
  glDeleteTextures(1,&tex_id);
}

}
