# FindOgg - finds the Ogg library
#
# This module uses the following environment variables:
#
#   OGG_DIR
#     A directory to use as hint for locating Ogg
#
# This module provides the following imported targets:
#
#   Ogg::Ogg
#     The Ogg library
#
# This module defines the following result variables:
#
#   OGG_FOUND
#     True if the system has Ogg
#   OGG_VERSION
#     The version of the found Ogg (if available)
#   OGG_INCLUDE_DIRS
#     Include directories needed to use Ogg
#   OGG_LIBRARIES
#     Libraries needed to link to Ogg
#   OGG_DEFINITIONS
#     Definitions to use when compiling code that uses Ogg
#
# This module defines the following cache variables:
#
#   OGG_INCLUDE_DIR
#     The directory containing 'ogg/ogg.h'
#   OGG_LIBRARY
#     The path to the Ogg library
#

# try to obtain package data from pkg-config
find_package(PkgConfig QUIET)
pkg_check_modules(PC_OGG QUIET ogg)

# locate Ogg header
find_path(OGG_INCLUDE_DIR
  NAMES
    ogg/ogg.h
  HINTS
    ENV OGG_DIR
    ${PC_OGG_INCLUDEDIR}
    ${PC_OGG_INCLUDE_DIRS}
  PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /sw # Fink
    /opt/local # DarwinPorts
    /opt/csw # Blastwave
    /opt
  PATH_SUFFIXES
    include
)

# locate Ogg library
find_library(OGG_LIBRARY
  NAMES
    ogg
  HINTS
    ENV OGG_DIR
    ${PC_OGG_LIBDIR}
    ${PC_OGG_LIBRARY_DIRS}
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

# extract Ogg version
set(OGG_VERSION ${PC_OGG_VERSION})

# handle finding Ogg
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Ogg
  FOUND_VAR OGG_FOUND
  REQUIRED_VARS
    OGG_LIBRARY
    OGG_INCLUDE_DIR
  VERSION_VAR OGG_VERSION
)

# set user variables for compiling with Ogg
if(OGG_FOUND)
  set(OGG_LIBRARIES ${OGG_LIBRARY})
  set(OGG_INCLUDE_DIRS ${OGG_INCLUDE_DIR})
  set(OGG_DEFINITIONS ${PC_OGG_CFLAGS_OTHER})
endif()

# create an imported target for Ogg
if(OGG_FOUND AND NOT TARGET Ogg::Ogg)
  add_library(Ogg::Ogg UNKNOWN IMPORTED)
  set_target_properties(Ogg::Ogg PROPERTIES
    IMPORTED_LOCATION "${OGG_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${OGG_INCLUDE_DIR}"
    INTERFACE_COMPILE_OPTIONS "${OGG_DEFINITIONS}"
  )
endif()

# mark cache variables as advanced
mark_as_advanced(
  OGG_LIBRARY
  OGG_INCLUDE_DIR
)
