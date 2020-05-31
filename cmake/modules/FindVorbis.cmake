# FindVorbis - finds the Vorbis library
#
# This module uses the following environment variables:
#
#   VORBIS_DIR
#     A directory to use as hint for locating Vorbis
#
# This module provides the following imported targets:
#
#   Vorbis::Vorbis
#     The Vorbis library
#
# This module defines the following result variables:
#
#   VORBIS_FOUND
#     True if the system has Vorbis
#   VORBIS_VERSION
#     The version of the found Vorbis (if available)
#   VORBIS_INCLUDE_DIRS
#     Include directories needed to use Vorbis
#   VORBIS_LIBRARIES
#     Libraries needed to link to Vorbis
#   VORBIS_DEFINITIONS
#     Definitions to use when compiling code that uses Vorbis
#
# This module defines the following cache variables:
#
#   VORBIS_INCLUDE_DIR
#     The directory containing 'vorbis/codec.h'
#   VORBIS_LIBRARY
#     The path to the Vorbis library
#

# try to obtain package data from pkg-config
find_package(PkgConfig QUIET)
pkg_check_modules(PC_VORBIS QUIET vorbis)

# locate Vorbis header
find_path(VORBIS_INCLUDE_DIR
  NAMES
    vorbis/codec.h
  HINTS
    ENV VORBIS_DIR
    ${PC_VORBIS_INCLUDEDIR}
    ${PC_VORBIS_INCLUDE_DIRS}
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

# locate Vorbis library
find_library(VORBIS_LIBRARY
  NAMES
    vorbis
  HINTS
    ENV VORBIS_DIR
    ${PC_VORBIS_LIBDIR}
    ${PC_VORBIS_LIBRARY_DIRS}
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

# extract Vorbis version
set(VORBIS_VERSION ${PC_VORBIS_VERSION})

# handle finding Vorbis
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Vorbis
  FOUND_VAR VORBIS_FOUND
  REQUIRED_VARS
    VORBIS_LIBRARY
    VORBIS_INCLUDE_DIR
  VERSION_VAR VORBIS_VERSION
)

# set user variables for compiling with Vorbis
if(VORBIS_FOUND)
  set(VORBIS_LIBRARIES ${VORBIS_LIBRARY})
  set(VORBIS_INCLUDE_DIRS ${VORBIS_INCLUDE_DIR})
  set(VORBIS_DEFINITIONS ${PC_VORBIS_CFLAGS_OTHER})
endif()

# create an imported target for Vorbis
if(VORBIS_FOUND AND NOT TARGET Vorbis::Vorbis)
  add_library(Vorbis::Vorbis UNKNOWN IMPORTED)
  set_target_properties(Vorbis::Vorbis PROPERTIES
    IMPORTED_LOCATION "${VORBIS_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${VORBIS_INCLUDE_DIR}"
    INTERFACE_COMPILE_OPTIONS "${VORBIS_DEFINITIONS}"
  )
endif()

# mark cache variables as advanced
mark_as_advanced(
  VORBIS_LIBRARY
  VORBIS_INCLUDE_DIR
)
