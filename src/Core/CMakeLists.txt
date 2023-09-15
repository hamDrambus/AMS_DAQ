# Include the dependencies
include_directories(SYSTEM ${DAQLING_DEPS_INCLUDE_DIRS})

# Create core library
add_library(DaqlingCore SHARED
  Core.cpp
  Command.cpp
  ConnectionManager.cpp
  ModuleManager.cpp
  ModuleLoader.cpp
  Statistics.cpp
  ConnectionLoader.cpp
  Sender.cpp
  Receiver.cpp
  Queue.cpp
)
add_compile_definitions(TDAQ_PACKAGE_NAME="core")


# Link the dependencies
#target_link_libraries(DaqlingCore ${DAQLING_DEPS_LIBRARIES})
#target_link_libraries(DaqlingCore ${DAQLING_LIBRARIES})
target_link_libraries(DaqlingCore
    PRIVATE
        ${DAQLING_LIBRARIES}
        ${CMAKE_DL_LIBS}
    PUBLIC
        ${DAQLING_DEPS_LIBRARIES}
        ${Boost_LIBRARIES}
        ${ZMQ_LIBRARIES}
        ${TBB_LIBRARY}
        # CMAKE bug https://gitlab.kitware.com/cmake/cmake/-/issues/19530
        #Boost::timer
        #Boost::program_options
        #Boost::filesystem
        pthread
)

# Define compile-time library extension
target_compile_definitions(DaqlingCore PRIVATE SHARED_LIBRARY_SUFFIX="${CMAKE_SHARED_LIBRARY_SUFFIX}")
# Link the DL libraries
#target_link_libraries(DaqlingCore ${CMAKE_DL_LIBS})

# Create install target
install(TARGETS DaqlingCore
    COMPONENT application
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib)
