
function(immer_external_dep target name)
  ExternalProject_Add(${name} EXCLUDE_FROM_ALL 1 "${ARGN}")
  add_dependencies(${target} ${name})
endfunction()

function(immer_external_dep_nobuild target name)
  immer_external_dep(${target} ${name} "${ARGN}"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND "")
endfunction()

function(immer_target_name_for out_target out_file file)
  get_filename_component(_extension ${_file} EXT)

  file(RELATIVE_PATH _relative ${PROJECT_SOURCE_DIR} ${file})
  string(REPLACE "${_extension}" "" _name ${_relative})
  string(REGEX REPLACE "/" "-" _name ${_name})
  set(${out_target} "${_name}" PARENT_SCOPE)

  file(RELATIVE_PATH _relative ${CMAKE_CURRENT_LIST_DIR} ${file})
  string(REPLACE "${_extension}" "" _name ${_relative})
  string(REGEX REPLACE "/" "-" _name ${_name})
  set(${out_file} "${_name}" PARENT_SCOPE)
endfunction()

function(immer_canonicalize_cmake_booleans)
  foreach(var ${ARGN})
    if(${var})
      set(${var} 1 PARENT_SCOPE)
    else()
      set(${var} 0 PARENT_SCOPE)
    endif()
  endforeach()
endfunction()
