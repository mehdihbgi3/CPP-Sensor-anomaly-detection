add_library(sensorcore_core STATIC
    src/Sensor.cpp
    src/SensorReading.cpp
    src/Alert.cpp
 "src/SensorProducer.cpp" "src/DataManager.cpp")

target_include_directories(sensorcore_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(sensorcore_core
    PRIVATE
        project_warnings
        spdlog::spdlog
)

target_compile_features(sensorcore_core PUBLIC cxx_std_20)