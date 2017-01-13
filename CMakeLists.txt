
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
#  =====================================================================

option(CHECK_COVERAGE "compile with test coverage support")

option(CHECK_BENCHMARKS "run benchmarks on make check")
option(CHECK_BENCHMARKS_LONG "run benchmarks for more inputs")
option(CHECK_BENCHMARKS_LONGER "run benchmarks for more inputs")

option(CHECK_FREE_LIST "run checks with use free list heap")

option(CHECK_SLOW_TESTS "run slow tests")

set(PRETTYPRINT_INCLUDE_DIR
  "${CMAKE_BINARY_DIR}/prettyprint-prefix/src/prettyprint")
set(DOCTEST_INCLUDE_DIR
  "${CMAKE_BINARY_DIR}/doctest-prefix/src/doctest/doctest")
set(CATCH_INCLUDE_DIR
  "${CMAKE_BINARY_DIR}/catch-prefix/src/catch/single_include")
set(NONIUS_INCLUDE_DIR
  "${CMAKE_BINARY_DIR}/nonius-prefix/src/nonius/include")
set(STEADY_INCLUDE_DIR
  "${CMAKE_BINARY_DIR}/steady-prefix/src/steady/steady")
set(LIBRRB_INCLUDE_DIR
  "${CMAKE_BINARY_DIR}/librrb-prefix/include")
set(LIBGC_INCLUDE_DIR
  "${CMAKE_BINARY_DIR}/libgc-prefix/include")
set(LIBGC_LIBS
  "${CMAKE_BINARY_DIR}/libgc-prefix/lib/libgc.a")
set(LIBRRB_LIBS
  "${CMAKE_BINARY_DIR}/librrb-prefix/lib/librrb.a"
  ${LIBGC_LIBS})

#  Deps
#  =====================================================================

include(ExternalProject)

add_custom_target(deps COMMENT "Download and prepare dependencies")
add_custom_target(deps-test COMMENT "Get test dependencies")
add_custom_target(deps-debug COMMENT "Get debug dependencies")
add_custom_target(deps-benchmark COMMENT "Get benchmarking dependencies")
add_dependencies(deps deps-test deps-benchmark deps-debug)

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

immer_external_dep_nobuild(deps-test doctest
  GIT_REPOSITORY https://github.com/onqtam/doctest.git)

immer_external_dep_nobuild(deps-test catch
  # GIT_REPOSITORY git@github.com:philsquared/Catch.git
  # This fork has --break for GDB
  GIT_REPOSITORY https://github.com/lorenzhs/Catch.git)

immer_external_dep_nobuild(deps-debug prettyprint
  GIT_REPOSITORY https://github.com/louisdx/cxx-prettyprint.git)

immer_external_dep_nobuild(deps-benchmark nonius
  GIT_REPOSITORY https://github.com/arximboldi/nonius.git
  GIT_TAG devel)

immer_external_dep_nobuild(deps-benchmark steady
  GIT_REPOSITORY https://github.com/marcusz/steady)

immer_external_dep_nobuild(deps-benchmark libatomic_ops
  GIT_REPOSITORY https://github.com/ivmai/libatomic_ops.git
  UPDATE_COMMAND "")

immer_external_dep(deps-test libgc
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
  COMMAND ln -sf ${CMAKE_BINARY_DIR}/libatomic_ops-prefix/src/libatomic_ops <SOURCE_DIR>/libatomic_ops)

immer_external_dep(deps-benchmark librrb
  DEPENDS libgc
  GIT_REPOSITORY https://github.com/hyPiRion/c-rrb.git
  GIT_TAG development
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

add_definitions(-DIMMER_HAS_LIBGC=1)

if (CHECK_FREE_LIST)
  add_definitions(-DIMMER_FREE_LIST=1)
endif()

if (CHECK_SLOW_TESTS)
  add_definitions(-DIMMER_SLOW_TESTS=1)
endif()

if (CHECK_COVERAGE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
endif()

#  Tests
#  =====================================================================

set(immer_include_dirs
  ${PROJECT_SOURCE_DIR}
  ${PRETTYPRINT_INCLUDE_DIR}
  ${Boost_INCLUDE_DIR}
  ${LIBGC_INCLUDE_DIR})

set(immer_test_include_dirs
  ${CATCH_INCLUDE_DIR}
  ${DOCTEST_INCLUDE_DIR})
set(immer_test_libs
  ${LIBGC_LIBS}
  ${CMAKE_THREAD_LIBS_INIT})

set(immer_benchmark_include_dirs
  ${NONIUS_INCLUDE_DIR}
  ${LIBRRB_INCLUDE_DIR}
  ${STEADY_INCLUDE_DIR})
set(immer_benchmark_libs
  ${LIBRRB_LIBS}
  ${CMAKE_THREAD_LIBS_INIT})

enable_testing()

add_custom_target(check
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Buld and run all the tests and examples.")

function(immer_target_name_for out file)
  get_filename_component(_extension ${_file} EXT)
  file(RELATIVE_PATH _relative ${PROJECT_SOURCE_DIR} ${file})
  string(REPLACE "${_extension}" "" _name ${_relative})
  string(REGEX REPLACE "/" "-" _name ${_name})
  set(${out} "${_name}" PARENT_SCOPE)
endfunction()

add_custom_target(tests
  COMMENT "Build all the unit tests.")
add_dependencies(check tests)

file(GLOB_RECURSE immer_unit_tests "test/*.cpp")
foreach(_file IN LISTS immer_unit_tests)
  immer_target_name_for(_target "${_file}")
  add_executable(${_target} EXCLUDE_FROM_ALL "${_file}")
  add_dependencies(tests ${_target})
  target_link_libraries(${_target} PUBLIC ${immer_test_libs})
  target_compile_definitions(${_target} PUBLIC
    DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
    CATCH_CONFIG_MAIN)
  target_include_directories(${_target} PUBLIC ${immer_include_dirs})
  target_include_directories(${_target} SYSTEM PUBLIC ${immer_test_include_dirs})
  add_test(${_target} ${_target})
endforeach()

#  Benchmarks
#  =====================================================================

add_custom_target(benchmarks
  COMMENT "Build all benchmarks.")

execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE immer_git_commit_hash
  OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
  COMMAND git status --porcelain
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE immer_git_status
  OUTPUT_STRIP_TRAILING_WHITESPACE)

if (NOT immer_git_status STREQUAL "")
  set(immer_git_commit_hash "${immer_git_commit_hash}+")
endif()

site_name(immer_hostname)

get_filename_component(immer_compiler_name "${CMAKE_CXX_COMPILER}" NAME)

set(immer_benchmark_report_base_dir "${CMAKE_SOURCE_DIR}/reports")
set(immer_benchmark_report_dir
  "${immer_benchmark_report_base_dir}/report_${immer_git_commit_hash}_${immer_hostname}_${immer_compiler_name}")

if (CHECK_FREE_LIST)
  set(immer_benchmark_report_dir "${immer_benchmark_report_dir}_free-list")
endif()

if (CHECK_BENCHMARKS_LONGER)
  set(immer_benchmark_param "N:*:1:10:7")
  set(immer_benchmark_report_dir "${immer_benchmark_report_dir}_longer")
elseif(CHECK_BENCHMARKS_LONG)
  set(immer_benchmark_param "N:*:10:10:4")
  set(immer_benchmark_report_dir "${immer_benchmark_report_dir}_long")
elseif(CHECK_BENCHMARKS)
  set(immer_benchmark_param "N:1000")
endif()

if(immer_benchmark_param)
  add_dependencies(check benchmarks)
endif()

add_custom_target(benchmark-report-dir
  COMMAND ${CMAKE_COMMAND}
  -E make_directory ${immer_benchmark_report_dir})

file(GLOB immer_benchmarks "benchmark/*.cpp")
foreach(_file IN LISTS immer_benchmarks)
  immer_target_name_for(_target "${_file}")
  add_executable(${_target} EXCLUDE_FROM_ALL "${_file}")
  add_dependencies(benchmarks ${_target})
  add_dependencies(${_target} benchmark-report-dir)
  target_link_libraries(${_target} PUBLIC ${immer_benchmark_libs})
  target_compile_definitions(${_target} PUBLIC
    NONIUS_RUNNER
    IMMER_BENCHMARK_LIBRRB=1
    IMMER_BENCHMARK_STEADY=1
    IMMER_BENCHMARK_EXPERIMENTAL=0)
  target_include_directories(${_target} PUBLIC ${immer_include_dirs})
  target_include_directories(${_target} SYSTEM PUBLIC ${immer_benchmark_include_dirs})
  if (immer_benchmark_param)
    add_test(${_target} ${_target} -v
      -t ${_target}
      -r html
      -s 20
      -p ${immer_benchmark_param}
      -o ${immer_benchmark_report_dir}/${_target}.html)
  endif()
endforeach()

add_custom_target(upload-benchmark-reports
  COMMAND
  scp -P 5488 -o StrictHostKeyChecking=no -p
      -i ${CMAKE_SOURCE_DIR}/tools/ssh-key
      -r ${immer_benchmark_report_base_dir}/*
      raskolnikov@sinusoid.es:public/misc/immer/reports/)

add_custom_target(copy-benchmark-reports
  COMMAND
  cp -p -r ${immer_benchmark_report_base_dir}/*
      ~/public/misc/immer/reports/)

#  examples
#  =====================================================================

add_custom_target(examples
  COMMENT "Build all examples.")
add_dependencies(check examples)

file(GLOB_RECURSE immer_examples "example/*.cpp")
foreach(_file IN LISTS immer_examples)
  immer_target_name_for(_target "${_file}")
  add_executable(${_target} EXCLUDE_FROM_ALL "${_file}")
  add_dependencies(examples ${_target})
  target_link_libraries(${_target} PUBLIC ${immer_test_libs})
  target_compile_definitions(${_target} PUBLIC
    DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
    CATCH_CONFIG_MAIN)
  target_include_directories(${_target} PUBLIC ${immer_include_dirs})
  target_include_directories(${_target} SYSTEM PUBLIC ${immer_test_include_dirs})
  add_test(${_target} ${_target})
endforeach()

#  documentation
#  =====================================================================

add_custom_target(doxygen
  COMMAND doxygen doxygen.config
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/doc")

add_custom_target(docs
  COMMAND make html
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/doc")
add_dependencies(docs doxygen)

add_custom_target(upload-docs
  COMMAND
  scp -o StrictHostKeyChecking=no -p
      -i ${CMAKE_SOURCE_DIR}/tools/ssh-key
      -r ${CMAKE_SOURCE_DIR}/doc/_build/html/*
      raskolnikov@sinusoid.es:public/immer/)
