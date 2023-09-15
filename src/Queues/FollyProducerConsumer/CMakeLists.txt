# Define queue
daqling_queue(queue_name)

# include parent dir
get_filename_component(PARENT_DIR ../ ABSOLUTE)
target_include_directories(${queue_name} PUBLIC ${PARENT_DIR})


# Add source file to library
daqling_target_sources(${queue_name}
    FollyProducerConsumer.cpp
)

# In case you need other external libraries (uncomment and modify according for your system)
#target_include_directories (${queue_name} PUBLIC <path>/ExternalLibName/include/)
#target_link_libraries(${queue_name} PUBLIC ExternalLibName)

# Provide install target
daqling_target_install(${queue_name})
