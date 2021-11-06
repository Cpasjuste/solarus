# Default compilation flags.

# Compile as C++17.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set exception handling model and disable warnings in MSVC compilers.
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /w")
endif()

# Compile in release mode by default.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif()

# Compile libraries as shared by default.
if(NOT DEFINED BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS ON CACHE BOOL
        "Choose if building static or shared (default) libraries."
        FORCE)
endif()

# Warnings and errors.

# Be less pedantic in release builds for users.
set(CMAKE_CXX_FLAGS_RELEASE "-Wno-error -Wall -Wextra -Wno-unknown-pragmas -Wno-fatal-errors ${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Be more pedantic in debug mode for developers.
set(CMAKE_CXX_FLAGS_DEBUG "-Werror -Wall -Wextra -Wno-error=deprecated-declarations -pedantic ${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wsuggest-override")
endif()

# Platform-specific flags.
if(WIN32)
    # MinGW: disable the console by default.
    if(CMAKE_COMPILER_IS_GNUCXX)
        set(CMAKE_CXX_FLAGS "-mwindows ${CMAKE_CXX_FLAGS}")
    endif()
endif()

if(GCWZERO)
    add_definitions(-DGCWZERO)
endif()
