#include <gtest/gtest.h>

#include <random>

#include "analysis/AnomalyDetector.hpp"

namespace sensorcore::analysis::test {

class AnomalyDetectorTest : public ::testing::Test {
protected:
    AnomalyDetector detector_;

    std::vector<double> createNormalData(size_t count, double mean, double stdDev) {
        std::vector<double> data;
        std::mt19937 gen(42);
        std::normal_distribution<> dist(mean, stdDev);

        for (size_t i = 0; i < count; ++i) {
            data.push_back(dist(gen));
        }
        return data;
    }
};

TEST_F(AnomalyDetectorTest, ZScoreDetectsObviousOutlier) {
    std::vector<double> values = {10, 11, 10, 12, 11, 10, 100, 11, 10, 12};

    AnomalyConfig config;
    config.method = AnomalyMethod::ZScore;
    config.zScoreThreshold = 2.0;
    detector_.setConfig(config);

    auto result = detector_.detect(values);

    EXPECT_TRUE(result.hasAnomalies());
    EXPECT_EQ(result.anomalyCount, 1u);
    EXPECT_EQ(result.anomalies[0].index, 6u);
    EXPECT_DOUBLE_EQ(result.anomalies[0].value, 100.0);
}

TEST_F(AnomalyDetectorTest, ZScoreNoAnomaliesInNormalData) {
    auto values = createNormalData(100, 50.0, 5.0);

    AnomalyConfig config;
    config.method = AnomalyMethod::ZScore;
    config.zScoreThreshold = 4.0;
    detector_.setConfig(config);

    auto result = detector_.detect(values);

    EXPECT_LT(result.anomalyPercentage, 1.0);
}

TEST_F(AnomalyDetectorTest, ZScoreDetectsMultipleOutliers) {
    std::vector<double> values = {10, 11, 200, 12, 11, -100, 10, 11, 12, 10};

    AnomalyConfig config;
    config.method = AnomalyMethod::ZScore;
    config.zScoreThreshold = 2.0;
    detector_.setConfig(config);

    auto result = detector_.detect(values);

    EXPECT_TRUE(result.hasAnomalies());
    EXPECT_GE(result.anomalyCount, 1u);
}

TEST_F(AnomalyDetectorTest, IQRDetectsOutliers) {
    std::vector<double> values = {1, 2, 3, 4, 5, 6, 7, 8, 9, 100};

    AnomalyConfig config;
    config.method = AnomalyMethod::IQR;
    config.iqrMultiplier = 1.5;
    detector_.setConfig(config);

    auto result = detector_.detect(values);

    EXPECT_TRUE(result.hasAnomalies());
    bool found100 = false;
    for (const auto& a : result.anomalies) {
        if (a.value == 100.0)
            found100 = true;
    }
    EXPECT_TRUE(found100);
}

TEST_F(AnomalyDetectorTest, IQRCalculatesCorrectBounds) {
    std::vector<double> values;
    for (int i = 1; i <= 100; ++i) {
        values.push_back(static_cast<double>(i));
    }

    AnomalyConfig config;
    config.method = AnomalyMethod::IQR;
    config.iqrMultiplier = 1.5;
    detector_.setConfig(config);

    auto result = detector_.detect(values);

    EXPECT_LT(result.lowerBound, 0);
    EXPECT_GT(result.upperBound, 100);
}

TEST_F(AnomalyDetectorTest, ModifiedZScoreIsRobust) {
    std::vector<double> values = {1, 2, 2, 3, 3, 3, 4, 4, 100, 200, 300};

    AnomalyConfig config;
    config.method = AnomalyMethod::ModifiedZScore;
    config.modifiedZThreshold = 3.5;
    detector_.setConfig(config);

    auto result = detector_.detect(values);

    EXPECT_TRUE(result.hasAnomalies());
    EXPECT_GE(result.anomalyCount, 2u);
}

TEST_F(AnomalyDetectorTest, MADCalculation) {
    std::vector<double> values = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    double median = 5.0;

    double mad = AnomalyDetector::calculateMAD(values, median);

    EXPECT_NEAR(mad, 2.0, 0.1);
}

TEST_F(AnomalyDetectorTest, ThresholdDetectsAboveMax) {
    std::vector<double> values = {10, 20, 30, 150, 25, 15};

    AnomalyConfig config;
    config.method = AnomalyMethod::Threshold;
    config.absoluteMax = 100.0;
    detector_.setConfig(config);

    auto result = detector_.detect(values);

    EXPECT_EQ(result.anomalyCount, 1u);
    EXPECT_DOUBLE_EQ(result.anomalies[0].value, 150.0);
}

TEST_F(AnomalyDetectorTest, ThresholdDetectsBelowMin) {
    std::vector<double> values = {10, 20, -50, 30, 25, 15};

    AnomalyConfig config;
    config.method = AnomalyMethod::Threshold;
    config.absoluteMin = 0.0;
    detector_.setConfig(config);

    auto result = detector_.detect(values);

    EXPECT_EQ(result.anomalyCount, 1u);
    EXPECT_DOUBLE_EQ(result.anomalies[0].value, -50.0);
}

TEST_F(AnomalyDetectorTest, ThresholdDetectsBothBounds) {
    std::vector<double> values = {50, -100, 60, 200, 55};

    AnomalyConfig config;
    config.method = AnomalyMethod::Threshold;
    config.absoluteMin = 0.0;
    config.absoluteMax = 100.0;
    detector_.setConfig(config);

    auto result = detector_.detect(values);

    EXPECT_EQ(result.anomalyCount, 2u);
}

TEST_F(AnomalyDetectorTest, RateOfChangeDetectsSuddenJump) {
    std::vector<double> values = {10, 11, 12, 100, 101, 102};

    AnomalyConfig config;
    config.method = AnomalyMethod::RateOfChange;
    config.maxRateOfChange = 20.0;
    detector_.setConfig(config);

    auto result = detector_.detect(values);

    EXPECT_TRUE(result.hasAnomalies());
    EXPECT_EQ(result.anomalies[0].index, 3u);
}

TEST_F(AnomalyDetectorTest, RateOfChangeAllowsGradualChange) {
    std::vector<double> values = {10, 15, 20, 25, 30, 35, 40};

    AnomalyConfig config;
    config.method = AnomalyMethod::RateOfChange;
    config.maxRateOfChange = 10.0;
    detector_.setConfig(config);

    auto result = detector_.detect(values);

    EXPECT_FALSE(result.hasAnomalies());
}

TEST_F(AnomalyDetectorTest, SinglePointDetection) {
    std::vector<double> historical = {10, 11, 10, 12, 11, 10, 11, 12, 10, 11};

    auto normalResult = detector_.detectSinglePoint(11.0, historical);
    EXPECT_FALSE(normalResult.has_value());

    auto anomalyResult = detector_.detectSinglePoint(100.0, historical);
    EXPECT_TRUE(anomalyResult.has_value());
    EXPECT_DOUBLE_EQ(anomalyResult->value, 100.0);
}

TEST_F(AnomalyDetectorTest, EmptyDataReturnsNoAnomalies) {
    std::vector<double> empty;

    auto result = detector_.detect(empty);

    EXPECT_FALSE(result.hasAnomalies());
    EXPECT_EQ(result.totalPoints, 0u);
}

TEST_F(AnomalyDetectorTest, SingleValueNoAnomalies) {
    std::vector<double> single = {42.0};

    auto result = detector_.detect(single);

    EXPECT_FALSE(result.hasAnomalies());
}

TEST_F(AnomalyDetectorTest, ConstantValuesNoAnomalies) {
    std::vector<double> constant(100, 50.0);

    auto result = detector_.detect(constant);

    EXPECT_FALSE(result.hasAnomalies());
}

TEST_F(AnomalyDetectorTest, AnomalySeverityClassification) {
    EXPECT_EQ(classifyAnomaly(2.5), AnomalySeverity::Low);
    EXPECT_EQ(classifyAnomaly(3.5), AnomalySeverity::Medium);
    EXPECT_EQ(classifyAnomaly(4.5), AnomalySeverity::High);
    EXPECT_EQ(classifyAnomaly(5.5), AnomalySeverity::Critical);
}

TEST_F(AnomalyDetectorTest, AnomaliesSortedByScore) {
    std::vector<double> values = {10, 50, 11, 100, 12, 200};

    auto result = detector_.detect(values);

    for (size_t i = 1; i < result.anomalies.size(); ++i) {
        EXPECT_GE(result.anomalies[i - 1].score, result.anomalies[i].score);
    }
}

}  