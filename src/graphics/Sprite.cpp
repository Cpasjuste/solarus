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
#include "solarus/core/Game.h"
#include "solarus/core/Map.h"
#include "solarus/core/PixelBits.h"
#include "solarus/core/Size.h"
#include "solarus/core/System.h"
#include "solarus/graphics/Color.h"
#include "solarus/graphics/Sprite.h"
#include "solarus/graphics/SpriteAnimation.h"
#include "solarus/graphics/SpriteAnimationDirection.h"
#include "solarus/graphics/SpriteAnimationSet.h"
#include "solarus/graphics/Surface.h"
#include "solarus/graphics/Shader.h"
#include "solarus/lua/LuaContext.h"
#include "solarus/lua/LuaTools.h"
#include "solarus/movements/Movement.h"
#include <lua.hpp>
#include <limits>
#include <memory>
#include <sstream>
#include <iostream>

namespace Solarus {

std::map<std::string, SpriteAnimationSet*> Sprite::all_animation_sets;

/**
 * \brief Initializes the sprites system.
 */
void Sprite::initialize() {
}

/**
 * \brief Uninitializes the sprites system.
 */
void Sprite::quit() {

  // delete the animations loaded
  for (auto& kvp: all_animation_sets) {
    delete kvp.second;
  }
  all_animation_sets.clear();
}

/**
 * \brief Returns the sprite animation set corresponding to the specified id.
 *
 * The animation set may be created if it is new, or just retrieved from
 * memory if it way already used before.
 *
 * \param id id of the animation set
 * \return the corresponding animation set
 */
SpriteAnimationSet& Sprite::get_animation_set(const std::string& id) {

  SpriteAnimationSet* animation_set = nullptr;
  auto it = all_animation_sets.find(id);
  if (it != all_animation_sets.end()) {
    animation_set = it->second;
  }
  else {
    animation_set = new SpriteAnimationSet(id);
    all_animation_sets[id] = animation_set;
  }

  Debug::check_assertion(animation_set != nullptr, "No animation set");

  return *animation_set;
}

/**
 * \brief Creates a sprite with the specified animation set.
 * \param id name of an animation set
 */
Sprite::Sprite(const std::string& id):
  Drawable(),
  animation_set_id(id),
  animation_set(get_animation_set(id)),
  current_animation(nullptr),
  current_direction(0),
  current_frame(-1),
  frame_changed(false),
  frame_delay(0),
  next_frame_date(0),
  ignore_suspend(false),
  paused(false),
  finished(false),
  synchronize_to(nullptr),
  blink_delay(0),
  blink_is_sprite_visible(true),
  blink_next_change_date(0),
  finished_callback_ref() {

  set_current_animation(animation_set.get_default_animation());
}

/**
 * \brief Returns the id of the animation set of this sprite.
 * \return the animation set id of this sprite
 */
const std::string& Sprite::get_animation_set_id() const {
  return animation_set_id;
}

/**
 * \brief Returns the animation set of this sprite.
 *
 * If several sprites have the same animation set, they share the same instance of animation set
 * and the same pointer is returned here.
 *
 * \return the animation set of this sprite
 */
const SpriteAnimationSet& Sprite::get_animation_set() const {
  return animation_set;
}

/**
 * \brief When the sprite is drawn on a map, sets the tileset.
 *
 * This function must be called if this sprite image depends on the map's tileset.
 *
 * \param tileset The tileset.
 */
void Sprite::set_tileset(const Tileset& tileset) {
  animation_set.set_tileset(tileset);
}

/**
 * \brief Enables the pixel-perfect collision detection for the animation set of this sprite.
 *
 * All sprites that use the same animation set as this one will be affected.
 */
void Sprite::enable_pixel_collisions() {
  animation_set.enable_pixel_collisions();
}

/**
 * \brief Returns whether the pixel-perfect collisions are enabled for the animation set of this sprite.
 * \return true if the pixel-perfect collisions are enabled
 */
bool Sprite::are_pixel_collisions_enabled() const {
  return animation_set.are_pixel_collisions_enabled();
}

/**
 * \brief Returns the size of the current frame.
 * \return The size of the current frame.
 */
Size Sprite::get_size() const {

  if (current_animation == nullptr) {
    return Size();
  }
  return current_animation->get_direction(current_direction).get_size();
}

/**
 * \brief Returns the maximum frame size of the animation set of this sprite.
 * \return The maximum frame size.
 */
const Size& Sprite::get_max_size() const {
  return animation_set.get_max_size();
}

/**
 * \brief Returns the origin point of a frame for the current animation and
 * the current direction.
 * \return The origin point of a frame.
 */
Point Sprite::get_origin() const {

  if (current_animation == nullptr) {
    return Point();
  }

  return current_animation->get_direction(current_direction).get_origin();
}

/**
 * \brief Returns a rectangle big enough to contain a frame of any animation
 * and direction.
 *
 * The rectangle is anchored to the origin point of the sprite,
 * so it has has negative top and left coordinates.
 *
 * \return The maximum frame size.
 */
const Rectangle& Sprite::get_max_bounding_box() const {

  return animation_set.get_max_bounding_box();
}

/**
 * \brief Returns the frame delay of the current animation.
 *
 * A value of 0 (only for 1-frame animations) means that the
 * animation must continue to be drawn: in this case,
 * is_animation_finished() always returns false.
 *
 * \return The delay between two frames for the current animation
 * in milliseconds.
 */
uint32_t Sprite::get_frame_delay() const {
  return frame_delay;
}

/**
 * \brief Sets the frame delay of the current animation.
 *
 * A value of 0 (only for 1-frame animations) means that the
 * animation will continue to be drawn.
 *
 * \param frame_delay The delay between two frames for the current animation
 * in milliseconds.
 */
void Sprite::set_frame_delay(uint32_t frame_delay) {
  this->frame_delay = frame_delay;
}

/**
 * \brief Returns the date if the next frame change.
 * \return The next frame date or 0 if there is no frame change scheduled.
 */
uint32_t Sprite::get_next_frame_date() const {
  return next_frame_date;
}


/**
 * \brief Returns the next frame of the current frame.
 * \return The next frame of the current frame, or -1 if the animation is
 * finished.
 */
int Sprite::get_next_frame() const {

  if (current_animation == nullptr) {
    return -1;
  }

  return current_animation->get_next_frame(current_direction, current_frame);
}

/**
 * \brief Returns the current animation of the sprite.
 * \return the name of the current animation of the sprite
 */
const std::string& Sprite::get_current_animation() const {
  return current_animation_name;
}

/**
 * \brief Sets the current animation of the sprite.
 *
 * If the sprite is already playing another animation, this animation is interrupted.
 * If the sprite is already playing the same animation, nothing is done.
 *
 * \param animation_name name of the new animation of the sprite
 */
void Sprite::set_current_animation(const std::string& animation_name) {

  if (animation_name != this->current_animation_name || !is_animation_started()) {

    this->current_animation_name = animation_name;
    if (animation_set.has_animation(animation_name)) {
      this->current_animation = &animation_set.get_animation(animation_name);
      set_frame_delay(current_animation->get_frame_delay());
    }
    else {
      this->current_animation = nullptr;
    }

    int old_direction = this->current_direction;
    if (current_direction < 0
        || current_direction >= get_nb_directions()) {
      current_direction = 0;
    }

    set_current_frame(0, false);
    set_finished_callback(ScopedLuaRef());

    LuaContext* lua_context = get_lua_context();
    if (lua_context != nullptr) {
      lua_context->sprite_on_animation_changed(*this, current_animation_name);
      if (current_direction != old_direction) {
        lua_context->sprite_on_direction_changed(*this, current_animation_name, current_direction);
      }
      lua_context->sprite_on_frame_changed(*this, current_animation_name, 0);
    }
  }
}

/**
 * \brief Returns whether this sprite has an animation with the specified name.
 * \param animation_name an animation name
 * \return true if this animation exists
 */
bool Sprite::has_animation(const std::string& animation_name) const {
  return animation_set.has_animation(animation_name);
}

/**
 * \brief Returns the number of directions in the current animation of this
 * sprite.
 * \return The number of directions.
 */
int Sprite::get_nb_directions() const {

  if (current_animation == nullptr) {
    return 0;
  }
  return current_animation->get_nb_directions();
}

/**
 * \brief Returns the current direction of the sprite's animation.
 * \return the current direction
 */
int Sprite::get_current_direction() const {
  return current_direction;
}

/**
 * \brief Sets the current direction of the sprite's animation and restarts the animation.
 *
 * If the specified direction is the current direction, nothing is done.
 *
 * \param current_direction the current direction
 */
void Sprite::set_current_direction(int current_direction) {

  if (current_direction != this->current_direction) {

    if (current_direction < 0
        || current_direction >= get_nb_directions()) {
      std::ostringstream oss;
      oss << "Illegal direction " << current_direction
          << " for sprite '" << get_animation_set_id()
          << "' in animation '" << current_animation_name << "'";
      Debug::error(oss.str());
      return;
    }

    this->current_direction = current_direction;

    set_current_frame(0, false);

    LuaContext* lua_context = get_lua_context();
    if (lua_context != nullptr) {
      lua_context->sprite_on_direction_changed(*this, current_animation_name, current_direction);
      lua_context->sprite_on_frame_changed(*this, current_animation_name, 0);
    }
  }
}

/**
 * \brief Returns the number of frames in the current direction of the current
 * animation of this sprite.
 * \return The number of frames.
 */
int Sprite::get_nb_frames() const {

  if (current_animation == nullptr) {
    return 0;
  }
  return current_animation->get_direction(current_direction).get_nb_frames();
}

/**
 * \brief Returns the current frame of the sprite's animation.
 * \return the current frame
 */
int Sprite::get_current_frame() const {
  return current_frame;
}

/**
 * \brief Sets the current frame of the sprite's animation.
 *
 * If the animation was finished, it is restarted.
 * If the animation is suspended, it remains suspended
 * but the specified frame is drawn.
 *
 * \param current_frame The current frame.
 * \param notify_script \c true to notify the Lua sprite if any.
 * Don't set notify_script to \c false unless you do the notification
 * yourself later.
 */
void Sprite::set_current_frame(int current_frame, bool notify_script) {

  finished = false;
  next_frame_date = System::now() + get_frame_delay();

  if (current_frame != this->current_frame) {
    this->current_frame = current_frame;
    set_frame_changed(true);

    if (notify_script) {
      LuaContext* lua_context = get_lua_context();
      if (lua_context != nullptr) {
        lua_context->sprite_on_frame_changed(
            *this, current_animation_name, current_frame);
      }
    }
  }
}

/**
 * \brief Returns the rectangle of the current frame.
 * \return The current frame's rectangle.
 */
Rectangle Sprite::get_current_frame_rectangle() const {

  if (current_animation == nullptr) {
    return Rectangle();
  }

  return current_animation->get_direction(current_direction).get_frame(current_frame);
}

/**
 * \brief Returns whether the frame of this sprite has just changed.
 * \return true if the frame of this sprite has just changed.
 */
bool Sprite::has_frame_changed() const {
  return frame_changed;
}

/**
 * \brief Sets whether the frame has just changed.
 * \param frame_changed true if the frame has just changed.
 */
void Sprite::set_frame_changed(bool frame_changed) {

  this->frame_changed = frame_changed;
}

/**
 * \brief Makes this sprite always synchronized with another one as soon as
 * they have the same animation name.
 * \param other the sprite to synchronize to, or nullptr to stop any previous synchronization
 */
void Sprite::set_synchronized_to(const SpritePtr& other) {
  this->synchronize_to = other;
}

/**
 * \brief Returns true if the animation is started.
 *
 * It can be suspended.
 *
 * \return true if the animation is started, false otherwise
 */
bool Sprite::is_animation_started() const {
  return !is_animation_finished();
}

/**
 * \brief Starts the animation.
 */
void Sprite::start_animation() {
  restart_animation();
}

/**
 * \brief Restarts the animation.
 */
void Sprite::restart_animation() {
  set_current_frame(0);
  set_paused(false);
}

/**
 * \brief Stops the animation.
 */
void Sprite::stop_animation() {
  finished = true;
}

/**
 * \brief Suspends or resumes the animation.
 *
 * Nothing is done if the parameter specified does not change.
 *
 * \param suspended true to suspend the animation, false to resume it
 */
void Sprite::set_suspended(bool suspended) {

  if (suspended != is_suspended() &&
      !ignore_suspend) {

    Drawable::set_suspended(suspended);

    // compte next_frame_date if the animation is being resumed
    if (!suspended) {
      uint32_t now = System::now();
      next_frame_date = now + get_frame_delay();
      blink_next_change_date = now;
    }
    else {
      blink_is_sprite_visible = true;
    }
  }
}

/**
 * \brief Returns whether this sprite keeps playing when the game is suspended.
 * \return \c true if the sprite continues its animation even when the game is
 * suspended.
 */
bool Sprite::get_ignore_suspend() const {
  return ignore_suspend;
}

/**
 * \brief Sets whether this sprite should keep playing its animation when the game is suspended.
 *
 * This will ignore subsequent calls to set_suspended().
 *
 * \param ignore_suspend true to make the sprite continue its animation even
 * when the game is suspended
 */
void Sprite::set_ignore_suspend(bool ignore_suspend) {
  set_suspended(false);
  this->ignore_suspend = ignore_suspend;
}

/**
 * \brief Returns true if the animation is paused.
 * \return true if the animation is paused, false otherwise
 */
bool Sprite::is_paused() const {
  return paused;
}

/**
 * \brief Pauses or resumes the animation.
 *
 * Nothing is done if the parameter specified does not change.
 *
 * \param paused true to pause the animation, false to resume it
 */
void Sprite::set_paused(bool paused) {

  if (paused != this->paused) {
    this->paused = paused;

    // compte next_frame_date if the animation is being resumed
    if (!paused) {
      uint32_t now = System::now();
      next_frame_date = now + get_frame_delay();
      blink_next_change_date = now;
    }
    else {
      blink_is_sprite_visible = true;
    }
  }
}

/**
 * \brief Returns true if the animation is looping.
 * \return true if the animation is looping
 */
bool Sprite::is_animation_looping() const {

  if (current_animation == nullptr) {
    return false;
  }
  return current_animation->is_looping();
}

/**
 * \brief Returns true if the animation is finished.
 *
 * The animation is finished after the last frame is reached
 * and if the frame delay is not zero (a frame delay
 * of zero should be used only for 1-frame animations).
 *
 * \return true if the animation is finished
 */
bool Sprite::is_animation_finished() const {
  return finished;
}

/**
 * \brief Returns true if the last frame is reached.
 * \return true if the last frame is reached
 */
bool Sprite::is_last_frame_reached() const {

  return get_current_frame() == get_nb_frames() - 1;
}

/**
 * \brief Returns whether the sprite is blinking.
 * \return true if the sprite is blinking
 */
bool Sprite::is_blinking() const {
  return blink_delay != 0;
}

/**
 * \brief Sets the blink delay of this sprite.
 * \param blink_delay Blink delay of the sprite in milliseconds,
 * or zero to stop blinking.
 */
void Sprite::set_blinking(uint32_t blink_delay) {
  this->blink_delay = blink_delay;

  if (blink_delay > 0) {
    blink_is_sprite_visible = false;
    blink_next_change_date = System::now();
  }
}

/**
 * \brief Tests whether this sprite's pixels are overlapping another sprite.
 * \param other Another sprite.
 * \param x1 X coordinate of this sprite's origin point on the map,
 * before applying its current movement if any.
 * \param y1 Y coordinate of this sprite's origin point on the map,
 * before applying its current movement if any.
 * \param x2 X coordinate of the other sprite's origin point on the map,
 * before applying its current movement if any.
 * \param y2 Y coordinate of the other sprite's origin point on the map,
 * before applying its current movement if any.
 * \return \c true if the sprites are overlapping.
 */
bool Sprite::test_collision(const Sprite& other, int x1, int y1, int x2, int y2) const {

  if (current_animation == nullptr || other.current_animation == nullptr) {
    return false;
  }

  if (!is_animation_started() || !other.is_animation_started()) {
    // The animation is not running.
    return false;
  }

  if (!are_pixel_collisions_enabled()) {
    Debug::error(std::string("Pixel-precise collisions are not enabled for sprite '") +
                 get_animation_set_id() + "'");
    return false;
  }

  if (!other.are_pixel_collisions_enabled()) {
    Debug::error(std::string("Pixel-precise collisions are not enabled for sprite '") +
                 other.get_animation_set_id() + "'");
    return false;
  }

  const SpriteAnimationDirection& direction1 = current_animation->get_direction(current_direction);
  const Point& origin1 = direction1.get_origin();
  Point location1 = { x1 - origin1.x, y1 - origin1.y };
  location1 += get_xy();
  const PixelBits& pixel_bits1 = direction1.get_pixel_bits(current_frame);

  const SpriteAnimationDirection& direction2 = other.current_animation->get_direction(other.current_direction);
  const Point& origin2 = direction2.get_origin();
  Point location2 = { x2 - origin2.x, y2 - origin2.y };
  location2 += other.get_xy();
  const PixelBits& pixel_bits2 = direction2.get_pixel_bits(other.current_frame);

  return pixel_bits1.test_collision(pixel_bits2,
                                    Transform(location1,get_origin(),get_scale(),get_rotation()),
                                    Transform(location2,other.get_origin(),other.get_scale(),other.get_rotation()));
}

/**
 * \brief Checks whether the frame has to be changed.
 *
 * If the frame changes, next_frame_date is updated.
 */
void Sprite::update() {

  Drawable::update();

  if (is_suspended() || paused) {
    return;
  }

  LuaContext* lua_context = get_lua_context();

  frame_changed = false;
  uint32_t now = System::now();

  // Update the current frame.
  if (synchronize_to == nullptr
      || current_animation_name != synchronize_to->get_current_animation()
      || synchronize_to->get_current_direction() > get_nb_directions()
      || synchronize_to->get_current_frame() > get_nb_frames()) {

    // Update frames normally (with time).
    while (!finished &&
        !is_suspended() &&
        !paused &&
        get_frame_delay() > 0 &&
        now >= next_frame_date
    ) {
      int next_frame = get_next_frame();

      // Test if the animation is finished.
      if (next_frame == -1) {
        finished = true;
        notify_finished();
      }
      else {
        current_frame = next_frame;
        uint32_t old_next_frame_date = next_frame_date;
        next_frame_date += get_frame_delay();
        if (next_frame_date < old_next_frame_date) {
          // Overflow. Can happen with data files generated with the editor
          // before 1.4 (frame_delay is too big).
          next_frame_date = std::numeric_limits<uint32_t>::max();
        }
      }
      set_frame_changed(true);

      if (lua_context != nullptr) {
        lua_context->sprite_on_frame_changed(*this, current_animation_name, current_frame);
      }
    }
  }
  else {
    // Take the same frame as the other sprite.
    if (synchronize_to->is_animation_finished()) {
      finished = true;
      notify_finished();
    }
    else {
      int other_frame = synchronize_to->get_current_frame();
      if (other_frame != current_frame) {
        current_frame = other_frame;
        next_frame_date = now + get_frame_delay();
        set_frame_changed(true);

        if (lua_context != nullptr) {
          lua_context->sprite_on_frame_changed(*this, current_animation_name, current_frame);
        }
      }
    }
  }

  // Update the special effects.
  if (is_blinking()) {
    // The sprite is blinking.

    while (now >= blink_next_change_date) {
      blink_is_sprite_visible = !blink_is_sprite_visible;
      blink_next_change_date += blink_delay;
    }
  }
}

/**
 * @brief Sprite::draw_intermediate
 * @param region
 * @param dst_surface
 * @param dst_position
 */
/*void Sprite::draw_intermediate() const {
    get_intermediate_surface().clear();
    current_animation->draw(
        get_intermediate_surface(),
        get_origin(),
        current_direction,
        current_frame);
}*/

/**
 * \brief Draws the sprite on a surface, with its current animation,
 * direction and frame.
 * \param dst_surface The destination surface.
 * \param infos draw infos, region is ignored in this case
 */
void Sprite::raw_draw(Surface& dst_surface,const DrawInfos& infos) const {

  if (current_animation == nullptr) {
    return;
  }

  if (!is_animation_finished()
      && (blink_delay == 0 || blink_is_sprite_visible)) {

    current_animation->draw(
          dst_surface,
          infos.dst_position,
          current_direction,
          current_frame,
          infos);
  }
}

/**
 * @brief compute region of the spritesheet to draw given crop region
 * @param region
 * @return clipped region
 */
Rectangle Sprite::clamp_region(const Rectangle& region) const {
  Rectangle src_position(region);
  const Point& origin = get_origin();
  src_position.add_xy(origin);
  const Size& frame_size = get_size();
  if (src_position.get_x() < 0) {
    src_position.set_width(src_position.get_width() + src_position.get_x());
    src_position.set_x(0);
  }
  if (src_position.get_x() + src_position.get_width() > frame_size.width) {
    src_position.set_width(frame_size.width - src_position.get_x());
  }
  if (src_position.get_y() < 0) {
    src_position.set_height(src_position.get_height() + src_position.get_y());
    src_position.set_y(0);
  }
  if (src_position.get_y() + src_position.get_height() > frame_size.height) {
    src_position.set_height(frame_size.height - src_position.get_y());
  }
  return src_position;
}

/**
 * \brief Draws a subrectangle of the current frame of this sprite.
 * \param region The subrectangle to draw, relative to the origin point.
 * It may be bigger than the frame: in this case it will be clipped.
 * \param dst_surface The destination surface.
 * \param dst_position Coordinates on the destination surface.
 * The origin point of the sprite will appear at these coordinates.
 */
void Sprite::raw_draw_region(Surface& dst_surface, const DrawInfos& infos) const {
  if (current_animation == nullptr) {
    return;
  }

  if (!is_animation_finished()
      && (blink_delay == 0 || blink_is_sprite_visible)) {

    //draw_intermediate();

    // If the region is bigger than the current frame, clip it.
    // Otherwise, more than the current frame could be visible.
    Rectangle src_position = clamp_region(infos.region);


    struct CropProxy : DrawProxy {
      CropProxy(const Rectangle& r, const Point& p) : crop_region(r), origin(p) {}
      void draw(Surface& dst_surface, const Surface& src_surface, const DrawInfos& infos) const override {
        //Compute area to keep in sprite sheet
        Rectangle crop_win = Rectangle(crop_region.get_xy()+infos.region.get_xy(),crop_region.get_bottom_right()+infos.region.get_xy());

        //Adapt destination
        Point dest = infos.dst_position+crop_region.get_xy();

        //Use given Proxy to draw result
        //infos.proxy.draw(dst_surface,src_surface,infos);
        infos.proxy.draw(dst_surface,src_surface,DrawInfos(infos,crop_win&infos.region,dest));
      }
      const Rectangle& crop_region;
      const Point origin;
    };

    //Instantiate crop proxy with current draw parameters
    CropProxy cropProxy{
      src_position,
      get_origin()
    };

    current_animation->draw(
          dst_surface,
          infos.dst_position,
          current_direction,
          current_frame,
          DrawInfos(infos,DrawProxyChain<2>(DrawProxyChain<2>::Proxies{{cropProxy,infos.proxy}})));
  }
}

Rectangle Sprite::get_region() const {
  return Rectangle(-get_origin(),get_size());
}

/**
 * \brief Draws a transition effect on this drawable object.
 * \param transition The transition effect to apply.
 */
/*void Sprite::draw_transition(Transition& transition) {
  //TODO reinvent that
  //transition.draw(get_intermediate_surface());
}*/

/**
 * \brief Returns the surface where transitions on this drawable object
 * are applied.
 * \return The surface for transitions.
 */
/*Surface& Sprite::get_transition_surface() {
  //TODO meh
  return get_intermediate_surface();
}*/

/**
 * \brief Returns the intermediate surface used for transitions and other
 * effects for this sprite.
 *
 * Creates this intermediate surface if it does not exist yet.
 *
 * \return The intermediate surface of this sprite.
 */
Surface& Sprite::get_intermediate_surface() const {

  if (intermediate_surface == nullptr) {
    intermediate_surface = Surface::create(get_max_size(),true);
  }
  return *intermediate_surface;
}

/**
 * \brief Returns the Lua registry ref to what to do when the current
 * animation finishes.
 * \return A Lua ref to a function or string (the name of an animation),
 * or an empty ref.
 */
const ScopedLuaRef& Sprite::get_finished_callback() const {
  return finished_callback_ref;
}

/**
 * \brief Sets what to do when the current animation finishes.
 * \param finished_callback_ref A Lua ref to a function or string
 * (the name of an animation), or an empty ref.
 */
void Sprite::set_finished_callback(const ScopedLuaRef& finished_callback_ref) {

  if (!finished_callback_ref.is_empty()) {
    Debug::check_assertion(get_lua_context() != nullptr, "Undefined Lua context");
  }

  this->finished_callback_ref = finished_callback_ref;
}

/**
 * \brief Returns the name identifying this type in Lua.
 * \return the name identifying this type in Lua
 */
const std::string& Sprite::get_lua_type_name() const {
  return LuaContext::sprite_module_name;
}

/**
 * \brief Performs appropriate notifications when the current animation finishes.
 */
void Sprite::notify_finished() {

  LuaContext* lua_context = get_lua_context();
  if (lua_context != nullptr) {
    lua_State* l = lua_context->get_internal_state();

    // Sprite callback.
    if (!finished_callback_ref.is_empty()) {
      // The callback may be a function or a string.
      finished_callback_ref.push(l);
      finished_callback_ref.clear();
      if (lua_isstring(l, -1)) {
        // Name of a next animation to set.
        std::string animation = lua_tostring(l, -1);
        lua_pop(l, 1);
        set_current_animation(animation);
      }
      else {
        // Function to call.
        LuaTools::call_function(l, 0, 0, "sprite callback");
      }
    }

    // Sprite event.
    lua_context->sprite_on_animation_finished(*this, current_animation_name);
  }

}

}

