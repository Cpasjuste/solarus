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
#ifndef SOLARUS_CAMERA_H
#define SOLARUS_CAMERA_H

#include "solarus/core/Common.h"
#include "solarus/core/Rectangle.h"
#include "solarus/core/FRectangle.h"
#include "solarus/entities/Entity.h"
#include "solarus/entities/EntityPtr.h"
#include "solarus/core/Scale.h"
#include "solarus/graphics/SurfacePtr.h"
#include "solarus/graphics/Transition.h"
#include <memory>

namespace Solarus {

class Map;
class Separator;
class Surface;
class TargetMovement;

/**
 * \brief Manages the visible area of the map.
 *
 * The camera determines the visible area of the map.
 * The camera may either be tracking an entity
 * (usually the hero)
 * or be controlled manually by a script.
 */
class Camera : public Entity {

  public:

    enum class SurfaceMode {
      ABSOLUTE,
      MAP
    };

    static constexpr EntityType ThisType = EntityType::CAMERA;

    explicit Camera(const std::string& name);

    EntityType get_type() const override;

    bool can_be_drawn() const override;
    void set_suspended(bool suspended) override;
    void notify_movement_started() override;
    void notify_size_changed() override;
    void notify_being_removed() override;
    bool is_separator_obstacle(Separator& separator, const Rectangle& candidate_position) override;

    const SurfacePtr& get_surface() const;

    Point get_position_on_screen() const;
    void set_position_on_screen(const Point& position_on_screen);
    Point get_position_to_track(const Point& tracked_xy) const;

    void start_tracking(const EntityPtr& entity);
    void start_manual();

    EntityPtr get_tracked_entity() const;
    void notify_tracked_entity_traversing_separator(Separator& separator);
    bool is_traversing_separator() const;

    Rectangle apply_map_bounds(const Rectangle& area) const;
    Rectangle apply_separators(const Rectangle& area) const;
    Rectangle apply_separators_and_map_bounds(const Rectangle& area) const;

    void reset_view();
    void apply_view();

    void set_surface_mode();

    void notify_window_size_changed(const Size& new_size);

    Rectangle get_viewport_rectangle() const;

    void set_viewport(const FRectangle& viewport);
    const FRectangle& get_viewport() const;

    void set_zoom(const Scale& zoom);
    const Scale& get_zoom() const;

    void set_rotation(float rotation);
    float get_rotation() const;

    void set_transition(std::unique_ptr<Transition> transition);
    std::unique_ptr<Transition>& get_transition();

    void draw(const SurfacePtr& dst_surface) const;
private:
    void create_surface(const Size& size);
    void update_view(const Size& viewport_size);

    SurfacePtr surface;           /**< Surface where this camera draws its entities. */
    std::unique_ptr<Transition>
        transition;               /**< Ongoing transition */
    Point position_on_screen;     /**< Where to draw this camera on the screen. Used by Legacy LetterBoxing mode. */
    FRectangle viewport;          /**< Relative geometry of the camera on screen. Used by dynamic video modes. */
    Scale zoom;                   /**< Level of zoom of this camera compared to 1:1 cam. */
    Scale zoom_corr;              /**< Correction factor for the zoom, to compensate integer rounding of the camera size */
    float rotation;               /**< Rotation of this camera */
};

}

#endif

