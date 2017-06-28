
find_package(PkgConfig)

add_custom_target(deps-guile COMMENT "Get guile dependencies")
add_dependencies(deps deps-guile)

set(CALLABLE_TRAITS_INCLUDE_DIR
  "${DEPS_DIR}/Source/callable_traits/include")

immer_external_dep_nobuild(deps-guile callable_traits
  GIT_REPOSITORY https://github.com/badair/callable_traits.git)

pkg_check_modules(Guile guile-2.2)

set(GUILE_EXTENSION_DIR ${CMAKE_CURRENT_BINARY_DIR})
configure_file(immer.scm.in immer.scm)

add_library(guile-immer SHARED
  src/immer.cpp)
target_include_directories(guile-immer PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CALLABLE_TRAITS_INCLUDE_DIR}
  ${Guile_INCLUDE_DIRS})
target_link_libraries(guile-immer PUBLIC
  immer
  ${Guile_LIBRARIES})

add_custom_target(guile DEPENDS guile-immer)
