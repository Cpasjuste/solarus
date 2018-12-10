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
#include "solarus/graphics/BlendModeInfo.h"
#include "solarus/graphics/Drawable.h"
#include "solarus/graphics/Surface.h"
#include "solarus/graphics/TransitionFade.h"
#include "solarus/lua/ExportableToLuaPtr.h"
#include "solarus/lua/LuaContext.h"
#include "solarus/lua/LuaTools.h"
#include "solarus/movements/Movement.h"
#include <lua.hpp>

/* This file contains common code for all drawable types known by Lua,
 * i.e. surfaces, text surfaces and sprites.
 */

namespace Solarus {

/**
 * \brief Returns whether a value is a userdata of a type.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return true if the value at this index is a drawable.
 */
bool LuaContext::is_drawable(lua_State* l, int index) {
  return is_surface(l, index)
      || is_text_surface(l, index)
      || is_sprite(l, index);
}

/**
 * \brief Check that the userdata at the specified index is a drawable
 * object (surface, text surface of sprite) and returns it.
 * \param l a Lua context
 * \param index an index in the stack
 * \return The drawable.
 */
DrawablePtr LuaContext::check_drawable(lua_State* l, int index) {

  if (is_drawable(l, index)) {
    const ExportableToLuaPtr& userdata = *(static_cast<ExportableToLuaPtr*>(
      lua_touserdata(l, index)
    ));
    return std::static_pointer_cast<Drawable>(userdata);
  }
  else {
    LuaTools::type_error(l, index, "drawable");
    throw;
  }
}

/**
 * \brief Returns whether a drawable object was created by this script.
 * \param drawable A drawable object.
 * \return true if the drawable object was created by this script.
 */
bool LuaContext::has_drawable(const DrawablePtr& drawable) {

  return drawables.find(drawable) != drawables.end();
}

/**
 * \brief Registers a drawable object created by Lua.
 * \param drawable a drawable object
 */
void LuaContext::add_drawable(const DrawablePtr& drawable) {

  Debug::check_assertion(!has_drawable(drawable),
      "This drawable object is already registered");

  drawables.insert(drawable);
}

/**
 * \brief Unregisters a drawable object created by Lua.
 * \param drawable a drawable object
 */
void LuaContext::remove_drawable(const DrawablePtr& drawable) {

  Debug::check_assertion(has_drawable(drawable),
      "This drawable object was not created by Lua");

  drawables_to_remove.insert(drawable);
}

/**
 * \brief Cleanups all drawable objects registered.
 */
void LuaContext::destroy_drawables() {

  drawables.clear();
  drawables_to_remove.clear();
}

/**
 * \brief Updates all drawable objects created by this script.
 */
void LuaContext::update_drawables() {

  // Update all drawables.
  for (const DrawablePtr& drawable: drawables) {
    if (has_drawable(drawable)) {
      drawable->update();
    }
  }

  // Remove the ones that should be removed.
  for (const DrawablePtr& drawable: drawables_to_remove) {
    drawables.erase(drawable);
  }
  drawables_to_remove.clear();
}

/**
 * \brief Implementation of drawable:draw().
 * \param l the Lua context that is calling this function
 * \return number of values to return to Lua
 */
int LuaContext::drawable_api_draw(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Drawable& drawable = *check_drawable(l, 1);
    SurfacePtr dst_surface = check_surface(l, 2);
    int x = LuaTools::opt_int(l, 3, 0);
    int y = LuaTools::opt_int(l, 4, 0);
    drawable.draw(dst_surface, x, y);

    return 0;
  });
}

/**
 * \brief Implementation of drawable:draw_region().
 * \param l the Lua context that is calling this function
 * \return number of values to return to Lua
 */
int LuaContext::drawable_api_draw_region(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Drawable& drawable = *check_drawable(l, 1);
    Rectangle region = {
        LuaTools::check_int(l, 2),
        LuaTools::check_int(l, 3),
        LuaTools::check_int(l, 4),
        LuaTools::check_int(l, 5)
    };
    SurfacePtr dst_surface = check_surface(l, 6);
    Point dst_position = {
        LuaTools::opt_int(l, 7, 0),
        LuaTools::opt_int(l, 8, 0)
    };
    drawable.draw_region(region, dst_surface, dst_position);

    return 0;
  });
}

/**
 * \brief Implementation of drawable:get_blend_mode().
 * \param l the Lua context that is calling this function
 * \return Number of values to return to Lua.
 */
int LuaContext::drawable_api_get_blend_mode(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Drawable& drawable = *check_drawable(l, 1);

    BlendMode blend_mode = drawable.get_blend_mode();

    push_string(l, enum_to_name(blend_mode));

    return 1;
  });
}

/**
 * \brief Implementation of drawable:set_blend_mode().
 * \param l the Lua context that is calling this function
 * \return Number of values to return to Lua.
 */
int LuaContext::drawable_api_set_blend_mode(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Drawable& drawable = *check_drawable(l, 1);
    BlendMode blend_mode = LuaTools::check_enum<BlendMode>(l, 2);

    drawable.set_blend_mode(blend_mode);

    return 0;
  });
}

/**
 * \brief Implementation of drawable:set_shader().
 * \param l the Lua context that is calling this function
 * \return Number of values to return to Lua.
 */
int LuaContext::drawable_api_set_shader(lua_State* l) {
  return state_boundary_handle(l,[&]{
    Drawable& drawable = *check_drawable(l,1);
    ShaderPtr shader = nullptr;
    if (!lua_isnil(l, 2)) {
      if (is_shader(l, 2)) {
        shader = check_shader(l, 2);
      }
      else {
        LuaTools::type_error(l, 2, "shader or nil");
      }
    }
    drawable.set_shader(shader);
    return 0;
  });
}

/**
 * \brief Implementation of drawable:get_shader().
 * \param l the Lua context that is calling this function
 * \return Number of values to return to Lua.
 */
int LuaContext::drawable_api_get_shader(lua_State* l) {
  return state_boundary_handle(l,[&]{
    const Drawable& drawable = *check_drawable(l,1);
    const ShaderPtr& shader = drawable.get_shader();
    if(shader) {
      push_shader(l,*shader);
    } else {
      lua_pushnil(l);
    }
    return 1;
  });
}


/**
 * \brief Implementation of drawable:get_color_modulation().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::drawable_api_get_color_modulation(lua_State* l) {
  return state_boundary_handle(l, [&] {
    const Drawable& drawable = *check_drawable(l, 1);

    const Color& color = drawable.get_color_modulation();

    push_color(l,color);
    return 1;
  });
}

/**
 * \brief Implementation of drawable:set_color_modulation().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::drawable_api_set_color_modulation(lua_State* l) {
  return state_boundary_handle(l, [&] {
    Drawable& drawable = *check_drawable(l, 1);
    Color color = LuaTools::check_color(l,2);

    drawable.set_color_modulation(color);

    return 0;
  });
}


/**
 * \brief Implementation of drawable:get_opacity().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::drawable_api_get_opacity(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const Drawable& drawable = *check_drawable(l, 1);

    uint8_t opacity = drawable.get_opacity();

    lua_pushinteger(l, opacity);
    return 1;
  });
}

/**
 * \brief Implementation of drawable:set_opacity().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::drawable_api_set_opacity(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Drawable& drawable = *check_drawable(l, 1);
    uint8_t opacity = (uint8_t) LuaTools::check_int(l, 2);

    drawable.set_opacity(opacity);

    return 0;
  });
}

/**
 * \brief Implementation of drawable:fade_in().
 * \param l the Lua context that is calling this function
 * \return number of values to return to Lua
 */
int LuaContext::drawable_api_fade_in(lua_State* l) {

  return state_boundary_handle(l, [&] {
    uint32_t delay = 20;
    ScopedLuaRef callback_ref;

    Drawable& drawable = *check_drawable(l, 1);

    if (lua_gettop(l) >= 2) {
      // the second argument can be the delay or the callback
      int index = 2;
      if (lua_isnumber(l, index)) {
        delay = lua_tonumber(l, index);
        index++;
      }
      // the next argument (if any) is the callback
      callback_ref = LuaTools::opt_function(l, index);
    }

    TransitionFade* transition(new TransitionFade(
        Transition::Direction::OPENING
    ));
    transition->clear_color();
    transition->set_delay(delay);
    drawable.start_transition(
        std::unique_ptr<Transition>(transition),
        callback_ref
    );

    return 0;
  });
}

/**
 * \brief Implementation of drawable:fade_out().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::drawable_api_fade_out(lua_State* l) {

  return state_boundary_handle(l, [&] {
    uint32_t delay = 20;
    ScopedLuaRef callback_ref;

    Drawable& drawable = *check_drawable(l, 1);

    if (lua_gettop(l) >= 2) {
      // the second argument can be the delay or the callback
      int index = 2;
      if (lua_isnumber(l, index)) {
        delay = lua_tonumber(l, index);
        index++;
      }
      // the next argument (if any) is the callback
      callback_ref = LuaTools::opt_function(l, index);
    }

    TransitionFade* transition(new TransitionFade(
        Transition::Direction::CLOSING
    ));
    transition->clear_color();
    transition->set_delay(delay);
    drawable.start_transition(
        std::unique_ptr<Transition>(transition),
        callback_ref
    );

    return 0;
  });
}

/**
 * \brief Implementation of drawable:get_xy().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::drawable_api_get_xy(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Drawable& drawable = *check_drawable(l, 1);

    lua_pushinteger(l, drawable.get_xy().x);
    lua_pushinteger(l, drawable.get_xy().y);
    return 2;
  });
}

/**
 * \brief Implementation of drawable:set_xy().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::drawable_api_set_xy(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Drawable& drawable = *check_drawable(l, 1);
    int x = LuaTools::check_int(l, 2);
    int y = LuaTools::check_int(l, 3);

    drawable.set_xy(Point(x, y));

    return 0;
  });
}

/**
 * \brief Implementation of drawable:set_rotation().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::drawable_api_set_rotation(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Drawable& drawable = *check_drawable(l, 1);
    double rot = LuaTools::check_number(l, 2);

    drawable.set_rotation(rot);

    return 0;
  });
}

/**
 * \brief Implementation of drawable:get_rotation().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::drawable_api_get_rotation(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const Drawable& drawable = *check_drawable(l, 1);
    double rot = drawable.get_rotation();

    lua_pushnumber(l,rot);
    return 1;
  });
}

/**
 * \brief Implementation of drawable:set_scale().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::drawable_api_set_scale(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Drawable& drawable = *check_drawable(l, 1);
    double x = LuaTools::check_number(l, 2);
    double y = LuaTools::check_number(l, 3);

    drawable.set_scale(Scale(x, y));

    return 0;
  });
}

/**
 * \brief Implementation of drawable:get_scale().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::drawable_api_get_scale(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const Drawable& drawable = *check_drawable(l, 1);
    const Scale& s = drawable.get_scale();

    lua_pushnumber(l,s.x);
    lua_pushnumber(l,s.y);

    return 2;
  });
}

/**
 * \brief Implementation of drawable:set_transform_origin().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::drawable_api_set_transformation_origin(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Drawable& drawable = *check_drawable(l, 1);
    double x = LuaTools::check_number(l, 2);
    double y = LuaTools::check_number(l, 3);

    drawable.set_transformation_origin(Point(x,y));

    return 0;
  });
}

/**
 * \brief Implementation of drawable:get_transform_origin().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::drawable_api_get_transformation_origin(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const Drawable& drawable = *check_drawable(l, 1);
    const Point& o = drawable.get_transformation_origin();

    lua_pushnumber(l,o.x);
    lua_pushnumber(l,o.y);
    return 2;
  });
}

/**
 * \brief Implementation of drawable:get_movement().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::drawable_api_get_movement(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Drawable& drawable = *check_drawable(l, 1);
    std::shared_ptr<Movement> movement = drawable.get_movement();
    if (movement == nullptr) {
      lua_pushnil(l);
    }
    else {
      push_movement(l, *movement);
    }

    return 1;
  });
}

/**
 * \brief Implementation of drawable:stop_movement().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::drawable_api_stop_movement(lua_State* l) {

  return state_boundary_handle(l, [&] {
    Drawable& drawable = *check_drawable(l, 1);

    drawable.stop_movement();

    return 0;
  });
}

/**
 * \brief Finalizer of types sprite, surface and text surface.
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::drawable_meta_gc(lua_State* l) {

  return state_boundary_handle(l, [&] {
    LuaContext& lua_context = get();
    DrawablePtr drawable = check_drawable(l, 1);

    if (lua_context.has_drawable(drawable)) {
      // This drawable was created from Lua.
      lua_context.remove_drawable(drawable);
    }
    userdata_meta_gc(l);

    return 0;
  });
}

}

