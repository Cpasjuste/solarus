# Default installation directories.
set(SOLARUS_GUI_SHARE_INSTALL_DESTINATION
  "${SOLARUS_SHARE_INSTALL_DESTINATION}/solarus-gui" CACHE PATH
  "GUI shared files install destination")

# Set files to install.
install(TARGETS solarus-gui solarus-launcher
  ARCHIVE DESTINATION ${SOLARUS_LIBRARY_INSTALL_DESTINATION}
  LIBRARY DESTINATION ${SOLARUS_LIBRARY_INSTALL_DESTINATION}
  RUNTIME DESTINATION ${SOLARUS_EXECUTABLE_INSTALL_DESTINATION}
)
install(DIRECTORY
  "${CMAKE_CURRENT_SOURCE_DIR}/include/solarus"
  DESTINATION ${SOLARUS_HEADERS_INSTALL_DESTINATION}
)
install(FILES
  ${solarus-gui_FORMS_HEADERS}
  DESTINATION "${SOLARUS_HEADERS_INSTALL_DESTINATION}/solarus/gui"
)
install(FILES
  ${solarus-gui_TRANSLATIONS_QM}
  DESTINATION "${SOLARUS_GUI_SHARE_INSTALL_DESTINATION}/translations"
)

# FreeDesktop compatible icons
if(UNIX AND NOT APPLE)
  foreach(SUFFIX IN ITEMS Launcher Runner)
    # Pixmap icons for sizes under 48x48 pixels
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/images/icon/solarus_launcher_icon_16.png
      DESTINATION ${SOLARUS_SHARE_INSTALL_DESTINATION}/icons/hicolor/16x16/apps RENAME ${SOLARUS_APP_ID}.${SUFFIX}.png)
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/images/icon/solarus_launcher_icon_20.png
      DESTINATION ${SOLARUS_SHARE_INSTALL_DESTINATION}/icons/hicolor/20x20/apps RENAME ${SOLARUS_APP_ID}.${SUFFIX}.png)
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/images/icon/solarus_launcher_icon_24.png
      DESTINATION ${SOLARUS_SHARE_INSTALL_DESTINATION}/icons/hicolor/24x24/apps RENAME ${SOLARUS_APP_ID}.${SUFFIX}.png)
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/images/icon/solarus_launcher_icon_32.png
      DESTINATION ${SOLARUS_SHARE_INSTALL_DESTINATION}/icons/hicolor/32x32/apps RENAME ${SOLARUS_APP_ID}.${SUFFIX}.png)
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/images/icon/solarus_launcher_icon_40.png
      DESTINATION ${SOLARUS_SHARE_INSTALL_DESTINATION}/icons/hicolor/40x40/apps RENAME ${SOLARUS_APP_ID}.${SUFFIX}.png)
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/images/icon/solarus_launcher_icon_48.png
      DESTINATION ${SOLARUS_SHARE_INSTALL_DESTINATION}/icons/hicolor/48x48/apps RENAME ${SOLARUS_APP_ID}.${SUFFIX}.png)

    # Pixmap icon for desktop that don't support multiple sizes
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/images/icon/solarus_launcher_icon_512.png
      DESTINATION ${SOLARUS_SHARE_INSTALL_DESTINATION}/pixmaps RENAME ${SOLARUS_APP_ID}.${SUFFIX}.png)

    # Vector icons, automatically chosen for sizes above 48x48 pixels
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/images/icon/solarus_launcher_icon.svg
      DESTINATION ${SOLARUS_SHARE_INSTALL_DESTINATION}/icons/hicolor/scalable/apps RENAME ${SOLARUS_APP_ID}.${SUFFIX}.svg)
    install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/images/icon/solarus_launcher_icon_symbolic.svg
      DESTINATION ${SOLARUS_SHARE_INSTALL_DESTINATION}/icons/hicolor/symbolic/apps RENAME ${SOLARUS_APP_ID}.${SUFFIX}-symbolic.svg)
  endforeach()
endif(UNIX AND NOT APPLE)

# FreeDesktop compatible start menu launcher
if(UNIX AND NOT APPLE)
  configure_file (resources/solarus-launcher.desktop.in resources/${SOLARUS_APP_ID}.Launcher.desktop @ONLY)
  install (FILES ${CMAKE_CURRENT_BINARY_DIR}/resources/${SOLARUS_APP_ID}.Launcher.desktop
    DESTINATION ${SOLARUS_SHARE_INSTALL_DESTINATION}/applications)
endif(UNIX AND NOT APPLE)

# AppStream compatible software gallery metadata
if(UNIX AND NOT APPLE)
  configure_file (resources/solarus-launcher.appdata.xml.in resources/${SOLARUS_APP_ID}.appdata.xml @ONLY)
  install (FILES ${CMAKE_CURRENT_BINARY_DIR}/resources/${SOLARUS_APP_ID}.appdata.xml
    DESTINATION ${SOLARUS_SHARE_INSTALL_DESTINATION}/metainfo)
endif(UNIX AND NOT APPLE)

# Linux Manpage
if(UNIX AND NOT APPLE)
  install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/solarus-launcher.6
    DESTINATION ${SOLARUS_MANUAL_INSTALL_DESTINATION}/man6)
endif(UNIX AND NOT APPLE)
