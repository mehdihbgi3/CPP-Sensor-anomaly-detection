#include "gui/ChartWidget.hpp"

#include <QPainterPath>
#include <algorithm>
#include <cmath>

namespace sensorcore::gui {

ChartWidget::ChartWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(400, 300);
    setAutoFillBackground(true);

    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(30, 30, 30));
    setPalette(pal);
}

void ChartWidget::addDataPoint(double value) {
    data_.push_back(value);
    if (data_.size() > maxPoints_) {
        data_.pop_front();
    }

    if (!data_.empty()) {
        auto [minIt, maxIt] = std::minmax_element(data_.begin(), data_.end());
        double range = *maxIt - *minIt;
        double padding = range * 0.1;
        minValue_ = std::min(minValue_, *minIt - padding);
        maxValue_ = std::max(maxValue_, *maxIt + padding);
    }

    update();
}

void ChartWidget::setData(const std::vector<double>& data) {
    data_.clear();
    for (double v : data) {
        data_.push_back(v);
    }

    if (!data_.empty()) {
        auto [minIt, maxIt] = std::minmax_element(data_.begin(), data_.end());
        double range = *maxIt - *minIt;
        double padding = range * 0.1;
        minValue_ = *minIt - padding;
        maxValue_ = *maxIt + padding;
    }

    update();
}

void ChartWidget::clear() {
    data_.clear();
    anomalies_.clear();
    update();
}

void ChartWidget::setThresholds(double warningLow, double warningHigh, double criticalLow,
                                double criticalHigh) {
    warningLow_ = warningLow;
    warningHigh_ = warningHigh;
    criticalLow_ = criticalLow;
    criticalHigh_ = criticalHigh;

    minValue_ = std::min(minValue_, criticalLow - 10);
    maxValue_ = std::max(maxValue_, criticalHigh + 10);

    update();
}

void ChartWidget::setStatistics(const analysis::StatisticalResult& stats) {
    stats_ = stats;
    update();
}

void ChartWidget::setAnomalies(const std::vector<analysis::Anomaly>& anomalies) {
    anomalies_ = anomalies;
    update();
}

void ChartWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    drawGrid(painter);
    drawThresholds(painter);
    drawData(painter);
    drawAnomalies(painter);
    drawStatistics(painter);
    drawTitle(painter);
}

void ChartWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    update();
}

void ChartWidget::drawGrid(QPainter& painter) {
    painter.setPen(QPen(QColor(60, 60, 60), 1));

    int chartWidth = width() - marginLeft_ - marginRight_;
    int chartHeight = height() - marginTop_ - marginBottom_;

    for (int i = 0; i <= 5; ++i) {
        int y = marginTop_ + i * chartHeight / 5;
        painter.drawLine(marginLeft_, y, width() - marginRight_, y);

        double value = maxValue_ - i * (maxValue_ - minValue_) / 5;
        painter.setPen(Qt::white);
        painter.drawText(5, y + 5, QString::number(value, 'f', 1));
        painter.setPen(QPen(QColor(60, 60, 60), 1));
    }

    for (int i = 0; i <= 10; ++i) {
        int x = marginLeft_ + i * chartWidth / 10;
        painter.drawLine(x, marginTop_, x, height() - marginBottom_);
    }

    painter.setPen(QPen(QColor(100, 100, 100), 2));
    painter.drawRect(marginLeft_, marginTop_, chartWidth, chartHeight);
}

void ChartWidget::drawThresholds(QPainter& painter) {
    auto drawThresholdLine = [&](double value, const QColor& color, const QString& label) {
        QPointF left = dataToScreen(0, value);
        QPointF right = dataToScreen(static_cast<int>(maxPoints_), value);

        painter.setPen(QPen(color, 2, Qt::DashLine));
        painter.drawLine(QPointF(marginLeft_, left.y()), QPointF(width() - marginRight_, left.y()));

        painter.setPen(color);
        painter.drawText(QPointF(width() - marginRight_ + 5, left.y() + 4), label);
    };

    drawThresholdLine(criticalHigh_, QColor(255, 0, 0, 150), "Crit H");
    drawThresholdLine(warningHigh_, QColor(255, 165, 0, 150), "Warn H");
    drawThresholdLine(warningLow_, QColor(255, 165, 0, 150), "Warn L");
    drawThresholdLine(criticalLow_, QColor(255, 0, 0, 150), "Crit L");
}

void ChartWidget::drawData(QPainter& painter) {
    if (data_.size() < 2)
        return;

    QPainterPath path;
    bool first = true;

    int offset = static_cast<int>(maxPoints_ - data_.size());

    for (size_t i = 0; i < data_.size(); ++i) {
        QPointF pt = dataToScreen(static_cast<int>(i) + offset, data_[i]);

        if (first) {
            path.moveTo(pt);
            first = false;
        } else {
            path.lineTo(pt);
        }
    }

    QPainterPath fillPath = path;
    fillPath.lineTo(dataToScreen(static_cast<int>(data_.size()) + offset - 1, minValue_));
    fillPath.lineTo(dataToScreen(offset, minValue_));
    fillPath.closeSubpath();

    QLinearGradient gradient(0, marginTop_, 0, height() - marginBottom_);
    gradient.setColorAt(0, QColor(0, 150, 255, 100));
    gradient.setColorAt(1, QColor(0, 150, 255, 20));
    painter.fillPath(fillPath, gradient);

    painter.setPen(QPen(QColor(0, 200, 255), 2));
    painter.drawPath(path);

    if (!data_.empty()) {
        QPointF lastPt = dataToScreen(static_cast<int>(data_.size()) + offset - 1, data_.back());
        painter.setBrush(QColor(0, 255, 100));
        painter.setPen(Qt::white);
        painter.drawEllipse(lastPt, 5, 5);
    }
}

void ChartWidget::drawAnomalies(QPainter& painter) {
    if (anomalies_.empty() || data_.empty())
        return;

    painter.setBrush(QColor(255, 0, 0, 200));
    painter.setPen(QPen(Qt::white, 2));

    int offset = static_cast<int>(maxPoints_ - data_.size());

    for (const auto& anomaly : anomalies_) {
        if (anomaly.index < data_.size()) {
            QPointF pt = dataToScreen(static_cast<int>(anomaly.index) + offset, anomaly.value);
            painter.drawEllipse(pt, 8, 8);
        }
    }
}

void ChartWidget::drawStatistics(QPainter& painter) {
    if (stats_.count == 0)
        return;

    QPointF meanLeft = dataToScreen(0, stats_.mean);
    QPointF meanRight = dataToScreen(static_cast<int>(maxPoints_), stats_.mean);

    painter.setPen(QPen(QColor(0, 255, 0), 2, Qt::DotLine));
    painter.drawLine(QPointF(marginLeft_, meanLeft.y()),
                     QPointF(width() - marginRight_, meanLeft.y()));
}

void ChartWidget::drawTitle(QPainter& painter) {
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(14);
    font.setBold(true);
    painter.setFont(font);

    painter.drawText(QRect(0, 5, width(), 30), Qt::AlignCenter, title_);
}

QPointF ChartWidget::dataToScreen(int index, double value) const {
    int chartWidth = width() - marginLeft_ - marginRight_;
    int chartHeight = height() - marginTop_ - marginBottom_;

    double x = marginLeft_ + (static_cast<double>(index) / maxPoints_) * chartWidth;
    double y = marginTop_ + ((maxValue_ - value) / (maxValue_ - minValue_)) * chartHeight;

    return QPointF(x, y);
}

}  