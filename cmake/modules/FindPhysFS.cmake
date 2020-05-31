# FindPhysFS - finds the PhysFS library
#
# This module uses the following environment variables:
#
#   PHYSFS_DIR
#     A directory to use as hint for locating PhysFS
#
# This module provides the following imported targets:
#
#   PhysFS::PhysFS
#     The PhysFS library
#
# This module defines the following result variables:
#
#   PHYSFS_FOUND
#     True if the system has PhysFS
#   PHYSFS_VERSION
#     The version of the found PhysFS (if available)
#   PHYSFS_INCLUDE_DIRS
#     Include directories needed to use PhysFS
#   PHYSFS_LIBRARIES
#     Libraries needed to link to PhysFS
#   PHYSFS_DEFINITIONS
#     Definitions to use when compiling code that uses PhysFS
#
# This module defines the following cache variables:
#
#   PHYSFS_INCLUDE_DIR
#     The directory containing 'physfs.h'
#   PHYSFS_LIBRARY
#     The path to the PhysFS library
#

# try to obtain package data from pkg-config
find_package(PkgConfig QUIET)
pkg_check_modules(PC_PHYSFS QUIET physfs)

# locate PhysFS header
find_path(PHYSFS_INCLUDE_DIR
  NAMES
    physfs.h
  HINTS
    ENV PHYSFS_DIR
    ${PC_PHYSFS_INCLUDEDIR}
    ${PC_PHYSFS_INCLUDE_DIRS}
  PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /sw # Fink
    /opt/local # DarwinPorts
    /opt/csw # Blastwave
    /opt
  PATH_SUFFIXES
    physfs
    include/physfs
    include
)

# locate PhysFS library
find_library(PHYSFS_LIBRARY
  NAMES
    physfs
  HINTS
    ENV PHYSFS_DIR
    ${PC_PHYSFS_LIBDIR}
    ${PC_PHYSFS_LIBRARY_DIRS}
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

# extract PhysFS version
set(PHYSFS_VERSION ${PC_PHYSFS_VERSION})
if(NOT PHYSFS_VERSION AND PHYSFS_INCLUDE_DIR AND EXISTS "${PHYSFS_INCLUDE_DIR}/physfs.h")
  file(STRINGS "${PHYSFS_INCLUDE_DIR}/physfs.h" PHYSFS_VERSION_MAJOR_LINE REGEX "^#define[ \t]+PHYSFS_VER_MAJOR[ \t]+[0-9]+$")
  file(STRINGS "${PHYSFS_INCLUDE_DIR}/physfs.h" PHYSFS_VERSION_MINOR_LINE REGEX "^#define[ \t]+PHYSFS_VER_MINOR[ \t]+[0-9]+$")
  file(STRINGS "${PHYSFS_INCLUDE_DIR}/physfs.h" PHYSFS_VERSION_PATCH_LINE REGEX "^#define[ \t]+PHYSFS_VER_PATCH[ \t]+[0-9]+$")
  string(REGEX REPLACE "^#define[ \t]+PHYSFS_VER_MAJOR[ \t]+([0-9]+)$" "\\1" PHYSFS_VERSION_MAJOR "${PHYSFS_VERSION_MAJOR_LINE}")
  string(REGEX REPLACE "^#define[ \t]+PHYSFS_VER_MINOR[ \t]+([0-9]+)$" "\\1" PHYSFS_VERSION_MINOR "${PHYSFS_VERSION_MINOR_LINE}")
  string(REGEX REPLACE "^#define[ \t]+PHYSFS_VER_PATCH[ \t]+([0-9]+)$" "\\1" PHYSFS_VERSION_PATCH "${PHYSFS_VERSION_PATCH_LINE}")
  set(PHYSFS_VERSION ${PHYSFS_VERSION_MAJOR}.${PHYSFS_VERSION_MINOR}.${PHYSFS_VERSION_PATCH})
  unset(PHYSFS_VERSION_MAJOR_LINE)
  unset(PHYSFS_VERSION_MINOR_LINE)
  unset(PHYSFS_VERSION_PATCH_LINE)
  unset(PHYSFS_VERSION_MAJOR)
  unset(PHYSFS_VERSION_MINOR)
  unset(PHYSFS_VERSION_PATCH)
endif()

# handle finding PhysFS
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PhysFS
  FOUND_VAR PHYSFS_FOUND
  REQUIRED_VARS
    PHYSFS_LIBRARY
    PHYSFS_INCLUDE_DIR
  VERSION_VAR PHYSFS_VERSION
)

# set user variables for compiling with PhysFS
if(PHYSFS_FOUND)
  set(PHYSFS_LIBRARIES ${PHYSFS_LIBRARY})
  set(PHYSFS_INCLUDE_DIRS ${PHYSFS_INCLUDE_DIR})
  set(PHYSFS_DEFINITIONS ${PC_PHYSFS_CFLAGS_OTHER})
endif()

# create an imported target for PhysFS
if(PHYSFS_FOUND AND NOT TARGET PhysFS::PhysFS)
  add_library(PhysFS::PhysFS UNKNOWN IMPORTED)
  set_target_properties(PhysFS::PhysFS PROPERTIES
    IMPORTED_LOCATION "${PHYSFS_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${PHYSFS_INCLUDE_DIR}"
    INTERFACE_COMPILE_OPTIONS "${PHYSFS_DEFINITIONS}"
  )
endif()

# mark cache variables as advanced
mark_as_advanced(
  PHYSFS_LIBRARY
  PHYSFS_INCLUDE_DIR
)
