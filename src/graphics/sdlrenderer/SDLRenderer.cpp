#include <solarus/graphics/sdlrenderer/SDLRenderer.h>
#include <solarus/graphics/sdlrenderer/SDLSurfaceImpl.h>
#include <solarus/graphics/sdlrenderer/SDLShader.h>
#include <solarus/graphics/Video.h>
#include <solarus/graphics/Surface.h>
#include <solarus/core/Debug.h>
#include <solarus/graphics/Shader.h>

#include <SDL_render.h>
#include <SDL_hints.h>

namespace Solarus {

SDLRenderer* SDLRenderer::instance = nullptr;

void SDLRenderer::SurfaceDraw::draw(Surface& dst_surface, const Surface& src_surface, const DrawInfos& params) const {
  SDLRenderer::get().draw(dst_surface.get_impl(),src_surface.get_impl(),params);
}

SDLRenderer::SDLRenderer(SDL_Renderer* a_renderer) : renderer(a_renderer) {
  if(!renderer) {
    auto rgba_format = Video::get_pixel_format();
    software_screen.reset(SDL_CreateRGBSurface(
        0,
        320,
        240,
        32,
        rgba_format->Rmask,
        rgba_format->Gmask,
        rgba_format->Bmask,
        rgba_format->Amask
    ));
    renderer = SDL_CreateSoftwareRenderer(software_screen.get());

  }
  Debug::check_assertion(!instance,"Creating two SDL renderer");
  instance = this; //Set this renderer as the unique instance
}

RendererPtr SDLRenderer::create(SDL_Window* window) {
  if(!window) {
    //No window... asked for a software renderer
    return RendererPtr(new SDLRenderer(nullptr));
  }

  // Set OpenGL as the default renderer driver when available, to avoid using Direct3d.
  SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "opengl", SDL_HINT_DEFAULT);

  // Set the default OpenGL built-in shader (nearest).
  SDL_SetHint(SDL_HINT_RENDER_OPENGL_SHADERS, "1");

  auto renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED);
  if(not renderer) {
    renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
  }
  if(not renderer) {
    return nullptr;
  } else {
    //Init shaders :
    if(!SDLShader::initialize()) {
      //return nullptr; //TODO Set some flags
    }

    //auto qs = Video::get_quest_size();

    return RendererPtr(new SDLRenderer(renderer));
  }
}

SurfaceImplPtr SDLRenderer::create_texture(int width, int height) {
  return SurfaceImplPtr(new SDLSurfaceImpl(renderer,width,height));
}

SurfaceImplPtr SDLRenderer::create_texture(SDL_Surface_UniquePtr&& surface) {
  return SurfaceImplPtr(new SDLSurfaceImpl(renderer,std::move(surface)));
}

SurfaceImplPtr SDLRenderer::create_window_surface(SDL_Window* /*w*/, int width, int height) {
  return SurfaceImplPtr(new SDLSurfaceImpl(renderer,width,height,true));
}

ShaderPtr SDLRenderer::create_shader(const std::string& shader_id) {
  return std::make_shared<SDLShader>(shader_id);
}

ShaderPtr SDLRenderer::create_shader(const std::string& vertex_source, const std::string& fragment_source, double scaling_factor) {
  return std::make_shared<SDLShader>(vertex_source, fragment_source, scaling_factor);
}

void SDLRenderer::set_render_target(SurfaceImpl& texture) {
  set_render_target(texture.as<SDLSurfaceImpl>().targetable().get_texture());
}

void SDLRenderer::set_render_target(SDL_Texture* target) {
  if(target != render_target || !valid_target) {
    SDL_SetRenderTarget(renderer,target);
    render_target=target;
    valid_target = true;
  }
}

void SDLRenderer::draw(SurfaceImpl& dst, const SurfaceImpl& src, const DrawInfos& infos) {
  SDLSurfaceImpl& sdst = dst.as<SDLSurfaceImpl>().targetable();
  const SDLSurfaceImpl& ssrc = src.as<SDLSurfaceImpl>();

  set_render_target(sdst.get_texture());
  Rectangle dst_rect = infos.dst_rectangle();
  if(!ssrc.get_texture()) {
    Debug::error("Could not draw screen on another surface");
  }

  SDL_BlendMode mode = make_sdl_blend_mode(dst,src,infos.blend_mode);
  SOLARUS_CHECK_SDL_HIGHER(SDL_SetTextureBlendMode(ssrc.get_texture(),mode),-1);
  SOLARUS_CHECK_SDL(SDL_SetTextureAlphaMod(ssrc.get_texture(),infos.opacity));
  if(infos.should_use_ex()) {
    SDL_Point origin= infos.sdl_origin();
    SOLARUS_CHECK_SDL(SDL_RenderCopyEx(renderer,ssrc.get_texture(),infos.region,dst_rect,infos.rotation,&origin,infos.flips()));
  } else {
    SOLARUS_CHECK_SDL(SDL_RenderCopy(renderer,ssrc.get_texture(),infos.region,dst_rect));
  }
  sdst.surface_dirty = true;
}

void SDLRenderer::clear(SurfaceImpl& dst) {
  SDLSurfaceImpl& sdst = dst.as<SDLSurfaceImpl>().targetable();
  set_render_target(sdst.get_texture());

  SOLARUS_CHECK_SDL(SDL_SetRenderDrawColor(renderer,0,0,0,0));
  SOLARUS_CHECK_SDL(SDL_SetTextureBlendMode(sdst.get_texture(),SDL_BLENDMODE_BLEND));
  SOLARUS_CHECK_SDL(SDL_RenderClear(renderer));

  sdst.surface_dirty = true;
}

void SDLRenderer::fill(SurfaceImpl& dst, const Color& color, const Rectangle& where, BlendMode mode) {
  SDLSurfaceImpl& sdst = dst.as<SDLSurfaceImpl>().targetable();
  set_render_target(sdst.get_texture());

  Uint8 r,g,b,a;
  color.get_components(r,g,b,a);
  SOLARUS_CHECK_SDL(SDL_SetRenderDrawColor(renderer,r,g,b,a));
  SOLARUS_CHECK_SDL(SDL_SetRenderDrawBlendMode(renderer,make_sdl_blend_mode(mode)));
  SOLARUS_CHECK_SDL(SDL_RenderFillRect(renderer,where));

  sdst.surface_dirty = true;
}

void SDLRenderer::invalidate(const SurfaceImpl& surf) {
  const auto& ssurf = surf.as<SDLSurfaceImpl>();

  if(render_target == ssurf.get_texture()) {
    valid_target = false;
  }
}

std::string SDLRenderer::get_name() const {
  SDL_RendererInfo renderer_info;
  SDL_GetRendererInfo(renderer, &renderer_info);
  return std::string("SDLRenderer : ") + renderer_info.name;
}

void SDLRenderer::render(SDL_Window* /*window*/, const SurfacePtr &quest_surface, const ShaderPtr &shader) {
  //Declare local clear screen routine

  set_render_target(nullptr);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  //SDL_RenderSetClipRect(renderer, nullptr);
  SDL_RenderClear(renderer);


  auto size = quest_surface->get_size();
  SDL_RenderSetLogicalSize(renderer,size.width,size.height);

  //See if there is a shader to apply
  const SDLSurfaceImpl& qs = quest_surface->get_impl().as<SDLSurfaceImpl>();
  //Render the final surface with sdl if necessary
    // SDL rendering.

    //Set blending mode to none to simply replace any on_screen material
  if(shader){
    SDLShader& s = const_cast<SDLShader&>(shader->as<SDLShader>());
    s.render(*quest_surface,Rectangle(quest_surface->get_size()),quest_surface->get_size(),Point(),true);
  } else {
    SDL_SetTextureBlendMode(qs.get_texture(),SDL_BLENDMODE_NONE);
    SDL_RenderCopy(renderer, qs.get_texture(), Rectangle(size), nullptr);
  }
}

void SDLRenderer::present(SDL_Window* /*window*/) {
  SDL_RenderPresent(renderer);
}

void SDLRenderer::on_window_size_changed(const Rectangle& viewport) {
  SDL_RenderSetViewport(renderer,viewport);
}

/**
 * @brief compute sdl blendmode to use when writing a surface onto another
 * @param dst_surface written to surface
 * @param src_surface read from surface
 * @param blend_mode  solarus blend mode
 * @return a sdl blendmode taking premultiply into account
 */
SDL_BlendMode SDLRenderer::make_sdl_blend_mode(const SurfaceImpl& dst_surface, const SurfaceImpl& src_surface, BlendMode blend_mode) {
  if(dst_surface.is_premultiplied()) { //TODO refactor this a bit
    switch(blend_mode) {
      case BlendMode::NONE:
        return SDL_BLENDMODE_NONE;
      case BlendMode::MULTIPLY:
        return SDL_BLENDMODE_MOD;
      case BlendMode::ADD:
        return SDL_BLENDMODE_ADD;
      case BlendMode::BLEND:
        return SDL_ComposeCustomBlendMode(
              SDL_BLENDFACTOR_SRC_ALPHA,
              SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
              SDL_BLENDOPERATION_ADD,
              SDL_BLENDFACTOR_ONE,
              SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
              SDL_BLENDOPERATION_ADD);
    }
  } else {
    //Straight destination
    if(src_surface.is_premultiplied() && blend_mode == BlendMode::BLEND)
      return SDL_ComposeCustomBlendMode(
            SDL_BLENDFACTOR_ONE,
            SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            SDL_BLENDOPERATION_ADD,
            SDL_BLENDFACTOR_ONE,
            SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            SDL_BLENDOPERATION_ADD);
    switch(blend_mode) { //TODO check other modes
      case BlendMode::NONE:
        return SDL_BLENDMODE_NONE;
      case BlendMode::MULTIPLY:
        return SDL_BLENDMODE_MOD;
      case BlendMode::ADD:
        return SDL_BLENDMODE_ADD;
      case BlendMode::BLEND:
        return SDL_BLENDMODE_BLEND;
    }
  }
  return SDL_BLENDMODE_BLEND;
}

SDL_BlendMode SDLRenderer::make_sdl_blend_mode(BlendMode blend_mode) {
  switch (blend_mode) {
  case BlendMode::ADD:
    return SDL_BLENDMODE_ADD;
  case BlendMode::MULTIPLY:
    return SDL_BLENDMODE_MOD;
  case BlendMode::BLEND:
    return SDL_BLENDMODE_BLEND;
  case BlendMode::NONE:
    return SDL_BLENDMODE_NONE;
  }
  return SDL_BLENDMODE_BLEND;
}

SDLRenderer::~SDLRenderer() {
  SDL_DestroyRenderer(renderer);
}

}
