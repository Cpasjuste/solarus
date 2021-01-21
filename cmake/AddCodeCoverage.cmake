# Enable compilation of binaries with code coverage instrumentation.
set(SOLARUS_CODE_COVERAGE "OFF" CACHE BOOL "Enable compilation of binaries with code coverage instrumentation.")
if(SOLARUS_CODE_COVERAGE)
  add_compile_options(-fprofile-arcs -ftest-coverage)
  link_libraries(gcov)
endif()
