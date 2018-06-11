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
#include "solarus/core/Logger.h"
#include "solarus/core/QuestFiles.h"
#include "solarus/core/System.h"
#include "solarus/core/Transform.h"
#include "solarus/graphics/Shader.h"
#include "solarus/graphics/Video.h"
#include "solarus/graphics/Surface.h"
#include "solarus/lua/LuaContext.h"
#include "solarus/lua/LuaTools.h"
#include "solarus/graphics/VertexArray.h"
#include "solarus/graphics/RenderTexture.h"

#include "solarus/third_party/glm/gtc/type_ptr.hpp"
#include "solarus/third_party/glm/gtx/transform.hpp"
#include "solarus/third_party/glm/gtx/matrix_transform_2d.hpp"

namespace Solarus {

VertexArray Shader::screen_quad(TRIANGLES);

/**
 * \brief Constructor.
 * \param shader_id The id of the shader to load (filename without extension).
 */
Shader::Shader(const std::string& shader_id):
    shader_id(shader_id),
    data(),
    valid(true),
    error() {
}

/**
 * \brief Destructor.
 */
Shader::~Shader() {
}

/**
 * \brief Returns whether this shader is valid.
 * \return \c true if the shader was successfully loaded.
 */
bool Shader::is_valid() const {
  return valid;
}

/**
 * \brief Sets whether this shader is valid.
 * \param valid \c true to indicate that the shader was successfully loaded.
 */
void Shader::set_valid(bool valid) {
  this->valid = valid;
}

/**
 * \brief Returns the error message of the last operation if any.
 * \return The error message or an empty string.
 */
std::string Shader::get_error() const {
  return error;
}

/**
 * \brief Sets the error message of the last operation.
 *
 * There should be an error message when \c is_valid() returns \c false.
 *
 * \param error The error message or an empty string.
 */
void Shader::set_error(const std::string& error) {
  this->error = error;
}

/**
 * \brief Returns the id of the shader.
 * \return The id of the shader.
 */
const std::string& Shader::get_id() const {

  return shader_id;
}

/**
 * \brief Returns the shader information loaded from the data file.
 * \return The shader data.
 */
const ShaderData& Shader::get_data() const {
  return data;
}

/**
 * @brief Returns the vertex source of the data or the default vertex source.
 * @return The vertex source.
 */
std::string Shader::get_vertex_source() const {

  const std::string& file_name = "shaders/" + get_data().get_vertex_file();
  if (file_name != "") {
    if (QuestFiles::data_file_exists(file_name)) {
      return QuestFiles::data_file_read(file_name);
    }
    Logger::error("Cannot find vertex shader file '" + file_name + "'");
  }
  return default_vertex_source();
}

/**
 * @brief Returns the fragment source of the data or the default fragment source.
 * @return The fragment source.
 */
std::string Shader::get_fragment_source() const {

  const std::string& file_name = "shaders/" + get_data().get_fragment_file();
  if (file_name != "") {
    if (QuestFiles::data_file_exists(file_name)) {
      return QuestFiles::data_file_read(file_name);
    }
    Logger::error("Cannot find fragment shader file '" + file_name + "'");
  }
  return default_fragment_source();
}

/**
 * \brief Sets the shader information.
 * \param data The shader data.
 */
void Shader::set_data(const ShaderData& data) {
  this->data = data;
}

/**
 * \fn Shader::set_uniform_1b
 * \brief Uploads a boolean uniform value to this shader program.
 *
 * Does nothing if there is no such uniform in the shader program.
 *
 * \param uniform_name Name of the uniform to set.
 * \param value The boolean value to set.
 */
void Shader::set_uniform_1b(const std::string&, bool) {
  // TODO make pure virtual
}

/**
 * \fn Shader::set_uniform_1i
 * \brief Uploads an integer uniform value to this shader program.
 *
 * Does nothing if there is no such uniform in the shader program.
 *
 * \param uniform_name Name of the uniform to set.
 * \param value The integer value to set.
 */
void Shader::set_uniform_1i(const std::string&, int) {
  // TODO make pure virtual
}

/**
 * \fn Shader::set_uniform_1f
 * \brief Uploads a float uniform value to this shader program.
 *
 * Does nothing if there is no such uniform in the shader program.
 *
 * \param uniform_name Name of the uniform to set.
 * \param value The float value to set.
 */
void Shader::set_uniform_1f(const std::string&, float) {
  // TODO make pure virtual
}

/**
 * \fn Shader::set_uniform_2f
 * \brief Uploads a vec2 uniform value to this shader program.
 *
 * Does nothing if there is no such uniform in the shader program.
 *
 * \param uniform_name Name of the uniform to set.
 * \param value_1 The first float value to set.
 * \param value_2 The second float value to set.
 */
void Shader::set_uniform_2f(const std::string&, float, float) {
  // TODO make pure virtual
}

/**
 * \fn Shader::set_uniform_3f
 * \brief Uploads a vec3 uniform value to this shader program.
 *
 * Does nothing if there is no such uniform in the shader program.
 *
 * \param uniform_name Name of the uniform to set.
 * \param value_1 The first float value to set.
 * \param value_2 The third float value to set.
 * \param value_3 The second float value to set.
 */
void Shader::set_uniform_3f(const std::string&, float, float, float) {
  // TODO make pure virtual
}

/**
 * \fn Shader::set_uniform_4f
 * \brief Uploads a vec4 uniform value to this shader program.
 *
 * Does nothing if there is no such uniform in the shader program.
 *
 * \param uniform_name Name of the uniform to set.
 * \param value_1 The first float value to set.
 * \param value_2 The second float value to set.
 * \param value_3 The third float value to set.
 * \param value_4 The fourth float value to set.
 */
void Shader::set_uniform_4f(const std::string&, float, float, float, float) {
  // TODO make pure virtual
}

/**
 * \fn Shader::set_uniform_texture
 * \brief Uploads a 2D texture uniform value to this shader program.
 *
 * Does nothing if there is no such uniform in the shader program.
 *
 * \param uniform_name Name of the uniform to set.
 * \param value The 2D texture value value to set.
 * \return \c true in case of success.
 */
bool Shader::set_uniform_texture(const std::string&, const SurfacePtr&) {
  // TODO make pure virtual
  return false;
}

void Shader::compute_matrices(const Size& surface_size, const Rectangle& region, const Size& dst_size, const Point& dst_position, bool flip_y,
                                     glm::mat4& viewport, glm::mat4& dst, glm::mat4& scale, glm::mat3 &uvm) {
  viewport = glm::ortho<float>(0,dst_size.width,0,dst_size.height); //Specify float as type to avoid integral division
  dst = glm::translate(glm::mat4(),glm::vec3(dst_position.x,dst_position.y,0));
  scale = glm::scale(glm::mat4(),glm::vec3(region.get_width(),region.get_height(),1));

  float uxf = 1.f/surface_size.width;
  float uyf = 1.f/surface_size.height;

  glm::mat3 uv_scale = glm::scale(
        glm::mat3(1),
        glm::vec2(
          region.get_width()*uxf,
          region.get_height()*uyf
          )
        );
  glm::mat3 uv_trans = glm::translate(glm::mat3(),glm::vec2(region.get_left()*uxf,region.get_top()*uyf));
  uvm = uv_trans*uv_scale;
  if(!flip_y){
    uvm = glm::scale(uvm,glm::vec2(1,-1));
    uvm = glm::translate(uvm,glm::vec2(0,-1));
  }
}

/**
 * @brief Render given surface on currently bound rendertarget
 * @param surface surface to draw
 * @param region region of the src surface to draw
 * @param dst_size size of the destination surface
 * @param dst_position position where to draw surface on destination
 * @param flip_y flip the drawing upside-down
 */
void Shader::render(const Surface& surface, const Rectangle& region, const Size& dst_size, const Point & dst_position, bool flip_y) {
  //TODO compute mvp and uv_matrix here
  glm::mat4 viewport,dst,scale;
  glm::mat3 uvm;
  compute_matrices(surface.get_size(),region,dst_size,dst_position,flip_y,viewport,dst,scale,uvm);
  //Set input size
  const Size& size = flip_y ? Video::get_output_size() : dst_size;
  set_uniform_1i(Shader::TIME_NAME, System::now());
  set_uniform_2f(Shader::OUTPUT_SIZE_NAME, size.width, size.height);
  set_uniform_2f(Shader::INPUT_SIZE_NAME, region.get_width(), region.get_height());
  render(screen_quad,surface,viewport*dst*scale,uvm);
}

void Shader::draw(Surface& dst_surface, const Surface &src_surface, const DrawInfos &infos) const {
    dst_surface.request_render().with_target([&](SDL_Renderer* r){
      SDL_BlendMode target = Surface::make_sdl_blend_mode(dst_surface.get_internal_surface(),
                                                          src_surface.get_internal_surface(),
                                                          infos.blend_mode); //TODO fix alpha premult here
      SDL_BlendMode current;
      SDL_GetRenderDrawBlendMode(r,&current);
      if(target != current) { //Blend mode need change
        SDL_SetRenderDrawBlendMode(r,target);
        SDL_RenderDrawPoint(r,-100,-100); //Draw a point offscreen to force blendmode change
      }
      //TODO fix this ugliness
      Shader* that = const_cast<Shader*>(this);
      that->set_uniform_1f(OPACITY_NAME,infos.opacity/256.f);
      //TODO compute mvp and uv_matrix here
      const auto& dst_position = infos.dst_position;
      const auto& region = infos.region;

      Size dst_size = dst_surface.get_size();
      glm::mat4 viewport,dst,scale;
      glm::mat3 uvm;
      compute_matrices(src_surface.get_size(),region,dst_size,dst_position,false,viewport,dst,scale,uvm);
      glm::mat4 transform = Transform(dst_position,infos.transform_origin,infos.scale,infos.rotation).get_glm_transform() * scale;
      //Set input size
      const Size& size = dst_size;
      that->set_uniform_1i(Shader::TIME_NAME, System::now());
      that->set_uniform_2f(Shader::OUTPUT_SIZE_NAME, size.width, size.height);
      that->set_uniform_2f(Shader::INPUT_SIZE_NAME, region.get_width(), region.get_height());
      that->render(screen_quad,src_surface,viewport*transform,uvm);
    });
}

/**
 * \fn Shader::load
 * \brief Loads this shader.
 *
 * Parses the shader data file and compiles GLSL shaders.
 */
void Shader::load() {
  // TODO make pure virtual
}

/**
 * \brief Returns the name identifying this type in Lua.
 * \return The name identifying this type in Lua.
 */
const std::string& Shader::get_lua_type_name() const {
  return LuaContext::shader_module_name;
}

}
