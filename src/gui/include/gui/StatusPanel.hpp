#pragma once

#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QWidget>

#include "analysis/AnomalyDetector.hpp"
#include "analysis/StatisticalAnalyzer.hpp"

namespace sensorcore::gui {

class StatusPanel : public QWidget {
    Q_OBJECT

public:
    explicit StatusPanel(QWidget* parent = nullptr);

    void updateStatistics(const analysis::StatisticalResult& stats);
    void updateAnomalyStatus(const analysis::AnomalyDetectionResult& result);
    void setCurrentValue(double value, const QString& status);

private:
    void setupUi();
    QLabel* createValueLabel(const QString& initialText = "---");

    QLabel* currentValueLabel_ = nullptr;
    QLabel* currentStatusLabel_ = nullptr;

    QLabel* meanLabel_ = nullptr;
    QLabel* stdDevLabel_ = nullptr;
    QLabel* minLabel_ = nullptr;
    QLabel* maxLabel_ = nullptr;
    QLabel* medianLabel_ = nullptr;
    QLabel* countLabel_ = nullptr;

    QLabel* anomalyCountLabel_ = nullptr;
    QLabel* anomalyPercentLabel_ = nullptr;
    QLabel* lastAnomalyLabel_ = nullptr;
};

}  