# FindLuaJIT - finds the LuaJIT library
#
# This module uses the following environment variables:
#
#   LUAJIT_DIR
#     A directory to use as hint for locating LuaJIT
#
# This module provides the following imported targets:
#
#   Lua::LuaJIT
#     The LuaJIT library
#
# This module defines the following result variables:
#
#   LUAJIT_FOUND
#     True if the system has LuaJIT
#   LUAJIT_VERSION
#     The version of the found LuaJIT (if available)
#   LUAJIT_INCLUDE_DIRS
#     Include directories needed to use LuaJIT
#   LUAJIT_LIBRARIES
#     Libraries needed to link to LuaJIT
#   LUAJIT_DEFINITIONS
#     Definitions to use when compiling code that uses LuaJIT
#
# This module defines the following cache variables:
#
#   LUAJIT_INCLUDE_DIR
#     The directory containing 'luajit.h'
#   LUAJIT_LIBRARY
#     The path to the LuaJIT library
#

# try to obtain package data from pkg-config
find_package(PkgConfig QUIET)
pkg_check_modules(PC_LUAJIT QUIET luajit)

# locate LuaJIT header
find_path(LUAJIT_INCLUDE_DIR
  NAMES
    luajit.h
  HINTS
    ENV LUAJIT_DIR
    ${PC_LUAJIT_INCLUDEDIR}
    ${PC_LUAJIT_INCLUDE_DIRS}
  PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /sw # Fink
    /opt/local # DarwinPorts
    /opt/csw # Blastwave
    /opt
  PATH_SUFFIXES
    luajit-2.1
    luajit-2.0
    luajit-5_1-2.1
    luajit-5_1-2.0
    include/luajit-2.1
    include/luajit-2.0
    include/luajit-5_1-2.1
    include/luajit-5_1-2.0
    include
)

# locate LuaJIT library
find_library(LUAJIT_LIBRARY
  NAMES
    luajit-2.1
    luajit-2.0
    luajit-5.1
  HINTS
    ENV LUAJIT_DIR
    ${PC_LUAJIT_LIBDIR}
    ${PC_LUAJIT_LIBRARY_DIRS}
  PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /sw # Fink
    /opt/local # DarwinPorts
    /opt/csw # Blastwave
    /opt
  PATH_SUFFIXES
    lib
    lib64
)

# extract LuaJIT version
set(LUAJIT_VERSION ${PC_LUAJIT_VERSION})
if(NOT LUAJIT_VERSION AND LUAJIT_INCLUDE_DIR AND EXISTS "${LUAJIT_INCLUDE_DIR}/luajit.h")
  file(STRINGS "${LUAJIT_INCLUDE_DIR}/luajit.h" LUAJIT_VERSION_LINE REGEX "^#define[ \t]+LUAJIT_VERSION[ \t]+\"LuaJIT .+\"")
  string(REGEX REPLACE "^#define[ \t]+LUAJIT_VERSION[ \t]+\"LuaJIT ([^\"]+)\".*" "\\1" LUAJIT_VERSION "${LUAJIT_VERSION_LINE}")
  unset(LUAJIT_VERSION_LINE)
endif()

# handle finding LuaJIT
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LuaJIT
  FOUND_VAR LUAJIT_FOUND
  REQUIRED_VARS
    LUAJIT_LIBRARY
    LUAJIT_INCLUDE_DIR
  VERSION_VAR LUAJIT_VERSION
)

# set user variables for compiling with LuaJIT
if(LUAJIT_FOUND)
  set(LUAJIT_LIBRARIES ${LUAJIT_LIBRARY})
  set(LUAJIT_INCLUDE_DIRS ${LUAJIT_INCLUDE_DIR})
  set(LUAJIT_DEFINITIONS ${PC_LUAJIT_CFLAGS_OTHER})

  if (UNIX AND NOT APPLE AND NOT BEOS)
    find_library(LUAJIT_MATH_LIBRARY m)
    list(APPEND LUAJIT_LIBRARIES "${LUAJIT_MATH_LIBRARY}")
    unset(LUAJIT_MATH_LIBRARY)

    # include dl library for statically-linked LuaJIT library
    get_filename_component(LUAJIT_LIB_EXT ${LUAJIT_LIBRARY} EXT)
    if(LUAJIT_LIB_EXT STREQUAL CMAKE_STATIC_LIBRARY_SUFFIX)
      list(APPEND LUAJIT_LIBRARIES "${CMAKE_DL_LIBS}")
    endif()
    unset(LUAJIT_LIB_EXT)
  endif()
endif()

# create an imported target for LuaJIT
if(LUAJIT_FOUND AND NOT TARGET Lua::LuaJIT)
  add_library(Lua::LuaJIT UNKNOWN IMPORTED)
  set_target_properties(Lua::LuaJIT PROPERTIES
    IMPORTED_LOCATION "${LUAJIT_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${LUAJIT_INCLUDE_DIR}"
    INTERFACE_COMPILE_OPTIONS "${LUAJIT_DEFINITIONS}"
  )
endif()

# mark cache variables as advanced
mark_as_advanced(
  LUAJIT_LIBRARY
  LUAJIT_INCLUDE_DIR
)
