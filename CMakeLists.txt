cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "x86_64")
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# enable debug
set(GB_DEBUG On)

if(GB_DEBUG)
    add_compile_definitions(GB_DEBUG)
endif()

project(GatoBot VERSION 1.4.0)

#set(GEODE_LINK_NIGHTLY On)
file(GLOB_RECURSE SOURCES
	src/*.cpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC 
    src
    external/ffmpeg/src
)

# link ffmpeg
# https://github.com/getsentry/sentry-native/issues/173
set(CMAKE_CXX_FLAGS           "${CMAKE_CXX_FLAGS} /D \"SAFESEH:NO\"")
set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO")

file(GLOB FFMPEG_LIBS
  external/ffmpeg/bin/win32/avcodec.lib
  external/ffmpeg/bin/win32/avutil.lib
)


# set up geode
if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

target_link_libraries(${PROJECT_NAME} ${FFMPEG_LIBS})

setup_geode_mod(${PROJECT_NAME})
