add_library(sensorcore_acquisition STATIC
    src/SimulatedSource.cpp
    src/CSVFileSource.cpp
)

target_include_directories(sensorcore_acquisition
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(sensorcore_acquisition
    PUBLIC
        sensorcore_core
    PRIVATE
        project_warnings
        spdlog::spdlog
)