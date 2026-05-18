#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "sensorcore/Sensor.hpp"
#include "sensorcore/SensorProducer.hpp"
#include "sensorcore/SensorReading.hpp"
#include "sensorcore/ThreadSafeQueue.hpp"

namespace sensorcore {

class DataManager {
public:
    using ReadingCallback = std::function<void(const SensorReading&)>;

    DataManager();
    ~DataManager();

    DataManager(const DataManager&) = delete;
    DataManager& operator=(const DataManager&) = delete;

    void addSensor(std::shared_ptr<Sensor> sensor, SensorProducer::Config config = {});

    void startAll();
    void stopAll();

    [[nodiscard]] bool isRunning() const;

    [[nodiscard]] std::shared_ptr<ThreadSafeQueue<SensorReading>> getQueue() const {
        return queue_;
    }

    [[nodiscard]] const std::vector<std::shared_ptr<Sensor>>& sensors() const { return sensors_; }

    [[nodiscard]] SensorProducer* getProducer(const std::string& sensorId);
    [[nodiscard]] uint64_t totalReadingsGenerated() const;

    size_t processReadings(const ReadingCallback& callback);
    bool processOneReading(const ReadingCallback& callback,
                           std::chrono::milliseconds timeout = std::chrono::milliseconds(10));

private:
    std::shared_ptr<ThreadSafeQueue<SensorReading>> queue_;
    std::vector<std::shared_ptr<Sensor>> sensors_;
    std::unordered_map<std::string, std::unique_ptr<SensorProducer>> producers_;
};

}  