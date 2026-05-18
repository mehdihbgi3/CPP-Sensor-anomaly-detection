#pragma once

#include <QPainter>
#include <QWidget>
#include <deque>
#include <vector>

#include "analysis/AnomalyDetector.hpp"
#include "analysis/StatisticalAnalyzer.hpp"

namespace sensorcore::gui {

class ChartWidget : public QWidget {
    Q_OBJECT

public:
    explicit ChartWidget(QWidget* parent = nullptr);

    void addDataPoint(double value);
    void setData(const std::vector<double>& data);
    void clear();

    void setThresholds(double warningLow, double warningHigh, double criticalLow,
                       double criticalHigh);

    void setStatistics(const analysis::StatisticalResult& stats);
    void setAnomalies(const std::vector<analysis::Anomaly>& anomalies);

    void setTitle(const QString& title) {
        title_ = title;
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void drawGrid(QPainter& painter);
    void drawData(QPainter& painter);
    void drawThresholds(QPainter& painter);
    void drawAnomalies(QPainter& painter);
    void drawStatistics(QPainter& painter);
    void drawTitle(QPainter& painter);

    QPointF dataToScreen(int index, double value) const;

    std::deque<double> data_;
    size_t maxPoints_ = 200;

    double minValue_ = 0.0;
    double maxValue_ = 100.0;
    double warningLow_ = 20.0;
    double warningHigh_ = 80.0;
    double criticalLow_ = 10.0;
    double criticalHigh_ = 90.0;

    analysis::StatisticalResult stats_;
    std::vector<analysis::Anomaly> anomalies_;

    QString title_ = "Sensor Data";

    int marginLeft_ = 60;
    int marginRight_ = 20;
    int marginTop_ = 40;
    int marginBottom_ = 40;
};

}  