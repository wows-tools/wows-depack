cmake_minimum_required(VERSION 3.12)

project(wows-depack)

# Project version (sematic versionning)
set(wows-depack_VERSION_MAJOR 0)
set(wows-depack_VERSION_MINOR 1)
set(wows-depack_VERSION_PATCH 0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

find_package(ZLIB REQUIRED)
find_package(PCRE REQUIRED)
find_library(CUNIT_LIBRARY cunit)

set(wows-depack_VERSION
    ${wows-depack_VERSION_MAJOR}.${wows-depack_VERSION_MINOR}.${wows-depack_VERSION_PATCH}
)

add_custom_target(
  tag
  COMMAND git tag -a ${wows-depack_VERSION} -m
          "tagging version ${wows-depack_VERSION}"
  COMMAND git push origin ${wows-depack_VERSION})

# set version as a definition
set(CMAKE_C_FLAGS
    "${CMAKE_CXX_FLAGS} -DBFD_VERSION='\"${wows-depack_VERSION}\"'")
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -DBFD_VERSION='\"${wows-depack_VERSION}\"'")

# Options
option(DEBUG "compile with debug symbol" OFF)
option(STATIC "compile statically" OFF)
option(USE_CLANG "build application with clang" OFF)
option(USE_GCC "build application with gcc" OFF)
option(FORCELE "force little endian architecture" OFF)
option(COVERAGE "Enable code coverage" OFF)

if(USE_CLANG)
  set(CMAKE_CXX_COMPILER "clang++")
  set(CMAKE_CC_COMPILER "clang")
endif(USE_CLANG)

if(USE_GCC)
  set(CMAKE_CXX_COMPILER "g++")
  set(CMAKE_CC_COMPILER "gcc")
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
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  # Mac OS X specific code
  set(EXTERNAL_ICONV "iconv")
  set(EXTERNAL_ARGP "argp")
  add_definitions(-DDARWIN)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

# linking directories
link_directories(${CMAKE_BINARY_DIR}/ /usr/local/lib /usr/lib/)

# headers directories
include_directories(./inc/ ${ZLIB_INCLUDE_DIRS} ${PCRE_INCLUDE_DIRS}
                    /usr/include/ /sw/include/)

add_library(
  wows-depack
  ${SHARED}
  lib/utils.c
  lib/index-parser.c
  lib/index-printer.c
  lib/hashmap.c
  lib/hashmap-helpers.c
  lib/memory-helpers.c
  lib/inode.c
  lib/error.c)

set_target_properties(
  wows-depack PROPERTIES VERSION ${wows-depack_VERSION}
                         SOVERSION ${wows-depack_VERSION_MAJOR})

# Compile the executable
add_executable(wows-depack-cli wows-depack.c)
target_link_libraries(wows-depack ${ZLIB_LIBRARIES} ${PCRE_LIBRARY})

target_link_libraries(wows-depack-cli wows-depack ${ZLIB_LIBRARIES}
                      ${PCRE_LIBRARY})

if(CUNIT_LIBRARY)
  if(COVERAGE)
    set(CMAKE_BUILD_TYPE Debug)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
    include(CodeCoverage)
    setup_target_for_coverage(NAME coverage EXECUTABLE wows-depack-test
                              DEPENDENCIES wows-depack-test)

    add_dependencies(coverage wows-depack-test)
  endif(COVERAGE)

  add_executable(wows-depack-test tests/tests.c)
  target_link_libraries(wows-depack-test wows-depack ${ZLIB_LIBRARIES}
                        ${PCRE_LIBRARY} ${CUNIT_LIBRARY})

  # Add the test target
  add_custom_target(
    tests
    COMMAND wows-depack-test
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running tests")
  add_dependencies(tests wows-depack-test)

else()
  message(WARNING "CUnit not found; not building tests.")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall")

if(NOT LIB_INSTALL_DIR)
  set(LIB_INSTALL_DIR lib)
endif()

if(NOT BIN_INSTALL_DIR)
  set(BIN_INSTALL_DIR bin)
endif()

if(NOT INCLUDE_INSTALL_DIR)
  set(INCLUDE_INSTALL_DIR include)
endif()

# install binaries and library
install(
  TARGETS wows-depack wows-depack-cli
  RUNTIME DESTINATION ${BIN_INSTALL_DIR}
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR})
