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
#ifdef SOLARUS_HAVE_OPENGL

#include "solarus/core/Logger.h"
#include "solarus/core/QuestFiles.h"
#include "solarus/core/Size.h"
#include "solarus/core/System.h"
#include "solarus/graphics/GlArbShader.h"
#include "solarus/graphics/ShaderData.h"
#include "solarus/graphics/Surface.h"
#include "solarus/graphics/VertexArray.h"
#include "solarus/graphics/Video.h"
#include "solarus/graphics/RenderTexture.h"
#include "solarus/third_party/glm/gtc/type_ptr.hpp"
#include "solarus/third_party/glm/gtx/transform.hpp"
#include "solarus/third_party/glm/gtx/matrix_transform_2d.hpp"
#include <sstream>


namespace Solarus {

constexpr auto DEFAULT_VERTEX =
    R"(
    #if __VERSION__ >= 130
    #define COMPAT_VARYING out
    #define COMPAT_ATTRIBUTE in
    #else
    #define COMPAT_VARYING varying
    #define COMPAT_ATTRIBUTE attribute
    #endif

    #ifdef GL_ES
    precision mediump float;
    #define COMPAT_PRECISION mediump
    #else
    #define COMPAT_PRECISION
    #endif

    uniform mat4 sol_mvp_matrix;
    uniform mat3 sol_uv_matrix;
    COMPAT_ATTRIBUTE vec2 sol_vertex;
    COMPAT_ATTRIBUTE vec2 sol_tex_coord;
    COMPAT_ATTRIBUTE vec4 sol_color;

    COMPAT_VARYING vec2 sol_vtex_coord;
    COMPAT_VARYING vec4 sol_vcolor;
    void main() {
      gl_Position = sol_mvp_matrix * vec4(sol_vertex,0,1);
      sol_vcolor = sol_color;
      sol_vtex_coord = (sol_uv_matrix * vec3(sol_tex_coord,1)).xy;
    }
    )";

constexpr auto DEFAULT_FRAGMENT =
    R"(
    #if __VERSION__ >= 130
    #define COMPAT_VARYING in
    #define COMPAT_TEXTURE texture
    out vec4 FragColor;
    #else
    #define COMPAT_VARYING varying
    #define FragColor gl_FragColor
    #define COMPAT_TEXTURE texture2D
    #endif

    #ifdef GL_ES
    precision mediump float;
    #define COMPAT_PRECISION mediump
    #else
    #define COMPAT_PRECISION
    #endif

    uniform sampler2D sol_texture;
    COMPAT_VARYING vec2 sol_vtex_coord;
    COMPAT_VARYING vec4 sol_vcolor;
    void main() {
      vec4 tex_color = COMPAT_TEXTURE(sol_texture,sol_vtex_coord);
      FragColor = tex_color*sol_vcolor;
    }
    )";

namespace {

PFNGLATTACHOBJECTARBPROC glAttachObjectARB;
PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
PFNGLCOMPILESHADERARBPROC glCompileShaderARB;
PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB;
PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB;
PFNGLDELETEOBJECTARBPROC glDeleteObjectARB;
PFNGLGETINFOLOGARBPROC glGetInfoLogARB;
PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB;
PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB;
PFNGLLINKPROGRAMARBPROC glLinkProgramARB;
PFNGLSHADERSOURCEARBPROC glShaderSourceARB;
PFNGLUNIFORM1IARBPROC glUniform1iARB;
PFNGLUNIFORM1FARBPROC glUniform1fARB;
PFNGLUNIFORM2FARBPROC glUniform2fARB;
PFNGLUNIFORM3FARBPROC glUniform3fARB;
PFNGLUNIFORM4FARBPROC glUniform4fARB;
PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB;
PFNGLGETHANDLEARBPROC glGetHandleARB;

PFNGLUNIFORMMATRIX4FVARBPROC  glUniformMatrix4fvARB;
PFNGLUNIFORMMATRIX3FVARBPROC  glUniformMatrix3fvARB;

PFNGLGENBUFFERSARBPROC glGenBuffersARB;
PFNGLBINDBUFFERARBPROC glBindBufferARB;
PFNGLBUFFERDATAARBPROC glBufferDataARB;

PFNGLVERTEXATTRIBPOINTERARBPROC glVertexAttribPointerARB;
PFNGLENABLEVERTEXATTRIBARRAYARBPROC glEnableVertexAttribArrayARB;
PFNGLGETATTRIBLOCATIONARBPROC glGetAttribLocationARB;

/**
 * \brief Casts a pointer-to-object (void*) to a pointer-to-function.
 *
 * This function avoids compilation warnings when using SDL_GL_GetProcAddress(),
 * because directly casting a pointer-to-object to a pointer-to-function
 * is not allowed.
 *
 * \param object_ptr The pointer to cast.
 * \return The pointer to a function.
 */
template<typename FunctionPointerType>
FunctionPointerType get_proc_address_cast(void* object_ptr) {
  return *reinterpret_cast<FunctionPointerType*>(&object_ptr);
}

}  // Anonymous namespace

/**
 * \brief Initializes the GL ARB shader system.
 * \return \c true if GL ARB shaders are supported.
 */
bool GlArbShader::initialize() {

  // TODO Make sure that SDL doesn't disable GL ARB for render targets even if available.

  // Setting some parameters.
  glClearDepth(1.0); // Enables clearing of the depth buffer.
  glShadeModel(GL_SMOOTH); // Enables smooth color shading.

  //Init screen quad
  screen_quad.add_quad(Rectangle(0,0,1,1),Rectangle(0,1,1,-1),Color::white);

  // Initialize GL ARB functions.
  if (SDL_GL_ExtensionSupported("GL_ARB_shader_objects") &&
      SDL_GL_ExtensionSupported("GL_ARB_shading_language_100") &&
      SDL_GL_ExtensionSupported("GL_ARB_vertex_shader") &&
      SDL_GL_ExtensionSupported("GL_ARB_fragment_shader") &&
      SDL_GL_ExtensionSupported("GL_ARB_multitexture") &&
      SDL_GL_ExtensionSupported("GL_ARB_vertex_buffer_object")) {
    glAttachObjectARB = get_proc_address_cast<PFNGLATTACHOBJECTARBPROC>(SDL_GL_GetProcAddress("glAttachObjectARB"));
    glActiveTextureARB = get_proc_address_cast<PFNGLACTIVETEXTUREARBPROC>(SDL_GL_GetProcAddress("glActiveTextureARB"));
    glCompileShaderARB = get_proc_address_cast<PFNGLCOMPILESHADERARBPROC>(SDL_GL_GetProcAddress("glCompileShaderARB"));
    glCreateProgramObjectARB = get_proc_address_cast<PFNGLCREATEPROGRAMOBJECTARBPROC>(SDL_GL_GetProcAddress("glCreateProgramObjectARB"));
    glCreateShaderObjectARB = get_proc_address_cast<PFNGLCREATESHADEROBJECTARBPROC>(SDL_GL_GetProcAddress("glCreateShaderObjectARB"));
    glDeleteObjectARB = get_proc_address_cast<PFNGLDELETEOBJECTARBPROC>(SDL_GL_GetProcAddress("glDeleteObjectARB"));
    glGetInfoLogARB = get_proc_address_cast<PFNGLGETINFOLOGARBPROC>(SDL_GL_GetProcAddress("glGetInfoLogARB"));
    glGetObjectParameterivARB = get_proc_address_cast<PFNGLGETOBJECTPARAMETERIVARBPROC>(SDL_GL_GetProcAddress("glGetObjectParameterivARB"));
    glGetUniformLocationARB = get_proc_address_cast<PFNGLGETUNIFORMLOCATIONARBPROC>(SDL_GL_GetProcAddress("glGetUniformLocationARB"));
    glLinkProgramARB = get_proc_address_cast<PFNGLLINKPROGRAMARBPROC>(SDL_GL_GetProcAddress("glLinkProgramARB"));
    glShaderSourceARB = get_proc_address_cast<PFNGLSHADERSOURCEARBPROC>(SDL_GL_GetProcAddress("glShaderSourceARB"));
    glUniform1iARB = get_proc_address_cast<PFNGLUNIFORM1IARBPROC>(SDL_GL_GetProcAddress("glUniform1iARB"));
    glUniform1fARB = get_proc_address_cast<PFNGLUNIFORM1FARBPROC>(SDL_GL_GetProcAddress("glUniform1fARB"));
    glUniform2fARB = get_proc_address_cast<PFNGLUNIFORM2FARBPROC>(SDL_GL_GetProcAddress("glUniform2fARB"));
    glUniform3fARB = get_proc_address_cast<PFNGLUNIFORM3FARBPROC>(SDL_GL_GetProcAddress("glUniform3fARB"));
    glUniform4fARB = get_proc_address_cast<PFNGLUNIFORM4FARBPROC>(SDL_GL_GetProcAddress("glUniform4fARB"));
    glUseProgramObjectARB = get_proc_address_cast<PFNGLUSEPROGRAMOBJECTARBPROC>(SDL_GL_GetProcAddress("glUseProgramObjectARB"));
    glGetHandleARB = get_proc_address_cast<PFNGLGETHANDLEARBPROC>(SDL_GL_GetProcAddress("glGetHandleARB"));

    glUniformMatrix4fvARB = get_proc_address_cast<PFNGLUNIFORMMATRIX4FVARBPROC>(SDL_GL_GetProcAddress("glUniformMatrix4fvARB"));
    glUniformMatrix3fvARB = get_proc_address_cast<PFNGLUNIFORMMATRIX3FVARBPROC>(SDL_GL_GetProcAddress("glUniformMatrix3fvARB"));

    glGenBuffersARB = get_proc_address_cast<PFNGLGENBUFFERSARBPROC>(SDL_GL_GetProcAddress("glGenBuffersARB"));
    glBindBufferARB = get_proc_address_cast<PFNGLBINDBUFFERARBPROC>(SDL_GL_GetProcAddress("glBindBufferARB"));
    glBufferDataARB = get_proc_address_cast<PFNGLBUFFERDATAARBPROC>(SDL_GL_GetProcAddress("glBufferDataARB"));
    glVertexAttribPointerARB = get_proc_address_cast<PFNGLVERTEXATTRIBPOINTERARBPROC>(SDL_GL_GetProcAddress("glVertexAttribPointerARB"));
    glEnableVertexAttribArrayARB = get_proc_address_cast<PFNGLENABLEVERTEXATTRIBARRAYARBPROC>(SDL_GL_GetProcAddress("glEnableVertexAttribArrayARB"));
    glGetAttribLocationARB = get_proc_address_cast<PFNGLGETATTRIBLOCATIONARBPROC>(SDL_GL_GetProcAddress("glGetAttribLocationARB"));

    if (glAttachObjectARB &&
        glActiveTextureARB &&
        glCompileShaderARB &&
        glCreateProgramObjectARB &&
        glCreateShaderObjectARB &&
        glDeleteObjectARB &&
        glGetInfoLogARB &&
        glGetObjectParameterivARB &&
        glGetUniformLocationARB &&
        glLinkProgramARB &&
        glShaderSourceARB &&
        glUniform1iARB &&
        glUniform1fARB &&
        glUniform2fARB &&
        glUniform3fARB &&
        glUniform4fARB &&
        glUseProgramObjectARB &&
        glGetHandleARB &&

        glGenBuffersARB &&
        glBindBufferARB &&
        glBufferDataARB &&
        glVertexAttribPointerARB &&
        glEnableVertexAttribArrayARB &&
        glGetAttribLocationARB
        ) {
      Logger::info("Using ARB GL Shaders");
      return true;
    }
  }

  return false;
}

/**
 * \brief Constructor.
 * \param shader_id Id of the shader to load.
 */
GlArbShader::GlArbShader(const std::string& shader_id):
    Shader(shader_id),
    program(0),
    vertex_shader(0),
    fragment_shader(0) {

  glGetError();

  // Load the shader.
  load();

  // Set up constant uniform variables.
  GLenum gl_error = glGetError();
  GLhandleARB previous_program = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
  glUseProgramObjectARB(program);

  set_uniform_1i(TEXTURE_NAME,0);

  const Size& quest_size = Video::get_quest_size();
  set_uniform_2f(INPUT_SIZE_NAME, quest_size.width, quest_size.height);

  position_location = glGetAttribLocationARB(program,POSITION_NAME);
  tex_coord_location = glGetAttribLocationARB(program,TEXCOORD_NAME);
  color_location = glGetAttribLocationARB(program,COLOR_NAME);

  glUseProgramObjectARB(previous_program);

  if (gl_error != GL_NO_ERROR) {
    std::ostringstream oss;
    oss << "OpenGL error: " << gl_error;
    set_error(oss.str());
    set_valid(false);
  }
}

/**
 * \brief Destructor.
 */
GlArbShader::~GlArbShader() {

  // TODO use glDeleteProgramARB?
  glDeleteObjectARB(vertex_shader);
  glDeleteObjectARB(fragment_shader);
  glDeleteObjectARB(program);
}

std::string GlArbShader::default_vertex_source() const {
  return DEFAULT_VERTEX;
}

std::string GlArbShader::default_fragment_source() const {
  return DEFAULT_FRAGMENT;
}

/**
 * \copydoc Shader::load
 */
void GlArbShader::load() {

  set_valid(true);

  // Load the shader data file.
  const std::string shader_file_name =
      "shaders/" + get_id() + ".dat";

  ShaderData data;
  bool success = data.import_from_quest_file(shader_file_name);
  if (!success) {
    set_valid(false);
    return;
  }
  set_data(data);
  uniform_locations.clear();

  // Create the vertex and fragment shaders.
  vertex_shader = create_shader(GL_VERTEX_SHADER_ARB, get_vertex_source().c_str());
  if (!is_valid()) {
    return;
  }
  fragment_shader = create_shader(GL_FRAGMENT_SHADER_ARB, get_fragment_source().c_str());
  if (!is_valid()) {
    return;
  }

  // Create a program object with both shaders.
  program = glCreateProgramObjectARB();
  glAttachObjectARB(program, vertex_shader);
  glAttachObjectARB(program, fragment_shader);

  glLinkProgramARB(program);

  // Check the link status.
  GLint status;
  glGetObjectParameterivARB(program, GL_OBJECT_LINK_STATUS_ARB, &status);

  position_location = glGetAttribLocationARB(program,POSITION_NAME);
  tex_coord_location = glGetAttribLocationARB(program,TEXCOORD_NAME);
  color_location = glGetAttribLocationARB(program,COLOR_NAME);

  if (status == 0) {
    GLint length = 0;
    char* info = nullptr;

    glGetObjectParameterivARB(program, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
    info = SDL_stack_alloc(char, length + 1);
    glGetInfoLogARB(program, length, nullptr, info);
    set_error(std::string("Failed to link shader ") + get_id() + std::string(" :\n") + info);
    set_valid(false);
    SDL_stack_free(info);
  }
}

/**
 * \brief Compile a shader from source.
 * \param type The type of the shader to fill and compile.
 * \param source Sources to compile.
 * \return The shader created and compiled.
 */
GLhandleARB GlArbShader::create_shader(unsigned int type, const char* source) {

  GLint status;

  // Create the shader object.
  GLhandleARB shader = glCreateShaderObjectARB(type);

  glShaderSourceARB(shader, 1, &source, nullptr);
  glCompileShaderARB(shader);

  // Check the compile status.
  glGetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB, &status);
  if (status == 0) {
    GLint length;
    char* info;

    glGetObjectParameterivARB(shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
    info = SDL_stack_alloc(char, length + 1);
    glGetInfoLogARB(shader, length, nullptr, info);
    set_error(std::string("Failed to compile shader ") + get_id() + std::string(" :\n") + info);
    set_valid(false);
    SDL_stack_free(info);
  }

  return shader;
}

/**
 * \copydoc Shader::render
 */
void GlArbShader::render(const VertexArray& array, const Surface& texture, const glm::mat4 &mvp_matrix, const glm::mat3 &uv_matrix) {
  if(array.vertex_buffer == 0) {
    //Generate vertex-buffer
    glGenBuffersARB(1,&array.vertex_buffer);
  }
  glBindBufferARB(GL_ARRAY_BUFFER,array.vertex_buffer);
  if(array.buffer_dirty) {
    //Upload vertex buffer //TODO use glSubData if array size <= previous size
    glBufferDataARB(GL_ARRAY_BUFFER,array.vertex_count()*sizeof(Vertex),array.data(),GL_DYNAMIC_DRAW);
    array.buffer_dirty = false;
  }
  GLhandleARB previous_program = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
  glUseProgramObjectARB(program);

  glm::mat4 mvp = mvp_matrix; //TODO do more than identity
  glUniformMatrix4fvARB(get_uniform_location(Shader::MVP_MATRIX_NAME),1,GL_FALSE,glm::value_ptr(mvp));

  glm::mat3 uvm = uv_matrix;
  glUniformMatrix3fvARB(get_uniform_location(Shader::UV_MATRIX_NAME),1,GL_FALSE,glm::value_ptr(uvm));

  glEnableVertexAttribArrayARB(position_location);
  glVertexAttribPointerARB(position_location,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,position));

  glEnableVertexAttribArrayARB(tex_coord_location);
  glVertexAttribPointerARB(tex_coord_location,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex,texcoords));

  glEnableVertexAttribArrayARB(color_location);
  glVertexAttribPointerARB(color_location,4,GL_UNSIGNED_BYTE,GL_TRUE,sizeof(Vertex),(void*)offsetof(Vertex,color));

  glActiveTextureARB(GL_TEXTURE0_ARB + 0);  // Texture unit 0.
  SDL_GL_BindTexture(texture.get_internal_surface().get_texture(), nullptr, nullptr);

  for (const auto& kvp : uniform_textures) {
    const GLuint texture_unit = kvp.second.unit;
    glActiveTextureARB(GL_TEXTURE0_ARB + texture_unit);
    SDL_GL_BindTexture(kvp.second.surface->get_internal_surface().get_texture(),nullptr,nullptr);
  }
  glDrawArrays((GLenum)array.get_primitive_type(),0,array.vertex_count());

  for (const auto& kvp : uniform_textures) {
    const GLuint texture_unit = kvp.second.unit;
    glActiveTextureARB(GL_TEXTURE0_ARB + texture_unit);
    SDL_GL_UnbindTexture(kvp.second.surface->get_internal_surface().get_texture());
  }

  glActiveTextureARB(GL_TEXTURE0_ARB + 0);

  glUseProgramObjectARB(previous_program);
  glBindBufferARB(GL_ARRAY_BUFFER,0);
}

/**
 * \brief Returns the location of a uniform value in the shader program.
 * \param uniform_name Name of the uniform to find.
 * \return The uniform location or \c -1.
 */
GLint GlArbShader::get_uniform_location(const std::string& uniform_name) const {
  const auto& it = uniform_locations.find(uniform_name);
  if (it != uniform_locations.end()) {
    return it->second;
  }

  const GLint location = glGetUniformLocationARB(program, uniform_name.c_str());
  uniform_locations.insert(std::make_pair(uniform_name, location));
  return location;
}

/**
 * \copydoc Shader::set_uniform1f
 */
void GlArbShader::set_uniform_1b(const std::string& uniform_name, bool value) {

  const GLint location = get_uniform_location(uniform_name);
  if (location == -1) {
    return;
  }

  GLhandleARB previous_program = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
  glUseProgramObjectARB(program);
  glUniform1iARB(location, (value ? 1 : 0));
  glUseProgramObjectARB(previous_program);
}

/**
 * \copydoc Shader::set_uniform_1i
 */
void GlArbShader::set_uniform_1i(const std::string& uniform_name, int value) {

  const GLint location = get_uniform_location(uniform_name);
  if (location == -1) {
    return;
  }

  GLhandleARB previous_program = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
  glUseProgramObjectARB(program);
  glUniform1iARB(location, value);
  glUseProgramObjectARB(previous_program);
}

/**
 * \copydoc Shader::set_uniform_1f
 */
void GlArbShader::set_uniform_1f(const std::string& uniform_name, float value) {

  const GLint location = get_uniform_location(uniform_name);
  if (location == -1) {
    return;
  }

  GLhandleARB previous_program = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
  glUseProgramObjectARB(program);
  glUniform1fARB(location, value);
  glUseProgramObjectARB(previous_program);
}

/**
 * \copydoc Shader::set_uniform_2f
 */
void GlArbShader::set_uniform_2f(const std::string& uniform_name, float value_1, float value_2) {

  const GLint location = get_uniform_location(uniform_name);
  if (location == -1) {
    return;
  }

  GLhandleARB previous_program = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
  glUseProgramObjectARB(program);
  glUniform2fARB(location, value_1, value_2);
  glUseProgramObjectARB(previous_program);
}

/**
 * \copydoc Shader::set_uniform_3f
 */
void GlArbShader::set_uniform_3f(
    const std::string& uniform_name, float value_1, float value_2, float value_3) {

  const GLint location = get_uniform_location(uniform_name);
  if (location == -1) {
    return;
  }

  GLhandleARB previous_program = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
  glUseProgramObjectARB(program);
  glUniform3fARB(location, value_1, value_2, value_3);
  glUseProgramObjectARB(previous_program);
}

/**
 * \copydoc Shader::set_uniform_4f
 */
void GlArbShader::set_uniform_4f(
    const std::string& uniform_name, float value_1, float value_2, float value_3, float value_4) {

  const GLint location = get_uniform_location(uniform_name);
  if (location == -1) {
    return;
  }

  GLhandleARB previous_program = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
  glUseProgramObjectARB(program);
  glUniform4fARB(location, value_1, value_2, value_3, value_4);
  glUseProgramObjectARB(previous_program);
}

/**
 * \copydoc Shader::set_uniform_texture
 */
bool GlArbShader::set_uniform_texture(const std::string& uniform_name, const SurfacePtr& value) {
  const GLint location = get_uniform_location(uniform_name);

  if (location == -1) {
    // Not an error.
    return true;
  }

  auto it = uniform_textures.find(uniform_name);
  if(it != uniform_textures.end()) {
    it->second.surface = value;
    return true; //Nothing else to do
  }
  //else find a new texture unit

  GLhandleARB previous_program = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
  glUseProgramObjectARB(program);

  int texture_unit = ++current_texture_unit;
  uniform_textures[uniform_name] = TextureUniform{value,(GLuint)texture_unit};

  glUniform1iARB(location, texture_unit);

  glUseProgramObjectARB(previous_program);
  return true;
}

}
#endif // SOLARUS_HAVE_OPENGL
