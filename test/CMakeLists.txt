# Targets
# =======

add_custom_target(tests COMMENT "Build all the unit tests.")
add_dependencies(check tests)

# The variable MEMORYCHECK_SUPPRESSIONS_FILE must be set before including CTest,
# because including it triggers the generation of the config
# DartConfiguration.tcl.
set(MEMORYCHECK_SUPPRESSIONS_FILE
    "${PROJECT_SOURCE_DIR}/test/valgrind_suppress.txt"
    CACHE FILEPATH "")
set(VALGRIND_COMMAND_OPTIONS
    "--suppressions=${PROJECT_SOURCE_DIR}/test/extra/persist/valgrind.supp")
find_package(Catch2 REQUIRED)
include(CTest)

file(GLOB_RECURSE immer_unit_tests "*.cpp")
foreach(TMP_PATH ${immer_unit_tests})
  string(FIND ${TMP_PATH} persist EXCLUDE_DIR_FOUND)
  if(NOT ${EXCLUDE_DIR_FOUND} EQUAL -1)
    list(REMOVE_ITEM immer_unit_tests ${TMP_PATH})
  endif()
endforeach(TMP_PATH)

foreach(_file IN LISTS immer_unit_tests)
  immer_target_name_for(_target _output "${_file}")
  add_executable(${_target} EXCLUDE_FROM_ALL "${_file}")
  set_target_properties(${_target} PROPERTIES OUTPUT_NAME ${_output})
  add_dependencies(tests ${_target})
  target_compile_definitions(
    ${_target}
    PUBLIC IMMER_OSS_FUZZ_DATA_PATH="${CMAKE_CURRENT_SOURCE_DIR}/oss-fuzz/data"
           IMMER_THROW_ON_INVALID_STATE=1)
  target_link_libraries(${_target} PUBLIC immer-dev Catch2::Catch2WithMain)
  set_target_properties(${_target} PROPERTIES CXX_VISIBILITY_PRESET hidden)
  add_test("test/${_output}" ${_output})
endforeach()

if(immer_BUILD_PERSIST_TESTS)
  add_subdirectory(extra/persist)
endif()
