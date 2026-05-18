#pragma once

#include <chrono>
#include <string>

#include "sensorcore/Types.hpp"

namespace sensorcore {

struct SensorReading {
    std::string sensorId;
    double value;
    std::chrono::system_clock::time_point timestamp;
    ReadingStatus status;

    SensorReading() = default;

    SensorReading(std::string id, double val, ReadingStatus stat = ReadingStatus::Normal)
        : sensorId(std::move(id)),
          value(val),
          timestamp(std::chrono::system_clock::now()),
          status(stat) {}
};

} 