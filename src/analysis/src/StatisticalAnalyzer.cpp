#include "analysis/StatisticalAnalyzer.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace sensorcore::analysis {

StatisticalAnalyzer::StatisticalAnalyzer(Config config) : config_(std::move(config)) {}

StatisticalResult StatisticalAnalyzer::analyze(const std::vector<SensorReading>& readings) const {
    return analyze(extractValues(readings));
}

StatisticalResult StatisticalAnalyzer::analyze(const std::vector<double>& values) const {
    if (values.empty()) {
        throw std::invalid_argument("Cannot analyze empty dataset");
    }

    StatisticalResult result;
    result.count = values.size();

    result.sum = std::accumulate(values.begin(), values.end(), 0.0);
    result.mean = calculateMean(values);
    result.variance = calculateVariance(values, result.mean);
    result.standardDeviation = std::sqrt(result.variance);

    auto [minIt, maxIt] = std::minmax_element(values.begin(), values.end());
    result.min = *minIt;
    result.max = *maxIt;
    result.range = result.max - result.min;

    result.percentile25 = calculatePercentile(values, 25.0);
    result.median = calculatePercentile(values, 50.0);
    result.percentile75 = calculatePercentile(values, 75.0);
    result.percentile95 = calculatePercentile(values, 95.0);
    result.percentile99 = calculatePercentile(values, 99.0);
    result.iqr = result.percentile75 - result.percentile25;

    if (result.standardDeviation > 0) {
        result.skewness = calculateSkewness(values, result.mean, result.standardDeviation);
        result.kurtosis = calculateKurtosis(values, result.mean, result.standardDeviation);
        result.coefficientOfVariation = result.standardDeviation / std::abs(result.mean);
    }

    return result;
}

MovingStatistics StatisticalAnalyzer::computeMovingStats(const std::vector<double>& values,
                                                         size_t windowSize) const {
    MovingStatistics result;

    if (values.empty() || windowSize == 0) {
        return result;
    }

    size_t actualWindow = std::min(windowSize, values.size());
    size_t startIdx = values.size() - actualWindow;

    double sum = 0.0;
    for (size_t i = startIdx; i < values.size(); ++i) {
        sum += values[i];
    }
    result.movingAverage = sum / actualWindow;

    result.exponentialMovingAverage = values[0];
    for (size_t i = 1; i < values.size(); ++i) {
        result.exponentialMovingAverage =
            config_.emaAlpha * values[i] +
            (1.0 - config_.emaAlpha) * result.exponentialMovingAverage;
    }

    double sqSum = 0.0;
    for (size_t i = startIdx; i < values.size(); ++i) {
        double diff = values[i] - result.movingAverage;
        sqSum += diff * diff;
    }
    result.movingStdDev = std::sqrt(sqSum / actualWindow);

    return result;
}

double StatisticalAnalyzer::calculateMean(const std::vector<double>& values) {
    if (values.empty())
        return 0.0;
    return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
}

double StatisticalAnalyzer::calculateVariance(const std::vector<double>& values, double mean) {
    if (values.size() <= 1)
        return 0.0;

    double sqSum =
        std::accumulate(values.begin(), values.end(), 0.0, [mean](double acc, double val) {
            double diff = val - mean;
            return acc + diff * diff;
        });

    return sqSum / values.size();
}

double StatisticalAnalyzer::calculateStdDev(const std::vector<double>& values, double mean) {
    return std::sqrt(calculateVariance(values, mean));
}

double StatisticalAnalyzer::calculatePercentile(std::vector<double> values, double percentile) {
    if (values.empty())
        return 0.0;
    if (values.size() == 1)
        return values[0];

    std::sort(values.begin(), values.end());

    double index = (percentile / 100.0) * (values.size() - 1);
    size_t lower = static_cast<size_t>(std::floor(index));
    size_t upper = static_cast<size_t>(std::ceil(index));

    if (lower == upper || upper >= values.size()) {
        return values[lower];
    }

    double fraction = index - lower;
    return values[lower] * (1.0 - fraction) + values[upper] * fraction;
}

double StatisticalAnalyzer::calculateSkewness(const std::vector<double>& values, double mean,
                                              double stdDev) {
    if (values.size() < 3 || stdDev == 0)
        return 0.0;

    double n = static_cast<double>(values.size());
    double sum = 0.0;

    for (double val : values) {
        double normalized = (val - mean) / stdDev;
        sum += normalized * normalized * normalized;
    }

    return (n / ((n - 1) * (n - 2))) * sum;
}

double StatisticalAnalyzer::calculateKurtosis(const std::vector<double>& values, double mean,
                                              double stdDev) {
    if (values.size() < 4 || stdDev == 0)
        return 0.0;

    double n = static_cast<double>(values.size());
    double sum = 0.0;

    for (double val : values) {
        double normalized = (val - mean) / stdDev;
        sum += normalized * normalized * normalized * normalized;
    }

    double kurtosis = ((n * (n + 1)) / ((n - 1) * (n - 2) * (n - 3))) * sum;
    double correction = (3.0 * (n - 1) * (n - 1)) / ((n - 2) * (n - 3));

    return kurtosis - correction;
}

std::vector<double> StatisticalAnalyzer::extractValues(const std::vector<SensorReading>& readings) {
    std::vector<double> values;
    values.reserve(readings.size());

    for (const auto& reading : readings) {
        values.push_back(reading.value);
    }

    return values;
}

void OnlineStatistics::update(double value) {
    ++count_;

    min_ = std::min(min_, value);
    max_ = std::max(max_, value);

    double delta = value - mean_;
    mean_ += delta / count_;
    double delta2 = value - mean_;
    m2_ += delta * delta2;
}

void OnlineStatistics::reset() {
    count_ = 0;
    mean_ = 0.0;
    m2_ = 0.0;
    min_ = std::numeric_limits<double>::max();
    max_ = std::numeric_limits<double>::lowest();
}

double OnlineStatistics::variance() const {
    if (count_ < 2)
        return 0.0;
    return m2_ / count_;
}

double OnlineStatistics::standardDeviation() const {
    return std::sqrt(variance());
}

}  