#pragma once

#include <SDL_render.h>
#include <solarus/graphics/Renderer.h>


namespace Solarus {

#ifdef DEBUG
#define SOLARUS_CHECK_SDL_HIGHER(expr,bound) if((expr) < bound) Debug::error(std::string(SDL_GetError()) + "! " + __FILE__ + ":" + std::to_string(__LINE__));
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
  SDLRenderer(SDL_Renderer* window);
  static RendererPtr create(SDL_Window* window);
  SurfaceImplPtr create_texture(int width, int height) override;
  SurfaceImplPtr create_texture(SDL_Surface_UniquePtr &&surface) override;
  SurfaceImplPtr create_window_surface(SDL_Window* w, int width, int height) override;
  ShaderPtr create_shader(const std::string& shader_id) override;
  ShaderPtr create_shader(const std::string& vertex_source, const std::string& fragment_source, double scaling_factor) override;
  void set_render_target(SDL_Texture* target);
  void draw(SurfaceImpl& dst, const SurfaceImpl& src, const DrawInfos& infos) override;
  void clear(SurfaceImpl& dst) override;
  void fill(SurfaceImpl& dst, const Color& color, const Rectangle& where, BlendMode mode = BlendMode::BLEND) override;
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
  ~SDLRenderer() override;
private:
  static SDLRenderer* instance;

  SDL_BlendMode make_sdl_blend_mode(const SurfaceImpl &dst_surface, const SurfaceImpl &src_surface, BlendMode blend_mode);
  SDL_BlendMode make_sdl_blend_mode(BlendMode blend_mode);

  SDL_Surface_UniquePtr software_screen;
  SDL_Texture*  render_target = nullptr;
  bool valid_target = false;
  SDL_Renderer* renderer;
  SurfaceDraw surface_draw;
};
}
