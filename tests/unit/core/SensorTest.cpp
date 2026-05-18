#include <gtest/gtest.h>

#include "sensorcore/Sensor.hpp"

namespace sensorcore::test {

class SensorTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.id = SensorId{"TEMP-001"};
        config_.name = "Temperature Sensor 1";
        config_.type = SensorType::Temperature;
        config_.unit = Unit::Celsius;
        config_.validRange = {-40.0, 150.0};
        config_.thresholds = {
            .warningLow = 0.0, .warningHigh = 80.0, .criticalLow = -20.0, .criticalHigh = 100.0};
        config_.description = "Main reactor temperature";
        config_.location = "Building A, Room 101";
    }

    SensorConfig config_;
};

TEST_F(SensorTest, ConstructsWithValidConfig) {
    EXPECT_NO_THROW({ Sensor sensor(config_); });
}

TEST_F(SensorTest, ThrowsOnEmptyId) {
    config_.id = SensorId{""};

    EXPECT_THROW({ Sensor sensor(config_); }, std::invalid_argument);
}

TEST_F(SensorTest, ThrowsOnEmptyName) {
    config_.name = "";

    EXPECT_THROW({ Sensor sensor(config_); }, std::invalid_argument);
}

TEST_F(SensorTest, ThrowsOnInvalidRange) {
    config_.validRange = {100.0, 0.0};

    EXPECT_THROW({ Sensor sensor(config_); }, std::invalid_argument);
}

TEST_F(SensorTest, ThrowsOnCriticalThresholdsOutsideRange) {
    config_.thresholds.criticalHigh = 200.0;

    EXPECT_THROW({ Sensor sensor(config_); }, std::invalid_argument);
}

TEST_F(SensorTest, ThrowsOnWarningOutsideCritical) {
    config_.thresholds.warningHigh = 110.0;

    EXPECT_THROW({ Sensor sensor(config_); }, std::invalid_argument);
}

TEST_F(SensorTest, ReturnsCorrectId) {
    Sensor sensor(config_);
    EXPECT_EQ(sensor.id().value(), "TEMP-001");
}

TEST_F(SensorTest, ReturnsCorrectName) {
    Sensor sensor(config_);
    EXPECT_EQ(sensor.name(), "Temperature Sensor 1");
}

TEST_F(SensorTest, ReturnsCorrectType) {
    Sensor sensor(config_);
    EXPECT_EQ(sensor.type(), SensorType::Temperature);
}

TEST_F(SensorTest, ReturnsCorrectUnit) {
    Sensor sensor(config_);
    EXPECT_EQ(sensor.unit(), Unit::Celsius);
}

TEST_F(SensorTest, InitialStateIsUnknown) {
    Sensor sensor(config_);
    EXPECT_EQ(sensor.state(), SensorState::Unknown);
}

TEST_F(SensorTest, ValidatesNormalReading) {
    Sensor sensor(config_);

    auto result = sensor.validateReading(50.0);

    EXPECT_EQ(result.status, ReadingStatus::Normal);
    EXPECT_TRUE(result.message.empty());
}

TEST_F(SensorTest, ValidatesWarningHighReading) {
    Sensor sensor(config_);

    auto result = sensor.validateReading(85.0);

    EXPECT_EQ(result.status, ReadingStatus::Warning);
}

TEST_F(SensorTest, ValidatesWarningLowReading) {
    Sensor sensor(config_);

    auto result = sensor.validateReading(-5.0);

    EXPECT_EQ(result.status, ReadingStatus::Warning);
}

TEST_F(SensorTest, ValidatesCriticalHighReading) {
    Sensor sensor(config_);

    auto result = sensor.validateReading(105.0);

    EXPECT_EQ(result.status, ReadingStatus::Critical);
}

TEST_F(SensorTest, ValidatesCriticalLowReading) {
    Sensor sensor(config_);

    auto result = sensor.validateReading(-25.0);

    EXPECT_EQ(result.status, ReadingStatus::Critical);
}

TEST_F(SensorTest, ValidatesOutOfRangeReading) {
    Sensor sensor(config_);

    auto result = sensor.validateReading(200.0);

    EXPECT_EQ(result.status, ReadingStatus::Invalid);
}

TEST_F(SensorTest, ValidatesBoundaryAtWarningHigh) {
    Sensor sensor(config_);

    auto result = sensor.validateReading(80.0);

    EXPECT_EQ(result.status, ReadingStatus::Warning);
}

TEST_F(SensorTest, ValidatesJustBelowWarningHigh) {
    Sensor sensor(config_);

    auto result = sensor.validateReading(79.9);

    EXPECT_EQ(result.status, ReadingStatus::Normal);
}

TEST_F(SensorTest, UpdatesName) {
    Sensor sensor(config_);

    sensor.setName("New Name");

    EXPECT_EQ(sensor.name(), "New Name");
}

TEST_F(SensorTest, ThrowsOnSetEmptyName) {
    Sensor sensor(config_);

    EXPECT_THROW({ sensor.setName(""); }, std::invalid_argument);
}

TEST_F(SensorTest, UpdatesState) {
    Sensor sensor(config_);

    sensor.setState(SensorState::Normal);

    EXPECT_EQ(sensor.state(), SensorState::Normal);
}

TEST_F(SensorTest, UpdatesValidThresholds) {
    Sensor sensor(config_);

    ThresholdSet newThresholds{
        .warningLow = 10.0, .warningHigh = 70.0, .criticalLow = -10.0, .criticalHigh = 90.0};

    EXPECT_NO_THROW({ sensor.setThresholds(newThresholds); });

    EXPECT_EQ(sensor.thresholds().warningHigh, 70.0);
}

}  