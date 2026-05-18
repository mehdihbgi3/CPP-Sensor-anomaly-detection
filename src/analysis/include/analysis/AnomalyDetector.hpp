#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "StatisticalAnalyzer.hpp"
#include "sensorcore/SensorReading.hpp"

namespace sensorcore::analysis {

enum class AnomalyMethod { ZScore, ModifiedZScore, IQR, Threshold, RateOfChange, Combined };

struct Anomaly {
    size_t index;
    double value;
    double score;
    AnomalyMethod detectedBy;
    std::string reason;
    Timestamp timestamp;

    bool operator<(const Anomaly& other) const { return score > other.score; }
};

struct AnomalyDetectionResult {
    std::vector<Anomaly> anomalies;
    size_t totalPoints;
    size_t anomalyCount;
    double anomalyPercentage;

    double mean;
    double stdDev;
    double median;
    double mad;
    double q1, q3, iqr;
    double lowerBound, upperBound;

    bool hasAnomalies() const { return !anomalies.empty(); }
};

struct AnomalyConfig {
    AnomalyMethod method = AnomalyMethod::ZScore;

    double zScoreThreshold = 3.0;

    double modifiedZThreshold = 3.5;

    double iqrMultiplier = 1.5;

    std::optional<double> absoluteMin;
    std::optional<double> absoluteMax;

    double maxRateOfChange = 10.0;

    double zScoreWeight = 0.4;
    double iqrWeight = 0.3;
    double rateWeight = 0.3;
    double combinedThreshold = 0.5;
};

class AnomalyDetector {
public:
    explicit AnomalyDetector(AnomalyConfig config = {});

    AnomalyDetectionResult detect(const std::vector<SensorReading>& readings) const;
    AnomalyDetectionResult detect(const std::vector<double>& values) const;

    std::vector<Anomaly> detectByZScore(const std::vector<double>& values, double threshold) const;

    std::vector<Anomaly> detectByModifiedZScore(const std::vector<double>& values,
                                                double threshold) const;

    std::vector<Anomaly> detectByIQR(const std::vector<double>& values, double multiplier) const;

    std::vector<Anomaly> detectByThreshold(const std::vector<double>& values,
                                           std::optional<double> minVal,
                                           std::optional<double> maxVal) const;

    std::vector<Anomaly> detectByRateOfChange(const std::vector<double>& values,
                                              double maxRate) const;

    static double calculateMAD(const std::vector<double>& values, double median);
    static double calculateModifiedZScore(double value, double median, double mad);

    void setConfig(const AnomalyConfig& config) { config_ = config; }
    const AnomalyConfig& config() const { return config_; }

    std::optional<Anomaly> detectSinglePoint(double value,
                                             const std::vector<double>& historicalData) const;

private:
    AnomalyConfig config_;

    void addTimestamps(std::vector<Anomaly>& anomalies,
                       const std::vector<SensorReading>& readings) const;
};

enum class AnomalySeverity { Low, Medium, High, Critical };

inline AnomalySeverity classifyAnomaly(double score) {
    if (score >= 5.0)
        return AnomalySeverity::Critical;
    if (score >= 4.0)
        return AnomalySeverity::High;
    if (score >= 3.0)
        return AnomalySeverity::Medium;
    return AnomalySeverity::Low;
}

inline std::string severityToString(AnomalySeverity severity) {
    switch (severity) {
    case AnomalySeverity::Low:
        return "Low";
    case AnomalySeverity::Medium:
        return "Medium";
    case AnomalySeverity::High:
        return "High";
    case AnomalySeverity::Critical:
        return "Critical";
    default:
        return "Unknown";
    }
}

}  