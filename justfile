[private]
default:
    @cd {{ invocation_directory() }}; just --choose

_mk-dir name:
    rm -rf {{ name }}
    mkdir {{ name }}

build-valgrind-path := "build-valgrind-" + os() + "-" + arch()

# Create a build directory for a Debug build without ASAN, so that valgrind can work
mk-build-valgrind: (_mk-dir build-valgrind-path)
    cd {{ build-valgrind-path }} ; cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Debug -Dimmer_BUILD_TESTS=ON -Dimmer_BUILD_ARCHIVE_TESTS=ON -Dimmer_BUILD_EXAMPLES=OFF -DCXX_STANDARD=20

[linux]
run-valgrind:
    cd {{ build-valgrind-path }} ; ninja tests && ctest -D ExperimentalMemCheck

[linux]
run-valgrind-archive:
    cd {{ build-valgrind-path }} ; ninja archive-tests && valgrind --quiet --error-exitcode=99 --leak-check=full --errors-for-leak-kinds=all \
                --suppressions=../test/extra/archive/valgrind.supp \
                ./test/extra/archive/archive-tests

build-asan-path := "build-asan-" + os() + "-" + arch()

# Create a build directory for a Debug build with ASAN enabled
mk-build-asan: (_mk-dir build-asan-path)
    cd {{ build-asan-path }} ; cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON -Dimmer_BUILD_TESTS=ON -Dimmer_BUILD_ARCHIVE_TESTS=ON -Dimmer_BUILD_EXAMPLES=OFF -DCXX_STANDARD=20

run-tests-asan:
    cd {{ build-asan-path }} ; ninja tests && ninja test
