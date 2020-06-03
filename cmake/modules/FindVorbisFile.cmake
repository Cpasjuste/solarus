# FindVorbisFile - finds the VorbisFile library
#
# This module uses the following environment variables:
#
#   VORBISFILE_DIR
#     A directory to use as hint for locating VorbisFile
#
# This module provides the following imported targets:
#
#   Vorbis::File
#     The VorbisFile library
#
# This module defines the following result variables:
#
#   VORBISFILE_FOUND
#     True if the system has VorbisFile
#   VORBISFILE_VERSION
#     The version of the found VorbisFile (if available)
#   VORBISFILE_INCLUDE_DIRS
#     Include directories needed to use VorbisFile
#   VORBISFILE_LIBRARIES
#     Libraries needed to link to VorbisFile
#   VORBISFILE_DEFINITIONS
#     Definitions to use when compiling code that uses VorbisFile
#
# This module defines the following cache variables:
#
#   VORBISFILE_INCLUDE_DIR
#     The directory containing 'vorbis/vorbisfile.h'
#   VORBISFILE_LIBRARY
#     The path to the VorbisFile library
#

# try to obtain package data from pkg-config
find_package(PkgConfig QUIET)
pkg_check_modules(PC_VORBISFILE QUIET vorbisfile)

# locate VorbisFile header
find_path(VORBISFILE_INCLUDE_DIR
  NAMES
    vorbis/vorbisfile.h
  HINTS
    ENV VORBISFILE_DIR
    ${PC_VORBISFILE_INCLUDEDIR}
    ${PC_VORBISFILE_INCLUDE_DIRS}
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

# locate VorbisFile library
find_library(VORBISFILE_LIBRARY
  NAMES
    vorbisfile
  HINTS
    ENV VORBISFILE_DIR
    ${PC_VORBISFILE_LIBDIR}
    ${PC_VORBISFILE_LIBRARY_DIRS}
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

# extract VorbisFile version
set(VORBISFILE_VERSION ${PC_VORBISFILE_VERSION})

# handle finding VorbisFile
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VorbisFile
  FOUND_VAR VORBISFILE_FOUND
  REQUIRED_VARS
    VORBISFILE_LIBRARY
    VORBISFILE_INCLUDE_DIR
  VERSION_VAR VORBISFILE_VERSION
)

# set user variables for compiling with VorbisFile
if(VORBISFILE_FOUND)
  set(VORBISFILE_LIBRARIES ${VORBISFILE_LIBRARY})
  set(VORBISFILE_INCLUDE_DIRS ${VORBISFILE_INCLUDE_DIR})
  set(VORBISFILE_DEFINITIONS ${PC_VORBISFILE_CFLAGS_OTHER})
endif()

# create an imported target for VorbisFile
if(VORBISFILE_FOUND AND NOT TARGET Vorbis::File)
  add_library(Vorbis::File UNKNOWN IMPORTED)
  set_target_properties(Vorbis::File PROPERTIES
    IMPORTED_LOCATION "${VORBISFILE_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${VORBISFILE_INCLUDE_DIR}"
    INTERFACE_COMPILE_OPTIONS "${VORBISFILE_DEFINITIONS}"
  )
endif()

# mark cache variables as advanced
mark_as_advanced(
  VORBISFILE_LIBRARY
  VORBISFILE_INCLUDE_DIR
)
