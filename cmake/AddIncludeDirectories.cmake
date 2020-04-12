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
