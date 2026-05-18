# Qt5 GUI Application

set(GUI_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/SensorWidget.cpp
    src/ChartWidget.cpp
    src/StatusPanel.cpp
)

set(GUI_HEADERS
    include/gui/MainWindow.hpp
    include/gui/SensorWidget.hpp
    include/gui/ChartWidget.hpp
    include/gui/StatusPanel.hpp
)

add_executable(SensorCorePro WIN32
    ${GUI_SOURCES}
    ${GUI_HEADERS}
)

target_include_directories(SensorCorePro
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(SensorCorePro
    PRIVATE
        sensorcore_core
        sensorcore_analysis
        sensorcore_services
        Qt5::Widgets
        Qt5::Core
        Qt5::Gui
        Qt5::PrintSupport
        spdlog::spdlog
        project_warnings
)

target_compile_features(SensorCorePro PRIVATE cxx_std_20)

# Copy Qt DLLs to output directory (Windows)
if(WIN32)
    add_custom_command(TARGET SensorCorePro POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt5::Widgets>
            $<TARGET_FILE:Qt5::Core>
            $<TARGET_FILE:Qt5::Gui>
            $<TARGET_FILE:Qt5::PrintSupport>
            $<TARGET_FILE_DIR:SensorCorePro>
    )
endif()