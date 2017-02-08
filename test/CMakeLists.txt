
#  Config
#  ======

set(DOCTEST_INCLUDE_DIR
  "${DEPS_DIR}/Source/doctest/doctest")
set(CATCH_INCLUDE_DIR
  "${DEPS_DIR}/Source/catch/single_include")

#  Dependencies
#  ============

add_custom_target(deps-test COMMENT "Get test dependencies")
add_dependencies(deps deps-test)

immer_external_dep_nobuild(deps-test doctest
  GIT_REPOSITORY https://github.com/onqtam/doctest.git)

immer_external_dep_nobuild(deps-test catch
  # GIT_REPOSITORY git@github.com:philsquared/Catch.git
  # This fork has --break for GDB
  GIT_REPOSITORY https://github.com/lorenzhs/Catch.git)

#  Targets
#  =======

set(immer_test_include_dirs
  ${Boost_INCLUDE_DIR}
  ${PRETTYPRINT_INCLUDE_DIR}
  ${LIBGC_INCLUDE_DIR}
  ${CATCH_INCLUDE_DIR}
  ${DOCTEST_INCLUDE_DIR})
set(immer_test_libs
  ${LIBGC_LIBS}
  ${CMAKE_THREAD_LIBS_INIT})

add_custom_target(tests
  COMMENT "Build all the unit tests.")
add_dependencies(check tests)

file(GLOB_RECURSE immer_unit_tests "*.cpp")
foreach(_file IN LISTS immer_unit_tests)
  immer_target_name_for(_target _output "${_file}")
  add_executable(${_target} EXCLUDE_FROM_ALL "${_file}")
  set_target_properties(${_target} PROPERTIES OUTPUT_NAME ${_output})
  add_dependencies(tests ${_target})
  target_compile_definitions(${_target} PUBLIC
    DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
    CATCH_CONFIG_MAIN)
  target_link_libraries(${_target} PUBLIC ${immer_test_libs})
  target_include_directories(${_target} PUBLIC ${immer_include_dirs})
  target_include_directories(${_target} SYSTEM PUBLIC ${immer_test_include_dirs})
  add_test("test/${_output}" ${_output})
endforeach()
