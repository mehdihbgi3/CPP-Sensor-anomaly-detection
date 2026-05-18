#include "analysis/AnomalyDetector.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <map>
#include <numeric>
#include <sstream>

namespace sensorcore::analysis {

AnomalyDetector::AnomalyDetector(AnomalyConfig config) : config_(std::move(config)) {}

AnomalyDetectionResult AnomalyDetector::detect(const std::vector<SensorReading>& readings) const {
    std::vector<double> values;
    values.reserve(readings.size());
    for (const auto& r : readings) {
        values.push_back(r.value);
    }

    auto result = detect(values);
    addTimestamps(result.anomalies, readings);
    return result;
}

AnomalyDetectionResult AnomalyDetector::detect(const std::vector<double>& values) const {
    AnomalyDetectionResult result;
    result.totalPoints = values.size();

    if (values.empty()) {
        return result;
    }

    StatisticalAnalyzer analyzer;
    auto stats = analyzer.analyze(values);

    result.mean = stats.mean;
    result.stdDev = stats.standardDeviation;
    result.median = stats.median;
    result.q1 = stats.percentile25;
    result.q3 = stats.percentile75;
    result.iqr = stats.iqr;
    result.mad = calculateMAD(values, stats.median);

    switch (config_.method) {
    case AnomalyMethod::ZScore:
        result.anomalies = detectByZScore(values, config_.zScoreThreshold);
        result.lowerBound = result.mean - config_.zScoreThreshold * result.stdDev;
        result.upperBound = result.mean + config_.zScoreThreshold * result.stdDev;
        break;

    case AnomalyMethod::ModifiedZScore:
        result.anomalies = detectByModifiedZScore(values, config_.modifiedZThreshold);
        result.lowerBound = result.median - config_.modifiedZThreshold * result.mad * 1.4826;
        result.upperBound = result.median + config_.modifiedZThreshold * result.mad * 1.4826;
        break;

    case AnomalyMethod::IQR:
        result.anomalies = detectByIQR(values, config_.iqrMultiplier);
        result.lowerBound = result.q1 - config_.iqrMultiplier * result.iqr;
        result.upperBound = result.q3 + config_.iqrMultiplier * result.iqr;
        break;

    case AnomalyMethod::Threshold:
        result.anomalies = detectByThreshold(values, config_.absoluteMin, config_.absoluteMax);
        result.lowerBound = config_.absoluteMin.value_or(stats.min);
        result.upperBound = config_.absoluteMax.value_or(stats.max);
        break;

    case AnomalyMethod::RateOfChange:
        result.anomalies = detectByRateOfChange(values, config_.maxRateOfChange);
        break;

    case AnomalyMethod::Combined: {
        auto zAnomalies = detectByZScore(values, config_.zScoreThreshold);
        auto iqrAnomalies = detectByIQR(values, config_.iqrMultiplier);
        auto rateAnomalies = detectByRateOfChange(values, config_.maxRateOfChange);

        std::map<size_t, Anomaly> combined;
        for (const auto& a : zAnomalies) {
            combined[a.index] = a;
            combined[a.index].score *= config_.zScoreWeight;
        }
        for (const auto& a : iqrAnomalies) {
            if (combined.count(a.index)) {
                combined[a.index].score += a.score * config_.iqrWeight;
            } else {
                combined[a.index] = a;
                combined[a.index].score *= config_.iqrWeight;
            }
        }
        for (const auto& a : rateAnomalies) {
            if (combined.count(a.index)) {
                combined[a.index].score += a.score * config_.rateWeight;
            } else {
                combined[a.index] = a;
                combined[a.index].score *= config_.rateWeight;
            }
        }

        for (auto it = combined.begin(); it != combined.end(); ++it) {
            if (it->second.score >= config_.combinedThreshold) {
                Anomaly anomaly = it->second;
                anomaly.detectedBy = AnomalyMethod::Combined;
                result.anomalies.push_back(anomaly);
            }
        }
        break;
    }
    }

    std::sort(result.anomalies.begin(), result.anomalies.end());

    result.anomalyCount = result.anomalies.size();
    result.anomalyPercentage =
        result.totalPoints > 0 ? (100.0 * result.anomalyCount / result.totalPoints) : 0.0;

    return result;
}

std::vector<Anomaly> AnomalyDetector::detectByZScore(const std::vector<double>& values,
                                                     double threshold) const {
    std::vector<Anomaly> anomalies;

    if (values.size() < 2)
        return anomalies;

    double mean = StatisticalAnalyzer::calculateMean(values);
    double stdDev = StatisticalAnalyzer::calculateStdDev(values, mean);

    if (stdDev == 0)
        return anomalies;

    for (size_t i = 0; i < values.size(); ++i) {
        double zScore = std::abs((values[i] - mean) / stdDev);

        if (zScore > threshold) {
            std::stringstream reason;
            reason << "Z-score " << std::fixed << std::setprecision(2) << zScore
                   << " exceeds threshold " << threshold;

            Anomaly anomaly;
            anomaly.index = i;
            anomaly.value = values[i];
            anomaly.score = zScore;
            anomaly.detectedBy = AnomalyMethod::ZScore;
            anomaly.reason = reason.str();
            anomalies.push_back(anomaly);
        }
    }

    return anomalies;
}

std::vector<Anomaly> AnomalyDetector::detectByModifiedZScore(const std::vector<double>& values,
                                                             double threshold) const {
    std::vector<Anomaly> anomalies;

    if (values.size() < 2)
        return anomalies;

    std::vector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    double median = StatisticalAnalyzer::calculatePercentile(sorted, 50.0);

    double mad = calculateMAD(values, median);

    if (mad == 0)
        return anomalies;

    for (size_t i = 0; i < values.size(); ++i) {
        double modZ = calculateModifiedZScore(values[i], median, mad);

        if (modZ > threshold) {
            std::stringstream reason;
            reason << "Modified Z-score " << std::fixed << std::setprecision(2) << modZ
                   << " exceeds threshold " << threshold;

            Anomaly anomaly;
            anomaly.index = i;
            anomaly.value = values[i];
            anomaly.score = modZ;
            anomaly.detectedBy = AnomalyMethod::ModifiedZScore;
            anomaly.reason = reason.str();
            anomalies.push_back(anomaly);
        }
    }

    return anomalies;
}

std::vector<Anomaly> AnomalyDetector::detectByIQR(const std::vector<double>& values,
                                                  double multiplier) const {
    std::vector<Anomaly> anomalies;

    if (values.size() < 4)
        return anomalies;

    double q1 = StatisticalAnalyzer::calculatePercentile(values, 25.0);
    double q3 = StatisticalAnalyzer::calculatePercentile(values, 75.0);
    double iqr = q3 - q1;

    if (iqr == 0)
        return anomalies;

    double lowerBound = q1 - multiplier * iqr;
    double upperBound = q3 + multiplier * iqr;

    for (size_t i = 0; i < values.size(); ++i) {
        if (values[i] < lowerBound || values[i] > upperBound) {
            double deviation = values[i] < lowerBound ? (lowerBound - values[i]) / iqr
                                                      : (values[i] - upperBound) / iqr;

            std::stringstream reason;
            reason << "Value " << std::fixed << std::setprecision(2) << values[i]
                   << " outside IQR bounds [" << lowerBound << ", " << upperBound << "]";

            Anomaly anomaly;
            anomaly.index = i;
            anomaly.value = values[i];
            anomaly.score = deviation + multiplier;
            anomaly.detectedBy = AnomalyMethod::IQR;
            anomaly.reason = reason.str();
            anomalies.push_back(anomaly);
        }
    }

    return anomalies;
}

std::vector<Anomaly> AnomalyDetector::detectByThreshold(const std::vector<double>& values,
                                                        std::optional<double> minVal,
                                                        std::optional<double> maxVal) const {
    std::vector<Anomaly> anomalies;

    for (size_t i = 0; i < values.size(); ++i) {
        bool isAnomaly = false;
        std::stringstream reason;
        double score = 0.0;

        if (minVal && values[i] < *minVal) {
            isAnomaly = true;
            score = *minVal - values[i];
            reason << "Value " << values[i] << " below minimum " << *minVal;
        }

        if (maxVal && values[i] > *maxVal) {
            isAnomaly = true;
            score = values[i] - *maxVal;
            reason << "Value " << values[i] << " above maximum " << *maxVal;
        }

        if (isAnomaly) {
            Anomaly anomaly;
            anomaly.index = i;
            anomaly.value = values[i];
            anomaly.score = score;
            anomaly.detectedBy = AnomalyMethod::Threshold;
            anomaly.reason = reason.str();
            anomalies.push_back(anomaly);
        }
    }

    return anomalies;
}

std::vector<Anomaly> AnomalyDetector::detectByRateOfChange(const std::vector<double>& values,
                                                           double maxRate) const {
    std::vector<Anomaly> anomalies;

    if (values.size() < 2)
        return anomalies;

    for (size_t i = 1; i < values.size(); ++i) {
        double rate = std::abs(values[i] - values[i - 1]);

        if (rate > maxRate) {
            std::stringstream reason;
            reason << "Rate of change " << std::fixed << std::setprecision(2) << rate
                   << " exceeds maximum " << maxRate;

            Anomaly anomaly;
            anomaly.index = i;
            anomaly.value = values[i];
            anomaly.score = rate / maxRate;
            anomaly.detectedBy = AnomalyMethod::RateOfChange;
            anomaly.reason = reason.str();
            anomalies.push_back(anomaly);
        }
    }

    return anomalies;
}

double AnomalyDetector::calculateMAD(const std::vector<double>& values, double median) {
    if (values.empty())
        return 0.0;

    std::vector<double> deviations;
    deviations.reserve(values.size());

    for (double v : values) {
        deviations.push_back(std::abs(v - median));
    }

    return StatisticalAnalyzer::calculatePercentile(deviations, 50.0);
}

double AnomalyDetector::calculateModifiedZScore(double value, double median, double mad) {
    if (mad == 0)
        return 0.0;
    return 0.6745 * std::abs(value - median) / mad;
}

std::optional<Anomaly> AnomalyDetector::detectSinglePoint(
    double value, const std::vector<double>& historicalData) const {
    if (historicalData.empty())
        return std::nullopt;

    auto values = historicalData;
    values.push_back(value);

    auto result = detect(values);

    for (const auto& anomaly : result.anomalies) {
        if (anomaly.index == values.size() - 1) {
            return anomaly;
        }
    }

    return std::nullopt;
}

void AnomalyDetector::addTimestamps(std::vector<Anomaly>& anomalies,
                                    const std::vector<SensorReading>& readings) const {
    for (auto& anomaly : anomalies) {
        if (anomaly.index < readings.size()) {
            anomaly.timestamp = readings[anomaly.index].timestamp;
        }
    }
}

}  