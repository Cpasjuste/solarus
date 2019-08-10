# This file adds the necessary dependencies of the Solarus library.

# Whether LuaJIT should be used instead of vanilla Lua.
option(SOLARUS_USE_LUAJIT "Use LuaJIT instead of default Lua (recommended)" ON)

# Tell FindOpengl to use modern GL lib system
set(OpenGL_GL_PREFERENCE GLVND)

# Find dependencies.
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules/")
find_package(SDL2 "2.0.6" REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(OpenGL)
if(NOT OPENGL_FOUND)
  find_package(OpenGLES2 REQUIRED)
endif()
find_package(OpenAL REQUIRED)
find_package(Vorbis REQUIRED)
find_package(VorbisFile REQUIRED)
find_package(Ogg REQUIRED)
find_package(ModPlug REQUIRED)
find_package(PhysFS REQUIRED)
if(SOLARUS_USE_LUAJIT)
  find_package(LuaJit REQUIRED)
else()
  find_package(Lua "5.1" EXACT REQUIRED)
endif()

# Explicit link to libdl is needed for Lua on some systems.
find_library(DL_LIBRARY dl)
if("${DL_LIBRARY}" MATCHES DL_LIBRARY-NOTFOUND)
  set(DL_LIBRARY "")
endif()

