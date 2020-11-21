# FindModPlug - finds the ModPlug library
#
# This module uses the following environment variables:
#
#   MODPLUG_DIR
#     A directory to use as hint for locating ModPlug
#
# This module provides the following imported targets:
#
#   ModPlug::ModPlug
#     The ModPlug library
#
# This module defines the following result variables:
#
#   MODPLUG_FOUND
#     True if the system has ModPlug
#   MODPLUG_VERSION
#     The version of the found ModPlug (if available)
#   MODPLUG_INCLUDE_DIRS
#     Include directories needed to use ModPlug
#   MODPLUG_LIBRARIES
#     Libraries needed to link to ModPlug
#   MODPLUG_DEFINITIONS
#     Definitions to use when compiling code that uses ModPlug
#
# This module defines the following cache variables:
#
#   MODPLUG_INCLUDE_DIR
#     The directory containing 'modplug.h'
#   MODPLUG_LIBRARY
#     The path to the ModPlug library
#

# try to obtain package data from pkg-config
find_package(PkgConfig QUIET)
pkg_check_modules(PC_MODPLUG QUIET libmodplug)

# locate ModPlug header
find_path(MODPLUG_INCLUDE_DIR
  NAMES
    modplug.h
  HINTS
    ENV MODPLUG_DIR
    ${PC_MODPLUG_INCLUDEDIR}
    ${PC_MODPLUG_INCLUDE_DIRS}
  PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /sw # Fink
    /opt/local # DarwinPorts
    /opt/csw # Blastwave
    /opt
  PATH_SUFFIXES
    libmodplug
    modplug
    include/libmodplug
    include/modplug
    include
)

# locate ModPlug library
find_library(MODPLUG_LIBRARY
  NAMES
    modplug
  HINTS
    ENV MODPLUG_DIR
    ${PC_MODPLUG_LIBDIR}
    ${PC_MODPLUG_LIBRARY_DIRS}
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

# extract ModPlug version
set(MODPLUG_VERSION ${PC_MODPLUG_VERSION})

# handle finding ModPlug
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ModPlug
  FOUND_VAR MODPLUG_FOUND
  REQUIRED_VARS
    MODPLUG_LIBRARY
    MODPLUG_INCLUDE_DIR
  VERSION_VAR MODPLUG_VERSION
)

# set user variables for compiling with ModPlug
if(MODPLUG_FOUND)
  set(MODPLUG_LIBRARIES ${MODPLUG_LIBRARY})
  set(MODPLUG_INCLUDE_DIRS ${MODPLUG_INCLUDE_DIR})
  set(MODPLUG_DEFINITIONS ${PC_MODPLUG_CFLAGS_OTHER})
endif()

# create an imported target for ModPlug
if(MODPLUG_FOUND AND NOT TARGET ModPlug::ModPlug)
  add_library(ModPlug::ModPlug UNKNOWN IMPORTED)
  set_target_properties(ModPlug::ModPlug PROPERTIES
    IMPORTED_LOCATION "${MODPLUG_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${MODPLUG_INCLUDE_DIR}"
    INTERFACE_COMPILE_OPTIONS "${MODPLUG_DEFINITIONS}"
  )
endif()

# mark cache variables as advanced
mark_as_advanced(
  MODPLUG_LIBRARY
  MODPLUG_INCLUDE_DIR
)
