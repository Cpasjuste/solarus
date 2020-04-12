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
find_package(GLM REQUIRED)
find_package(OpenAL REQUIRED)
find_package(Vorbis REQUIRED)
find_package(VorbisFile REQUIRED)
find_package(Ogg REQUIRED)
find_package(ModPlug REQUIRED)
find_package(PhysFS REQUIRED)
if(SOLARUS_USE_LUAJIT)
  find_package(LuaJIT REQUIRED)
  set(LUA_FOUND ${LUAJIT_FOUND})
  set(LUA_INCLUDE_DIRS ${LUAJIT_INCLUDE_DIRS})
  set(LUA_LIBRARIES ${LUAJIT_LIBRARIES})
  set(LUA_DEFINITIONS ${LUAJIT_DEFINITIONS})
  set(LUA_INCLUDE_DIR ${LUAJIT_INCLUDE_DIR} CACHE PATH "Path to a file.")
  set(LUA_LIBRARY ${LUAJIT_LIBRARY} CACHE FILEPATH "Path to a library.")
else()
  find_package(Lua "5.1" EXACT REQUIRED)
  set(LUA_INCLUDE_DIRS ${LUA_INCLUDE_DIR})
endif()
