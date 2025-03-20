message(STATUS "cmake version ${CMAKE_VERSION}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

project(rkadk)

install(DIRECTORY include/ DESTINATION include/rkadk
        FILES_MATCHING PATTERN "*.h")

message("RKMEDIA_HEADER_DIR = ${RKMEDIA_HEADER_DIR}")
include_directories(${RKMEDIA_HEADER_DIR})
include_directories(include)

if(USE_ROCKIT)
    message("ROCKIT_HEADER_DIR = ${ROCKIT_HEADER_DIR}")
    include_directories(${ROCKIT_HEADER_DIR})
    add_definitions(-DROCKIT)
endif()

if(USE_RKAIQ)
    find_package(RkAiq REQUIRED)
    include_directories(${RKAIQ_INCLUDE_DIRS})
    add_definitions(-DRKAIQ)
endif()

#-Wextra
add_definitions(-Wall
		-Wshadow
		-ffast-math
		-Wignored-qualifiers
		-Werror-implicit-function-declaration
		-fPIC)

add_subdirectory(src)
add_subdirectory(examples)
