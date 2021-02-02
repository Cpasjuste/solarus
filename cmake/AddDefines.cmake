# Solarus-specific -D preprocessor constants.

add_definitions(-DSOLARUS_APP_ID=\"${SOLARUS_APP_ID}\")

if(SOLARUS_DEFAULT_QUEST)
  add_definitions(-DSOLARUS_DEFAULT_QUEST=\"${SOLARUS_DEFAULT_QUEST}\")
endif()

if(SOLARUS_BASE_WRITE_DIR)
  add_definitions(-DSOLARUS_BASE_WRITE_DIR=\"${SOLARUS_BASE_WRITE_DIR}\")
endif()

if(SOLARUS_WRITE_DIR)
  add_definitions(-DSOLARUS_WRITE_DIR=\"${SOLARUS_WRITE_DIR}\")
endif()

if(SOLARUS_DEFAULT_QUEST_WIDTH)
  add_definitions(-DSOLARUS_DEFAULT_QUEST_WIDTH=${SOLARUS_DEFAULT_QUEST_WIDTH})
endif()
if(SOLARUS_DEFAULT_QUEST_HEIGHT)
  add_definitions(-DSOLARUS_DEFAULT_QUEST_HEIGHT=${SOLARUS_DEFAULT_QUEST_HEIGHT})
endif()

if(SOLARUS_GL_ES)
  add_definitions(-DSOLARUS_GL_ES)
endif()

if(SOLARUS_FILE_LOGGING)
  add_definitions(-DSOLARUS_FILE_LOGGING)
endif()
