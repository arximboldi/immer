
cmake_minimum_required(VERSION 3.1)
project(immer-python)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_EXTENSIONS off)

option(USE_PYBIND "bind with pybind1" off)
option(USE_BOOST  "bind with boost::python" off)

set(immer_include_dir "${PROJECT_SOURCE_DIR}/../..")

if (USE_PYBIND)
  set(PYBIND11_CPP_STANDARD -std=c++14)
  find_package(Boost 1.56 REQUIRED)
  include_directories(
    ${immer_include_dir}
    ${Boost_INCLUDE_DIRS})
  add_subdirectory(lib/pybind11)
  pybind11_add_module(immer src/main-pybind.cpp)
elseif(USE_BOOST)
  find_package(PythonInterp)
  find_package(PythonLibs)
  find_package(Boost 1.56 COMPONENTS python)
  include_directories(
    ${immer_include_dir}
    ${Boost_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS})
  link_libraries(
    ${Boost_LIBRARIES}
    ${PYTHON_LIBRARIES})
  python_add_module(immer src/main-boost.cpp)
else()
  find_package(PythonInterp)
  find_package(PythonLibs)
  include_directories(
    ${immer_include_dir}
    ${PYTHON_INCLUDE_DIRS})
  link_libraries(
    ${PYTHON_LIBRARIES})
  python_add_module(immer src/main-raw.cpp)
endif()
