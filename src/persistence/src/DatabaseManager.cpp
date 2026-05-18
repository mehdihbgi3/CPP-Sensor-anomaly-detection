#include "persistence/DatabaseManager.hpp"

#include <sqlite3.h>

namespace sensorcore::persistence {

DatabaseManager::DatabaseManager(const std::string& dbPath) : dbPath_(dbPath) {
    int rc = sqlite3_open(dbPath.c_str(), &db_);
    if (rc != SQLITE_OK) {
        lastError_ = sqlite3_errmsg(db_);
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

DatabaseManager::~DatabaseManager() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool DatabaseManager::initialize() {
    if (!db_) return false;

    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS sensor_readings (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            sensor_id TEXT NOT NULL,
            value REAL NOT NULL,
            timestamp INTEGER NOT NULL,
            status INTEGER NOT NULL
        );
        CREATE INDEX IF NOT EXISTS idx_sensor_id ON sensor_readings(sensor_id);
        CREATE INDEX IF NOT EXISTS idx_timestamp ON sensor_readings(timestamp);
    )";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        lastError_ = errMsg;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool DatabaseManager::storeReading(const SensorReading& reading) {
    if (!db_) return false;

    const char* sql = "INSERT INTO sensor_readings (sensor_id, value, timestamp, status) VALUES (?, ?, ?, ?)";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        lastError_ = sqlite3_errmsg(db_);
        return false;
    }

    sqlite3_bind_text(stmt, 1, reading.sensorId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, reading.value);
    sqlite3_bind_int64(stmt, 3, toTimestamp(reading.timestamp));
    sqlite3_bind_int(stmt, 4, static_cast<int>(reading.status));

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        lastError_ = sqlite3_errmsg(db_);
        return false;
    }
    return true;
}

bool DatabaseManager::storeReadings(const std::vector<SensorReading>& readings) {
    if (!db_ || readings.empty()) return false;

    sqlite3_exec(db_, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
    
    for (const auto& reading : readings) {
        if (!storeReading(reading)) {
            sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);
            return false;
        }
    }
    
    sqlite3_exec(db_, "COMMIT", nullptr, nullptr, nullptr);
    return true;
}

std::vector<SensorReading> DatabaseManager::getReadings(
    const std::string& sensorId,
    std::optional<std::chrono::system_clock::time_point> startTime,
    std::optional<std::chrono::system_clock::time_point> endTime,
    size_t limit) {
    
    std::vector<SensorReading> readings;
    if (!db_) return readings;

    std::string sql = "SELECT sensor_id, value, timestamp, status FROM sensor_readings WHERE sensor_id = ?";
    
    if (startTime) sql += " AND timestamp >= ?";
    if (endTime) sql += " AND timestamp <= ?";
    sql += " ORDER BY timestamp DESC LIMIT ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        lastError_ = sqlite3_errmsg(db_);
        return readings;
    }

    int paramIdx = 1;
    sqlite3_bind_text(stmt, paramIdx++, sensorId.c_str(), -1, SQLITE_TRANSIENT);
    if (startTime) sqlite3_bind_int64(stmt, paramIdx++, toTimestamp(*startTime));
    if (endTime) sqlite3_bind_int64(stmt, paramIdx++, toTimestamp(*endTime));
    sqlite3_bind_int64(stmt, paramIdx, static_cast<int64_t>(limit));

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SensorReading reading;
        reading.sensorId = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        reading.value = sqlite3_column_double(stmt, 1);
        reading.timestamp = fromTimestamp(sqlite3_column_int64(stmt, 2));
        reading.status = static_cast<ReadingStatus>(sqlite3_column_int(stmt, 3));
        readings.push_back(reading);
    }

    sqlite3_finalize(stmt);
    return readings;
}

std::optional<DatabaseManager::SensorStats> DatabaseManager::getSensorStats(const std::string& sensorId) {
    if (!db_) return std::nullopt;

    const char* sql = R"(
        SELECT sensor_id, COUNT(*), MIN(value), MAX(value), AVG(value)
        FROM sensor_readings WHERE sensor_id = ? GROUP BY sensor_id
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        lastError_ = sqlite3_errmsg(db_);
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, sensorId.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        SensorStats stats;
        stats.sensorId = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        stats.totalReadings = static_cast<size_t>(sqlite3_column_int64(stmt, 1));
        stats.minValue = sqlite3_column_double(stmt, 2);
        stats.maxValue = sqlite3_column_double(stmt, 3);
        stats.avgValue = sqlite3_column_double(stmt, 4);
        sqlite3_finalize(stmt);
        return stats;
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

std::vector<std::string> DatabaseManager::getAllSensorIds() {
    std::vector<std::string> ids;
    if (!db_) return ids;

    const char* sql = "SELECT DISTINCT sensor_id FROM sensor_readings";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return ids;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ids.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }

    sqlite3_finalize(stmt);
    return ids;
}

size_t DatabaseManager::getReadingCount(const std::string& sensorId) {
    if (!db_) return 0;

    std::string sql = "SELECT COUNT(*) FROM sensor_readings";
    if (!sensorId.empty()) sql += " WHERE sensor_id = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }

    if (!sensorId.empty()) {
        sqlite3_bind_text(stmt, 1, sensorId.c_str(), -1, SQLITE_TRANSIENT);
    }

    size_t count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = static_cast<size_t>(sqlite3_column_int64(stmt, 0));
    }

    sqlite3_finalize(stmt);
    return count;
}

bool DatabaseManager::deleteOldReadings(std::chrono::system_clock::time_point before) {
    if (!db_) return false;

    const char* sql = "DELETE FROM sensor_readings WHERE timestamp < ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        lastError_ = sqlite3_errmsg(db_);
        return false;
    }

    sqlite3_bind_int64(stmt, 1, toTimestamp(before));
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

int64_t DatabaseManager::toTimestamp(std::chrono::system_clock::time_point tp) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        tp.time_since_epoch()).count();
}

std::chrono::system_clock::time_point DatabaseManager::fromTimestamp(int64_t ts) {
    return std::chrono::system_clock::time_point(std::chrono::milliseconds(ts));
}

}  