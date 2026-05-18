#pragma once

#include <QLabel>
#include <QMainWindow>
#include <QTimer>
#include <memory>
#include <vector>

#include "analysis/AnomalyDetector.hpp"
#include "analysis/StatisticalAnalyzer.hpp"
#include "sensorcore/Sensor.hpp"

namespace sensorcore::gui {

class SensorWidget;
class ChartWidget;
class StatusPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void updateSensorData();
    void onSensorSelected(int index);
    void onStartMonitoring();
    void onStopMonitoring();
    void onClearData();

private:
    void setupUi();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupSensors();
    void generateSimulatedReading();

    SensorWidget* sensorWidget_ = nullptr;
    ChartWidget* chartWidget_ = nullptr;
    StatusPanel* statusPanel_ = nullptr;
    QLabel* statusLabel_ = nullptr;

    QTimer* updateTimer_ = nullptr;

    std::vector<std::unique_ptr<Sensor>> sensors_;
    std::vector<std::vector<double>> sensorData_;
    int currentSensorIndex_ = 0;

    analysis::StatisticalAnalyzer analyzer_;
    analysis::AnomalyDetector detector_;

    bool isMonitoring_ = false;
    int tickCount_ = 0;
};

}  