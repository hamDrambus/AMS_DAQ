# Define connection
daqling_connection(connection_name)

# include parent dir
get_filename_component(PARENT_DIR ../ ABSOLUTE)
target_include_directories(${connection_name} PUBLIC ${PARENT_DIR})

# Add source file to library
daqling_target_sources(${connection_name}
    LocalSender.cpp
    LocalReceiver.cpp
)

# In case you need other external libraries (uncomment and modify according for your system)
#target_include_directories (${connection_name} PUBLIC <path>/ExternalLibName/include/)
#target_link_libraries(${connection_name} PUBLIC ExternalLibName)

# Provide install target
daqling_target_install(${connection_name})
