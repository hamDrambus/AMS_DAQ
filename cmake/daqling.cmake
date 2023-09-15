# Adapted from <https://gitlab.cern.ch/allpix-squared/allpix-squared/blob/276d03f811de400c704127ee341dbd1566225408/cmake/allpix.cmake>

macro(daqling_module name)
    # Get the name of the module
    get_filename_component(_daqling_module_dir ${CMAKE_CURRENT_SOURCE_DIR} NAME)

    # Build all modules by default if not specified otherwise
    option(BUILD_${_daqling_module_dir} "Build module in directory ${_daqling_module_dir}?" ON)
    message(STATUS "Building module ${_daqling_module_dir} \t - ${BUILD_${_daqling_module_dir}}")

    # Quit the file if not building this file or all modules
    if(NOT (BUILD_${_daqling_module_dir} OR BUILD_ALL_MODULES))
        return()
    endif()

    # Prepend with the Daqling module prefix to create the name of the module
    set(${name} "DaqlingModule${_daqling_module_dir}")

    # Set module class name
    set(_daqling_module_class "${_daqling_module_dir}Module")

    # Check if main header file is defined
    if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_daqling_module_class}.hpp")
        message(FATAL_ERROR "Header file ${_daqling_module_class}.hpp does not exist, cannot build module!")
    endif()

    # Define the library
    add_library(${${name}} SHARED "")

    # Add the current directory as include directory
    target_include_directories(${${name}} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    # Set the special header flags and add the special dynamic implementation file
    target_compile_definitions(${${name}} PRIVATE DAQLING_MODULE_NAME=${_daqling_module_class})
    target_compile_definitions(${${name}} PRIVATE DAQLING_MODULE_HEADER="${_daqling_module_class}.hpp")
    target_sources(${${name}} PRIVATE "${daqling_dir}/src/Core/dynamic_module_impl.cpp")
    set_property(SOURCE "${daqling_dir}/src/Core/dynamic_module_impl.cpp" APPEND PROPERTY OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${_daqling_module_class}.hpp")
endmacro()

macro(daqling_connection name)

    get_filename_component(_daqling_connection_dir ${CMAKE_CURRENT_SOURCE_DIR} NAME)


    set(${name} "DaqlingConnection${_daqling_connection_dir}")

    set(_daqling_sender_class "${_daqling_connection_dir}Sender")
    set(_daqling_receiver_class "${_daqling_connection_dir}Receiver")

    if (NOT ((EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_daqling_sender_class}.hpp") OR (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_daqling_receiver_class}.hpp")))
        message(FATAL_ERROR "neither Header file ${_daqling_sender_class}.hpp or ${_daqling_receiver_class}.hpp exists, cannot build connection!")
    endif()

    # Define the library
    add_library(${${name}} SHARED "")

    target_include_directories(${${name}} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

endmacro()

macro(daqling_queue name)

    get_filename_component(_daqling_queue_dir ${CMAKE_CURRENT_SOURCE_DIR} NAME)


    set(${name} "DaqlingQueue${_daqling_queue_dir}")

    set(_daqling_queue_class "${_daqling_queue_dir}")
    

    # Check if main header file is defined
    if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_daqling_queue_class}.hpp")
        message(FATAL_ERROR "Header file ${_daqling_queue_class}.hpp does not exist, cannot build queue!")
    endif()

    # Define the library
    add_library(${${name}} SHARED "")

    target_include_directories(${${name}} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

endmacro()
macro(daqling_resource name)

    get_filename_component(_daqling_resource_dir ${CMAKE_CURRENT_SOURCE_DIR} NAME)


    set(${name} "DaqlingResource${_daqling_resource_dir}")

    set(_daqling_resource_class "${_daqling_resource_dir}")
    

    # Check if main header file is defined
    if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_daqling_resource_class}.hpp")
        message(FATAL_ERROR "Header file ${_daqling_resource_class}.hpp does not exist, cannot build resource!")
    endif()

    # Define the library
    add_library(${${name}} SHARED "")

    target_include_directories(${${name}} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

endmacro()

macro(daqling_executable name)
    # Define the executable
    add_executable(${name} "")

    # Add the required logging symbols
    target_compile_definitions(${name} PRIVATE DAQLING_EXECUTABLE_NAME=${name})
    
endmacro()

# Add sources to the module
macro(daqling_target_sources name)
    # Get the list of sources
    set(_list_var "${ARGN}")
    list(REMOVE_ITEM _list_var ${name})

    # Add the library
    target_sources(${name} PRIVATE ${_list_var})

    # Link the standard daqling libraries
    target_link_libraries(${name} ${DAQLING_LIBRARIES} ${DAQLING_DEPS_LIBRARIES})
endmacro()

# Provide default install target for the module
macro(daqling_target_install name)
    install(TARGETS ${name}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
endmacro()
