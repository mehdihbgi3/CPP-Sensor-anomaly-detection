#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

#include "sensorcore/DataManager.hpp"
#include "sensorcore/SensorProducer.hpp"
#include "sensorcore/ThreadSafeQueue.hpp"

using namespace sensorcore;

class ThreadSafeQueueTest : public ::testing::Test {
protected:
    ThreadSafeQueue<int> queue;
};

TEST_F(ThreadSafeQueueTest, PushAndTryPop) {
    queue.push(42);
    auto result = queue.tryPop();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42);
}

TEST_F(ThreadSafeQueueTest, TryPopEmptyReturnsNullopt) {
    auto result = queue.tryPop();
    EXPECT_FALSE(result.has_value());
}

TEST_F(ThreadSafeQueueTest, SizeTracking) {
    EXPECT_EQ(queue.size(), 0);
    EXPECT_TRUE(queue.empty());

    queue.push(1);
    queue.push(2);
    queue.push(3);

    EXPECT_EQ(queue.size(), 3);
    EXPECT_FALSE(queue.empty());
}

TEST_F(ThreadSafeQueueTest, FIFOOrder) {
    queue.push(1);
    queue.push(2);
    queue.push(3);

    EXPECT_EQ(*queue.tryPop(), 1);
    EXPECT_EQ(*queue.tryPop(), 2);
    EXPECT_EQ(*queue.tryPop(), 3);
}

TEST_F(ThreadSafeQueueTest, ConcurrentPushPop) {
    constexpr int numItems = 1000;
    std::atomic<int> pushCount{0};
    std::atomic<int> popCount{0};

    std::vector<std::thread> producers;
    for (int i = 0; i < 4; ++i) {
        producers.emplace_back([this, &pushCount]() {
            for (int j = 0; j < 250; ++j) {
                queue.push(j);
                pushCount++;
            }
        });
    }

    std::thread consumer([this, &popCount, numItems]() {
        while (popCount < numItems) {
            if (queue.tryPop()) {
                popCount++;
            }
            std::this_thread::yield();
        }
    });

    for (auto& t : producers) {
        t.join();
    }
    consumer.join();

    EXPECT_EQ(pushCount.load(), numItems);
    EXPECT_EQ(popCount.load(), numItems);
}

class SensorProducerTest : public ::testing::Test {
protected:
    void SetUp() override {
        SensorConfig config;
        config.id = SensorId{"TEST-001"};
        config.name = "Test Sensor";
        config.type = SensorType::Temperature;
        config.unit = Unit::Celsius;
        config.validRange = {0.0, 100.0};
        config.thresholds = {20.0, 80.0, 10.0, 90.0};

        sensor = std::make_shared<Sensor>(config);
        queue = std::make_shared<ThreadSafeQueue<SensorReading>>();
    }

    std::shared_ptr<Sensor> sensor;
    std::shared_ptr<ThreadSafeQueue<SensorReading>> queue;
};

TEST_F(SensorProducerTest, StartsAndStops) {
    SensorProducer::Config config;
    config.sampleInterval = std::chrono::milliseconds(10);

    SensorProducer producer(sensor, queue, config);

    EXPECT_FALSE(producer.isRunning());

    producer.start();
    EXPECT_TRUE(producer.isRunning());

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    producer.stop();
    EXPECT_FALSE(producer.isRunning());
}

TEST_F(SensorProducerTest, GeneratesReadings) {
    SensorProducer::Config config;
    config.sampleInterval = std::chrono::milliseconds(10);

    SensorProducer producer(sensor, queue, config);
    producer.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    producer.stop();

    EXPECT_GT(producer.readingsGenerated(), 0);
    EXPECT_FALSE(queue->empty());
}

TEST_F(SensorProducerTest, ReadingsHaveCorrectSensorId) {
    SensorProducer::Config config;
    config.sampleInterval = std::chrono::milliseconds(10);

    SensorProducer producer(sensor, queue, config);
    producer.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    producer.stop();

    auto reading = queue->tryPop();
    ASSERT_TRUE(reading.has_value());
    EXPECT_EQ(reading->sensorId, "TEST-001");
}

class DataManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        SensorConfig config;
        config.id = SensorId{"TEMP-001"};
        config.name = "Temperature";
        config.type = SensorType::Temperature;
        config.unit = Unit::Celsius;
        config.validRange = {0.0, 100.0};
        config.thresholds = {20.0, 80.0, 10.0, 90.0};

        sensor1 = std::make_shared<Sensor>(config);

        config.id = SensorId{"PRES-001"};
        config.name = "Pressure";
        config.type = SensorType::Pressure;
        config.unit = Unit::Pascal;

        sensor2 = std::make_shared<Sensor>(config);
    }

    std::shared_ptr<Sensor> sensor1;
    std::shared_ptr<Sensor> sensor2;
};

TEST_F(DataManagerTest, AddSensorsAndStart) {
    DataManager manager;

    SensorProducer::Config config;
    config.sampleInterval = std::chrono::milliseconds(10);

    manager.addSensor(sensor1, config);
    manager.addSensor(sensor2, config);

    EXPECT_EQ(manager.sensors().size(), 2);

    manager.startAll();
    EXPECT_TRUE(manager.isRunning());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    manager.stopAll();
    EXPECT_FALSE(manager.isRunning());
}

TEST_F(DataManagerTest, ProcessReadings) {
    DataManager manager;

    SensorProducer::Config config;
    config.sampleInterval = std::chrono::milliseconds(10);

    manager.addSensor(sensor1, config);
    manager.startAll();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::vector<SensorReading> readings;
    manager.processReadings([&readings](const SensorReading& r) { readings.push_back(r); });

    manager.stopAll();

    EXPECT_GT(readings.size(), 0);
}

TEST_F(DataManagerTest, GetProducer) {
    DataManager manager;
    manager.addSensor(sensor1);

    auto* producer = manager.getProducer("TEMP-001");
    EXPECT_NE(producer, nullptr);

    auto* nonExistent = manager.getProducer("FAKE-001");
    EXPECT_EQ(nonExistent, nullptr);
}