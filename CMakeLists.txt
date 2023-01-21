cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 17)

project(GatoBot)

# src files
file(GLOB_RECURSE SOURCE_FILES 
    src/*.cpp 

    # nativefiledialog files
    libraries/nativefiledialog/src/nfd_common.c 
    libraries/nativefiledialog/src/nfd_win.cpp 
)
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

set(CMAKE_SIZEOF_VOID_P 4)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
  # ensure 32 bit on clang
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -target i386-pc-windows-msvc")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -target i386-pc-windows-msvc")
  add_definitions("--target=i386-pc-windows-msvc")
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
  libraries/
  libraries/minhook/include
  libraries/gd.h/include
  libraries/gd.h/
  libraries/nativefiledialog/src
  libraries/nativefiledialog/src/include
)

add_subdirectory(libraries/minhook)
add_subdirectory(libraries/cocos-headers)

find_library(OPENGL32_LIBRARY_PATH opengl32)

target_link_libraries(${PROJECT_NAME} minhook cocos2d opengl32 ${CMAKE_SOURCE_DIR}/libraries/glew32.lib)