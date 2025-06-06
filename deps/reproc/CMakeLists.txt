# 3.13 is needed for `target_link_options`.
cmake_minimum_required(VERSION 3.12...3.16)

# TODO: Remove once CMake 3.17 is released.
if(CMAKE_GENERATOR MATCHES "Unix Makefiles|Ninja")
  set(
    CMAKE_EXPORT_COMPILE_COMMANDS
    $ENV{CMAKE_EXPORT_COMPILE_COMMANDS}
    CACHE BOOL "Enable/Disable output of compile commands during generation."
  )
endif()

project(
  reproc
  VERSION 10.0.0
  DESCRIPTION "Cross-platform C99/C++11 process library"
  HOMEPAGE_URL "https://github.com/DaanDeMeyer/reproc"
  LANGUAGES C
)

# Common options and functions separated for easier reuse in other projects.
include(cmake/reproc.cmake)

option(REPROC++ "Build reproc++." ${REPROC_DEVELOP})
option(REPROC_TEST "Build tests." ${REPROC_DEVELOP})
option(REPROC_EXAMPLES "Build examples." ${REPROC_DEVELOP})

option(
  REPROC_MULTITHREADED
  "Use `pthread_sigmask` and link against the system's thread library"
  ON
)

if(REPROC++ OR REPROC_TEST)
  enable_language(CXX)
endif()

include(cmake/external.cmake)

if(REPROC_TEST)
  # reproc uses the doctest testing framework. All tests are grouped inside a
  # single executable. Use `target_sources(reproc-test PRIVATE <file>)` to add
  # new tests. Tests are executed by building the `test` target or running the
  # tests executable from within the build directory.

  # Generate the doctest implementation file.
  file(
    GENERATE
    OUTPUT ${PROJECT_BINARY_DIR}/doctest/impl.cpp
    CONTENT
      "
      #define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
      #define DOCTEST_CONFIG_NO_POSIX_SIGNALS
      #include <doctest.h>
      "
  )

  add_executable(reproc-test ${PROJECT_BINARY_DIR}/doctest/impl.cpp)

  reproc_add_common(reproc-test CXX 11 "")
  target_link_libraries(reproc-test PRIVATE doctest)
  set_target_properties(reproc-test PROPERTIES OUTPUT_NAME test)
endif()

add_subdirectory(reproc)
if(REPROC++)
  add_subdirectory(reproc++)
endif()
