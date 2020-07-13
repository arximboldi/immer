
add_custom_target(fuzzers
  COMMENT "Build all fuzzers.")

if (CHECK_FUZZERS)
  add_dependencies(tests fuzzers)
endif()

file(GLOB_RECURSE immer_fuzzers "*.cpp")
foreach(_file IN LISTS immer_fuzzers)
  immer_target_name_for(_target _output "${_file}")
  add_executable(${_target} EXCLUDE_FROM_ALL "${_file}")
  set_target_properties(${_target} PROPERTIES OUTPUT_NAME ${_output})
  target_compile_options(${_target} PUBLIC "-fsanitize=fuzzer")
  target_link_libraries(${_target} PUBLIC "-fsanitize=fuzzer"
    immer-dev
    ${LIBGC_LIBS})
  target_include_directories(${_target} SYSTEM PUBLIC
    ${LIBGC_INCLUDE_DIR})
  add_dependencies(fuzzers ${_target})
  if (CHECK_FUZZERS)
    add_test("fuzzer/${_output}" ${_output} -max_total_time=1)
  endif()
endforeach()
