add_library(sensorcore_services STATIC
    src/MonitoringService.cpp
    
    src/AnalysisService.cpp
)

target_include_directories(sensorcore_services
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(sensorcore_services
    PUBLIC
        sensorcore_core
        sensorcore_analysis
        sensorcore_persistence
    PRIVATE
        project_warnings
        spdlog::spdlog
)