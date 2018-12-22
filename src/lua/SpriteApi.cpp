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
#include "solarus/core/CurrentQuest.h"
#include "solarus/graphics/Sprite.h"
#include "solarus/graphics/SpriteAnimationSet.h"
#include "solarus/graphics/SpriteAnimation.h"
#include "solarus/lua/LuaContext.h"
#include "solarus/lua/LuaTools.h"
#include <sstream>

namespace Solarus {

/**
 * Name of the Lua table representing the sprite module.
 */
const std::string LuaContext::sprite_module_name = "sol.sprite";

/**
 * \brief Initializes the sprite features provided to Lua.
 */
void LuaContext::register_sprite_module() {

  const std::vector<luaL_Reg> functions = {
      { "create", sprite_api_create }
  };

  std::vector<luaL_Reg> methods = {
      { "get_animation_set", sprite_api_get_animation_set },
      { "has_animation", sprite_api_has_animation },
      { "get_animation", sprite_api_get_animation },
      { "set_animation", sprite_api_set_animation },
      { "get_direction", sprite_api_get_direction },
      { "set_direction", sprite_api_set_direction },
      { "get_num_directions", sprite_api_get_num_directions },
      { "get_frame", sprite_api_get_frame },
      { "set_frame", sprite_api_set_frame },
      { "get_num_frames", sprite_api_get_num_frames },
      { "get_frame_delay", sprite_api_get_frame_delay },
      { "set_frame_delay", sprite_api_set_frame_delay },
      { "get_size", sprite_api_get_size },
      { "get_origin", sprite_api_get_origin },
      { "is_paused", sprite_api_is_paused },
      { "set_paused", sprite_api_set_paused },
      { "get_ignore_suspend", sprite_api_get_ignore_suspend },
      { "set_ignore_suspend", sprite_api_set_ignore_suspend },
      { "synchronize", sprite_api_synchronize },
      { "draw", drawable_api_draw },
      { "draw_region", drawable_api_draw_region },
      { "get_blend_mode", drawable_api_get_blend_mode },
      { "set_blend_mode", drawable_api_set_blend_mode },
      { "fade_in", drawable_api_fade_in },
      { "fade_out", drawable_api_fade_out },
      { "get_xy", drawable_api_get_xy },
      { "set_xy", drawable_api_set_xy },
      { "get_movement", drawable_api_get_movement },
      { "stop_movement", drawable_api_stop_movement },
  };

  if (CurrentQuest::is_format_at_least({ 1, 6 })) {
    methods.insert(methods.end(), {
      { "stop_animation", sprite_api_stop_animation },
      { "is_animation_started", sprite_api_is_animation_started },
      { "set_shader", drawable_api_set_shader },
      { "get_shader", drawable_api_get_shader },
      { "set_color_modulation", drawable_api_set_color_modulation },
      { "get_color_modulation", drawable_api_get_color_modulation },
      { "get_opacity", drawable_api_get_opacity },
      { "set_opacity", drawable_api_set_opacity },
      { "get_frame_src_xy", sprite_api_get_frame_src_xy },
      { "set_rotation", drawable_api_set_rotation },
      { "get_rotation", drawable_api_get_rotation },
      { "set_scale", drawable_api_set_scale },
      { "get_scale", drawable_api_get_scale },
      { "set_transformation_origin", drawable_api_set_transformation_origin },
      { "get_transformation_origin", drawable_api_get_transformation_origin },
    });
  }

  const std::vector<luaL_Reg> metamethods = {
      { "__gc", drawable_meta_gc },
      { "__newindex", userdata_meta_newindex_as_table },
      { "__index", userdata_meta_index_as_table },
  };
  register_type(sprite_module_name, functions, methods, metamethods);
}

/**
 * \brief Returns whether a value is a userdata of type sprite.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return \c true if the value at this index is a sprite.
 */
bool LuaContext::is_sprite(lua_State* l, int index) {
  return is_userdata(l, index, sprite_module_name);
}

/**
 * \brief Checks that the userdata at the specified index of the stack is a
 * sprite and returns it.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return The sprite.
 */
SpritePtr LuaContext::check_sprite(lua_State* l, int index) {
  return std::static_pointer_cast<Sprite>(check_userdata(
      l, index, sprite_module_name
  ));
}

/**
 * \brief Pushes a sprite userdata onto the stack.
 * \param l A Lua context.
 * \param sprite A sprite.
 */
void LuaContext::push_sprite(lua_State* l, Sprite& sprite) {

  push_userdata(l, sprite);
}

/**
 * \brief Implementation of sol.sprite.create().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::sprite_api_create(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const std::string& animation_set_id = LuaTools::check_string(l, 1);

    // TODO if the file does not exist, make a Lua error instead of an assertion error.
    SpritePtr sprite = std::make_shared<Sprite>(animation_set_id);
    get().add_drawable(sprite);

    push_sprite(l, *sprite);
    return 1;
  });
}

/**
 * \brief Implementation of sprite:get_animation_set().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::sprite_api_get_animation_set(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const Sprite& sprite = *check_sprite(l, 1);

    const std::string& animation_set_id = sprite.get_animation_set_id();

    push_string(l, animation_set_id);
    return 1;
  });
}

/**
 * \brief Implementation of sprite:has_animation().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::sprite_api_has_animation(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const Sprite& sprite = *check_sprite(l, 1);
    const std::string& animation_name = LuaTools::check_string(l, 2);

    lua_pushboolean(l, sprite.has_animation(animation_name));
    return 1;
  });
}

/**
 * \brief Implementation of sprite:get_animation().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::sprite_api_get_animation(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const Sprite& sprite = *check_sprite(l, 1);

    const std::string& animation_name = sprite.get_current_animation();

    push_string(l, animation_name);
    return 1;
  });
}

/**
 * \brief Implementation of sprite:set_animation().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::sprite_api_set_animation(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Sprite& sprite = *check_sprite(l, 1);
    const std::string& animation_name = LuaTools::check_string(l, 2);
    ScopedLuaRef callback_ref;
    if (lua_gettop(l) >= 3) {
      if (!lua_isfunction(l, 3) && !lua_isstring(l, 3)) {
        LuaTools::type_error(l, 3, "function or string");
      }
      callback_ref = LuaTools::create_ref(l, 3);
    }

    if (!sprite.has_animation(animation_name)) {
      LuaTools::arg_error(l, 2,
          std::string("Animation '") + animation_name
          + "' does not exist in sprite '" + sprite.get_animation_set_id() + "'"
      );
    }

    sprite.set_current_animation(animation_name);
    sprite.set_finished_callback(callback_ref);
    sprite.restart_animation();

    return 0;
  });
}

/**
 * \brief Implementation of sprite:stop_animation().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::sprite_api_stop_animation(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Sprite& sprite = *check_sprite(l, 1);

    sprite.stop_animation();

    return 0;
  });
}

/**
 * \brief Implementation of sprite:is_animation_started().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::sprite_api_is_animation_started(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const Sprite& sprite = *check_sprite(l, 1);

    lua_pushboolean(l, sprite.is_animation_started());
    return 1;
  });
}

/**
 * \brief Implementation of sprite:get_direction().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::sprite_api_get_direction(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const Sprite& sprite = *check_sprite(l, 1);

    lua_pushinteger(l, sprite.get_current_direction());
    return 1;
  });
}

/**
 * \brief Implementation of sprite:set_direction().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::sprite_api_set_direction(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Sprite& sprite = *check_sprite(l, 1);
    int direction = LuaTools::check_int(l, 2);

    if (direction < 0 || direction >= sprite.get_nb_directions()) {
      std::ostringstream oss;
      oss << "Illegal direction " << direction
          << " for sprite '" + sprite.get_animation_set_id()
          << "' in animation '" + sprite.get_current_animation() + "'";
      LuaTools::arg_error(l, 2, oss.str());
    }
    sprite.set_current_direction(direction);

    return 0;
  });
}

/**
 * \brief Implementation of sprite:get_num_directions().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::sprite_api_get_num_directions(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const Sprite& sprite = *check_sprite(l, 1);
    const std::string& animation_name = LuaTools::opt_string(l, 2, sprite.get_current_animation());
    if (!sprite.has_animation(animation_name)) {
      LuaTools::arg_error(l, 2,
          std::string("Animation '") + animation_name
          + "' does not exist in sprite '" + sprite.get_animation_set_id() + "'"
      );
    }

    const int num_directions = sprite.get_animation_set().get_animation(animation_name).get_nb_directions();

    lua_pushinteger(l, num_directions);
    return 1;
  });
}

/**
 * \brief Implementation of sprite:get_frame().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::sprite_api_get_frame(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const Sprite& sprite = *check_sprite(l, 1);

    lua_pushinteger(l, sprite.get_current_frame());
    return 1;
  });
}

/**
 * \brief Implementation of sprite:set_frame().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::sprite_api_set_frame(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Sprite& sprite = *check_sprite(l, 1);
    int frame = LuaTools::check_int(l, 2);

    if (frame < 0 || frame >= sprite.get_nb_frames()) {
      std::ostringstream oss;
      oss << "Illegal frame " << frame
          << " for sprite '" << sprite.get_animation_set_id()
          << "' in direction " << sprite.get_current_direction()
          << " of animation '" << sprite.get_current_animation() << "'";
      LuaTools::arg_error(l, 2, oss.str());
    }
    sprite.set_current_frame(frame);

    return 0;
  });
}

/**
 * \brief Implementation of sprite:get_num_frames().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::sprite_api_get_num_frames(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const Sprite& sprite = *check_sprite(l, 1);
    std::string animation_name = sprite.get_current_animation();
    int direction = sprite.get_current_direction();
    if (CurrentQuest::is_format_at_least({ 1, 6 })) {
      if (lua_gettop(l) > 1) {
        animation_name = LuaTools::check_string(l, 2);
        direction = LuaTools::check_int(l, 3);
      }
    }

    if (!sprite.has_animation(animation_name)) {
      LuaTools::arg_error(l, 2,
          std::string("Animation '") + animation_name +
          "' does not exist in sprite '" + sprite.get_animation_set_id() + "'"
      );
    }

    const SpriteAnimation& animation = sprite.get_animation_set().get_animation(animation_name);
    if (direction < 0 || direction >= animation.get_nb_directions()) {
      LuaTools::arg_error(l, 2,
          std::string("Illegal direction '") + lua_tostring(l, 2) + " in animation '" + animation_name +
          "' of sprite '" + sprite.get_animation_set_id() + "'"
      );
    }

    lua_pushinteger(l, animation.get_direction(direction).get_nb_frames());
    return 1;
  });
}

/**
 * \brief Implementation of sprite:get_frame_delay().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::sprite_api_get_frame_delay(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const Sprite& sprite = *check_sprite(l, 1);
    std::string animation_name = sprite.get_current_animation();

    if (CurrentQuest::is_format_at_least({ 1, 6 })) {
      animation_name = LuaTools::opt_string(l, 2, sprite.get_current_animation());
    }
    if (!sprite.has_animation(animation_name)) {
      LuaTools::arg_error(l, 2,
          std::string("Animation '") + animation_name
          + "' does not exist in sprite '" + sprite.get_animation_set_id() + "'"
      );
    }

    const SpriteAnimation& animation = sprite.get_animation_set().get_animation(animation_name);
    uint32_t frame_delay = animation.get_frame_delay();
    if (frame_delay == 0) {
      lua_pushnil(l);
    }
    else {
      lua_pushinteger(l, frame_delay);
    }

    return 1;
  });
}

/**
 * \brief Implementation of sprite:set_frame_delay().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::sprite_api_set_frame_delay(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Sprite& sprite = *check_sprite(l, 1);
    uint32_t delay = 0;
    if (!lua_isnil(l, 2)) {
      delay = uint32_t(LuaTools::check_int(l, 2));
    }

    sprite.set_frame_delay(delay);

    return 0;
  });
}

/**
 * \brief Implementation of sprite:get_size().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::sprite_api_get_size(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const Sprite& sprite = *check_sprite(l, 1);
    std::string animation_name = sprite.get_current_animation();
    int direction = sprite.get_current_direction();
    if (CurrentQuest::is_format_at_least({ 1, 6 })) {
      if (lua_gettop(l) > 1) {
        animation_name = LuaTools::check_string(l, 2);
        direction = LuaTools::check_int(l, 3);
      }
    }

    if (!sprite.has_animation(animation_name)) {
      LuaTools::arg_error(l, 2,
          std::string("Animation '") + animation_name +
          "' does not exist in sprite '" + sprite.get_animation_set_id() + "'"
      );
    }

    const SpriteAnimation& animation = sprite.get_animation_set().get_animation(animation_name);
    if (direction < 0 || direction >= animation.get_nb_directions()) {
      LuaTools::arg_error(l, 2,
          std::string("Illegal direction '") + lua_tostring(l, 2) + " in animation '" + animation_name +
          "' of sprite '" + sprite.get_animation_set_id() + "'"
      );
    }

    const Size& size = animation.get_direction(direction).get_size();

    lua_pushinteger(l, size.width);
    lua_pushinteger(l, size.height);
    return 2;
  });
}

/**
 * \brief Implementation of sprite:get_origin().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::sprite_api_get_origin(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const Sprite& sprite = *check_sprite(l, 1);
    std::string animation_name = sprite.get_current_animation();
    int direction = sprite.get_current_direction();
    if (CurrentQuest::is_format_at_least({ 1, 6 })) {
      if (lua_gettop(l) > 1) {
        animation_name = LuaTools::check_string(l, 2);
        direction = LuaTools::check_int(l, 3);
      }
    }

    if (!sprite.has_animation(animation_name)) {
      LuaTools::arg_error(l, 2,
          std::string("Animation '") + animation_name +
          "' does not exist in sprite '" + sprite.get_animation_set_id() + "'"
      );
    }

    const SpriteAnimation& animation = sprite.get_animation_set().get_animation(animation_name);
    if (direction < 0 || direction >= animation.get_nb_directions()) {
      LuaTools::arg_error(l, 2,
          std::string("Illegal direction '") + lua_tostring(l, 2) + " in animation '" + animation_name +
          "' of sprite '" + sprite.get_animation_set_id() + "'"
      );
    }

    const Point& origin = animation.get_direction(direction).get_origin();

    lua_pushinteger(l, origin.x);
    lua_pushinteger(l, origin.y);
    return 2;
  });
}

/**
 * \brief Implementation of sprite:get_frame_src_xy().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::sprite_api_get_frame_src_xy(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const Sprite& sprite = *check_sprite(l, 1);
    std::string animation_name = sprite.get_current_animation();
    int direction = sprite.get_current_direction();
    int frame = sprite.get_current_frame();
    if (lua_gettop(l) > 1) {
      animation_name = LuaTools::check_string(l, 2);
      direction = LuaTools::check_int(l, 3);
      frame = LuaTools::check_int(l, 4);
    }

    if (!sprite.has_animation(animation_name)) {
      LuaTools::arg_error(l, 2,
          std::string("Animation '") + animation_name +
          "' does not exist in sprite '" + sprite.get_animation_set_id() + "'"
      );
    }

    const SpriteAnimation& animation = sprite.get_animation_set().get_animation(animation_name);
    if (direction < 0 || direction >= animation.get_nb_directions()) {
      LuaTools::arg_error(l, 2,
          std::string("Illegal direction '") + lua_tostring(l, 2) + " in animation '" + animation_name +
          "' of sprite '" + sprite.get_animation_set_id() + "'"
      );
    }

    if (frame < 0 || frame >= animation.get_direction(direction).get_nb_frames()) {
      LuaTools::arg_error(l, 2,
          std::string("Illegal frame '") + lua_tostring(l, 4) + " in animation '" + animation_name +
          "' and direction " + lua_tostring(l, 2) + " of sprite '" + sprite.get_animation_set_id() + "'"
      );
    }

    const Rectangle& box = animation.get_direction(direction).get_frame(frame);

    lua_pushinteger(l, box.get_x());
    lua_pushinteger(l, box.get_y());
    return 2;
  });
}

/**
 * \brief Implementation of sprite:is_paused().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::sprite_api_is_paused(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const Sprite& sprite = *check_sprite(l, 1);

    lua_pushboolean(l, sprite.is_paused());

    return 1;
  });
}

/**
 * \brief Implementation of sprite:set_paused().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::sprite_api_set_paused(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Sprite& sprite = *check_sprite(l, 1);
    bool paused = LuaTools::opt_boolean(l, 2, true);

    sprite.set_paused(paused);

    return 0;
  });
}

/**
 * \brief Implementation of sprite:get_ignore_suspend().
 * \param l the Lua context that is calling this function
 * \return number of values to return to Lua
 */
int LuaContext::sprite_api_get_ignore_suspend(lua_State* l) {

  return state_boundary_handle(l, [&] {
    std::shared_ptr<Sprite> sprite = check_sprite(l, 1);

    lua_pushboolean(l, sprite->get_ignore_suspend());
    return 1;
  });
}

/**
 * \brief Implementation of sprite:set_ignore_suspend().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::sprite_api_set_ignore_suspend(lua_State *l) {

  return state_boundary_handle(l, [&] {
    Sprite& sprite = *check_sprite(l, 1);
    bool ignore_suspend = LuaTools::opt_boolean(l, 2, true);

    sprite.set_ignore_suspend(ignore_suspend);

    return 0;
  });
}

/**
 * \brief Implementation of sprite:synchronize().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::sprite_api_synchronize(lua_State *l) {

  return state_boundary_handle(l, [&] {
    Sprite& sprite = *check_sprite(l, 1);

    if (!lua_isnil(l, 2)) {
      const SpritePtr& reference_sprite = check_sprite(l, 2);
      sprite.set_synchronized_to(reference_sprite);
    }
    else {
      sprite.set_synchronized_to(nullptr);
    }

    return 0;
  });
}

/**
 * \brief Calls the on_animation_finished() method of a Lua sprite.
 *
 * Does nothing if the method is not defined.
 *
 * \param sprite A sprite whose animation has just finished.
 * \param animation Name of the animation finished.
 */
void LuaContext::sprite_on_animation_finished(Sprite& sprite,
    const std::string& animation) {

  if (!userdata_has_field(sprite, "on_animation_finished")) {
    return;
  }
  run_on_main([this,&sprite,&animation](lua_State* l){
    push_sprite(l, sprite);
    on_animation_finished(animation);
    lua_pop(l, 1);
  });
}

/**
 * \brief Calls the on_animation_changed() method of a Lua sprite.
 *
 * Does nothing if the method is not defined.
 *
 * \param sprite A sprite whose animation has just changed.
 * \param animation Name of the new animation.
 */
void LuaContext::sprite_on_animation_changed(
    Sprite& sprite, const std::string& animation) {

  if (!userdata_has_field(sprite, "on_animation_changed")) {
    return;
  }

  run_on_main([this,&sprite,&animation](lua_State* l){
    push_sprite(l, sprite);
    on_animation_changed(animation);
    lua_pop(current_l, 1);
  });
}

/**
 * \brief Calls the on_direction_changed() method of a Lua sprite.
 *
 * Does nothing if the method is not defined.
 *
 * \param sprite A sprite whose direction has just changed.
 * \param animation Name of the current animation.
 * \param direction The new direction.
 */
void LuaContext::sprite_on_direction_changed(Sprite& sprite,
    const std::string& animation, int direction) {

  if (!userdata_has_field(sprite, "on_direction_changed")) {
    return;
  }
  run_on_main([this,&sprite,&animation,direction](lua_State* l){
    push_sprite(l, sprite);
    on_direction_changed(animation, direction);
    lua_pop(l, 1);
  });
}

/**
 * \brief Calls the on_frame_changed() method of a Lua sprite.
 *
 * Does nothing if the method is not defined.
 *
 * \param sprite A sprite whose frame has just changed.
 * \param animation Name of the current animation.
 * \param frame The new frame.
 */
void LuaContext::sprite_on_frame_changed(Sprite& sprite,
    const std::string& animation, int frame) {

  if (!userdata_has_field(sprite, "on_frame_changed")) {
    return;
  }
  run_on_main([this,&sprite,&animation,frame](lua_State* l){
    push_sprite(l, sprite);
    on_frame_changed(animation, frame);
    lua_pop(l, 1);
  });
}

}

