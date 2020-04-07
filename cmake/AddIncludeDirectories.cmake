# Include paths.
include_directories(
    "${CMAKE_BINARY_DIR}/include"  # For config.h.
    "${CMAKE_SOURCE_DIR}/include"
)

# Third-party include paths, marked as system ones to disable their warnings.
include_directories(SYSTEM
    "${CMAKE_SOURCE_DIR}/include/solarus/third_party"
    "${CMAKE_SOURCE_DIR}/include/solarus/third_party/snes_spc"
)

# External include paths, marked as system ones to disable their warnings.
include_directories(SYSTEM
    "${MODPLUG_INCLUDE_DIRS}"  # Before SDL2 because we want the sndfile.h of ModPlug.
    "${SDL2_INCLUDE_DIRS}"
    "${SDL2_IMAGE_INCLUDE_DIRS}"
    "${SDL2_TTF_INCLUDE_DIRS}"
    "${GLM_INCLUDE_DIRS}"
    "${OPENAL_INCLUDE_DIRS}"
    "${VORBIS_INCLUDE_DIRS}"
    "${VORBISFILE_INCLUDE_DIRS}"
    "${OGG_INCLUDE_DIRS}"
    "${LUA_INCLUDE_DIRS}"
    "${PHYSFS_INCLUDE_DIRS}"
)

if (OPENGL_INCLUDE_DIR)
    include_directories(SYSTEM
        "${OPENGL_INCLUDE_DIR}"
    )
endif()
