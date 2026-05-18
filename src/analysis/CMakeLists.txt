add_library(sensorcore_analysis STATIC
    src/StatisticalAnalyzer.cpp
    src/AnomalyDetector.cpp
    src/TrendAnalyzer.cpp
)

target_include_directories(sensorcore_analysis
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(sensorcore_analysis
    PUBLIC
        sensorcore_core
    PRIVATE
        project_warnings
        spdlog::spdlog
)