# Solarus-specific -D options.

# AppID to report to the window system on Linux/Wayland.
# From a user PoV this also determines were the corresponding XDG metadata is installed in the system.
set(SOLARUS_APP_ID "org.solarus_games.solarus" CACHE STRING "AppID base for installed XDG metadata on Linux/BSD.")

# Quest to launch if none is specified at runtime.
set(SOLARUS_DEFAULT_QUEST "." CACHE STRING "Path to the quest to launch if none is specified at runtime.")

# Base directory where to write files.
# If blank it will be set depending on the OS (typically the user's home directory).
set(SOLARUS_BASE_WRITE_DIR "" CACHE STRING "Base directory where to write files, if blank it will be set depending on the OS (typically the user's home directory).")

# Directory where to write savegames and other files saved by quests.
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(SOLARUS_INITIAL_WRITE_DIR "Solarus")
else()
  set(SOLARUS_INITIAL_WRITE_DIR ".solarus")
endif()
set(SOLARUS_WRITE_DIR ${SOLARUS_INITIAL_WRITE_DIR} CACHE STRING "Directory where Solarus savegames are stored, relative to the base write directory.")

# Quest size.
if(PANDORA)
  set(SOLARUS_INITIAL_DEFAULT_QUEST_WIDTH 400)
else()
  set(SOLARUS_INITIAL_DEFAULT_QUEST_WIDTH 320)
endif()
set(SOLARUS_DEFAULT_QUEST_WIDTH ${SOLARUS_INITIAL_DEFAULT_QUEST_WIDTH} CACHE STRING "Default width of the quest screen in pixels.")
set(SOLARUS_DEFAULT_QUEST_HEIGHT 240 CACHE STRING "Default height of the quest screen in pixels.")

# Use OpenGL ES implementation.
set(SOLARUS_GL_ES "OFF" CACHE BOOL "Use OpenGL ES implementation.")

# Enable logging of errors to file.
set(SOLARUS_FILE_LOGGING "ON" CACHE BOOL "Enable logging of errors to file.")

# Hack to make Lua open files with _wfopen_s() instead of fopen() on Windows
# for Unicode filenames support.
# This setting only has an effet on Windows.
# Systems other that Windows don't need this and can just call fopen() with UTF-8.
set(SOLARUS_LUA_WINDOWS_WFOPEN_WORKAROUND ON BOOL "Enable workaround to make Lua open filenames with Unicode on Windows")

