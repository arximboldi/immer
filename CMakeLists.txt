
cmake_policy(SET CMP0048 NEW) # enable project VERSION
cmake_policy(SET CMP0056 NEW) # honor link flags in try_compile()

project(immer VERSION 0.4.0)
cmake_minimum_required(VERSION 3.5.1)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_CXX_EXTENSIONS off)

#  Options
#  =======

set(DEPS_DIR "${CMAKE_BINARY_DIR}/deps")

option(ENABLE_COVERAGE "compile with test coverage support")
option(ENABLE_BOOST_COROUTINE "run benchmarks with boost coroutine")
option(DISABLE_FREE_LIST "disables the free list heap")
option(DISABLE_THREAD_SAFETY "disables thread safety by default")

set(LIBGC_INCLUDE_DIR
  "${DEPS_DIR}/Install/libgc/include")
set(LIBGC_LIBS
  "${DEPS_DIR}/Install/libgc/lib/libgc.a")
set(PRETTYPRINT_INCLUDE_DIR
  "${DEPS_DIR}/Source/prettyprint")

#  Dependencies
#  ============

include(ExternalProject)

set_directory_properties(PROPERTIES EP_BASE "${DEPS_DIR}")

add_custom_target(deps COMMENT "Download and prepare dependencies")
add_custom_target(deps-core COMMENT "Get core dependencies")
add_dependencies(deps deps-core)

function(immer_external_dep target name)
  ExternalProject_Add(${name} EXCLUDE_FROM_ALL 1 "${ARGN}")
  add_dependencies(${target} ${name})
endfunction()

function(immer_external_dep_nobuild target name)
  immer_external_dep(${target} ${name} "${ARGN}"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND "")
endfunction()

if (ENABLE_BOOST_COROUTINE)
  find_package(Boost 1.56 REQUIRED COMPONENTS coroutine)
else ()
  find_package(Boost 1.56 REQUIRED)
endif ()

find_package(Threads)

find_program(CCACHE ccache)
if (CCACHE)
  message(STATUS "Using ccache: ${CCACHE}")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE})
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE})
else()
  message(STATUS "Could not find ccache")
endif()

immer_external_dep_nobuild(deps-core prettyprint
  GIT_REPOSITORY https://github.com/louisdx/cxx-prettyprint.git)

immer_external_dep_nobuild(deps-core libatomic_ops
  GIT_REPOSITORY https://github.com/ivmai/libatomic_ops.git
  UPDATE_COMMAND "")

immer_external_dep(deps-core libgc
  DEPENDS libatomic_ops
  GIT_REPOSITORY https://github.com/ivmai/bdwgc.git
  CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> CFLAGS=-O3
  BUILD_COMMAND make
  INSTALL_COMMAND make install)
ExternalProject_Add_Step(libgc autoreconf
  DEPENDEES download update patch
  DEPENDERS configure
  COMMAND autoreconf -vif <SOURCE_DIR>)
ExternalProject_Add_Step(libgc ln-libatomic_ops
  DEPENDEES download update patch
  DEPENDERS configure
  COMMAND ln -sf ${DEPS_DIR}/Source/libatomic_ops <SOURCE_DIR>/libatomic_ops)

#  Config
#  ======

add_definitions(-DIMMER_HAS_LIBGC=1)

if (DISABLE_FREE_LIST)
  add_definitions(-DIMMER_NO_FREE_LIST=1)
endif()

if (DISABLE_THREAD_SAFETY)
  add_definitions(-DIMMER_NO_THREAD_SAFETY=1)
endif()

if (CHECK_SLOW_TESTS)
  add_definitions(-DIMMER_SLOW_TESTS=1)
endif()

if (ENABLE_COVERAGE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
endif()

set(immer_include_dirs ${PROJECT_SOURCE_DIR})

#  Targets
#  =======

enable_testing()

add_custom_target(check
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Buld and run all the tests and examples.")

function(immer_target_name_for out_target out_file file)
  get_filename_component(_extension ${_file} EXT)

  file(RELATIVE_PATH _relative ${PROJECT_SOURCE_DIR} ${file})
  string(REPLACE "${_extension}" "" _name ${_relative})
  string(REGEX REPLACE "/" "-" _name ${_name})
  set(${out_target} "${_name}" PARENT_SCOPE)

  file(RELATIVE_PATH _relative ${CMAKE_CURRENT_LIST_DIR} ${file})
  string(REPLACE "${_extension}" "" _name ${_relative})
  string(REGEX REPLACE "/" "-" _name ${_name})
  set(${out_file} "${_name}" PARENT_SCOPE)
endfunction()

add_subdirectory(test)
add_subdirectory(benchmark)
add_subdirectory(example)
add_subdirectory(doc)
