cmake_minimum_required (VERSION 2.6)

project (bf-deflate)

# Project version (sematic versionning)
set(bf-deflate_VERSION_MAJOR 0)
set(bf-deflate_VERSION_MINOR 1)
set(bf-deflate_VERSION_PATCH 0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

find_package(ZLIB REQUIRED)

set(bf-deflate_VERSION ${bf-deflate_VERSION_MAJOR}.${bf-deflate_VERSION_MINOR}.${bf-deflate_VERSION_PATCH})

add_custom_target(tag
    COMMAND git tag -a ${bf-deflate_VERSION} -m "tagging version ${bf-deflate_VERSION}"
    COMMAND git push origin ${bf-deflate_VERSION}
)


# set version as a definition
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -DBFD_VERSION='\"${bf-deflate_VERSION}\"'")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBFD_VERSION='\"${bf-deflate_VERSION}\"'")

# Options
option(DEBUG     "compile with debug symbol"    OFF)
option(STATIC    "compile statically"           OFF)
option(USE_CLANG "build application with clang" OFF)
option(USE_GCC   "build application with gcc"   OFF)
option(FORCELE   "force little endian architecture"   OFF)

if(USE_CLANG)
    set(CMAKE_CXX_COMPILER "clang++")
    set(CMAKE_CC_COMPILER  "clang"  )
endif(USE_CLANG)

if(USE_GCC)
    set(CMAKE_CXX_COMPILER "g++")
    set(CMAKE_CC_COMPILER  "gcc")
endif(USE_GCC)

if(DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g")
    set(CMAKE_BUILD_TYPE Debug)
endif(DEBUG)

if(STATIC)
    set(SHARED "")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(BUILD_SHARED_LIBRARIES OFF)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
else(STATIC)
    set(SHARED "SHARED")
endif(STATIC)

if(UNIX)
  link_libraries(m)
endif(UNIX)

# Build external dependancies if we are on OSX
IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   # Mac OS X specific code
   set(EXTERNAL_ICONV "iconv")
   set(EXTERNAL_ARGP "argp")
   add_definitions(-DDARWIN)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

# linking directories
LINK_DIRECTORIES(
    ${CMAKE_BINARY_DIR}/
    /usr/local/lib
    /usr/lib/
)

# headers directories
include_directories(
    ./inc/
    ${ZLIB_INCLUDE_DIRS}
    /usr/include/ 
    /sw/include/
)

# Compile the executable
add_executable(bf-deflate bf-deflate.c)
target_link_libraries(bf-deflate
    ${ZLIB_LIBRARIES}
) 


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall")

if (NOT BIN_INSTALL_DIR)
    set(BIN_INSTALL_DIR bin)
endif ()

# install binaries and library
INSTALL(TARGETS bf-deflate bf-deflate
  RUNTIME DESTINATION ${BIN_INSTALL_DIR}
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)
