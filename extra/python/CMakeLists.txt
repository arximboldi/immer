
cmake_minimum_required(VERSION 3.1)
project(immer-python)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSIONS off)
set(PYBIND11_CPP_STANDARD -std=c++14)

find_package(Boost 1.56 REQUIRED)

set(immer_include_dir "${PROJECT_SOURCE_DIR}/../..")

include_directories(
  ${immer_include_dir}
  ${Boost_INCLUDE_DIR})

add_subdirectory(lib/pybind11)
pybind11_add_module(immer src/main.cpp)
