# Define module
daqling_module(module_name)

# Add source file to library
daqling_target_sources(${module_name}
    MetricsSimulatorModule.cpp
)

# Provide install target
daqling_target_install(${module_name})
