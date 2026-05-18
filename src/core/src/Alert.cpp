#include "sensorcore/Alert.hpp"

#include <iomanip>
#include <random>
#include <sstream>

namespace sensorcore {

Alert::Alert(AlertId id, SensorId sensorId, AlertSeverity severity, std::string message,
             Timestamp triggeredAt)
    : id_(std::move(id)),
      sensorId_(std::move(sensorId)),
      severity_(severity),
      message_(std::move(message)),
      triggeredAt_(triggeredAt) {}

void Alert::acknowledge(const std::string& user, Timestamp time) {
    if (isAcknowledged()) {
        return;
    }
    acknowledgedAt_ = time;
    acknowledgedBy_ = user;
}

AlertId generateAlertId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;

    std::stringstream ss;
    ss << "ALT-" << std::hex << std::setfill('0') << std::setw(16) << dis(gen);
    return AlertId(ss.str());
}

}  