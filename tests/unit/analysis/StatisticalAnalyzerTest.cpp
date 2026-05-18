#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cmath>
#include <random>

#include "analysis/StatisticalAnalyzer.hpp"

namespace sensorcore::analysis::test {

using ::testing::DoubleNear;

class StatisticalAnalyzerTest : public ::testing::Test {
protected:
    StatisticalAnalyzer analyzer_;

    std::vector<SensorReading> createReadings(const std::vector<double>& values) {
        std::vector<SensorReading> readings;
        for (size_t i = 0; i < values.size(); ++i) {
            readings.emplace_back("TEST-001", values[i], ReadingStatus::Normal);
        }
        return readings;
    }

    bool nearEqual(double a, double b, double epsilon = 1e-6) { return std::abs(a - b) < epsilon; }
};

TEST_F(StatisticalAnalyzerTest, CalculatesMeanCorrectly) {
    std::vector<double> values = {1.0, 2.0, 3.0, 4.0, 5.0};

    double mean = StatisticalAnalyzer::calculateMean(values);

    EXPECT_DOUBLE_EQ(mean, 3.0);
}

TEST_F(StatisticalAnalyzerTest, CalculatesMeanOfSingleValue) {
    std::vector<double> values = {42.0};

    double mean = StatisticalAnalyzer::calculateMean(values);

    EXPECT_DOUBLE_EQ(mean, 42.0);
}

TEST_F(StatisticalAnalyzerTest, CalculatesMeanWithNegativeValues) {
    std::vector<double> values = {-5.0, -3.0, 0.0, 3.0, 5.0};

    double mean = StatisticalAnalyzer::calculateMean(values);

    EXPECT_DOUBLE_EQ(mean, 0.0);
}

TEST_F(StatisticalAnalyzerTest, CalculatesVarianceCorrectly) {
    std::vector<double> values = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
    double mean = StatisticalAnalyzer::calculateMean(values);

    double variance = StatisticalAnalyzer::calculateVariance(values, mean);

    EXPECT_NEAR(variance, 4.0, 0.001);
}

TEST_F(StatisticalAnalyzerTest, CalculatesStdDevCorrectly) {
    std::vector<double> values = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
    double mean = StatisticalAnalyzer::calculateMean(values);

    double stdDev = StatisticalAnalyzer::calculateStdDev(values, mean);

    EXPECT_NEAR(stdDev, 2.0, 0.001);
}

TEST_F(StatisticalAnalyzerTest, VarianceOfConstantValuesIsZero) {
    std::vector<double> values = {5.0, 5.0, 5.0, 5.0, 5.0};
    double mean = StatisticalAnalyzer::calculateMean(values);

    double variance = StatisticalAnalyzer::calculateVariance(values, mean);

    EXPECT_DOUBLE_EQ(variance, 0.0);
}

TEST_F(StatisticalAnalyzerTest, CalculatesMedianOddCount) {
    std::vector<double> values = {1.0, 2.0, 3.0, 4.0, 5.0};

    double median = StatisticalAnalyzer::calculatePercentile(values, 50.0);

    EXPECT_DOUBLE_EQ(median, 3.0);
}

TEST_F(StatisticalAnalyzerTest, CalculatesMedianEvenCount) {
    std::vector<double> values = {1.0, 2.0, 3.0, 4.0};

    double median = StatisticalAnalyzer::calculatePercentile(values, 50.0);

    EXPECT_NEAR(median, 2.5, 0.1);
}

TEST_F(StatisticalAnalyzerTest, CalculatesPercentile25) {
    std::vector<double> values = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    double p25 = StatisticalAnalyzer::calculatePercentile(values, 25.0);

    EXPECT_NEAR(p25, 3.0, 0.5);
}

TEST_F(StatisticalAnalyzerTest, CalculatesPercentile95) {
    std::vector<double> values;
    for (int i = 1; i <= 100; ++i) {
        values.push_back(static_cast<double>(i));
    }

    double p95 = StatisticalAnalyzer::calculatePercentile(values, 95.0);

    EXPECT_NEAR(p95, 95.0, 1.0);
}

TEST_F(StatisticalAnalyzerTest, FullAnalysisFromReadings) {
    auto readings = createReadings({10.0, 20.0, 30.0, 40.0, 50.0});

    auto result = analyzer_.analyze(readings);

    EXPECT_EQ(result.count, 5u);
    EXPECT_DOUBLE_EQ(result.mean, 30.0);
    EXPECT_DOUBLE_EQ(result.min, 10.0);
    EXPECT_DOUBLE_EQ(result.max, 50.0);
    EXPECT_DOUBLE_EQ(result.range, 40.0);
}

TEST_F(StatisticalAnalyzerTest, FullAnalysisFromValues) {
    std::vector<double> values = {100.0, 200.0, 300.0, 400.0, 500.0};

    auto result = analyzer_.analyze(values);

    EXPECT_EQ(result.count, 5u);
    EXPECT_DOUBLE_EQ(result.mean, 300.0);
    EXPECT_DOUBLE_EQ(result.sum, 1500.0);
}

TEST_F(StatisticalAnalyzerTest, AnalysisThrowsOnEmptyInput) {
    std::vector<double> empty;

    EXPECT_THROW({ analyzer_.analyze(empty); }, std::invalid_argument);
}

TEST_F(StatisticalAnalyzerTest, AnalysisHandlesSingleValue) {
    std::vector<double> values = {42.0};

    auto result = analyzer_.analyze(values);

    EXPECT_EQ(result.count, 1u);
    EXPECT_DOUBLE_EQ(result.mean, 42.0);
    EXPECT_DOUBLE_EQ(result.min, 42.0);
    EXPECT_DOUBLE_EQ(result.max, 42.0);
    EXPECT_DOUBLE_EQ(result.variance, 0.0);
}

TEST_F(StatisticalAnalyzerTest, CalculatesIQRCorrectly) {
    std::vector<double> values = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};

    auto result = analyzer_.analyze(values);

    EXPECT_NEAR(result.iqr, 6.0, 1.0);
}

TEST_F(StatisticalAnalyzerTest, CalculatesCoefficientOfVariation) {
    std::vector<double> values = {10.0, 20.0, 30.0, 40.0, 50.0};

    auto result = analyzer_.analyze(values);

    double expectedCV = result.standardDeviation / result.mean;
    EXPECT_DOUBLE_EQ(result.coefficientOfVariation, expectedCV);
}

TEST_F(StatisticalAnalyzerTest, SymmetricDistributionHasZeroSkewness) {
    std::vector<double> values = {10, 20, 30, 40, 50, 60, 70, 80, 90};

    auto result = analyzer_.analyze(values);

    EXPECT_NEAR(result.skewness, 0.0, 0.1);
}

TEST_F(StatisticalAnalyzerTest, RightSkewedDistributionHasPositiveSkewness) {
    std::vector<double> values = {1, 1, 1, 2, 2, 3, 5, 10, 20};

    auto result = analyzer_.analyze(values);

    EXPECT_GT(result.skewness, 0.0);
}

TEST_F(StatisticalAnalyzerTest, OnlineStatisticsMatchesBatchCalculation) {
    std::vector<double> values = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};

    double batchMean = StatisticalAnalyzer::calculateMean(values);
    double batchVar = StatisticalAnalyzer::calculateVariance(values, batchMean);

    OnlineStatistics online;
    for (double v : values) {
        online.update(v);
    }

    EXPECT_NEAR(online.mean(), batchMean, 0.0001);
    EXPECT_NEAR(online.variance(), batchVar, 0.0001);
}

TEST_F(StatisticalAnalyzerTest, OnlineStatisticsTracksMinMax) {
    OnlineStatistics online;

    online.update(5.0);
    online.update(2.0);
    online.update(8.0);
    online.update(1.0);
    online.update(9.0);

    EXPECT_DOUBLE_EQ(online.min(), 1.0);
    EXPECT_DOUBLE_EQ(online.max(), 9.0);
}

TEST_F(StatisticalAnalyzerTest, OnlineStatisticsResetWorks) {
    OnlineStatistics online;

    online.update(10.0);
    online.update(20.0);
    online.reset();
    online.update(5.0);

    EXPECT_EQ(online.count(), 1u);
    EXPECT_DOUBLE_EQ(online.mean(), 5.0);
}

TEST_F(StatisticalAnalyzerTest, MovingAverageCalculation) {
    std::vector<double> values = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    auto moving = analyzer_.computeMovingStats(values, 3);

    EXPECT_NEAR(moving.movingAverage, 9.0, 0.001);
}

TEST_F(StatisticalAnalyzerTest, HandlesLargeDataset) {
    std::vector<double> values;
    std::mt19937 gen(42);
    std::normal_distribution<> dist(100.0, 15.0);

    for (int i = 0; i < 10000; ++i) {
        values.push_back(dist(gen));
    }

    auto result = analyzer_.analyze(values);

    EXPECT_EQ(result.count, 10000u);
    EXPECT_NEAR(result.mean, 100.0, 1.0);
    EXPECT_NEAR(result.standardDeviation, 15.0, 1.0);
}

TEST_F(StatisticalAnalyzerTest, HandlesNegativeValues) {
    std::vector<double> values = {-100, -50, 0, 50, 100};

    auto result = analyzer_.analyze(values);

    EXPECT_DOUBLE_EQ(result.mean, 0.0);
    EXPECT_DOUBLE_EQ(result.min, -100.0);
    EXPECT_DOUBLE_EQ(result.max, 100.0);
}

TEST_F(StatisticalAnalyzerTest, HandlesVerySmallValues) {
    std::vector<double> values = {1e-10, 2e-10, 3e-10, 4e-10, 5e-10};

    auto result = analyzer_.analyze(values);

    EXPECT_NEAR(result.mean, 3e-10, 1e-12);
}

TEST_F(StatisticalAnalyzerTest, HandlesVeryLargeValues) {
    std::vector<double> values = {1e10, 2e10, 3e10, 4e10, 5e10};

    auto result = analyzer_.analyze(values);

    EXPECT_NEAR(result.mean, 3e10, 1e8);
}

}  