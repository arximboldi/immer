# Targets
# =======

add_custom_target(
  doxygen
  COMMAND doxygen doxygen.config
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

add_custom_target(
  docs
  COMMAND make html
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
add_dependencies(docs doxygen)

add_custom_target(
  upload-docs COMMAND rsync -av ${CMAKE_CURRENT_SOURCE_DIR}/_build/html/*
                      root@sinusoid.es:/var/lib/syncthing/public/immer/)
