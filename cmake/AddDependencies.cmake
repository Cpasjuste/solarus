# This file adds the necessary dependencies of the Solarus library.

# Whether LuaJIT should be used instead of vanilla Lua.
option(SOLARUS_USE_LUAJIT "Use LuaJIT instead of default Lua (recommended)" ON)

# Tell FindOpengl to use modern GL lib system
set(OpenGL_GL_PREFERENCE GLVND)

# Find dependencies.
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules/")
find_package(SDL2 "2.0.6" REQUIRED)
if(SDL2_VERSION_STRING VERSION_EQUAL 2.0.10)
  message(WARNING "SDL2 2.0.10 has known PNG transparency issues that affect Solarus graphics, please upgrade or downgrade your SDL2 version.")
endif()
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
else()
  find_package(Lua "5.1" EXACT REQUIRED)
  add_library(Lua::Lua UNKNOWN IMPORTED)
  set_target_properties(Lua::Lua PROPERTIES
    IMPORTED_LOCATION "${LUA_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${LUA_INCLUDE_DIR}"
  )
endif()
