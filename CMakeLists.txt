
cmake_policy(SET CMP0048 NEW) # enable project VERSION
cmake_policy(SET CMP0056 NEW) # honor link flags in try_compile()

project(immu VERSION 0.4.0)
cmake_minimum_required(VERSION 3.5.1)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#  Options
#  =====================================================================

set(DOCTEST_INCLUDE_DIR
  "${CMAKE_BINARY_DIR}/doctest-prefix/src/doctest/doctest")
set(EGGS_VARIANT_INCLUDE_DIR
  "${CMAKE_BINARY_DIR}/eggs-variant-prefix/src/eggs-variant/include")

#  Deps
#  =====================================================================

include(ExternalProject)
add_custom_target(deps
  COMMENT "Download and prepare dependencies")

ExternalProject_Add(doctest
  GIT_REPOSITORY git@github.com:onqtam/doctest.git
  EXCLUDE_FROM_ALL 1
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND "")
add_dependencies(deps doctest)

ExternalProject_Add(eggs-variant
  GIT_REPOSITORY git@github.com:eggs-cpp/variant.git
  EXCLUDE_FROM_ALL 1
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND "")
add_dependencies(deps eggs-variant)

find_package(Boost 1.56 REQUIRED)

find_program(CCACHE ccache)
if (CCACHE)
  message(STATUS "Using ccache: ${CCACHE}")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE})
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE})
else()
  message(STATUS "Could not find ccache")
endif()

#  Tests
#  =====================================================================

set(immu_include_dirs
  ${PROJECT_SOURCE_DIR}
  ${Boost_INCLUDE_DIR}
  ${EGGS_VARIANT_INCLUDE_DIR})
set(immu_test_include_dirs
  ${immu_include_dirs}
  ${DOCTEST_INCLUDE_DIR})

enable_testing()

add_custom_target(check
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Buld and run all the tests and examples.")

add_custom_target(tests
  COMMENT "Build all the unit tests.")
add_dependencies(check tests)

function(immu_target_name_for out file)
  get_filename_component(_extension ${_file} EXT)
  file(RELATIVE_PATH _relative ${PROJECT_SOURCE_DIR} ${file})
  string(REPLACE "${_extension}" "" _name ${_relative})
  string(REGEX REPLACE "/" "-" _name ${_name})
  set(${out} "${_name}" PARENT_SCOPE)
endfunction()

file(GLOB_RECURSE UNIT_TESTS "test/*.cpp")
foreach(_file IN LISTS UNIT_TESTS)
  immu_target_name_for(_target "${_file}")
  add_executable(${_target} EXCLUDE_FROM_ALL "${_file}")
  add_dependencies(tests ${_target})
  target_compile_definitions(${_target} PUBLIC DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN)
  target_include_directories(${_target} PUBLIC ${immu_test_include_dirs})
  add_test(${_target} ${_target})
endforeach()
