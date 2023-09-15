daqling_executable(daqling)
daqling_target_sources(daqling
    daqling.cpp
)
daqling_target_install(daqling)

target_link_libraries(daqling rt)
