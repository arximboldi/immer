
#  Targets
#  =======

add_custom_target(examples
  COMMENT "Build all examples.")
add_dependencies(check examples)

file(GLOB_RECURSE immer_examples "*.cpp")
foreach(_file IN LISTS immer_examples)
  immer_target_name_for(_target _output "${_file}")
  add_executable(${_target} EXCLUDE_FROM_ALL "${_file}")
  add_dependencies(examples ${_target})
  set_target_properties(${_target} PROPERTIES OUTPUT_NAME ${_output})
  target_link_libraries(${_target} PUBLIC
    immer
    ${LIBGC_LIBS}
    ${CMAKE_THREAD_LIBS_INIT})
  target_include_directories(${_target} SYSTEM PUBLIC
    ${Boost_INCLUDE_DIR}
    ${PRETTYPRINT_INCLUDE_DIR}
    ${LIBGC_INCLUDE_DIR})
  add_test("example/${_output}" ${_output})
endforeach()
