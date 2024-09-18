find_package(fmt REQUIRED)
find_package(cereal REQUIRED)
find_package(xxHash 0.8 CONFIG REQUIRED)

if(${CXX_STANDARD} LESS 17)
  message(FATAL_ERROR "persist requires C++17")
endif()

include(CTest)

add_executable(
  persist-tests EXCLUDE_FROM_ALL
  test_array.cpp
  test_vectors.cpp
  test_champ.cpp
  test_xxhash.cpp
  test_box.cpp
  test_special_pool.cpp
  test_special_pool_auto.cpp
  test_conversion.cpp
  test_circular_dependency_conversion.cpp
  test_table_box_recursive.cpp
  test_for_docs.cpp
  test_containers_cereal.cpp
  test_hash_size.cpp
  ${PROJECT_SOURCE_DIR}/immer/extra/persist/xxhash/xxhash_64.cpp)
target_precompile_headers(
  persist-tests PRIVATE <immer/extra/persist/cereal/save.hpp>
  <immer/extra/persist/cereal/load.hpp>)
add_dependencies(tests persist-tests)
add_test("test/persist-tests" persist-tests)
target_include_directories(persist-tests PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(persist-tests PRIVATE fmt::fmt Catch2::Catch2WithMain
                                            xxHash::xxhash)

target_compile_options(persist-tests PRIVATE -O1 -fno-optimize-sibling-calls -g
                                             -fno-omit-frame-pointer)
target_compile_options(
  persist-tests PRIVATE -Wno-unused-function -Wno-c++20-designator
                        -Wimplicit-int-conversion)

if(ENABLE_ASAN)
  target_compile_options(
    persist-tests PRIVATE -fsanitize-coverage=trace-pc-guard -fsanitize=address)
  target_link_options(persist-tests PRIVATE -fsanitize=address)
endif()
target_compile_definitions(persist-tests PRIVATE BOOST_USE_ASAN=1)
target_compile_definitions(persist-tests PRIVATE IMMER_NO_FREE_LIST=1
                                                 IMMER_DEBUG_STATS=1)

install(TARGETS persist-tests DESTINATION bin)
install(FILES valgrind.supp DESTINATION bin)
