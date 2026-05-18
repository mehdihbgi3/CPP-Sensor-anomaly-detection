#include <gtest/gtest.h>

#include <filesystem>

#include "persistence/DatabaseManager.hpp"

using namespace sensorcore;
using namespace sensorcore::persistence;

class DatabaseManagerTest : public ::testing::Test {
protected:
    std::string testDbPath = "test_sensor_data.db";

    void SetUp() override { std::filesystem::remove(testDbPath); }

    void TearDown() override { std::filesystem::remove(testDbPath); }

    SensorReading createReading(const std::string& id, double value) {
        return SensorReading(id, value, ReadingStatus::Normal);
    }
};

TEST_F(DatabaseManagerTest, OpensAndInitializes) {
    DatabaseManager db(testDbPath);
    EXPECT_TRUE(db.isOpen());
    EXPECT_TRUE(db.initialize());
}

TEST_F(DatabaseManagerTest, StoreAndRetrieveReading) {
    DatabaseManager db(testDbPath);
    ASSERT_TRUE(db.initialize());

    auto reading = createReading("TEMP-001", 25.5);
    EXPECT_TRUE(db.storeReading(reading));

    auto readings = db.getReadings("TEMP-001");
    ASSERT_EQ(readings.size(), 1);
    EXPECT_EQ(readings[0].sensorId, "TEMP-001");
    EXPECT_DOUBLE_EQ(readings[0].value, 25.5);
}

TEST_F(DatabaseManagerTest, StoreBatchReadings) {
    DatabaseManager db(testDbPath);
    ASSERT_TRUE(db.initialize());

    std::vector<SensorReading> readings;
    for (int i = 0; i < 100; ++i) {
        readings.push_back(createReading("TEMP-001", 20.0 + i * 0.1));
    }

    EXPECT_TRUE(db.storeReadings(readings));
    EXPECT_EQ(db.getReadingCount("TEMP-001"), 100);
}

TEST_F(DatabaseManagerTest, GetSensorStats) {
    DatabaseManager db(testDbPath);
    ASSERT_TRUE(db.initialize());

    db.storeReading(createReading("TEMP-001", 10.0));
    db.storeReading(createReading("TEMP-001", 20.0));
    db.storeReading(createReading("TEMP-001", 30.0));

    auto stats = db.getSensorStats("TEMP-001");
    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(stats->totalReadings, 3);
    EXPECT_DOUBLE_EQ(stats->minValue, 10.0);
    EXPECT_DOUBLE_EQ(stats->maxValue, 30.0);
    EXPECT_DOUBLE_EQ(stats->avgValue, 20.0);
}

TEST_F(DatabaseManagerTest, GetAllSensorIds) {
    DatabaseManager db(testDbPath);
    ASSERT_TRUE(db.initialize());

    db.storeReading(createReading("TEMP-001", 25.0));
    db.storeReading(createReading("PRES-001", 100.0));
    db.storeReading(createReading("FLOW-001", 50.0));

    auto ids = db.getAllSensorIds();
    EXPECT_EQ(ids.size(), 3);
}

TEST_F(DatabaseManagerTest, ReadingCount) {
    DatabaseManager db(testDbPath);
    ASSERT_TRUE(db.initialize());

    db.storeReading(createReading("TEMP-001", 25.0));
    db.storeReading(createReading("TEMP-001", 26.0));
    db.storeReading(createReading("PRES-001", 100.0));

    EXPECT_EQ(db.getReadingCount(), 3);
    EXPECT_EQ(db.getReadingCount("TEMP-001"), 2);
    EXPECT_EQ(db.getReadingCount("PRES-001"), 1);
}

TEST_F(DatabaseManagerTest, MultipleSensors) {
    DatabaseManager db(testDbPath);
    ASSERT_TRUE(db.initialize());

    db.storeReading(createReading("TEMP-001", 25.0));
    db.storeReading(createReading("PRES-001", 100.0));

    auto tempReadings = db.getReadings("TEMP-001");
    auto presReadings = db.getReadings("PRES-001");

    EXPECT_EQ(tempReadings.size(), 1);
    EXPECT_EQ(presReadings.size(), 1);
    EXPECT_DOUBLE_EQ(tempReadings[0].value, 25.0);
    EXPECT_DOUBLE_EQ(presReadings[0].value, 100.0);
}