
#  Config
#  ======

option(BENCHMARK_DISABLE_GC "Disable gc during a measurement")

set(BENCHMARK_PARAM   "N:1000" CACHE STRING "Benchmark parameters")
set(BENCHMARK_SAMPLES "20"     CACHE STRING "Benchmark samples")


set(NONIUS_INCLUDE_DIR
  "${DEPS_DIR}/Source/nonius/include")
set(STEADY_INCLUDE_DIR
  "${DEPS_DIR}/Source/steady/steady")
set(LIBRRB_INCLUDE_DIR
  "${DEPS_DIR}/Install/librrb/include")
set(LIBRRB_LIBS
  "${DEPS_DIR}/Install/librrb/lib/librrb.a"
  ${LIBGC_LIBS})

#  Dependencies
#  ============

add_custom_target(deps-benchmark COMMENT "Get benchmarking dependencies")
add_dependencies(deps deps-benchmark)

immer_external_dep_nobuild(deps-benchmark nonius
  GIT_REPOSITORY https://github.com/arximboldi/nonius.git
  GIT_TAG devel)

immer_external_dep_nobuild(deps-benchmark steady
  GIT_REPOSITORY https://github.com/marcusz/steady)

immer_external_dep(deps-benchmark librrb
  DEPENDS libgc
  GIT_REPOSITORY https://github.com/hyPiRion/c-rrb.git
  GIT_TAG development
  BUILD_IN_SOURCE 1
  UPDATE_COMMAND ""
  CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> CFLAGS=-O3\ -I${DEPS_DIR}/Install/libgc/include LDFLAGS=-L${DEPS_DIR}/Install/libgc/lib
  BUILD_COMMAND make
  INSTALL_COMMAND make install)
ExternalProject_Add_Step(librrb autoreconf
  DEPENDEES download update patch
  DEPENDERS configure
  COMMAND autoreconf -vif <SOURCE_DIR>)

#  Targets
#  =======

set(immer_benchmark_include_dirs
  ${Boost_INCLUDE_DIR}
  ${PRETTYPRINT_INCLUDE_DIR}
  ${LIBGC_INCLUDE_DIR}
  ${NONIUS_INCLUDE_DIR}
  ${LIBRRB_INCLUDE_DIR}
  ${STEADY_INCLUDE_DIR})
set(immer_benchmark_libs
  ${LIBGC_LIBS}
  ${LIBRRB_LIBS}
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT})

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
set(immer_benchmark_report_dir "${immer_benchmark_report_base_dir}/report_\
${immer_git_commit_hash}_\
${immer_hostname}_\
${immer_compiler_name}_\
${BENCHMARK_PARAM}_\
s${BENCHMARK_SAMPLES}")

if(DISABLE_FREE_LIST)
  set(immer_benchmark_report_dir "${immer_benchmark_report_dir}_nofl")
endif()

if(DISABLE_THREAD_SAFETY)
  set(immer_benchmark_report_dir "${immer_benchmark_report_dir}_nots")
endif()

if(BENCHMARK_DISABLE_GC)
  set(immer_benchmark_report_dir "${immer_benchmark_report_dir}_nogc")
endif()

if(CHECK_BENCHMARKS)
  add_dependencies(check benchmarks)
endif()

add_custom_target(benchmark-report-dir
  COMMAND ${CMAKE_COMMAND}
  -E make_directory ${immer_benchmark_report_dir})

file(GLOB_RECURSE immer_benchmarks "*.cpp")
foreach(_file IN LISTS immer_benchmarks)
  immer_target_name_for(_target _output "${_file}")
  add_executable(${_target} EXCLUDE_FROM_ALL "${_file}")
  set_target_properties(${_target} PROPERTIES OUTPUT_NAME ${_output})
  add_dependencies(benchmarks ${_target})
  add_dependencies(${_target} benchmark-report-dir)
  target_compile_definitions(${_target} PUBLIC
    NONIUS_RUNNER
    IMMER_BENCHMARK_LIBRRB=1
    IMMER_BENCHMARK_STEADY=1
    IMMER_BENCHMARK_EXPERIMENTAL=0
    IMMER_BENCHMARK_DISABLE_GC=${BENCHMARK_DISABLE_GC}
    IMMER_BENCHMARK_BOOST_COROUTINE=${ENABLE_BOOST_COROUTINE})
  target_link_libraries(${_target} PUBLIC ${immer_benchmark_libs})
  target_include_directories(${_target} PUBLIC ${immer_include_dirs})
  target_include_directories(${_target} SYSTEM PUBLIC ${immer_benchmark_include_dirs})
  if(CHECK_BENCHMARKS)
    add_test("benchmark/${_output}" ${_output} -v
      -t ${_target}
      -r html
      -s ${BENCHMARK_SAMPLES}
      -p ${BENCHMARK_PARAM}
      -o ${immer_benchmark_report_dir}/${_target}.html)
  endif()
endforeach()

add_custom_target(upload-benchmark-reports
  COMMAND
  scp -P 5488 -o StrictHostKeyChecking=no -p
      -i ${CMAKE_SOURCE_DIR}/tools/travis/ssh-key
      -r ${immer_benchmark_report_base_dir}/*
      raskolnikov@sinusoid.es:public/misc/immer/reports/)

add_custom_target(copy-benchmark-reports
  COMMAND
  cp -p -r ${immer_benchmark_report_base_dir}/*
      ~/public/misc/immer/reports/)
