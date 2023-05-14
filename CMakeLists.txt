cmake_minimum_required(VERSION 3.3.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(GatoBot VERSION 1.0.0)

# build geode version
set(GB_BUILD_GEODE ON)

# src files
file(GLOB SOURCE_FILES 
    src/*.cpp  

    # nativefiledialog files
    libraries/nativefiledialog/src/nfd_common.c 
    libraries/nativefiledialog/src/nfd_win.cpp 
)

# funny definition
if(GB_BUILD_GEODE)
    add_compile_definitions(GB_GEODE)
endif()

# force C++ compiler
set_source_files_properties(libraries/nativefiledialog/src/nfd_common.c PROPERTIES LANGUAGE CXX)

if(GB_BUILD_GEODE)
    file(GLOB MAIN_SRC src/geode/*.cpp) 
else()
    file(GLOB MAIN_SRC src/standalone/*.cpp) 
endif()

list(APPEND SOURCE_FILES ${MAIN_SRC})

set(CMAKE_SIZEOF_VOID_P 4)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
  # ensure 32 bit on clang
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -target i386-pc-windows-msvc")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -target i386-pc-windows-msvc")
  add_definitions("--target=i386-pc-windows-msvc")
endif()

# headers
set(PROJECTHEADERS 
  libraries/
  libraries/nativefiledialog/src
  libraries/nativefiledialog/src/include
  libraries/gd.h/include
  libraries/gd.h/
)

find_library(OPENGL32_LIBRARY_PATH opengl32)
set(MOD_LIBS opengl32 ${CMAKE_SOURCE_DIR}/libraries/glew32.lib)

if(NOT GB_BUILD_GEODE)
  list(APPEND PROJECTHEADERS libraries/minhook/include)
  add_subdirectory(libraries/minhook)
  list(APPEND MOD_LIBS minhook)

  add_subdirectory(libraries/cocos-headers)
  list(APPEND MOD_LIBS cocos2d)
endif()

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECTHEADERS})
target_link_libraries(${PROJECT_NAME} ${MOD_LIBS})

if(GB_BUILD_GEODE)
  # Set up geode
  if (NOT DEFINED ENV{GEODE_SDK})
      message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
  else()
      message(STATUS "Found Geode: $ENV{GEODE_SDK}")
  endif()

  add_subdirectory($ENV{GEODE_SDK} $ENV{GEODE_SDK}/build)

  # change directory to geode folder for geode assets
  set(CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/geode)

  setup_geode_mod(${PROJECT_NAME})
endif()