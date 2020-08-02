# Declare the "solarus-run" executable target and its public/private sources
add_executable(solarus-run "")
target_sources(solarus-run
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main/Main.cpp"
)

# Set meta-information for Windows-based builds.
if(MINGW OR WIN32)
  set(WIN32_RUN_APPLICATION_DISPLAY_NAME "Solarus-Run")
  set(WIN32_RUN_PROJECT_VERSION "${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0")
  set(WIN32_RUN_PROJECT_VERSION_STR "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.0")
  set(WIN32_RUN_FILE_VERSION_STR "${WIN32_PROJECT_VERSION_STR}")
  set(WIN32_RUN_PROJECT_INTERNAL_NAME "${WIN32_APPLICATION_DISPLAY_NAME}")
  set(WIN32_RUN_ORIGINAL_FILE_NAME "solarus-run.exe")

  get_filename_component(WIN32_RUN_ICON_FILEPATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/icons/solarus_icon.ico"
    REALPATH
  )

  # Get absolute native paths for icon(s) defined in resources.rc.
  if(MINGW)
    set(WIN32_RUN_ICON_FILEPATH_WIN ${WIN32_RUN_ICON_FILEPATH})
  elseif(WIN32)
    string(REPLACE "/" "\\\\" WIN32_RUN_ICON_FILEPATH_WIN ${WIN32_ICON_FILEPATH})
  endif()
  
  # Configure resources.rc file with that information.
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/win32/resources.rc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/src/resources.rc"
  )

  # Add it as source file.
  target_sources(solarus-run
    PRIVATE
      "${CMAKE_CURRENT_BINARY_DIR}/src/resources.rc"
  )
endif()

# Declare the public/private libraries that "solarus-run" depends on
target_link_libraries(solarus-run
  PUBLIC
    solarus
    SDL2::Main
)
