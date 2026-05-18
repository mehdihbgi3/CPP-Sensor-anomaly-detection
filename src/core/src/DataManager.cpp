#include "sensorcore/DataManager.hpp"

namespace sensorcore {

DataManager::DataManager() : queue_(std::make_shared<ThreadSafeQueue<SensorReading>>()) {}

DataManager::~DataManager() {
    stopAll();
}

void DataManager::addSensor(std::shared_ptr<Sensor> sensor, SensorProducer::Config config) {
    const std::string& id = sensor->id().value();

    sensors_.push_back(sensor);
    producers_[id] = std::make_unique<SensorProducer>(sensor, queue_, config);
}

void DataManager::startAll() {
    queue_->reset();

    for (auto& [id, producer] : producers_) {
        producer->start();
    }
}

void DataManager::stopAll() {
    for (auto& [id, producer] : producers_) {
        producer->stop();
    }
    queue_->stop();
}

bool DataManager::isRunning() const {
    for (const auto& [id, producer] : producers_) {
        if (producer->isRunning()) {
            return true;
        }
    }
    return false;
}

SensorProducer* DataManager::getProducer(const std::string& sensorId) {
    auto it = producers_.find(sensorId);
    if (it != producers_.end()) {
        return it->second.get();
    }
    return nullptr;
}

uint64_t DataManager::totalReadingsGenerated() const {
    uint64_t total = 0;
    for (const auto& [id, producer] : producers_) {
        total += producer->readingsGenerated();
    }
    return total;
}

size_t DataManager::processReadings(const ReadingCallback& callback) {
    size_t count = 0;

    while (auto reading = queue_->tryPop()) {
        callback(*reading);
        ++count;
    }

    return count;
}

bool DataManager::processOneReading(const ReadingCallback& callback,
                                    std::chrono::milliseconds timeout) {
    auto reading = queue_->popWithTimeout(timeout);
    if (reading) {
        callback(*reading);
        return true;
    }
    return false;
}

}  