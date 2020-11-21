# FindGLM - finds the GLM library
#
# Module notes:
#
# - The minimum compatible version of GLM is 0.9.5.0
# - GLM is an include-only library
#
# This module uses the following environment variables:
#
#   GLM_DIR
#     A directory to use as hint for locating GLM
#
# This module provides the following imported targets:
#
#   GLM::GLM
#     The GLM library
#
# This module defines the following result variables:
#
#   GLM_FOUND
#     True if the system has GLM
#   GLM_VERSION
#     The version of the found GLM (if available)
#   GLM_INCLUDE_DIRS
#     Include directories needed to use GLM
#   GLM_DEFINITIONS
#     Definitions to use when compiling code that uses GLM
#
# This module defines the following cache variables:
#
#   GLM_INCLUDE_DIR
#     The directory containing 'glm/glm.hpp'
#

# try to obtain package data from pkg-config
find_package(PkgConfig QUIET)
pkg_check_modules(PC_GLM QUIET glm)

# locate GLM header
find_path(GLM_INCLUDE_DIR
  NAMES
    glm/glm.hpp
  HINTS
    ENV GLM_DIR
    ${PC_GLM_INCLUDEDIR}
    ${PC_GLM_INCLUDE_DIRS}
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

# extract GLM version
set(GLM_VERSION ${PC_GLM_VERSION})
if(NOT GLM_VERSION AND GLM_INCLUDE_DIR AND EXISTS "${GLM_INCLUDE_DIR}/glm/detail/setup.hpp")
  file(STRINGS "${GLM_INCLUDE_DIR}/glm/detail/setup.hpp" GLM_VERSION_MAJOR_LINE REGEX "^#define[ \t]+GLM_VERSION_MAJOR[ \t]+[0-9]+$")
  file(STRINGS "${GLM_INCLUDE_DIR}/glm/detail/setup.hpp" GLM_VERSION_MINOR_LINE REGEX "^#define[ \t]+GLM_VERSION_MINOR[ \t]+[0-9]+$")
  file(STRINGS "${GLM_INCLUDE_DIR}/glm/detail/setup.hpp" GLM_VERSION_PATCH_LINE REGEX "^#define[ \t]+GLM_VERSION_PATCH[ \t]+[0-9]+$")
  file(STRINGS "${GLM_INCLUDE_DIR}/glm/detail/setup.hpp" GLM_VERSION_REVISION_LINE REGEX "^#define[ \t]+GLM_VERSION_REVISION[ \t]+[0-9]+$")
  string(REGEX REPLACE "^#define[ \t]+GLM_VERSION_MAJOR[ \t]+([0-9]+)$" "\\1" GLM_VERSION_MAJOR "${GLM_VERSION_MAJOR_LINE}")
  string(REGEX REPLACE "^#define[ \t]+GLM_VERSION_MINOR[ \t]+([0-9]+)$" "\\1" GLM_VERSION_MINOR "${GLM_VERSION_MINOR_LINE}")
  string(REGEX REPLACE "^#define[ \t]+GLM_VERSION_PATCH[ \t]+([0-9]+)$" "\\1" GLM_VERSION_PATCH "${GLM_VERSION_PATCH_LINE}")
  string(REGEX REPLACE "^#define[ \t]+GLM_VERSION_REVISION[ \t]+([0-9]+)$" "\\1" GLM_VERSION_REVISION "${GLM_VERSION_REVISION_LINE}")
  set(GLM_VERSION ${GLM_VERSION_MAJOR}.${GLM_VERSION_MINOR}.${GLM_VERSION_PATCH}.${GLM_VERSION_REVISION})
  unset(GLM_VERSION_MAJOR_LINE)
  unset(GLM_VERSION_MINOR_LINE)
  unset(GLM_VERSION_PATCH_LINE)
  unset(GLM_VERSION_REVISION_LINE)
  unset(GLM_VERSION_MAJOR)
  unset(GLM_VERSION_MINOR)
  unset(GLM_VERSION_PATCH)
  unset(GLM_VERSION_REVISION)
endif()

# handle finding GLM
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLM
  FOUND_VAR GLM_FOUND
  REQUIRED_VARS
    GLM_INCLUDE_DIR
  VERSION_VAR GLM_VERSION
)

# set user variables for compiling with GLM
if(GLM_FOUND)
  set(GLM_INCLUDE_DIRS ${GLM_INCLUDE_DIR})
  set(GLM_DEFINITIONS ${PC_GLM_CFLAGS_OTHER})

  # enable GLM experimental extensions for GLM 0.9.9.0 to 0.9.9.3
  if((GLM_VERSION VERSION_EQUAL 0.9.9.0 OR GLM_VERSION VERSION_GREATER 0.9.9.0) AND (GLM_VERSION VERSION_EQUAL 0.9.9.3 OR GLM_VERSION VERSION_LESS 0.9.9.3))
    list(APPEND GLM_DEFINITIONS "-DGLM_ENABLE_EXPERIMENTAL")
  endif()
endif()

# create an imported target for GLM
if(GLM_FOUND AND NOT TARGET GLM::GLM)
  add_library(GLM::GLM INTERFACE IMPORTED)
  set_target_properties(GLM::GLM PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${GLM_INCLUDE_DIR}"
    INTERFACE_COMPILE_OPTIONS "${GLM_DEFINITIONS}"
  )
endif()

# mark cache variables as advanced
mark_as_advanced(
  GLM_INCLUDE_DIR
)
