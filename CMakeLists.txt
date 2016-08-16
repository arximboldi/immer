
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

option(CHECK_BENCHMARKS "run benchmarks on make check")
option(CHECK_BENCHMARKS_MORE "run benchmarks for more inputs")
option(CHECK_BENCHMARKS_MORE_MORE "run benchmarks for more inputs")

set(DOCTEST_INCLUDE_DIR
  "${CMAKE_BINARY_DIR}/doctest-prefix/src/doctest/doctest")
set(NONIUS_INCLUDE_DIR
  "${CMAKE_BINARY_DIR}/nonius-prefix/src/nonius/include")
set(LIBRRB_INCLUDE_DIR
  "${CMAKE_BINARY_DIR}/librrb-prefix/include")
set(LIBGC_INCLUDE_DIR
  "${CMAKE_BINARY_DIR}/libgc-prefix/include")
set(LIBRRB_LIBS
  "${CMAKE_BINARY_DIR}/librrb-prefix/lib/librrb.a"
  "${CMAKE_BINARY_DIR}/libgc-prefix/lib/libgc.a")

#  Deps
#  =====================================================================

include(ExternalProject)
add_custom_target(deps
  COMMENT "Download and prepare dependencies")

function(immu_external_dep name)
  ExternalProject_Add(${name} EXCLUDE_FROM_ALL 1 "${ARGN}")
  add_dependencies(deps ${name})
endfunction()

function(immu_external_dep_nobuild name)
  immu_external_dep(${name} "${ARGN}"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND "")
endfunction()

immu_external_dep_nobuild(doctest
  GIT_REPOSITORY git@github.com:onqtam/doctest.git)

immu_external_dep_nobuild(nonius
  GIT_REPOSITORY git@github.com:arximboldi/nonius.git
  GIT_TAG devel)

immu_external_dep_nobuild(libatomic_ops
  GIT_REPOSITORY git://github.com/ivmai/libatomic_ops.git
  UPDATE_COMMAND "")

immu_external_dep(libgc
  DEPENDS libatomic_ops
  GIT_REPOSITORY git://github.com/ivmai/bdwgc.git
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
  COMMAND ln -sf ${CMAKE_BINARY_DIR}/libatomic_ops-prefix/src/libatomic_ops <SOURCE_DIR>/libatomic_ops)

immu_external_dep(librrb
  DEPENDS libgc
  GIT_REPOSITORY git@github.com:hyPiRion/c-rrb.git
  BUILD_IN_SOURCE 1
  UPDATE_COMMAND ""
  CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> CFLAGS=-O3\ -I${CMAKE_BINARY_DIR}/libgc-prefix/include LDFLAGS=-L${CMAKE_BINARY_DIR}/libgc-prefix/lib
  BUILD_COMMAND make
  INSTALL_COMMAND make install)
ExternalProject_Add_Step(librrb autoreconf
  DEPENDEES download update patch
  DEPENDERS configure
  COMMAND autoreconf -vif <SOURCE_DIR>)

find_package(Boost 1.56 REQUIRED)
find_package(Threads)

find_program(CCACHE ccache)
if (CCACHE)
  message(STATUS "Using ccache: ${CCACHE}")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE})
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE})
else()
  message(STATUS "Could not find ccache")
endif()

#  Config
#  =====================================================================

add_definitions(-DIMMU_HAS_LIBGC=1)

#  Tests
#  =====================================================================

set(immu_include_dirs
  ${PROJECT_SOURCE_DIR}
  ${Boost_INCLUDE_DIR}
  ${LIBGC_INCLUDE_DIR})
set(immu_test_include_dirs
  ${immu_include_dirs}
  ${DOCTEST_INCLUDE_DIR})
set(immu_benchmark_include_dirs
  ${immu_include_dirs}
  ${NONIUS_INCLUDE_DIR}
  ${LIBRRB_INCLUDE_DIR})
set(immu_benchmark_libs
  ${LIBRRB_LIBS}
  ${CMAKE_THREAD_LIBS_INIT})

enable_testing()

add_custom_target(check
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Buld and run all the tests and examples.")

function(immu_target_name_for out file)
  get_filename_component(_extension ${_file} EXT)
  file(RELATIVE_PATH _relative ${PROJECT_SOURCE_DIR} ${file})
  string(REPLACE "${_extension}" "" _name ${_relative})
  string(REGEX REPLACE "/" "-" _name ${_name})
  set(${out} "${_name}" PARENT_SCOPE)
endfunction()

add_custom_target(tests
  COMMENT "Build all the unit tests.")
add_dependencies(check tests)

file(GLOB_RECURSE immu_unit_tests "test/*.cpp")
foreach(_file IN LISTS immu_unit_tests)
  immu_target_name_for(_target "${_file}")
  add_executable(${_target} EXCLUDE_FROM_ALL "${_file}")
  add_dependencies(tests ${_target})
  target_compile_definitions(${_target} PUBLIC DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN)
  target_include_directories(${_target} PUBLIC ${immu_test_include_dirs})
  add_test(${_target} ${_target})
endforeach()

#  Benchmarks
#  =====================================================================

add_custom_target(benchmarks
  COMMENT "Build all benchmarks.")

execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE)

get_filename_component(immu_compiler_name "${CMAKE_CXX_COMPILER}" NAME)
set(immu_benchmark_report_dir
  "../reports/report-${GIT_COMMIT_HASH}-${immu_compiler_name}")

if (CHECK_BENCHMARKS_MORE_MORE)
  set(immu_benchmark_param "N:*:1:10:7")
  set(immu_benchmark_report_dir "${immu_benchmark_report_dir}-more2")
elseif(CHECK_BENCHMARKS_MORE)
  set(immu_benchmark_param "N:*:10:10:4")
  set(immu_benchmark_report_dir "${immu_benchmark_report_dir}-more1")
elseif(CHECK_BENCHMARKS)
  set(immu_benchmark_param "N:1000")
endif()

if(immu_benchmark_param)
  add_dependencies(check benchmarks)
endif()

add_custom_target(make-benchmark-report-dir ALL
  COMMAND ${CMAKE_COMMAND}
  -E make_directory ${immu_benchmark_report_dir})
add_dependencies(benchmarks make-benchmark-report-dir)

file(GLOB immu_benchmarks "benchmark/*.cpp")
foreach(_file IN LISTS immu_benchmarks)
  immu_target_name_for(_target "${_file}")
  add_executable(${_target} EXCLUDE_FROM_ALL "${_file}")
  add_dependencies(benchmarks ${_target})
  target_link_libraries(${_target} PUBLIC ${immu_benchmark_libs})
  target_compile_definitions(${_target} PUBLIC NONIUS_RUNNER IMMU_BENCHMARK_LIBRRB=1)
  target_include_directories(${_target} PUBLIC ${immu_benchmark_include_dirs})
  if (immu_benchmark_param)
    add_test(${_target} ${_target} -v
      -t ${_target}
      -r html
      -s 20
      -p ${immu_benchmark_param}
      -o ${immu_benchmark_report_dir}/${_target}.html)
  endif()
endforeach()
