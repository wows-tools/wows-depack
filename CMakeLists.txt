cmake_minimum_required(VERSION 3.12)

project(wows-depack)

# Project version (sematic versionning)
set(wows-depack_VERSION_MAJOR 0)
set(wows-depack_VERSION_MINOR 4)
set(wows-depack_VERSION_PATCH 0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

if(STATIC)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  set(ZLIB_USE_STATIC_LIBS ON)
  set(BUILD_SHARED_LIBRARIES OFF)
  set(CMAKE_EXE_LINKER_FLAGS "-static")
else(STATIC)
  set(SHARED "SHARED")
endif(STATIC)

find_package(ZLIB REQUIRED)
find_package(PCRE2 REQUIRED)
# vcpkg Config mode exports a pcre2::pcre2-8 target rather than PCRE2_LIBRARY;
# normalize so the rest of the file can use PCRE2_LIBRARY unconditionally.
if(NOT PCRE2_LIBRARY)
  if(TARGET pcre2::pcre2-8)
    set(PCRE2_LIBRARY pcre2::pcre2-8)
  elseif(PCRE2_LIBRARIES)
    set(PCRE2_LIBRARY ${PCRE2_LIBRARIES})
  endif()
endif()
find_library(CUNIT_LIBRARY cunit)

# deps/compat provides posix_compat.h (endian + Windows POSIX shims) on all platforms.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps/compat)

# fmem: portable open_memstream-like API (used by tests).
add_subdirectory(deps/fmem)

if(WIN32)
  # Bundled mmap shim (mman-win32) — provides sys/mman.h and mmap/munmap via Win32 API.
  set(MMAN_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/deps/mman-win32/mmap.c)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps/mman-win32)
  # POSIX compat implementations (realpath, open_memstream, fclose wrapper).
  set(COMPAT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/deps/compat/posix_compat.c)
  set(MMAN_LIBRARIES "")
else()
  set(MMAN_SOURCES "")
  set(COMPAT_SOURCES "")
  set(MMAN_LIBRARIES "")
endif()

# Detect argp: glibc provides it in libc; macOS/BSD need an external library or our bundled one
include(CheckIncludeFile)
check_include_file("argp.h" HAVE_ARGP_H)
if(HAVE_ARGP_H)
  # argp.h found — it may live in libc (Linux) or need -largp (macOS with brew)
  find_library(_ARGP_SYSTEM_LIB argp)
  if(_ARGP_SYSTEM_LIB)
    set(ARGP_LIBRARIES ${_ARGP_SYSTEM_LIB})
  else()
    set(ARGP_LIBRARIES "")
  endif()
  set(ARGP_INCLUDE_DIRS "")
  message(STATUS "Using system argp")
else()
  # No system argp — build bundled argp-standalone via its own CMakeLists.txt.
  message(STATUS "System argp not found, building bundled argp from deps/argp/")
  add_subdirectory(deps/argp)
  set(ARGP_LIBRARIES argp-standalone)
  # deps/argp/include/argp.h is a thin wrapper that re-exports argp-standalone.h
  # so consumers can use the standard #include <argp.h>.
  set(ARGP_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/deps/argp/include)
endif()

set(wows-depack_VERSION
    ${wows-depack_VERSION_MAJOR}.${wows-depack_VERSION_MINOR}.${wows-depack_VERSION_PATCH}
)

add_custom_target(
  tag
  COMMAND git tag -a ${wows-depack_VERSION} -m
          "tagging version ${wows-depack_VERSION}"
  COMMAND git push origin ${wows-depack_VERSION})

# Expose version string to all targets.  add_compile_definitions handles
# quoting correctly on all platforms (no shell single-quote tricks needed).
add_compile_definitions(BFD_VERSION="${wows-depack_VERSION}")

# Options
option(DEBUG "compile with debug symbol" OFF)
option(STATIC "compile statically" OFF)
option(COVERAGE "Enable code coverage" OFF)
option(BUILD_DOC "Build documentation" OFF)
option(BUILD_TESTS "Build tests" OFF)

if(DEBUG)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g")
  set(CMAKE_BUILD_TYPE Debug)
endif(DEBUG)

# macOS may need -liconv for some libraries
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(EXTERNAL_ICONV "iconv")
  add_definitions(-DDARWIN)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

if(UNIX)
  link_libraries(m)
endif(UNIX)

# linking directories
link_directories(${CMAKE_BINARY_DIR}/ /usr/local/lib /usr/lib/)

# headers directories
include_directories(./inc/ ${ZLIB_INCLUDE_DIRS} ${PCRE2_INCLUDE_DIRS}
                    /usr/include/ /sw/include/)

file(GLOB WOWS_SOURCES lib/*.c)
add_library(wows-depack ${SHARED} ${WOWS_SOURCES} ${MMAN_SOURCES} ${COMPAT_SOURCES})
set_target_properties(
  wows-depack PROPERTIES VERSION ${wows-depack_VERSION}
                         SOVERSION ${wows-depack_VERSION_MAJOR})

# Compile the executable
add_executable(wows-depack-cli wows-depack.c)
target_include_directories(wows-depack-cli PRIVATE ${ARGP_INCLUDE_DIRS})
target_link_libraries(wows-depack ${ZLIB_LIBRARIES} ${PCRE2_LIBRARY} ${MMAN_LIBRARIES})

target_link_libraries(wows-depack-cli wows-depack ${ZLIB_LIBRARIES}
                      ${PCRE2_LIBRARY} ${ARGP_LIBRARIES} ${MMAN_LIBRARIES})

if(BUILD_TESTS)
  if(CUNIT_LIBRARY)

    include(CTest)
    add_test(NAME wows-depack-test COMMAND wows-depack-test)
    add_test(NAME hashmap-test COMMAND hashmap-test)
    add_test(NAME hashmap-bench COMMAND hashmap-test)
    set_property(TEST hashmap-bench PROPERTY ENVIRONMENT "BENCH=1")
    add_executable(wows-depack-test tests/tests.c)
    add_executable(hashmap-test lib/hashmap.c)
    set_target_properties(hashmap-test PROPERTIES COMPILE_FLAGS
                                                  "-DHASHMAP_TEST")
    target_link_libraries(wows-depack-test wows-depack ${ZLIB_LIBRARIES}
                          ${PCRE2_LIBRARY} ${CUNIT_LIBRARY} fmem::fmem)

    # Add the test target
    add_custom_target(
      tests
      COMMAND wows-depack-test
      COMMAND hashmap-test
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMENT "Running tests")
    add_dependencies(tests wows-depack-test)

    if(COVERAGE)
      set(CMAKE_BUILD_TYPE Debug)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
      include(CodeCoverage)
      setup_target_for_coverage(
        NAME
        coverage
        EXECUTABLE
        ctest
        -j
        ${n_cores}
        DEPENDENCIES
        wows-depack-test
        hashmap-test)

      add_dependencies(coverage wows-depack-test)
    endif(COVERAGE)

  else(CUNIT_LIBRARY)
    message(WARNING "CUnit not found; not building tests.")
  endif(CUNIT_LIBRARY)
endif(BUILD_TESTS)

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

# check if Doxygen is installed
if(BUILD_DOC)
  # set input and output files
  find_package(Doxygen)
  if(DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    # request to configure the file
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    message("Doxygen build started")

    # note the option ALL which allows to build the docs together with the
    # application
    add_custom_target(
      doc_doxygen ALL
      COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM)
  else(DOXYGEN_FOUND)
    message(
      "Doxygen need to be installed to generate the doxygen documentation")
  endif(DOXYGEN_FOUND)
endif(BUILD_DOC)

# install header file
install(FILES inc/wows-depack.h DESTINATION ${INCLUDE_INSTALL_DIR})
