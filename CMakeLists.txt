cmake_minimum_required(VERSION 3.17)
project(SdlBase)

# https://gitlab.kitware.com/cmake/community/-/wikis/doc/cmake/Useful-Variables
# https://gitlab.kitware.com/cmake/community/-/wikis/doc/tutorials/How-To-Write-Platform-Checks

set(CMAKE_CXX_STANDARD 14)

IF(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX /wd4068") # really picky warnings, as errors, suppress 'unknown pragma' -- most of the clang-lint stuff
    set(CMAKE_CXX_FLAGS_RELEASE "/O2") # When using MSVC compiler
    set(SDL2_LINK_DIR "${PROJECT_SOURCE_DIR}/lib/SDL2-devel-2.0.9-VC/SDL2-2.0.9/lib/x86/SDL2.lib")
ELSE()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_DEBUG} -O2")
    find_package(SDL2 REQUIRED)
    set(SDL2_LINK_DIR "${SDL2_LIBRARIES}")
ENDIF()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")


include_directories(.)
include_directories(src)

IF(WIN32)
include_directories(./lib/SDL2-devel-2.0.9-VC/SDL2-2.0.9/include)
link_directories(./lib/SDL2-devel-2.0.9-VC/SDL2-2.0.9/lib)
ELSE()
    include_directories(${SDL2_INCLUDE_DIRS})
ENDIF()

add_executable(SdlBase
        # exe entry point
        src/main.cpp
        # base type library
        src/types/MathBits.h src/types/RawData.h
        src/types/ArenaAllocator.cpp src/types/ArenaAllocator.h
        src/types/MemoryManager.cpp src/types/MemoryManager.h
        src/types/HashMap.cpp src/types/HashMap.h
        src/types/Heap.cpp src/types/Heap.h
        src/types/Vector.cpp src/types/Vector.h
        src/types/String.cpp src/types/String.h
        # user app entry point
        src/app/app_start.cpp src/app/app_start.h
        src/app/scene.h src/synth/map_synth.h src/synth/map_synth.cpp src/types/general.h src/app/scene.cpp src/app/shared_types.h)

configure_file(lib/SDL2-devel-2.0.9-VC/SDL2-2.0.9/lib/x86/SDL2.dll SDL2.dll COPYONLY)
target_link_libraries(SdlBase "${SDL2_LINK_DIR}")

