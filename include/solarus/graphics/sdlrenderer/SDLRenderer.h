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

#include <SDL_render.h>
#include <solarus/graphics/Renderer.h>


namespace Solarus {

#ifdef DEBUG
#define SOLARUS_CHECK_SDL_HIGHER(expr,bound) if((expr) < bound) Debug::error(std::string(SDL_GetError()) + "! " + __FILE__ + ":" + std::to_string(__LINE__))
#else
#define SOLARUS_CHECK_SDL_HIGHER(expr,bound) expr
#endif

#define SOLARUS_CHECK_SDL(expr) SOLARUS_CHECK_SDL_HIGHER(expr,0)

/**
 * @brief Legacy SDL renderer, used for window-less tests
 */
class SDLRenderer : public Renderer {
  friend class SDLSurfaceImpl;
  friend class SDLShader;
  /**
   * @brief terminal DrawProxy for simple surface draw
   */
  struct SurfaceDraw : public DrawProxy {
    virtual void draw(Surface& dst_surface, const Surface& src_surface, const DrawInfos& params) const override;
  };
public:
  SDLRenderer(SDL_Renderer* window, bool shaders);
  static RendererPtr create(SDL_Window* window, bool force_software);
  SurfaceImplPtr create_texture(int width, int height, int margin=0) override;
  SurfaceImplPtr create_texture(SDL_Surface_UniquePtr &&surface) override;
  SurfaceImplPtr create_window_surface(SDL_Window* w, int width, int height) override;
  ShaderPtr create_shader(const std::string& shader_id) override;
  ShaderPtr create_shader(const std::string& vertex_source, const std::string& fragment_source, double scaling_factor) override;
  void set_render_target(SDL_Texture* target);
  void bind_as_gl_target(SurfaceImpl& surf) override;
  void bind_as_gl_texture(const SurfaceImpl& surf) override;
  void draw(SurfaceImpl& dst, const SurfaceImpl& src, const DrawInfos& infos) override;
  void clear(SurfaceImpl& dst) override;
  void fill(SurfaceImpl& dst, const Color& color, const Rectangle& where, BlendMode mode = BlendMode::BLEND) override;
  void notify_target_changed(const SurfaceImpl& surf) override;
  void invalidate(const SurfaceImpl& surf) override;
  std::string get_name() const override;
  void present(SDL_Window* window) override;
  void on_window_size_changed(const Rectangle& viewport) override;
  static SDLRenderer& get(){
    return *instance;
  }
  const DrawProxy& default_terminal() const override {
    return surface_draw;
  }
  bool needs_window_workaround() const override {
    return true;
  }
  ~SDLRenderer() override;
private:
  static SDLRenderer* instance;

  SDL_BlendMode make_sdl_blend_mode(const SurfaceImpl &dst_surface, const SurfaceImpl &src_surface, BlendMode blend_mode);
  SDL_BlendMode make_sdl_blend_mode(BlendMode blend_mode);

  SDL_Surface_UniquePtr software_screen;
  SDL_Texture*  render_target = nullptr;
  bool valid_target = false;
  SDL_Renderer* renderer;
  bool shaders = false;
  SurfaceDraw surface_draw;
};
}
