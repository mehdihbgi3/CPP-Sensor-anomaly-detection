#pragma once

#include <algorithm>
#include <cmath>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <vector>

#include "sensorcore/SensorReading.hpp"

namespace sensorcore::analysis {

struct StatisticalResult {
    size_t count = 0;
    double sum = 0.0;
    double mean = 0.0;
    double variance = 0.0;
    double standardDeviation = 0.0;

    double min = 0.0;
    double max = 0.0;
    double range = 0.0;

    double median = 0.0;
    double percentile25 = 0.0;
    double percentile75 = 0.0;
    double percentile95 = 0.0;
    double percentile99 = 0.0;
    double iqr = 0.0;

    double skewness = 0.0;
    double kurtosis = 0.0;
    double coefficientOfVariation = 0.0;

    std::optional<double> rateOfChange;
};

struct MovingStatistics {
    double movingAverage = 0.0;
    double exponentialMovingAverage = 0.0;
    double movingStdDev = 0.0;
};

class StatisticalAnalyzer {
public:
    struct Config {
        size_t movingWindowSize = 100;
        double emaAlpha = 0.1;
    };

    explicit StatisticalAnalyzer(Config config = {});

    StatisticalResult analyze(const std::vector<SensorReading>& readings) const;

    StatisticalResult analyze(const std::vector<double>& values) const;

    MovingStatistics computeMovingStats(const std::vector<double>& values, size_t windowSize) const;

    static double calculateMean(const std::vector<double>& values);
    static double calculateVariance(const std::vector<double>& values, double mean);
    static double calculateStdDev(const std::vector<double>& values, double mean);
    static double calculatePercentile(std::vector<double> values, double percentile);
    static double calculateSkewness(const std::vector<double>& values, double mean, double stdDev);
    static double calculateKurtosis(const std::vector<double>& values, double mean, double stdDev);

    static std::vector<double> extractValues(const std::vector<SensorReading>& readings);

    void setConfig(const Config& config) { config_ = config; }
    const Config& config() const { return config_; }

private:
    Config config_;
};

class OnlineStatistics {
public:
    void update(double value);
    void reset();

    size_t count() const { return count_; }
    double mean() const { return mean_; }
    double variance() const;
    double standardDeviation() const;
    double min() const { return min_; }
    double max() const { return max_; }

private:
    size_t count_ = 0;
    double mean_ = 0.0;
    double m2_ = 0.0;
    double min_ = std::numeric_limits<double>::max();
    double max_ = std::numeric_limits<double>::lowest();
};

}  