include(GNUInstallDirs)

set(SOLARUS_LIBRARY_INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}" CACHE PATH "Library install destination")
# Install location for Debian-based systems
if(EXISTS '/etc/debian_version')
  set(SOLARUS_EXECUTABLE_INSTALL_DESTINATION "games" CACHE PATH "Binary install destination")
else()
  set(SOLARUS_EXECUTABLE_INSTALL_DESTINATION "${CMAKE_INSTALL_BINDIR}" CACHE PATH "Binary install destination")
endif()
set(SOLARUS_SHARE_INSTALL_DESTINATION "${CMAKE_INSTALL_DATADIR}" CACHE PATH "Shared files install destination")
set(SOLARUS_MANUAL_INSTALL_DESTINATION "${CMAKE_INSTALL_MANDIR}" CACHE PATH "Manual install destination")
set(SOLARUS_HEADERS_INSTALL_DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" CACHE PATH "Headers install destination")

# Files to install with make install.
# Install the shared library and the solarus-run executable.
install(TARGETS solarus solarus-run
  ARCHIVE DESTINATION ${SOLARUS_LIBRARY_INSTALL_DESTINATION}
  LIBRARY DESTINATION ${SOLARUS_LIBRARY_INSTALL_DESTINATION}
  RUNTIME DESTINATION ${SOLARUS_EXECUTABLE_INSTALL_DESTINATION}
)
# Install headers: useful for projects that use Solarus as a library.
install(DIRECTORY
  "${CMAKE_BINARY_DIR}/include/solarus"  # For config.h.
  "${CMAKE_SOURCE_DIR}/include/solarus"
  DESTINATION ${SOLARUS_HEADERS_INSTALL_DESTINATION}
)
# Linux Manpage
if(UNIX AND NOT APPLE)
  install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/solarus-run.6
    DESTINATION ${SOLARUS_MANUAL_INSTALL_DESTINATION}/man6)
endif(UNIX AND NOT APPLE)
