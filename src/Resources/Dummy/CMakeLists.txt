# Define connection
daqling_resource(resource_name)

# Add source file to library
daqling_target_sources(${resource_name}
    DummyResource.cpp
)

# In case you need other external libraries (uncomment and modify according for your system)
#target_include_directories (${resource_name} PUBLIC <path>/ExternalLibName/include/)
#target_link_libraries(${resource_name} PUBLIC ExternalLibName)

# Provide install target
daqling_target_install(${resource_name})
