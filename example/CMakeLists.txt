
#  Targets
#  =======

add_custom_target(examples
  COMMENT "Build all examples.")
add_dependencies(check examples)

file(GLOB_RECURSE immer_examples "*.cpp")
immer_filter_items(immer_examples ".*/hidden/.*")
foreach(_file IN LISTS immer_examples)
  immer_target_name_for(_target _output "${_file}")
  add_executable(${_target} EXCLUDE_FROM_ALL "${_file}")
  add_dependencies(examples ${_target})
  set_target_properties(${_target} PROPERTIES OUTPUT_NAME ${_output})
  target_link_libraries(${_target} PUBLIC immer-dev)
  add_test("example/${_output}" ${_output})
endforeach()

immer_target_name_for(_target _output
  "${CMAKE_CURRENT_SOURCE_DIR}/box/hidden/main.cpp")
add_executable(${_target} EXCLUDE_FROM_ALL
  "box/hidden/main.cpp"
  "box/hidden/hidden.cpp")
add_dependencies(examples ${_target})
set_target_properties(${_target} PROPERTIES OUTPUT_NAME ${_output})
target_link_libraries(${_target} PUBLIC immer-dev)
add_test("example/${_output}" ${_output})
