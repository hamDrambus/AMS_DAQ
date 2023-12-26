cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
set(PACKAGE daqling)
set(PACKAGE_VERSION 1.0.1)

project(${PACKAGE} VERSION ${PACKAGE_VERSION}
        DESCRIPTION "Data acquisition framework"
        LANGUAGES CXX)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build. Options are: Debug Release" FORCE)
endif(NOT CMAKE_BUILD_TYPE)

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
  message(FATAL_ERROR "Build type not recognized. Options are: Debug Release")
endif()

# Print version
message(STATUS "Building ${PACKAGE} version ${PACKAGE_VERSION}")
message(STATUS "Building \"${CMAKE_BUILD_TYPE}\"")

###############################################
# Setup the environment for the documentation #
###############################################

option(BUILD_DOCS_ONLY "Build documentation only" OFF)

find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in ${CMAKE_BINARY_DIR}/doc/Doxyfile @ONLY)
  add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/doc/Doxyfile
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/doc
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
    )
else(DOXYGEN_FOUND)
  message("Doxygen not found. Cannot build documentation.")
endif(DOXYGEN_FOUND)

# If only building docs, stop processing the rest of the CMake file:
if(BUILD_DOCS_ONLY)
  RETURN()
ENDIF()

###############################
# Setup the build environment #
###############################

include(cmake/daqling.cmake)



if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7)
  message(FATAL_ERROR "GCC version must be at least 7!")
endif()

# Require a C++17 compliant compiler
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#Find ZMQ Libraries
find_library(ZMQ_LIBRARY NAMES zmq)
set(ZMQ_LIBRARIES ${ZMQ_LIBRARY})

# Include Threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
FIND_PACKAGE(Threads REQUIRED)

#Find fmt library
FIND_PACKAGE(fmt REQUIRED)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (ENABLE_TBB)
  set(tbb_version $ENV{TBB_VERSION})
  set(TBB_ROOT_DIR $ENV{TBB_ROOT_DIR})
  find_package(TBB ${tbb_version} EXACT REQUIRED)
  include_directories(SYSTEM ${TBB_INCLUDE_DIRS})
  link_directories(${TBB_ROOT_DIR}/lib)
  set(DAQLING_DEPS_LIBRARIES ${DAQLING_DEPS_LIBRARIES} tbb)
endif (ENABLE_TBB)

if (ENABLE_BOOST)
  set(boost_version $ENV{BOOST_VERSION})
  set(BOOST_ROOT $ENV{BOOST_ROOT_DIR})
  find_package(Boost ${boost_version} EXACT REQUIRED COMPONENTS system filesystem regex)
  include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
endif (ENABLE_BOOST)

if(BUILD_WITHOUT_CPR)
  add_compile_definitions(BUILD_WITHOUT_CPR)
else()
  set(DAQLING_DEPS_LIBRARIES ${DAQLING_DEPS_LIBRARIES} cpr curl)
endif()
# set(USE_SYSTEM_CURL ON)
# set(BUILD_CPR_TESTS OFF)

add_compile_definitions(FOLLY_NO_CONFIG) # Required for building with Folly library

set(BUILD_SHARED_LIBS ON)

########################
# Define build targets #
########################

set(DAQLING_LIBRARIES "")
set(DAQLING_DEPS_LIBRARIES ${DAQLING_DEPS_LIBRARIES} Threads::Threads)

#Add ers library
set(DAQLING_DEPS_LIBRARIES ${DAQLING_DEPS_LIBRARIES} ers)

#Add fmt library
set(DAQLING_DEPS_LIBRARIES ${DAQLING_DEPS_LIBRARIES} fmt::fmt)

# https://cmake.org/cmake/help/v3.0/module/FindXMLRPC.html
find_package(XMLRPC 1.51.06 EXACT REQUIRED c++2 abyss-server)
set(DAQLING_DEPS_LIBRARIES ${DAQLING_DEPS_LIBRARIES} ${XMLRPC_LIBRARIES})

find_package(CURL QUIET REQUIRED)

# Set standard build flags
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wzero-as-null-pointer-constant -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Wredundant-decls -Wsign-conversion -Wstrict-null-sentinel -Wstrict-overflow=5 -Wundef -Werror -Wformat-security -fdiagnostics-color=auto -Wno-overloaded-virtual")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wzero-as-null-pointer-constant -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Wredundant-decls -Wsign-conversion -Wstrict-null-sentinel -Wstrict-overflow=5 -Wundef -Wformat-security -fdiagnostics-color=auto -Wno-overloaded-virtual")

# Disable some for convenience
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function")

option(ENABLE_SANITIZE "Whether to enable extra sanitize flags during debug builds" ON)

# Set flags on debug builds
if(CMAKE_BUILD_TYPE MATCHES Debug AND ((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -U_FORTIFY_SOURCE")

    if(${ENABLE_SANITIZE})
        message(STATUS "Adding extra sanitizer flags")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address -fsanitize=undefined")
    endif()

    message(STATUS "Running debug build, tests enabled")
    enable_testing()
endif()

# Set flags on release builds
if(CMAKE_BUILD_TYPE MATCHES Release)
  message(STATUS "Running release build, adding optimization flags")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
  add_compile_definitions(ERS_NO_DEBUG)

  if(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -qopt-prefetch -unroll-aggressive -march=native -mtune=native")
  endif()
endif()

set(daqling_dir ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT BUILD_Dummy)
  set(BUILD_Dummy OFF)
endif()
if(NOT BUILD_MetricsSimulator)
  set(BUILD_MetricsSimulator OFF)
endif()
if(NOT BUILD_ErsTest)
  set(BUILD_ErsTest OFF)
endif()

include_directories(${daqling_dir}/src)

# Pass properties and variables to parent
get_directory_property(hasParent PARENT_DIRECTORY)
if (hasParent)
  get_property(inc_dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
  set(daqling_include_dirs ${inc_dirs} PARENT_SCOPE)

  get_property(link_dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY LINK_DIRECTORIES)
  set(daqling_link_dirs ${link_dirs} PARENT_SCOPE)

  set(daqling_dir ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

  set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} PARENT_SCOPE)
endif (hasParent)

# Build Streams
add_subdirectory(src/Streams)
set(DAQLING_LIBRARIES ${DAQLING_LIBRARIES} DaqlingStreams)

# Build core daqling library
add_subdirectory(src/Core)
set(DAQLING_LIBRARIES ${DAQLING_LIBRARIES} DaqlingCore)

# Build modules
add_subdirectory(src/Modules)

# Build connections
add_subdirectory(src/Connections)

# Build queues
add_subdirectory(src/Queues)

# Build the executable
add_compile_definitions(TDAQ_PACKAGE_NAME="core")
add_subdirectory(src/Exec)

# Build tests
add_subdirectory(test)

# Add formatting targets
file(GLOB_RECURSE FORMAT_CXX_SOURCE_FILES src/*.[ch]pp test/*.[ch]pp)
include(cmake/clang-format.cmake)
include(cmake/clang-tidy.cmake)
