#include "sensorcore/SensorProducer.hpp"

#include <algorithm>

namespace sensorcore {

SensorProducer::SensorProducer(std::shared_ptr<Sensor> sensor,
                               std::shared_ptr<ThreadSafeQueue<SensorReading>> queue, Config config)
    : sensor_(std::move(sensor)), queue_(std::move(queue)), config_(config) {
    const auto& thresholds = sensor_->thresholds();
    lastValue_ = (thresholds.warningLow + thresholds.warningHigh) / 2.0;
}

SensorProducer::~SensorProducer() {
    stop();
}

void SensorProducer::start() {
    if (running_.load()) {
        return;
    }

    running_.store(true);
    thread_ = std::thread(&SensorProducer::producerLoop, this);
}

void SensorProducer::stop() {
    running_.store(false);

    if (thread_.joinable()) {
        thread_.join();
    }
}

void SensorProducer::producerLoop() {
    while (running_.load()) {
        double value = generateReading();

        auto result = sensor_->validateReading(value);

        SensorState newState = SensorState::Normal;
        if (result.status == ReadingStatus::Warning) {
            newState = SensorState::Warning;
        } else if (result.status == ReadingStatus::Critical) {
            newState = SensorState::Critical;
        } else if (result.status == ReadingStatus::Invalid) {
            newState = SensorState::Faulted;
        }
        sensor_->setState(newState);

        SensorReading reading(sensor_->id().value(), value, result.status);

        queue_->push(std::move(reading));
        readingsGenerated_.fetch_add(1);

        std::this_thread::sleep_for(config_.sampleInterval);
    }
}

double SensorProducer::generateReading() {
    const auto& thresholds = sensor_->thresholds();
    const auto& validRange = sensor_->validRange();

    double baseValue = (thresholds.warningLow + thresholds.warningHigh) / 2.0;
    double range = thresholds.warningHigh - thresholds.warningLow;

    std::normal_distribution<> noise(0, range * config_.noiseLevel);
    std::normal_distribution<> trend(0, range * config_.trendRate);

    double value = lastValue_ + trend(rng_) + noise(rng_);

    std::uniform_real_distribution<> chance(0, 1);
    if (chance(rng_) < config_.anomalyProbability) {
        std::uniform_real_distribution<> magnitude(1.5, 3.0);
        int sign = chance(rng_) > 0.5 ? 1 : -1;
        value = baseValue + sign * range * magnitude(rng_) * 0.3;
    }

    value += (baseValue - value) * 0.01;
    value = std::clamp(value, validRange.min, validRange.max);

    lastValue_ = value;
    return value;
}

}  