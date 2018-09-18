# Default compilation flags.

# Compile as C++11.
if(MINGW)
    # To avoid a compilation error in vorbisfile.h with fseeko64.
    set(CMAKE_CXX_FLAGS "-std=gnu++11 ${CMAKE_CXX_FLAGS}")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
    set(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++ ${CMAKE_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
else()
    set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")
endif()

# Compile in release mode by default.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif()

# Warnings and errors.

# Be less pedantic in release builds for users.
set(CMAKE_CXX_FLAGS_RELEASE "-Wno-error -Wall -Wextra -Wno-unknown-pragmas -Wno-fatal-errors ${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Be more pedantic in debug mode for developers.
set(CMAKE_CXX_FLAGS_DEBUG "-Werror -Wall -Wextra -pedantic ${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
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
