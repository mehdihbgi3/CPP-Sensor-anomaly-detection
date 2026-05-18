#include "sensorcore/Sensor.hpp"

#include <sstream>
#include <stdexcept>

namespace sensorcore {

Sensor::Sensor(const SensorConfig& config)
    : id_(config.id),
      name_(config.name),
      type_(config.type),
      unit_(config.unit),
      validRange_(config.validRange),
      thresholds_(config.thresholds),
      description_(config.description),
      location_(config.location),
      state_(SensorState::Unknown) {
    validateConfiguration();
}

void Sensor::validateConfiguration() const {
    if (id_.empty()) {
        throw std::invalid_argument("Sensor ID cannot be empty");
    }

    if (name_.empty()) {
        throw std::invalid_argument("Sensor name cannot be empty");
    }

    if (validRange_.min >= validRange_.max) {
        throw std::invalid_argument("Invalid range: min must be less than max");
    }

    validateThresholds(thresholds_);
}

void Sensor::validateThresholds(const ThresholdSet& thresholds) const {
    if (thresholds.criticalLow < validRange_.min || thresholds.criticalHigh > validRange_.max) {
        throw std::invalid_argument("Critical thresholds must be within valid range");
    }

    if (thresholds.warningLow < thresholds.criticalLow ||
        thresholds.warningHigh > thresholds.criticalHigh) {
        throw std::invalid_argument("Warning thresholds must be within critical thresholds");
    }

    if (thresholds.warningLow >= thresholds.warningHigh ||
        thresholds.criticalLow >= thresholds.criticalHigh) {
        throw std::invalid_argument("Low thresholds must be less than high thresholds");
    }
}

void Sensor::setName(const std::string& name) {
    if (name.empty()) {
        throw std::invalid_argument("Sensor name cannot be empty");
    }
    name_ = name;
}

void Sensor::setThresholds(const ThresholdSet& thresholds) {
    validateThresholds(thresholds);
    thresholds_ = thresholds;
}

void Sensor::setDescription(const std::string& description) {
    description_ = description;
}

void Sensor::setLocation(const std::string& location) {
    location_ = location;
}

ValidationResult Sensor::validateReading(double value) const {
    if (!isValueInRange(value)) {
        return {ReadingStatus::Invalid, "Value outside valid range"};
    }

    if (value <= thresholds_.criticalLow) {
        return {ReadingStatus::Critical, "Value below critical low threshold"};
    }
    if (value >= thresholds_.criticalHigh) {
        return {ReadingStatus::Critical, "Value above critical high threshold"};
    }

    if (value <= thresholds_.warningLow) {
        return {ReadingStatus::Warning, "Value below warning low threshold"};
    }
    if (value >= thresholds_.warningHigh) {
        return {ReadingStatus::Warning, "Value above warning high threshold"};
    }

    return {ReadingStatus::Normal, ""};
}

bool Sensor::isValueInRange(double value) const {
    return validRange_.contains(value);
}

}  