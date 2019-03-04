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
#include "solarus/core/CurrentQuest.h"
#include "solarus/core/FontResource.h"
#include "solarus/core/QuestFiles.h"
#include "solarus/graphics/TextSurface.h"
#include "solarus/graphics/Surface.h"
#include "solarus/lua/LuaContext.h"
#include "solarus/lua/LuaTools.h"

namespace Solarus {

using RenderingMode = TextSurface::RenderingMode;
using HorizontalAlignment = TextSurface::HorizontalAlignment;
using VerticalAlignment = TextSurface::VerticalAlignment;

/**
 * Name of the Lua table representing the text surface module.
 */
const std::string LuaContext::text_surface_module_name = "sol.text_surface";

namespace {

const std::map<RenderingMode, std::string> rendering_mode_names = {
    { RenderingMode::SOLID, "solid" },
    { RenderingMode::ANTIALIASING, "antialiasing" }
};

const std::map<HorizontalAlignment, std::string> horizontal_alignment_names = {
    { HorizontalAlignment::LEFT, "left" },
    { HorizontalAlignment::CENTER, "center" },
    { HorizontalAlignment::RIGHT, "right" }
};

const std::map<VerticalAlignment, std::string> vertical_alignment_names = {
    { VerticalAlignment::TOP, "top" },
    { VerticalAlignment::MIDDLE, "middle" },
    { VerticalAlignment::BOTTOM, "bottom" }
};

}

/**
 * \brief Initializes the text surface features provided to Lua.
 */
void LuaContext::register_text_surface_module() {

  // Functions of sol.surface.
  std::vector<luaL_Reg> functions = {
      { "create", text_surface_api_create }
  };

  // Methods of the text_surface type.
  std::vector<luaL_Reg> methods = {
      { "get_horizontal_alignment", text_surface_api_get_horizontal_alignment },
      { "set_horizontal_alignment", text_surface_api_set_horizontal_alignment },
      { "get_vertical_alignment", text_surface_api_get_vertical_alignment },
      { "set_vertical_alignment", text_surface_api_set_vertical_alignment },
      { "get_font", text_surface_api_get_font },
      { "set_font", text_surface_api_set_font },
      { "get_rendering_mode", text_surface_api_get_rendering_mode },
      { "set_rendering_mode", text_surface_api_set_rendering_mode },
      { "get_color", text_surface_api_get_color },
      { "set_color", text_surface_api_set_color },
      { "get_font_size", text_surface_api_get_font_size },
      { "set_font_size", text_surface_api_set_font_size },
      { "get_text", text_surface_api_get_text },
      { "set_text", text_surface_api_set_text },
      { "set_text_key", text_surface_api_set_text_key },
      { "get_size", text_surface_api_get_size },
      { "draw", drawable_api_draw },
      { "draw_region", drawable_api_draw_region },
      { "get_blend_mode", drawable_api_get_blend_mode },
      { "set_blend_mode", drawable_api_set_blend_mode },
      { "fade_in", drawable_api_fade_in },
      { "fade_out", drawable_api_fade_out },
      { "get_xy", drawable_api_get_xy },
      { "set_xy", drawable_api_set_xy },
      { "get_movement", drawable_api_get_movement },
      { "stop_movement", drawable_api_stop_movement }
  };

  if (CurrentQuest::is_format_at_least({ 1, 6 })) {
    methods.insert(methods.end(), {
      { "set_shader", drawable_api_set_shader },
      { "get_shader", drawable_api_get_shader },
      { "set_color_modulation", drawable_api_set_color_modulation },
      { "get_color_modulation", drawable_api_get_color_modulation },
      { "get_opacity", drawable_api_get_opacity },
      { "set_opacity", drawable_api_set_opacity },
      { "set_rotation", drawable_api_set_rotation },
      { "get_rotation", drawable_api_get_rotation },
      { "set_scale", drawable_api_set_scale },
      { "get_scale", drawable_api_get_scale },
      { "set_transformation_origin", drawable_api_set_transformation_origin },
      { "get_transformation_origin", drawable_api_get_transformation_origin },
    });

    functions.insert(functions.end(),{
       { "get_predicted_size", text_surface_api_get_predicted_size}
    });
  }

  const std::vector<luaL_Reg> metamethods = {
      { "__gc", drawable_meta_gc }
  };

  register_type(text_surface_module_name, functions, methods, metamethods);
}

/**
 * \brief Returns whether a value is a userdata of type text surface.
 * \param l A Lua context.
 * \param index An index in the stack.
 * \return true if the value at this index is a text surface.
 */
bool LuaContext::is_text_surface(lua_State* l, int index) {
  return is_userdata(l, index, text_surface_module_name);
}

/**
 * \brief Checks that the userdata at the specified index of the stack is a
 * text surface and returns it.
 * \param l a Lua context
 * \param index an index in the stack
 * \return the text surface
 */
std::shared_ptr<TextSurface> LuaContext::check_text_surface(lua_State* l, int index) {
  return std::static_pointer_cast<TextSurface>(check_userdata(
      l, index, text_surface_module_name
  ));
}

/**
 * \brief Pushes a text surface userdata onto the stack.
 * \param l a Lua context
 * \param text_surface a text surface
 */
void LuaContext::push_text_surface(lua_State* l, TextSurface& text_surface) {
  push_userdata(l, text_surface);
}

/**
 * \brief Implementation of sol.text_surface.create().
 * \param l The Lua context that is calling this function.
 * \return Number of values to return to Lua.
 */
int LuaContext::text_surface_api_create(lua_State* l) {

  return state_boundary_handle(l, [&] {
    std::shared_ptr<TextSurface> text_surface =
        std::make_shared<TextSurface>(0, 0);

    if (lua_gettop(l) > 0) {
      LuaTools::check_type(l, 1, LUA_TTABLE);

      const std::string& font_id = LuaTools::opt_string_field(
          l, 1, "font", FontResource::get_default_font_id()
      );
      RenderingMode rendering_mode =
          LuaTools::opt_enum_field<RenderingMode>(
              l, 1, "rendering_mode", rendering_mode_names, RenderingMode::SOLID
          );
      HorizontalAlignment horizontal_alignment =
          LuaTools::opt_enum_field<HorizontalAlignment>(
              l, 1, "horizontal_alignment", horizontal_alignment_names, HorizontalAlignment::LEFT
          );
      VerticalAlignment vertical_alignment =
          LuaTools::opt_enum_field<VerticalAlignment>(
              l, 1, "vertical_alignment", vertical_alignment_names, VerticalAlignment::MIDDLE
          );
      const Color& color = LuaTools::opt_color_field(l, 1, "color", Color::white);
      int font_size = LuaTools::opt_int_field(l, 1, "font_size", TextSurface::default_font_size);
      const std::string& text = LuaTools::opt_string_field(l, 1, "text", "");
      const std::string& text_key = LuaTools::opt_string_field(l, 1, "text_key", "");

      if (!FontResource::exists(font_id)) {
        LuaTools::error(l, std::string("No such font: '") + font_id + "'");
      }
      text_surface->set_font(font_id);
      text_surface->set_rendering_mode(rendering_mode);
      text_surface->set_horizontal_alignment(horizontal_alignment);
      text_surface->set_vertical_alignment(vertical_alignment);
      text_surface->set_text_color(color);
      text_surface->set_font_size(font_size);

      if (!text.empty()) {
        text_surface->set_text(text);
      }
      else if (!text_key.empty()) {
          if (!CurrentQuest::string_exists(text_key)) {
            LuaTools::error(l, std::string("No value with key '") + text_key
                + "' in strings.dat for language '"
                + CurrentQuest::get_language() + "'"
            );
          }
          text_surface->set_text(CurrentQuest::get_string(text_key));
      }
    }
    get().add_drawable(text_surface);

    push_text_surface(l, *text_surface);
    return 1;
  });
}

/**
 * @brief Implementation of text_surface.get_predicted_size()
 * @param l the lua state
 * @return number of values to return to lua
 */
int LuaContext::text_surface_api_get_predicted_size(lua_State* l) {
  return state_boundary_handle(l,[&]{
    std::string font_id = LuaTools::check_string(l,1);
    int font_size = LuaTools::check_int(l,2);
    std::string text = LuaTools::check_string(l,3);

    if (!FontResource::exists(font_id)) {
      LuaTools::error(l, std::string("No such font: '") + font_id + "'");
    }
    int w=0,h=0;
    if(FontResource::is_bitmap_font(font_id)) {
      int utf8len = 0;
      for(char c : text) {
        utf8len += (c & 0xc0) != 0x80;
      }
      const SurfacePtr& bitmap = FontResource::get_bitmap_font(font_id);
      const Size& bitmap_size = bitmap->get_size();
      int char_width = bitmap_size.width / 128;
      h = bitmap_size.height / 16;
      w = (char_width-1)*utf8len+1;
    } else {
      TTF_Font& font = FontResource::get_outline_font(font_id, font_size);
      TTF_SizeUTF8(&font,text.c_str(),&w,&h);
    }
    lua_pushinteger(l,w);
    lua_pushinteger(l,h);
    return 2;
  });
}

/**
 * \brief Implementation of text_surface:get_horizontal_alignment().
 * \param l the Lua context that is calling this function
 * \return number of values to return to Lua
 */
int LuaContext::text_surface_api_get_horizontal_alignment(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const TextSurface& text_surface = *check_text_surface(l, 1);

    HorizontalAlignment alignment = text_surface.get_horizontal_alignment();

    push_string(l, horizontal_alignment_names.find(alignment)->second);
    return 1;
  });
}

/**
 * \brief Implementation of text_surface:set_horizontal_alignment().
 * \param l the Lua context that is calling this function
 * \return number of values to return to Lua
 */
int LuaContext::text_surface_api_set_horizontal_alignment(lua_State* l) {

  return state_boundary_handle(l, [&] {
    TextSurface& text_surface = *check_text_surface(l, 1);
    TextSurface::HorizontalAlignment alignment =
        LuaTools::check_enum<TextSurface::HorizontalAlignment>(
            l, 2, horizontal_alignment_names);

    text_surface.set_horizontal_alignment(alignment);

    return 0;
  });
}

/**
 * \brief Implementation of text_surface:get_vertical_alignment().
 * \param l the Lua context that is calling this function
 * \return number of values to return to Lua
 */
int LuaContext::text_surface_api_get_vertical_alignment(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const TextSurface& text_surface = *check_text_surface(l, 1);

    TextSurface::VerticalAlignment alignment = text_surface.get_vertical_alignment();

    push_string(l, vertical_alignment_names.find(alignment)->second);
    return 1;
  });
}

/**
 * \brief Implementation of text_surface:set_vertical_alignment().
 * \param l the Lua context that is calling this function
 * \return number of values to return to Lua
 */
int LuaContext::text_surface_api_set_vertical_alignment(lua_State* l) {

  return state_boundary_handle(l, [&] {
    TextSurface& text_surface = *check_text_surface(l, 1);
    TextSurface::VerticalAlignment alignment =
        LuaTools::check_enum<TextSurface::VerticalAlignment>(
            l, 2, vertical_alignment_names);

    text_surface.set_vertical_alignment(alignment);

    return 0;
  });
}

/**
 * \brief Implementation of text_surface:get_font().
 * \param l the Lua context that is calling this function
 * \return number of values to return to Lua
 */
int LuaContext::text_surface_api_get_font(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const TextSurface& text_surface = *check_text_surface(l, 1);

    const std::string& font_id = text_surface.get_font();
    push_string(l, font_id);

    return 1;
  });
}

/**
 * \brief Implementation of text_surface:set_text().
 * \param l the Lua context that is calling this function
 * \return number of values to return to Lua
 */
int LuaContext::text_surface_api_set_font(lua_State* l) {

  return state_boundary_handle(l, [&] {
    TextSurface& text_surface = *check_text_surface(l, 1);
    const std::string& font_id = LuaTools::check_string(l, 2);

    if (!FontResource::exists(font_id)) {
      LuaTools::arg_error(l, 2, std::string("No such font: '") + font_id + "'");
    }
    text_surface.set_font(font_id);

    return 0;
  });
}

/**
 * \brief Implementation of text_surface:get_rendering_mode().
 * \param l the Lua context that is calling this function
 * \return number of values to return to Lua
 */
int LuaContext::text_surface_api_get_rendering_mode(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const TextSurface& text_surface = *check_text_surface(l, 1);

    TextSurface::RenderingMode mode = text_surface.get_rendering_mode();

    push_string(l, rendering_mode_names.find(mode)->second);
    return 1;
  });
}

/**
 * \brief Implementation of text_surface:set_rendering_mode().
 * \param l the Lua context that is calling this function
 * \return number of values to return to Lua
 */
int LuaContext::text_surface_api_set_rendering_mode(lua_State* l) {

  return state_boundary_handle(l, [&] {
    TextSurface& text_surface = *check_text_surface(l, 1);
    TextSurface::RenderingMode mode = LuaTools::check_enum<TextSurface::RenderingMode>(
        l, 2, rendering_mode_names);

    text_surface.set_rendering_mode(mode);

    return 0;
  });
}

/**
 * \brief Implementation of text_surface:get_color().
 * \param l the Lua context that is calling this function
 * \return number of values to return to Lua
 */
int LuaContext::text_surface_api_get_color(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const TextSurface& text_surface = *check_text_surface(l, 1);

    const Color& color = text_surface.get_text_color();

    push_color(l, color);
    return 1;
  });
}

/**
 * \brief Implementation of text_surface:set_color().
 * \param l the Lua context that is calling this function
 * \return number of values to return to Lua
 */
int LuaContext::text_surface_api_set_color(lua_State* l) {

  return state_boundary_handle(l, [&] {
    TextSurface& text_surface = *check_text_surface(l, 1);
    const Color& color = LuaTools::check_color(l, 2);

    text_surface.set_text_color(color);

    return 0;
  });
}

/**
 * \brief Implementation of text_surface:get_font_size().
 * \param l the Lua context that is calling this function
 * \return number of values to return to Lua
 */
int LuaContext::text_surface_api_get_font_size(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const TextSurface& text_surface = *check_text_surface(l, 1);

    lua_pushinteger(l, text_surface.get_font_size());
    return 1;
  });
}

/**
 * \brief Implementation of text_surface:set_font_size().
 * \param l the Lua context that is calling this function
 * \return number of values to return to Lua
 */
int LuaContext::text_surface_api_set_font_size(lua_State* l) {

  return state_boundary_handle(l, [&] {
    TextSurface& text_surface = *check_text_surface(l, 1);
    int font_size = LuaTools::check_int(l, 2);

    text_surface.set_font_size(font_size);

    return 0;
  });
}

/**
 * \brief Implementation of text_surface:get_text().
 * \param l the Lua context that is calling this function
 * \return number of values to return to Lua
 */
int LuaContext::text_surface_api_get_text(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const TextSurface& text_surface = *check_text_surface(l, 1);

    const std::string& text = text_surface.get_text();

    push_string(l, text);
    return 1;
  });
}

/**
 * \brief Implementation of text_surface:set_text().
 * \param l the Lua context that is calling this function
 * \return number of values to return to Lua
 */
int LuaContext::text_surface_api_set_text(lua_State* l) {

  return state_boundary_handle(l, [&] {
    TextSurface& text_surface = *check_text_surface(l, 1);
    std::string text;
    if (lua_gettop(l) >= 2 && !lua_isnil(l, 2)) {
      text = LuaTools::check_string(l, 2);
    }
    text_surface.set_text(text);

    return 0;
  });
}

/**
 * \brief Implementation of text_surface:set_text_key().
 * \param l the Lua context that is calling this function
 * \return number of values to return to Lua
 */
int LuaContext::text_surface_api_set_text_key(lua_State* l) {

  return state_boundary_handle(l, [&] {
    TextSurface& text_surface = *check_text_surface(l, 1);
    const std::string& key = LuaTools::check_string(l, 2);

    if (!CurrentQuest::string_exists(key)) {
      LuaTools::arg_error(l, 2, std::string("No value with key '") + key
          + "' in strings.dat for language '"
          + CurrentQuest::get_language() + "'"
      );
    }

    text_surface.set_text(CurrentQuest::get_string(key));

    return 0;
  });
}

/**
 * \brief Implementation of text_surface:get_size().
 * \param l the Lua context that is calling this function
 * \return number of values to return to Lua
 */
int LuaContext::text_surface_api_get_size(lua_State* l) {

  return state_boundary_handle(l, [&] {
    const TextSurface& text_surface = *check_text_surface(l, 1);

    lua_pushinteger(l, text_surface.get_width());
    lua_pushinteger(l, text_surface.get_height());

    return 2;
  });
}

}

