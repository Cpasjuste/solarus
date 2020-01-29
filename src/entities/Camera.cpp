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
#include "solarus/core/Game.h"
#include "solarus/core/Map.h"
#include "solarus/core/System.h"
#include "solarus/entities/Camera.h"
#include "solarus/entities/Entities.h"
#include "solarus/entities/EntityState.h"
#include "solarus/entities/Hero.h"
#include "solarus/entities/Separator.h"
#include "solarus/graphics/Surface.h"
#include "solarus/graphics/Video.h"
#include "solarus/lua/LuaContext.h"
#include "solarus/movements/TargetMovement.h"

#include <algorithm>
#include <list>

namespace Solarus {

namespace {

/**
 * \brief State of the camera when centered on an entity.
 */
class TrackingState: public Entity::State {

public:

  TrackingState(Camera& camera, const EntityPtr& tracked_entity);

  void update() override;
  bool is_traversing_separator() const;
  void traverse_separator(Separator& separator);
  void start(const State* previous) override;
  bool belongs_to_camera(const Camera& camera) const;
  void undo_hero_linking() const;

  const EntityPtr& get_tracked_entity() const;

private:

  EntityPtr tracked_entity;               /**< Entity the camera is tracking. */
  Rectangle separator_scrolling_position; /**< Current camera position while crossing a separator. */
  Rectangle separator_target_position;    /**< Target camera position when crossing a separator. */
  Point separator_scrolling_delta;        /**< increment to the camera position when crossing a separator. */
  uint32_t separator_next_scrolling_date; /**< Next camera position change when crossing a separator. */
  int separator_scrolling_direction4;     /**< Direction when scrolling. */
  std::shared_ptr<Separator>
      separator_traversed;                /**< Separator currently being traversed or nullptr. */
};

/**
 * \brief Creates a camera tracking state.
 * \param camera The camera to control.
 * \param tracked_entity The entity to track with this camera.
 */
TrackingState::TrackingState(Camera& camera, const EntityPtr& tracked_entity) :
  Entity::State("tracking"),
  tracked_entity(tracked_entity),
  separator_next_scrolling_date(0),
  separator_scrolling_direction4(0) {

  Debug::check_assertion(tracked_entity != nullptr,
      "Missing tracked entity");
  set_entity(camera);
}

/**
 * \brief Returns the entity tracked in this state.
 * \return The tracked entity.
 */
const EntityPtr& TrackingState::get_tracked_entity() const {
  return tracked_entity;
}

/**
 * \brief Updates the position of the camera when the camera is tracking
 * an entity.
 */
void TrackingState::update() {

  Camera& camera = get_entity<Camera>();
  if (separator_next_scrolling_date == 0) {
    // Normal case: not traversing a separator.

    // First compute camera coordinates ignoring map limits and separators.
    Rectangle next = camera.get_bounding_box();
    next.set_center(tracked_entity->get_center_point());

    // Then apply constraints of both separators and map limits.
    camera.set_bounding_box(camera.apply_separators_and_map_bounds(next));
    camera.notify_bounding_box_changed();
  }
  else {
    // The tracked entity is currently traversing a separator.
    // Update camera coordinates.
    uint32_t now = System::now();
    bool finished = false;
    while (separator_next_scrolling_date != 0
        && now >= separator_next_scrolling_date) {
      separator_scrolling_position.add_xy(separator_scrolling_delta);

      separator_next_scrolling_date += 1;

      if (separator_scrolling_position == separator_target_position) {
        // Finished.
        finished = true;
      }
    }

    if (finished) {
      separator_next_scrolling_date = 0;
      separator_traversed->notify_activated(separator_scrolling_direction4);
      separator_traversed = nullptr;
      separator_scrolling_direction4 = 0;
    }

    // Then only apply map limit constraints.
    // Ignore separators since we are currently crossing one of them.
    camera.set_bounding_box(camera.apply_map_bounds(separator_scrolling_position));
    camera.notify_bounding_box_changed();
  }
}

/**
 * \brief Returns whether the camera is currently scrolling on a separator.
 * \return \c truc if scrolling on a separator.
 */
bool TrackingState::is_traversing_separator() const {
  return separator_traversed != nullptr;
}

/**
 * \brief Starts traversing a separator.
 *
 * The tracked entity must touch the separator when you call this function.
 *
 * \param separator The separator to traverse.
 */
void TrackingState::traverse_separator(Separator& separator) {

  Camera& camera = get_entity<Camera>();

  // Save the current position of the camera.
  separator_scrolling_position = camera.get_bounding_box();

  // Start scrolling.
  separator_traversed = std::static_pointer_cast<Separator>(
      separator.shared_from_this()
  );
  separator_scrolling_delta = Point();
  separator_target_position = separator_scrolling_position;

  const Point& tracked_entity_center = tracked_entity->get_center_point();
  const Point& separator_center = separator.get_center_point();
  if (separator.is_horizontal()) {
    if (tracked_entity_center.y < separator_center.y) {
      separator_scrolling_direction4 = 3;
      separator_scrolling_delta.y = 1;
      separator_target_position.add_y(camera.get_height());
    }
    else {
      separator_scrolling_direction4 = 1;
      separator_scrolling_delta.y = -1;
      separator_target_position.add_y(-camera.get_height());
    }
  }
  else {
    if (tracked_entity_center.x < separator_center.x) {
      separator_scrolling_direction4 = 0;
      separator_scrolling_delta.x = 1;
      separator_target_position.add_x(camera.get_width());
    }
    else {
      separator_scrolling_direction4 = 2;
      separator_scrolling_delta.x = -1;
      separator_target_position.add_x(-camera.get_width());
    }
  }

  separator.notify_activating(separator_scrolling_direction4);
  separator_next_scrolling_date = System::now();

  // Move the tracked entity two pixels ahead to avoid to traverse the separator again.
  tracked_entity->set_xy(tracked_entity->get_xy() + 2 * separator_scrolling_delta);
  tracked_entity->notify_bounding_box_changed();
}

/**
 * @brief Undo the linking of this camera to the hero it was potentialy tracking
 */
void TrackingState::undo_hero_linking() const {
    EntityPtr entity = get_tracked_entity();
    if(entity->get_type() == EntityType::HERO) {
        //Was tracking a hero, unregister this camera as linked-one
        Hero& hero = entity->as<Hero>();
        CameraPtr old_cam = hero.get_linked_camera();
        if(old_cam and belongs_to_camera(*old_cam)){
            hero.set_linked_camera(nullptr);
        }
    }
}

/**
 * @brief Check if this tracking state is the one of a particular camera
 * @param acamera camera to test
 * @return true if camera belongs to this State
 */
bool TrackingState::belongs_to_camera(const Camera& acamera) const {
    const Camera& cam = get_entity().as<Camera>();
    return &cam == &acamera;
}

/**
 * @brief Called when this states starts, unlink the hero from previous camera
 * @param previous
 */
void TrackingState::start(const State* previous) {
    Entity::State::start(previous);
    if (previous && previous->get_name() == "tracking") {
        static_cast<const TrackingState*>(previous)->undo_hero_linking();
    }

    EntityPtr entity = get_tracked_entity();
    if(entity->get_type() == EntityType::HERO) {
        entity->as<Hero>().set_linked_camera(get_entity().shared_from_this_cast<Camera>());
    }
}


/**
 * \brief State of the camera when controlled by scripts.
 */
class ManualState: public Entity::State {

public:

  explicit ManualState(Camera& camera);
  void start(const State* previous) override;

};

/**
 * \brief Creates a camera manual state.
 * \param camera The camera to control.
 */
ManualState::ManualState(Camera& camera) :
  Entity::State("manual") {
  set_entity(camera);
}


/**
 * @brief Called when this states starts, unlink the hero from previous camera
 * @param previous
 */
void ManualState::start(const State* previous) {
    Entity::State::start(previous);
    if (previous && previous->get_name() == "tracking") {
        static_cast<const TrackingState*>(previous)->undo_hero_linking();
    }
}

}  // Anonymous namespace.

/**
 * \brief Creates a camera.
 * \param map The map.
 */
Camera::Camera(const std::string &name):
  Entity(name, 0, 0, Point(0, 0), Video::get_quest_size()),
  surface(nullptr),
  position_on_screen(0, 0),
  viewport(0.f, 0.f, 1.f, 1.f) {


  create_surface(get_size());
  //set_map(map);
  notify_window_size_changed(Video::get_window_size());
}

/**
 * \copydoc Entity::get_type()
 */
EntityType Camera::get_type() const {
  return ThisType;
}

/**
 * \copydoc Entity::can_be_drawn()
 */
bool Camera::can_be_drawn() const {

  // The camera itself is not drawn.
  // Entities only use its position to draw the map.
  return false;
}

/**
 * \brief Initializes the surface where this camera draws entities.
 *
 * This function should be called when the camera size is changed.
 */
void Camera::create_surface(const Size& size) {

  surface = Surface::create(size);
}

/**
 * \brief Returns the surface where this camera draws entities.
 * \return The camera surface.
 */
const SurfacePtr& Camera::get_surface() const {
  return surface;
}

/**
 * \brief Notifies this entity that its size has just changed.
 */
void Camera::notify_size_changed() {

  // The size thas changed: rebuild the surface.
  if(Video::get_geometry_mode() == Video::GeometryMode::LETTER_BOXING &&
     (surface == nullptr || get_size() != surface->get_size())) {
    create_surface(get_size());
  }
}

/**
 * \copydoc Entity::is_separator_obstacle
 */
bool Camera::is_separator_obstacle(Separator& separator, const Rectangle& candidate_position) {
  return separator.is_crossed_by(candidate_position);
}

/**
 * \brief Returns where this camera is displayed on the screen.
 * \return Position of the upper-left corner of the camera relative to the
 * quest screen.
 */
Point Camera::get_position_on_screen() const {
  return position_on_screen;
}

/**
 * \brief Sets where this camera is displayed on the screen.
 * \param position_on_screen position of the upper-left corner of the camera
 * relative to the quest screen.
 */
void Camera::set_position_on_screen(const Point& position_on_screen) {
  this->position_on_screen = position_on_screen;
}

/**
 * \brief Returns the position this camera should take to track the specified point.
 * \param tracked_xy A point in map coordinates.
 * \return Where this camera should be placed to be focused on this point,
 * respecting separators and map limits.
 */
Point Camera::get_position_to_track(const Point& tracked_xy) const {

  Point top_left_xy = tracked_xy - Point(get_width() / 2, get_height() / 2);
  Rectangle box(top_left_xy, get_size());

  return apply_separators_and_map_bounds(box).get_xy();
}

/**
 * \copydoc Entity::set_suspended()
 *
 * Reimplemented from Entity to do nothing, in particular to avoid suspending
 * the camera movement.
 *
 * TODO rename set_suspended() to notify_suspended() / notify_unsuspended().
 */
void Camera::set_suspended(bool /* suspended */) {

}

/**
 * \copydoc Entity::notify_movement_started()
 */
void Camera::notify_movement_started() {

  Entity::notify_movement_started();

  // When a movement is set, automatically switch to manual state.
  if (get_state_name() != "manual") {
    start_manual();
  }
}

/**
 * \brief Makes that the camera track the given entity.
 * \param tracked_entity The entity to track.
 */
void Camera::start_tracking(const EntityPtr& tracked_entity) {
  set_state(std::make_shared<TrackingState>(*this, tracked_entity));
}

/**
 * \brief Makes the camera stop tracking any entity.
 */
void Camera::start_manual() {
  set_state(std::make_shared<ManualState>(*this));
}

/**
 * \brief Returns the entity currently tracked by this camera if any.
 * \return The tracked entity, or nullptr if the camera is not in tracking
 * state.
 */
EntityPtr Camera::get_tracked_entity() const {

  if (get_state_name() != "tracking") {
    return nullptr;
  }

  return std::static_pointer_cast<TrackingState>(get_state())->get_tracked_entity();
}

/**
 * \brief Function called when the tracked entity crosses a separator.
 * \param separator The separator being traversed.
 */
void Camera::notify_tracked_entity_traversing_separator(Separator& separator) {

  if (get_state_name() != "tracking") {
    return;
  }

  std::static_pointer_cast<TrackingState>(get_state())->traverse_separator(separator);
}

/**
 * \brief Returns whether this camera is currently scrolling on a separator.
 * \return \c true if this camera is tracking an entity that traverses a
 * separator.
 */
bool Camera::is_traversing_separator() const {

  if (get_state_name() != "tracking") {
    return false;
  }

  return std::static_pointer_cast<TrackingState>(get_state())->is_traversing_separator();
}

/**
 * \brief Ensures that a rectangle does not cross map limits.
 * \param area The rectangle to check. Is should not be entirely inside the map.
 * It can be bigger than the map: in such a case, the resulting rectangle cannot
 * avoid to cross map limits, and it will be centered.
 * \return A rectangle corresponding to the first one but stopping on map limits.
 */
Rectangle Camera::apply_map_bounds(const Rectangle& area) const {

  int x = area.get_x();  // Top-left corner.
  int y = area.get_y();
  const int width = area.get_width();
  const int height = area.get_height();

  const Size& map_size = get_map().get_size();
  if (map_size.width < width) {
    x = (map_size.width - width) / 2;
  }
  else {
    x = std::min(std::max(x, 0),
        map_size.width - width);
  }

  if (map_size.height < height) {
    y = (map_size.height - height) / 2;
  }
  else {
    y = std::min(std::max(y, 0),
        map_size.height - height);
  }
  return Rectangle(x, y, width, height);
}

/**
 * \brief Ensures that a rectangle does not cross separators.
 * \param area The rectangle to check.
 * \return A rectangle corresponding to the first one but stopping on separators.
 */
Rectangle Camera::apply_separators(const Rectangle& area) const {

  int x = area.get_x();  // Top-left corner.
  int y = area.get_y();
  const int width = area.get_width();
  const int height = area.get_height();

  // TODO simplify: treat horizontal separators first and then all vertical ones.
  int adjusted_x = x;  // Updated coordinates after applying separators.
  int adjusted_y = y;
  std::vector<std::shared_ptr<const Separator>> applied_separators;
  const std::set<std::shared_ptr<const Separator>>& separators =
      get_entities().get_entities_by_type<Separator>();
  for (const std::shared_ptr<const Separator>& separator: separators) {

    if (separator->is_vertical()) {
      // Vertical separator.
      int separation_x = separator->get_x() + 8;

      if (x < separation_x && separation_x < x + width
          && separator->get_y() < y + height
          && y < separator->get_y() + separator->get_height()) {
        int left = separation_x - x;
        int right = x + width - separation_x;
        if (left > right) {
          adjusted_x = separation_x - width;
        }
        else {
          adjusted_x = separation_x;
        }
        applied_separators.push_back(separator);
      }
    }
    else {
      Debug::check_assertion(separator->is_horizontal(), "Invalid separator shape");

      // Horizontal separator.
      int separation_y = separator->get_y() + 8;
      if (y < separation_y && separation_y < y + height
          && separator->get_x() < x + width
          && x < separator->get_x() + separator->get_width()) {
        int top = separation_y - y;
        int bottom = y + height - separation_y;
        if (top > bottom) {
          adjusted_y = separation_y - height;
        }
        else {
          adjusted_y = separation_y;
        }
        applied_separators.push_back(separator);
      }
    }
  }  // End for each separator.

  bool must_adjust_x = true;
  bool must_adjust_y = true;
  if (adjusted_x != x && adjusted_y != y) {
    // Both directions were modified. Maybe it is a T configuration where
    // a separator deactivates another one.

    must_adjust_x = false;
    must_adjust_y = false;
    for (const std::shared_ptr<const Separator>& separator: applied_separators) {

      if (separator->is_vertical()) {
        // Vertical separator.
        int separation_x = separator->get_x() + 8;

        if (x < separation_x && separation_x < x + width
            && separator->get_y() < adjusted_y + height
            && adjusted_y < separator->get_y() + separator->get_height()) {
          must_adjust_x = true;
        }
      }
      else {
        // Horizontal separator.
        int separation_y = separator->get_y() + 8;

        if (y < separation_y && separation_y < y + height
            && separator->get_x() < adjusted_x + width
            && adjusted_x < separator->get_x() + separator->get_width()) {
          must_adjust_y = true;
        }
      }
    }  // End for each separator applied.
  }  // End if both directions.

  if (must_adjust_x) {
    x = adjusted_x;
  }
  if (must_adjust_y) {
    y = adjusted_y;
  }

  return Rectangle(x, y, width, height);
}

/**
 * @brief Camera::notify_window_size_changed
 * @param new_size
 */
void Camera::notify_window_size_changed(const Size& /*new_size*/) {
  Rectangle vrect = get_viewport_rectangle();
  int x = vrect.get_left();
  int y = vrect.get_top();
  int w = vrect.get_width();
  int h = vrect.get_height();
  switch (Video::get_geometry_mode()) {
  case Video::GeometryMode::DYNAMIC_QUEST_SIZE:
  case Video::GeometryMode::DYNAMIC_ABSOLUTE:
    set_position_on_screen({x, y});
    create_surface({w, h});
    surface->set_scale(Scale(1,1)); //Draw the surface 1:1 on screen
    update_view({w,h});
    break;
  default:
    break;
  }
}

/**
 * @brief Camera::update_view
 */
void Camera::update_view(const Size& viewport_size) {
  int w = viewport_size.width;
  int h = viewport_size.height;
  switch (Video::get_geometry_mode()) {
  case Video::GeometryMode::DYNAMIC_QUEST_SIZE:{
    Size quest_size = Video::get_quest_size();
    float qratio = quest_size.width / static_cast<float>(quest_size.height);
    float wratio = w / static_cast<float>(h);

    //Compute wanted size
    float cw = quest_size.width;
    float ch = quest_size.width/wratio;
    if(qratio > wratio) {
      ch = quest_size.height;
      cw = quest_size.height*wratio;
    }

    //Apply zoom
    cw /= zoom.x;
    ch /= zoom.y;

    int icw = cw;
    int ich = ch;

    zoom_corr.x = icw/cw;
    zoom_corr.y = ich/ch;

    set_size(Size(icw, ich));
    break;
  }
  case Video::GeometryMode::DYNAMIC_ABSOLUTE: {
    // Apply zoom
    float cw = w / zoom.x;
    float ch = h / zoom.y;

    int icw = cw;
    int ich = ch;

    zoom_corr.x = icw/cw;
    zoom_corr.y = ich/ch;

    set_size({icw, ich});
    break;
  }
  default:
    break;
  }
}

/**
 * @brief Camera::get_viewport_rectangle
 * @return
 */
Rectangle Camera::get_viewport_rectangle() const {
  Size wsize = Video::get_window_size();
  return Rectangle(
    wsize.width * viewport.left,
    wsize.height * viewport.top,
    std::ceil(wsize.width * viewport.width),
    std::ceil(wsize.height * viewport.height)
  );
}

/**
 * @brief set the fraction of the screen occupied by this camera
 * @param viewport
 */
void Camera::set_viewport(const FRectangle& viewport) {
  this->viewport = viewport;
  notify_window_size_changed(Video::get_window_size());
}

/**
 * @brief get the fraction of the screen occupied by this camera
 * @return
 */
const FRectangle& Camera::get_viewport() const {
  return viewport;
}

/**
 * @brief Camera::set_zoom
 * @param zoom
 */
void Camera::set_zoom(const Scale& zoom) {
  this->zoom = zoom;
  update_view(get_viewport_rectangle().get_size());
}

/**
 * @brief Camera::get_zoom
 * @return
 */
const Scale& Camera::get_zoom() const {
  return zoom;
}

/**
 * @brief Camera::set_rotation
 * @param rotation
 */
void Camera::set_rotation(float rotation) {
  this->rotation = rotation;
  update_view(get_viewport_rectangle().get_size());
}

/**
 * @brief Camera::get_rotation
 * @return
 */
float Camera::get_rotation() const {
  return rotation;
}

/**
 * \brief Ensures that a rectangle does not cross separators nor map bounds.
 * \param area The rectangle to check.
 * It is the responsibility of quest makers to put enough space between
 * separators (the space should be at least the quest size).
 * If separators are too close to each other for the rectangle to fit,
 * the camera will cross some of them.
 * If separators are too close to a limit of the map, the
 * the camera will cross them but will never cross map limits.
 * \return A rectangle corresponding to the first one but stopping on
 * separators and map bounds.
 */
Rectangle Camera::apply_separators_and_map_bounds(const Rectangle& area) const {
  return apply_map_bounds(apply_separators(area));
}

/**
 * @brief Sets the surface's view to the default one
 */
void Camera::reset_view() {
  surface->get_view().reset(Rectangle(surface->get_size()));
}

/**
 * @brief Compute and apply the view of this camera to its surface
 */
void Camera::apply_view() {
  //TODO add rotation and zoom
  surface->get_view().reset(get_bounding_box());
  surface->get_view().zoom(zoom_corr);
}

/**
 * @brief Sets the transition of this camera
 * @param transition
 */
void Camera::set_transition(std::unique_ptr<Transition> transition) {
  this->transition = std::move(transition);
}

/**
 * @brief Gets the transition associated with this camera
 * @return
 */
std::unique_ptr<Transition>& Camera::get_transition() {
 return transition;
}

/**
 * @copydoc Entity::notify_being_removed
 */
void Camera::notify_being_removed() {
  Entity::notify_being_removed();
}

/**
 * @brief Draw the content of this camera to a surface
 * @param dst_surface a surface
 */
void Camera::draw(const SurfacePtr& dst_surface) const {
  const auto& surf = get_surface();
  if(transition){
    surf->draw_with_transition(
          Rectangle(surf->get_size()),
          dst_surface,
          get_position_on_screen(),
          *transition);
  } else {
    surf->draw(dst_surface, get_position_on_screen());
  }
}

}
