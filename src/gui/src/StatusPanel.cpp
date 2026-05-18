#include "gui/StatusPanel.hpp"

#include <QFrame>
#include <QVBoxLayout>

namespace sensorcore::gui {

StatusPanel::StatusPanel(QWidget* parent) : QWidget(parent) {
    setupUi();
}

void StatusPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    auto* currentGroup = new QGroupBox("Current Reading", this);
    auto* currentLayout = new QGridLayout(currentGroup);

    currentLayout->addWidget(new QLabel("Value:"), 0, 0);
    currentValueLabel_ = createValueLabel("---");
    currentValueLabel_->setStyleSheet(
        "QLabel { font-size: 24px; font-weight: bold; color: #00ff00; }");
    currentLayout->addWidget(currentValueLabel_, 0, 1);

    currentLayout->addWidget(new QLabel("Status:"), 1, 0);
    currentStatusLabel_ = createValueLabel("---");
    currentLayout->addWidget(currentStatusLabel_, 1, 1);

    mainLayout->addWidget(currentGroup);

    auto* statsGroup = new QGroupBox("Statistics", this);
    auto* statsLayout = new QGridLayout(statsGroup);

    statsLayout->addWidget(new QLabel("Count:"), 0, 0);
    countLabel_ = createValueLabel("0");
    statsLayout->addWidget(countLabel_, 0, 1);

    statsLayout->addWidget(new QLabel("Mean:"), 1, 0);
    meanLabel_ = createValueLabel("---");
    statsLayout->addWidget(meanLabel_, 1, 1);

    statsLayout->addWidget(new QLabel("Std Dev:"), 2, 0);
    stdDevLabel_ = createValueLabel("---");
    statsLayout->addWidget(stdDevLabel_, 2, 1);

    statsLayout->addWidget(new QLabel("Min:"), 3, 0);
    minLabel_ = createValueLabel("---");
    statsLayout->addWidget(minLabel_, 3, 1);

    statsLayout->addWidget(new QLabel("Max:"), 4, 0);
    maxLabel_ = createValueLabel("---");
    statsLayout->addWidget(maxLabel_, 4, 1);

    statsLayout->addWidget(new QLabel("Median:"), 5, 0);
    medianLabel_ = createValueLabel("---");
    statsLayout->addWidget(medianLabel_, 5, 1);

    mainLayout->addWidget(statsGroup);

    auto* anomalyGroup = new QGroupBox("Anomaly Detection", this);
    auto* anomalyLayout = new QGridLayout(anomalyGroup);

    anomalyLayout->addWidget(new QLabel("Anomalies:"), 0, 0);
    anomalyCountLabel_ = createValueLabel("0");
    anomalyLayout->addWidget(anomalyCountLabel_, 0, 1);

    anomalyLayout->addWidget(new QLabel("Percentage:"), 1, 0);
    anomalyPercentLabel_ = createValueLabel("0.0%");
    anomalyLayout->addWidget(anomalyPercentLabel_, 1, 1);

    anomalyLayout->addWidget(new QLabel("Last Anomaly:"), 2, 0);
    lastAnomalyLabel_ = createValueLabel("None");
    lastAnomalyLabel_->setWordWrap(true);
    anomalyLayout->addWidget(lastAnomalyLabel_, 2, 1);

    mainLayout->addWidget(anomalyGroup);

    mainLayout->addStretch();
}

QLabel* StatusPanel::createValueLabel(const QString& initialText) {
    auto* label = new QLabel(initialText, this);
    label->setStyleSheet("QLabel { color: #00aaff; font-weight: bold; }");
    return label;
}

void StatusPanel::updateStatistics(const analysis::StatisticalResult& stats) {
    countLabel_->setText(QString::number(stats.count));
    meanLabel_->setText(QString::number(stats.mean, 'f', 2));
    stdDevLabel_->setText(QString::number(stats.standardDeviation, 'f', 2));
    minLabel_->setText(QString::number(stats.min, 'f', 2));
    maxLabel_->setText(QString::number(stats.max, 'f', 2));
    medianLabel_->setText(QString::number(stats.median, 'f', 2));
}

void StatusPanel::updateAnomalyStatus(const analysis::AnomalyDetectionResult& result) {
    anomalyCountLabel_->setText(QString::number(result.anomalyCount));
    anomalyPercentLabel_->setText(QString::number(result.anomalyPercentage, 'f', 1) + "%");

    if (result.anomalyCount > 0) {
        anomalyCountLabel_->setStyleSheet("QLabel { color: #ff0000; font-weight: bold; }");
    } else {
        anomalyCountLabel_->setStyleSheet("QLabel { color: #00ff00; font-weight: bold; }");
    }

    if (!result.anomalies.empty()) {
        const auto& last = result.anomalies.front();
        lastAnomalyLabel_->setText(
            QString("Index %1: %2").arg(last.index).arg(last.value, 0, 'f', 2));
    } else {
        lastAnomalyLabel_->setText("None");
    }
}

void StatusPanel::setCurrentValue(double value, const QString& status) {
    currentValueLabel_->setText(QString::number(value, 'f', 2));
    currentStatusLabel_->setText(status);

    if (status == "Normal") {
        currentValueLabel_->setStyleSheet(
            "QLabel { font-size: 24px; font-weight: bold; color: #00ff00; }");
        currentStatusLabel_->setStyleSheet("QLabel { color: #00ff00; font-weight: bold; }");
    } else if (status == "Warning") {
        currentValueLabel_->setStyleSheet(
            "QLabel { font-size: 24px; font-weight: bold; color: #ffa500; }");
        currentStatusLabel_->setStyleSheet("QLabel { color: #ffa500; font-weight: bold; }");
    } else if (status == "CRITICAL") {
        currentValueLabel_->setStyleSheet(
            "QLabel { font-size: 24px; font-weight: bold; color: #ff0000; }");
        currentStatusLabel_->setStyleSheet("QLabel { color: #ff0000; font-weight: bold; }");
    }
}

}  