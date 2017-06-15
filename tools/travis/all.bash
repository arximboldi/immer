
DEPS_DIR="${TRAVIS_BUILD_DIR}/deps"
WGET="travis_retry wget --continue --tries=20 --waitretry=10 --retry-connrefused --no-dns-cache --timeout 300 --no-check-certificate"

function dep-not-cached { [[ -z "$(ls -A ${DEPS_DIR}/$1 2> /dev/null)" ]]; }
function goto-deps      { mkdir -p ${DEPS_DIR} && cd ${DEPS_DIR}; }

function travis-before-install
{
    openssl aes-256-cbc -K $encrypted_1c8d51d72e41_key \
            -iv $encrypted_1c8d51d72e41_iv \
            -in tools/travis/ssh-key.enc \
            -out tools/travis/ssh-key -d
}

function install-cmake
{
    if dep-not-cached "cmake"; then
        goto-deps
        CMAKE_VERSION_LONG="3.5.1"
        CMAKE_VERSION_SHORT="3.5"
        CMAKE_URL="http://www.cmake.org/files/v${CMAKE_VERSION_SHORT}/cmake-${CMAKE_VERSION_LONG}-Linux-x86_64.tar.gz"
        mkdir -p cmake \
            && ${WGET} --quiet -O - ${CMAKE_URL} \
                | tar --strip-components=1 -xz -C cmake
    fi
    export PATH=${DEPS_DIR}/cmake/bin:${PATH}
}

function install-boost
{
    if dep-not-cached "boost"; then
        goto-deps
        BOOST_VERSION=1.61.0
        BOOST_FILE_VERSION=1_61_0
        BOOST_URL="http://sourceforge.net/projects/boost/files/boost/${BOOST_VERSION}/boost_${BOOST_FILE_VERSION}.tar.gz/download"
        mkdir -p boost \
            && ${WGET} --quiet -O - ${BOOST_URL} \
                | tar --strip-components=1 -xz -C boost
    fi
    export BOOST_PATH=${DEPS_DIR}/boost
}

function install-llvm
{
    LLVM_DIR=${DEPS_DIR}/llvm
    if dep-not-cached "llvm"; then
        goto-deps
        LLVM_VERSION=${LLVM}
        LLVM_URL="http://llvm.org/releases/${LLVM_VERSION}/llvm-${LLVM_VERSION}.src.tar.xz"
        LIBCXX_URL="http://llvm.org/releases/${LLVM_VERSION}/libcxx-${LLVM_VERSION}.src.tar.xz"
        LIBCXXABI_URL="http://llvm.org/releases/${LLVM_VERSION}/libcxxabi-${LLVM_VERSION}.src.tar.xz"
        CLANG_URL="http://llvm.org/releases/${LLVM_VERSION}/clang+llvm-${LLVM_VERSION}-x86_64-linux-gnu-ubuntu-14.04.tar.xz"
        mkdir -p ${LLVM_DIR} ${LLVM_DIR}/build ${LLVM_DIR}/projects/libcxx ${LLVM_DIR}/projects/libcxxabi ${LLVM_DIR}/clang
        ${WGET} --quiet -O - ${LLVM_URL}      | tar --strip-components=1 -xJ -C ${LLVM_DIR}
        ${WGET} --quiet -O - ${LIBCXX_URL}    | tar --strip-components=1 -xJ -C ${LLVM_DIR}/projects/libcxx
        ${WGET} --quiet -O - ${LIBCXXABI_URL} | tar --strip-components=1 -xJ -C ${LLVM_DIR}/projects/libcxxabi
        ${WGET} --quiet -O - ${CLANG_URL}     | tar --strip-components=1 -xJ -C ${LLVM_DIR}/clang
        (cd ${LLVM_DIR}/build && cmake .. -DCMAKE_INSTALL_PREFIX=${LLVM_DIR}/install -DCMAKE_CXX_COMPILER=clang++)
        (cd ${LLVM_DIR}/build/projects/libcxx && make install -j2)
        (cd ${LLVM_DIR}/build/projects/libcxxabi && make install -j2)
    fi
    export LLVM_CXXFLAGS="-nostdinc++ -isystem ${LLVM_DIR}/install/include/c++/v1 -Wno-reserved-id-macro -Qunused-arguments"
    export LLVM_LDFLAGS="-L ${LLVM_DIR}/install/lib -l c++ -l c++abi"
    export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${LLVM_DIR}/install/lib"
    export PATH="${LLVM_DIR}/clang/bin:${PATH}"
}

function install-doxygen
{
    if dep-not-cached "doxygen"; then
        goto-deps
        DOXYGEN_VERSION="1.8.11"
        DOXYGEN_URL="http://ftp.stack.nl/pub/users/dimitri/doxygen-${DOXYGEN_VERSION}.linux.bin.tar.gz"
        mkdir -p doxygen \
            && travis_retry ${WGET} --quiet -O - ${DOXYGEN_URL} \
                | tar --strip-components=1 -xz -C doxygen
    fi
    export PATH=${DEPS_DIR}/doxygen/bin:${PATH}
}

function install-sphinx
{
    pip install --user -r ${TRAVIS_BUILD_DIR}/doc/requirements.txt
    export PATH=~/bin:${PATH}
}

function travis-install
{
    if [[ "${COMPILER}" != "" ]];
    then
        export CXX=${COMPILER};
    fi
    install-cmake
    install-boost
    if [[ "${LLVM}" != "" ]]; then
        install-llvm
    fi
    if [[ "${DOCS}" == "true" ]]; then
        install-doxygen
        install-sphinx
    fi
}

function travis-before-script
{
    cd ${TRAVIS_BUILD_DIR}
    mkdir -p build && cd build
    ${COMPILER} --version

    if [[ "${CONFIGURATION}" == Release ]]; then
        BENCHMARK=true
    fi
    if [[ "${CONFIGURATION}" == Debug ]]; then
        # For now this is too slow on clang... :(
        # slow_tests=true
        slow_tests=false
    fi
    if [[ "${SANITIZE}" == true ]]; then
        disable_free_list=true
        sanitize_flags="-fsanitize=address -fsanitize=leak"
    fi

    cmake .. \
          -DCMAKE_CXX_COMPILER=${COMPILER} \
          -DCMAKE_CXX_FLAGS="${LLVM_CXXFLAGS} ${sanitize_flags}" \
          -DCMAKE_EXE_LINKER_FLAGS="${LLVM_LDFLAGS}" \
          -DCMAKE_BUILD_TYPE=${CONFIGURATION} \
          -DCHECK_BENCHMARKS=${BENCHMARK} \
          -DCHECK_SLOW_TESTS=${slow_tests} \
          -DENABLE_COVERAGE=${COVERAGE} \
          -DDISABLE_FREE_LIST=${disable_free_list} \
          -DBOOST_ROOT=${BOOST_PATH}
    make deps
}

function travis-script
{
    make check VERBOSE=1
}

function travis-after-success
{
    if [[ "${COVERAGE}" == true ]]; then
        gcov-6 -bcprs `dirname ${PWD}` `find . -name "*.gcno"`
        bash <(curl -s https://codecov.io/bash) -x gcov-6
    fi
    if [[ "${BENCHMARK}" == true ]]; then
       	chmod 600 ../tools/travis/ssh-key
        make upload-benchmark-reports
    fi
    if [[ "${DOCS}" == true \
              && "${TRAVIS_PULL_REQUEST}" == "false" \
              && "${TRAVIS_BRANCH}" == "master" ]];
    then
        chmod 600 ../tools/travis/ssh-key
        make docs
        make upload-docs
    fi
}
