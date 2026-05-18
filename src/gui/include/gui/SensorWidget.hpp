#pragma once

#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <vector>

#include "sensorcore/Sensor.hpp"

namespace sensorcore::gui {

class SensorWidget : public QWidget {
    Q_OBJECT

public:
    explicit SensorWidget(QWidget* parent = nullptr);

    void setSensors(const std::vector<std::unique_ptr<Sensor>>& sensors);
    void updateSensorStatus(int index, SensorState state, double lastValue);

signals:
    void sensorSelected(int index);

private:
    void setupUi();

    QListWidget* sensorList_ = nullptr;
    QLabel* detailsLabel_ = nullptr;
};

}  