# Add build targets for all modules.
# Macros used for declaring a module decide whether a module is built..
file(GLOB subdirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)
add_compile_definitions(TDAQ_PACKAGE_NAME="queues")

foreach(subdir ${subdirs})
  if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${subdir})
      add_subdirectory(${subdir})
  endif()
endforeach()