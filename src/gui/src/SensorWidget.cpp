#include "gui/SensorWidget.hpp"

#include <QVBoxLayout>

namespace sensorcore::gui {

SensorWidget::SensorWidget(QWidget* parent) : QWidget(parent) {
    setupUi();
}

void SensorWidget::setupUi() {
    auto* layout = new QVBoxLayout(this);

    auto* label = new QLabel("Configured Sensors:", this);
    layout->addWidget(label);

    sensorList_ = new QListWidget(this);
    sensorList_->setAlternatingRowColors(true);
    layout->addWidget(sensorList_);

    connect(sensorList_, &QListWidget::currentRowChanged, this, &SensorWidget::sensorSelected);

    detailsLabel_ = new QLabel(this);
    detailsLabel_->setWordWrap(true);
    detailsLabel_->setStyleSheet(
        "QLabel { background-color: #2a2a2a; padding: 10px; border-radius: 5px; }");
    layout->addWidget(detailsLabel_);
}

void SensorWidget::setSensors(const std::vector<std::unique_ptr<Sensor>>& sensors) {
    sensorList_->clear();

    for (const auto& sensor : sensors) {
        auto* item = new QListWidgetItem(QString::fromStdString(sensor->name()));
        item->setData(Qt::UserRole, QString::fromStdString(sensor->id().value()));
        sensorList_->addItem(item);
    }

    if (!sensors.empty()) {
        sensorList_->setCurrentRow(0);
    }
}

void SensorWidget::updateSensorStatus(int index, SensorState state, double lastValue) {
    if (index < 0 || index >= sensorList_->count())
        return;

    auto* item = sensorList_->item(index);

    QColor color;
    QString stateStr;
    switch (state) {
    case SensorState::Normal:
        color = QColor(0, 200, 0);
        stateStr = "Normal";
        break;
    case SensorState::Warning:
        color = QColor(255, 165, 0);
        stateStr = "Warning";
        break;
    case SensorState::Critical:
        color = QColor(255, 0, 0);
        stateStr = "CRITICAL";
        break;
    case SensorState::Faulted:
        color = QColor(128, 128, 128);
        stateStr = "Error";
        break;
    default:
        color = QColor(100, 100, 100);
        stateStr = "Unknown";
        break;
    }

    item->setForeground(color);

    if (index == sensorList_->currentRow()) {
        detailsLabel_->setText(QString("<b>Status:</b> %1<br>"
                                       "<b>Last Value:</b> %2<br>"
                                       "<b>ID:</b> %3")
                                   .arg(stateStr)
                                   .arg(lastValue, 0, 'f', 2)
                                   .arg(item->data(Qt::UserRole).toString()));
    }
}

}  