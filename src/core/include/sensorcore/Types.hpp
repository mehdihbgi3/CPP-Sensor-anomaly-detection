#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace sensorcore {

using Timestamp = std::chrono::system_clock::time_point;

class SensorId {
public:
    SensorId() = default;
    explicit SensorId(std::string id) : value_(std::move(id)) {}

    const std::string& value() const { return value_; }

    bool operator==(const SensorId& other) const { return value_ == other.value_; }
    bool operator!=(const SensorId& other) const { return !(*this == other); }
    bool operator<(const SensorId& other) const { return value_ < other.value_; }

    bool empty() const { return value_.empty(); }

private:
    std::string value_;
};

class AlertId {
public:
    AlertId() = default;
    explicit AlertId(std::string id) : value_(std::move(id)) {}

    const std::string& value() const { return value_; }

    bool operator==(const AlertId& other) const { return value_ == other.value_; }
    bool operator!=(const AlertId& other) const { return !(*this == other); }

private:
    std::string value_;
};

enum class SensorType {
    Temperature,
    Pressure,
    Vibration,
    FlowRate,
    Humidity,
    Level,
    Current,
    Voltage
};

enum class Unit {
    Celsius,
    Fahrenheit,
    Kelvin,
    Pascal,
    Bar,
    PSI,
    MetersPerSecondSquared,
    LitersPerMinute,
    CubicMetersPerHour,
    Percent,
    Meters,
    Amperes,
    Volts
};

enum class Quality { Good, Uncertain, Bad, Unknown };

enum class ReadingStatus { Normal, Warning, Critical, Invalid };

enum class AlertSeverity { Info, Warning, Critical, Emergency };

enum class SensorState { Unknown, Normal, Warning, Critical, Faulted, Offline };

struct Range {
    double min;
    double max;

    bool contains(double value) const { return value >= min && value <= max; }
};

struct ThresholdSet {
    double warningLow;
    double warningHigh;
    double criticalLow;
    double criticalHigh;
};

struct ValidationResult {
    ReadingStatus status;
    std::string message;
};

inline std::string sensorTypeToString(SensorType type) {
    switch (type) {
    case SensorType::Temperature:
        return "Temperature";
    case SensorType::Pressure:
        return "Pressure";
    case SensorType::Vibration:
        return "Vibration";
    case SensorType::FlowRate:
        return "FlowRate";
    case SensorType::Humidity:
        return "Humidity";
    case SensorType::Level:
        return "Level";
    case SensorType::Current:
        return "Current";
    case SensorType::Voltage:
        return "Voltage";
    default:
        return "Unknown";
    }
}

inline std::string unitToString(Unit unit) {
    switch (unit) {
    case Unit::Celsius:
        return "°C";
    case Unit::Fahrenheit:
        return "°F";
    case Unit::Kelvin:
        return "K";
    case Unit::Pascal:
        return "Pa";
    case Unit::Bar:
        return "bar";
    case Unit::PSI:
        return "psi";
    case Unit::MetersPerSecondSquared:
        return "m/s²";
    case Unit::LitersPerMinute:
        return "L/min";
    case Unit::CubicMetersPerHour:
        return "m³/h";
    case Unit::Percent:
        return "%";
    case Unit::Meters:
        return "m";
    case Unit::Amperes:
        return "A";
    case Unit::Volts:
        return "V";
    default:
        return "";
    }
}

}  