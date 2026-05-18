#pragma once

#include <optional>

#include "Types.hpp"

namespace sensorcore {

class Alert {
public:
    Alert(AlertId id, SensorId sensorId, AlertSeverity severity, std::string message,
          Timestamp triggeredAt);

    const AlertId& id() const { return id_; }
    const SensorId& sensorId() const { return sensorId_; }
    AlertSeverity severity() const { return severity_; }
    const std::string& message() const { return message_; }
    Timestamp triggeredAt() const { return triggeredAt_; }
    std::optional<Timestamp> acknowledgedAt() const { return acknowledgedAt_; }
    const std::string& acknowledgedBy() const { return acknowledgedBy_; }

    bool isAcknowledged() const { return acknowledgedAt_.has_value(); }
    bool isActive() const { return !isAcknowledged(); }

    void acknowledge(const std::string& user, Timestamp time);

    bool operator==(const Alert& other) const { return id_ == other.id_; }

private:
    AlertId id_;
    SensorId sensorId_;
    AlertSeverity severity_;
    std::string message_;
    Timestamp triggeredAt_;
    std::optional<Timestamp> acknowledgedAt_;
    std::string acknowledgedBy_;
};

AlertId generateAlertId();

}  