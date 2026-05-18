cmake_minimum_required(VERSION 3.20)

project(SensorCorePro
    VERSION 1.0.0
    DESCRIPTION "Industrial Real-Time Sensor Monitoring & Analysis Platform"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SENSORCORE_BUILD_TESTS "Build unit tests" ON)
option(SENSORCORE_BUILD_GUI "Build Qt GUI application" ON)

add_library(project_warnings INTERFACE)

if(MSVC)
    target_compile_options(project_warnings INTERFACE /W4)
    target_compile_definitions(project_warnings INTERFACE 
        _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING
        _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS
    )
else()
    target_compile_options(project_warnings INTERFACE
        -Wall -Wextra -Wpedantic -Werror
    )
endif()

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.12.0
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

find_package(Qt5 COMPONENTS Widgets Core Gui PrintSupport QUIET)

if(Qt5_FOUND AND SENSORCORE_BUILD_GUI)
    message(STATUS "Qt5 found: ${Qt5_VERSION}")
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)
else()
    message(STATUS "Qt5 not found, GUI will not be built")
    set(SENSORCORE_BUILD_GUI OFF)
endif()

add_subdirectory(src/core)
add_subdirectory(src/analysis)
add_subdirectory(src/acquisition)
add_subdirectory(src/persistence)
add_subdirectory(src/services)

if(SENSORCORE_BUILD_GUI)
    add_subdirectory(src/gui)
endif()

if(SENSORCORE_BUILD_TESTS)
    enable_testing()
    include(GoogleTest)
    add_subdirectory(tests)
endif()

install(TARGETS sensorcore_core sensorcore_analysis
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

message(STATUS "")
message(STATUS "--- SensorCorePro Configuration ---")
message(STATUS "  Version:      ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build Tests:  ${SENSORCORE_BUILD_TESTS}")
message(STATUS "  Build GUI:    ${SENSORCORE_BUILD_GUI}")
message(STATUS "  Qt5 Found:    ${Qt5_FOUND}")
message(STATUS "--------")
message(STATUS "")