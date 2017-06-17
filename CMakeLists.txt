
cmake_minimum_required(VERSION 3.5.1)
cmake_policy(SET CMP0048 NEW) # enable project VERSION
cmake_policy(SET CMP0056 NEW) # honor link flags in try_compile()
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(immer VERSION 0.4.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_CXX_EXTENSIONS off)

include(ImmerUtils)

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

#  Targets
#  =======

immer_canonicalize_cmake_booleans(
  DISABLE_FREE_LIST
  DISABLE_THREAD_SAFETY
  CHECK_SLOW_TESTS)

add_library(immer INTERFACE)
target_include_directories(immer INTERFACE
  $<BUILD_INTERFACE:${immer_BINARY_DIR}/>
  $<BUILD_INTERFACE:${immer_SOURCE_DIR}/>
  $<INSTALL_INTERFACE:include>)
target_compile_definitions(immer INTERFACE
  -DIMMER_HAS_LIBGC=1
  -DIMMER_NO_FREE_LIST=${DISABLE_FREE_LIST}
  -DIMMER_NO_THREAD_SAFETY=${DISABLE_THREAD_SAFETY}
  -DIMMER_SLOW_TESTS=${CHECK_SLOW_TESTS})
if (ENABLE_COVERAGE)
  target_compile_options(immer INTERFACE "--coverage")
  target_link_libraries(immer INTERFACE "--coverage")
endif()

#  Testing
#  =======

enable_testing()

set(CTEST_ENVIRONMENT "GC_INITIAL_HEAP_SIZE=8G")

add_custom_target(check
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Buld and run all the tests and examples.")

add_subdirectory(test)
add_subdirectory(benchmark)
add_subdirectory(example)
add_subdirectory(doc)
add_subdirectory(extra/fuzzer)
