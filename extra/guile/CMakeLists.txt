
find_package(Guile)

set(IMMER_GUILE_EXTENSION_DIR ${CMAKE_CURRENT_BINARY_DIR})
configure_file(immer.scm.in immer.scm)

add_library(guile-immer SHARED src/immer.cpp)
target_include_directories(guile-immer PUBLIC
  ${Guile_INCLUDE_DIRS})
target_link_libraries(guile-immer PUBLIC
  immer
  ${Guile_LIBRARIES})

add_custom_target(guile DEPENDS guile-immer)
