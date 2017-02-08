
#  Targets
#  =======

add_custom_target(doxygen
  COMMAND doxygen doxygen.config
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/doc")

add_custom_target(docs
  COMMAND make html
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/doc")
add_dependencies(docs doxygen)

add_custom_target(upload-docs
  COMMAND
  scp -P 5488 -o StrictHostKeyChecking=no -p
      -i ${CMAKE_SOURCE_DIR}/tools/travis/ssh-key
      -r ${CMAKE_SOURCE_DIR}/doc/_build/html/*
      raskolnikov@sinusoid.es:public/immer/)
