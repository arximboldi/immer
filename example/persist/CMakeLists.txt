
find_package(fmt REQUIRED)
find_package(cereal REQUIRED)
find_package(xxHash 0.8 CONFIG REQUIRED)

if(${CXX_STANDARD} LESS 17)
  message(FATAL_ERROR "persist requires C++17")
endif()

file(GLOB_RECURSE immer_persist_examples "*.cpp")

foreach(_file IN LISTS immer_persist_examples)
  immer_target_name_for(_target _output "${_file}")
  add_executable(${_target} EXCLUDE_FROM_ALL "${_file}")
  add_dependencies(examples ${_target})
  set_target_properties(${_target} PROPERTIES OUTPUT_NAME ${_output})
  target_link_libraries(${_target} PUBLIC immer-dev
    PRIVATE fmt::fmt
    xxHash::xxhash)
  add_test("example/${_output}" ${_output})
endforeach()
