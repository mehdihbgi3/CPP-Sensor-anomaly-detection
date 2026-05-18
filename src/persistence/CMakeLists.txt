find_package(unofficial-sqlite3 CONFIG REQUIRED)

add_library(sensorcore_persistence STATIC
    src/DatabaseManager.cpp
    src/SQLiteSensorRepository.cpp
    src/SQLiteReadingRepository.cpp
)
target_include_directories(sensorcore_persistence
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(sensorcore_persistence
    PUBLIC
        sensorcore_core
    PRIVATE
        project_warnings
        spdlog::spdlog
        unofficial::sqlite3::sqlite3
)