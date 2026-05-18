#pragma once

#include <atomic>
#include <memory>
#include <random>
#include <thread>

#include "sensorcore/Sensor.hpp"
#include "sensorcore/SensorReading.hpp"
#include "sensorcore/ThreadSafeQueue.hpp"

namespace sensorcore {

class SensorProducer {
public:
    struct Config {
        std::chrono::milliseconds sampleInterval{100};
        double noiseLevel = 0.05;
        double trendRate = 0.01;
        double anomalyProbability = 0.02;
    };

    SensorProducer(std::shared_ptr<Sensor> sensor,
                   std::shared_ptr<ThreadSafeQueue<SensorReading>> queue, Config config = {});

    ~SensorProducer();

    SensorProducer(const SensorProducer&) = delete;
    SensorProducer& operator=(const SensorProducer&) = delete;

    void start();
    void stop();

    [[nodiscard]] bool isRunning() const { return running_.load(); }
    [[nodiscard]] const Sensor& sensor() const { return *sensor_; }
    [[nodiscard]] uint64_t readingsGenerated() const { return readingsGenerated_.load(); }

private:
    void producerLoop();
    double generateReading();

    std::shared_ptr<Sensor> sensor_;
    std::shared_ptr<ThreadSafeQueue<SensorReading>> queue_;
    Config config_;

    std::thread thread_;
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> readingsGenerated_{0};

    std::mt19937 rng_{std::random_device{}()};
    double lastValue_{0.0};
};

}  