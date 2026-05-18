#pragma once

#include <stdexcept>
#include <string>

#include "Types.hpp"

namespace sensorcore {

struct SensorConfig {
    SensorId id;
    std::string name;
    SensorType type;
    Unit unit;
    Range validRange;
    ThresholdSet thresholds;
    std::string description;
    std::string location;
};

class Sensor {
public:
    explicit Sensor(const SensorConfig& config);

    const SensorId& id() const { return id_; }
    const std::string& name() const { return name_; }
    SensorType type() const { return type_; }
    Unit unit() const { return unit_; }
    const Range& validRange() const { return validRange_; }
    const ThresholdSet& thresholds() const { return thresholds_; }
    const std::string& description() const { return description_; }
    const std::string& location() const { return location_; }
    SensorState state() const { return state_; }

    void setName(const std::string& name);
    void setThresholds(const ThresholdSet& thresholds);
    void setDescription(const std::string& description);
    void setLocation(const std::string& location);
    void setState(SensorState state) { state_ = state; }

    ValidationResult validateReading(double value) const;
    bool isValueInRange(double value) const;

    bool operator==(const Sensor& other) const { return id_ == other.id_; }
    bool operator!=(const Sensor& other) const { return !(*this == other); }

private:
    void validateConfiguration() const;
    void validateThresholds(const ThresholdSet& thresholds) const;

private:
    SensorId id_;
    std::string name_;
    SensorType type_;
    Unit unit_;
    Range validRange_;
    ThresholdSet thresholds_;
    std::string description_;
    std::string location_;
    SensorState state_ = SensorState::Unknown;
};

}  