find_package(spdlog REQUIRED)
find_package(cereal REQUIRED)
find_package(xxHash 0.8 CONFIG REQUIRED)

if(${CXX_STANDARD} LESS 20)
  # Maybe no particular reason, but we're not targeting older compilers anyway.
  message(FATAL_ERROR "archive requires C++20")
endif()

include(CTest)

add_executable(
  archive-tests EXCLUDE_FROM_ALL
  test_vectors.cpp test_special_archive.cpp test_champ.cpp test_xxhash.cpp
  test_box.cpp
  ${PROJECT_SOURCE_DIR}/immer/extra/archive/xxhash/xxhash_64.cpp)
add_dependencies(tests archive-tests)
add_test("test/archive-tests" archive-tests)
target_include_directories(archive-tests PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(
  archive-tests PRIVATE spdlog::spdlog Catch2::Catch2WithMain xxHash::xxhash)

target_compile_options(archive-tests PRIVATE -O1 -fno-optimize-sibling-calls -g
                                             -fno-omit-frame-pointer)
target_compile_options(archive-tests PRIVATE -Wno-unused-function)

if(ENABLE_ASAN)
  target_compile_options(
    archive-tests PRIVATE -fsanitize-coverage=trace-pc-guard -fsanitize=address)
  target_link_options(archive-tests PRIVATE -fsanitize=address)
endif()
target_compile_definitions(archive-tests PRIVATE BOOST_USE_ASAN=1)
target_compile_definitions(
  archive-tests PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
                        IMMER_NO_FREE_LIST=1)

install(TARGETS archive-tests DESTINATION bin)
install(FILES valgrind.supp DESTINATION bin)
