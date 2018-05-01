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
#ifndef SOLARUS_SURFACE_H
#define SOLARUS_SURFACE_H

#include "solarus/core/Common.h"
#include "solarus/core/PixelBits.h"
#include "solarus/graphics/SurfacePtr.h"
#include "solarus/graphics/Drawable.h"
#include "solarus/graphics/SurfaceImpl.h"
#include "solarus/graphics/SDLPtrs.h"


#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Solarus {

class Color;
class Size;
class SoftwarePixelFilter;
class Surface;

/**
 * \brief Represents a graphic surface.
 *
 * A surface is a rectangle of pixels.
 * A surface can be drawn or blitted on another surface.
 * This class basically encapsulates a library-dependent surface object.
 */
class Surface: public Drawable {

    friend class Shader;
    friend class VertexArray; //TODO find cleaner way
  public:
    using SurfaceImpl_UniquePtr = std::unique_ptr<SurfaceImpl>;

    /**
     * \brief The base directory to use when opening image files.
     */
    enum ImageDirectory {
      DIR_DATA,        /**< The root directory of the data package. */
      DIR_SPRITES,     /**< The sprites subdirectory of the data package (default). */
      DIR_LANGUAGE     /**< The language-specific image directory of the data package, for the current language. */
    };

    Surface(int width, int height, bool premultiplied = false);
    explicit Surface(SurfaceImpl* impl, bool premultiplied = false);
    Surface(SDL_Surface* surf, bool premultiplied = false);
    ~Surface();

    // Surfaces should only created with std::make_shared.
    // This is what create() functions do, so you should call them rather than
    // constructors.
    // This is because they are always reference-counted with shared_ptr
    // internally for drawing.
    static SurfacePtr create(int width, int height, bool premultiplied = false);
    static SurfacePtr create(const Size& size, bool premultiplied = false);
    static SurfacePtr create(const std::string& file_name,
        ImageDirectory base_directory = DIR_SPRITES, bool premultiplied = false);

    int get_width() const;
    int get_height() const;
    virtual Size get_size() const override;

    void clear();
    void clear(const Rectangle& where);
    void fill_with_color(const Color& color);
    void fill_with_color(const Color& color, const Rectangle& where);
    uint8_t get_opacity() const;
    void set_opacity(uint8_t opacity);

    SurfaceImpl &get_internal_surface();
    const SurfaceImpl &get_internal_surface() const;
    RenderTexture &request_render();

    bool is_pixel_transparent(int index) const;

    std::string get_pixels() const;
    void set_pixels(const std::string& buffer);

    void render(SDL_Renderer *&renderer);

    // Implementation from Drawable.
    virtual void raw_draw(
        Surface& dst_surface,
        const Point& dst_position
    ) override;
    virtual void raw_draw_region(
        const Rectangle& region,
        Surface& dst_surface,
        const Point& dst_position
    ) override;
    virtual void shader_draw(
        const ShaderPtr& shader,
        Surface& dst_surface,
        const Point& dst_position
        ) override;
    virtual void shader_draw_region(
        const ShaderPtr& shader,
        const Rectangle& region,
        Surface& dst_surface,
        const Point& dst_position
        ) override;
    virtual void draw_transition(Transition& transition) override;
    void apply_pixel_filter(
        const SoftwarePixelFilter& pixel_filter, Surface& dst_surface) const;
    virtual Surface& get_transition_surface() override;
    SDL_BlendMode get_sdl_blend_mode() const;

    void bind_as_texture() const;
    void bind_as_target();

    const std::string& get_lua_type_name() const override;
  private:
    uint8_t opacity;
    uint32_t get_pixel(int index) const;
    uint32_t get_color_value(const Color& color) const;


    static SurfaceImpl* get_surface_from_file(
        const std::string& file_name,
        ImageDirectory base_directory);

    SurfaceImpl_UniquePtr
        internal_surface;                 /**< The SDL_Surface encapsulated. */
};

}

#endif

