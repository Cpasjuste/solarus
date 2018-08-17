#pragma once

#include "solarus/graphics/SurfaceImpl.h"
#include "solarus/graphics/Color.h"
#include "solarus/graphics/SDLPtrs.h"
#include "solarus/graphics/Video.h"
#include "solarus/core/Debug.h"
#include "DrawProxies.h"


namespace Solarus {

#ifdef DEBUG
#define SOLARUS_CHECK_SDL_HIGHER(expr,bound) if((expr) < bound) Debug::error(std::string(SDL_GetError()) + "! " + __FILE__ + ":" + std::to_string(__LINE__));
#else
#define SOLARUS_CHECK_SDL_HIGHER(expr,bound) expr
#endif

#define SOLARUS_CHECK_SDL(expr) SOLARUS_CHECK_SDL_HIGHER(expr,0)


/**
 * @brief SurfaceImpl representing mutable surface data
 *
 * RenderTextures are created when instantiating empty surface
 * that are likely to be filled or drawn to.
 */
class RenderTexture : public SurfaceImpl
{
public:
    RenderTexture(int width, int height);
    RenderTexture(SDL_Texture* texture, int width, int height);
    SDL_Texture* get_texture() const override;
    SDL_Surface* get_surface() const override;

    template<typename Func>
    /**
     * @brief setup draw environnement for drawing on the target texture
     * @param closure work to achieve while the target texture is bound
     */
    void with_target(Func closure) const {
      surface_dirty = true;
      auto renderer = Video::get_renderer();
      Video::set_render_target(target.get());
      closure(renderer);
    }

    int get_width() const override;
    int get_height() const override;

    void draw_other(const SurfaceImpl& texture, const DrawInfos& infos);
    //void draw_region_other(const Rectangle &src_rect, const SurfaceImpl &texture, const Point& dst_position);

    RenderTexture* to_render_texture() override;

    void fill_with_color(const Color& color, const Rectangle& where, SDL_BlendMode mode = SDL_BLENDMODE_BLEND);

    void clear();
    void clear(const Rectangle& where);
    ~RenderTexture(){
      Video::invalidate_target(target.get());
    }
private:
    mutable bool surface_dirty = true; /**< is the surface not up to date*/
    mutable SDL_Surface_UniquePtr surface; /**< cpu side pixels data */
    mutable SDL_Texture_UniquePtr target; /**< gpu side pixels data */
};

}
