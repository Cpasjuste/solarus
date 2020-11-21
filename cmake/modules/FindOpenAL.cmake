# FindOpenAL - finds the OpenAL library
#
# This module uses the following environment variables:
#
#   OPENAL_DIR
#     A directory to use as hint for locating OpenAL
#
# This module provides the following imported targets:
#
#   OpenAL::OpenAL
#     The OpenAL library
#
# This module defines the following result variables:
#
#   OPENAL_FOUND
#     True if the system has OpenAL
#   OPENAL_VERSION
#     The version of the found OpenAL (if available)
#   OPENAL_INCLUDE_DIRS
#     Include directories needed to use OpenAL
#   OPENAL_LIBRARIES
#     Libraries needed to link to OpenAL
#   OPENAL_DEFINITIONS
#     Definitions to use when compiling code that uses OpenAL
#
# This module defines the following cache variables:
#
#   OPENAL_INCLUDE_DIR
#     The directory containing 'al.h'
#   OPENAL_LIBRARY
#     The path to the OpenAL library
#

# try to obtain package data from pkg-config
find_package(PkgConfig QUIET)
pkg_check_modules(PC_OPENAL QUIET openal)

# locate OpenAL header
find_path(OPENAL_INCLUDE_DIR
  NAMES
    al.h
  HINTS
    ENV OPENAL_DIR
    ${PC_OPENAL_INCLUDEDIR}
    ${PC_OPENAL_INCLUDE_DIRS}
  PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /sw # Fink
    /opt/local # DarwinPorts
    /opt/csw # Blastwave
    /opt
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Creative\ Labs\\OpenAL\ 1.1\ Software\ Development\ Kit\\1.00.0000;InstallDir]
  PATH_SUFFIXES
    OpenAL
    AL
    include/OpenAL
    include/AL
    include
)

# locate OpenAL library
find_library(OPENAL_LIBRARY
  NAMES
    OpenAL
    al
    openal
    OpenAL32
  HINTS
    ENV OPENAL_DIR
    ${PC_OPENAL_LIBDIR}
    ${PC_OPENAL_LIBRARY_DIRS}
  PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /sw # Fink
    /opt/local # DarwinPorts
    /opt/csw # Blastwave
    /opt
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Creative\ Labs\\OpenAL\ 1.1\ Software\ Development\ Kit\\1.00.0000;InstallDir]
  PATH_SUFFIXES
    lib
    lib64
)

# extract OpenAL version
set(OPENAL_VERSION ${PC_OPENAL_VERSION})

# handle finding OpenAL
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenAL
  FOUND_VAR OPENAL_FOUND
  REQUIRED_VARS
    OPENAL_LIBRARY
    OPENAL_INCLUDE_DIR
  VERSION_VAR OPENAL_VERSION
)

# set user variables for compiling with OpenAL
if(OPENAL_FOUND)
  set(OPENAL_LIBRARIES ${OPENAL_LIBRARY})
  set(OPENAL_INCLUDE_DIRS ${OPENAL_INCLUDE_DIR})
  set(OPENAL_DEFINITIONS ${PC_OPENAL_CFLAGS_OTHER})
endif()

# create an imported target for OpenAL
if(OPENAL_FOUND AND NOT TARGET OpenAL::OpenAL)
  if(APPLE AND "${OPENAL_LIBRARY}" MATCHES "\\.framework$")
    add_library(OpenAL::OpenAL INTERFACE IMPORTED)
    set_target_properties(OpenAL::OpenAL PROPERTIES
      INTERFACE_LINK_LIBRARIES "${OPENAL_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${OPENAL_INCLUDE_DIR}"
      INTERFACE_COMPILE_OPTIONS "${OPENAL_DEFINITIONS}"
    )
  else()
    add_library(OpenAL::OpenAL UNKNOWN IMPORTED)
    set_target_properties(OpenAL::OpenAL PROPERTIES
      IMPORTED_LOCATION "${OPENAL_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${OPENAL_INCLUDE_DIR}"
      INTERFACE_COMPILE_OPTIONS "${OPENAL_DEFINITIONS}"
    )
  endif()
endif()

# mark cache variables as advanced
mark_as_advanced(
  OPENAL_LIBRARY
  OPENAL_INCLUDE_DIR
)
