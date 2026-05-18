#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <chrono>

#include "sensorcore/SensorReading.hpp"

struct sqlite3;

namespace sensorcore::persistence {

class DatabaseManager {
public:
    struct SensorStats {
        std::string sensorId;
        size_t totalReadings;
        double minValue;
        double maxValue;
        double avgValue;
    };

    explicit DatabaseManager(const std::string& dbPath);
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool initialize();
    bool storeReading(const SensorReading& reading);
    bool storeReadings(const std::vector<SensorReading>& readings);
    
    std::vector<SensorReading> getReadings(
        const std::string& sensorId,
        std::optional<std::chrono::system_clock::time_point> startTime = std::nullopt,
        std::optional<std::chrono::system_clock::time_point> endTime = std::nullopt,
        size_t limit = 1000);
    
    std::optional<SensorStats> getSensorStats(const std::string& sensorId);
    std::vector<std::string> getAllSensorIds();
    size_t getReadingCount(const std::string& sensorId = "");
    bool deleteOldReadings(std::chrono::system_clock::time_point before);
    
    [[nodiscard]] bool isOpen() const { return db_ != nullptr; }
    [[nodiscard]] const std::string& lastError() const { return lastError_; }

private:
    sqlite3* db_ = nullptr;
    std::string dbPath_;
    std::string lastError_;
    
    int64_t toTimestamp(std::chrono::system_clock::time_point tp);
    std::chrono::system_clock::time_point fromTimestamp(int64_t ts);
};

}  