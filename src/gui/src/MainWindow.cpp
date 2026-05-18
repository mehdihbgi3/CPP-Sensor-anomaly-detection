#include "gui/MainWindow.hpp"

#include <QAction>
#include <QDockWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <random>

#include "gui/ChartWidget.hpp"
#include "gui/SensorWidget.hpp"
#include "gui/StatusPanel.hpp"

namespace sensorcore::gui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("SensorCorePro - Industrial Sensor Monitoring");
    setMinimumSize(1200, 800);

    setupUi();
    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupSensors();

    updateTimer_ = new QTimer(this);
    connect(updateTimer_, &QTimer::timeout, this, &MainWindow::updateSensorData);
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    chartWidget_ = new ChartWidget(this);
    setCentralWidget(chartWidget_);

    auto* sensorDock = new QDockWidget("Sensors", this);
    sensorWidget_ = new SensorWidget(sensorDock);
    sensorDock->setWidget(sensorWidget_);
    sensorDock->setMinimumWidth(250);
    addDockWidget(Qt::LeftDockWidgetArea, sensorDock);

    connect(sensorWidget_, &SensorWidget::sensorSelected, this, &MainWindow::onSensorSelected);

    auto* statusDock = new QDockWidget("Statistics", this);
    statusPanel_ = new StatusPanel(statusDock);
    statusDock->setWidget(statusPanel_);
    statusDock->setMinimumWidth(280);
    addDockWidget(Qt::RightDockWidgetArea, statusDock);
}

void MainWindow::setupMenuBar() {
    auto* fileMenu = menuBar()->addMenu("&File");

    auto* exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    auto* controlMenu = menuBar()->addMenu("&Control");

    auto* startAction = controlMenu->addAction("&Start Monitoring");
    startAction->setShortcut(Qt::Key_F5);
    connect(startAction, &QAction::triggered, this, &MainWindow::onStartMonitoring);

    auto* stopAction = controlMenu->addAction("S&top Monitoring");
    stopAction->setShortcut(Qt::Key_F6);
    connect(stopAction, &QAction::triggered, this, &MainWindow::onStopMonitoring);

    controlMenu->addSeparator();

    auto* clearAction = controlMenu->addAction("&Clear Data");
    connect(clearAction, &QAction::triggered, this, &MainWindow::onClearData);

    auto* helpMenu = menuBar()->addMenu("&Help");

    auto* aboutAction = helpMenu->addAction("&About");
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "About SensorCorePro",
                           "<h2>SensorCorePro v1.0.0</h2>"
                           "<p>Industrial Real-Time Sensor Monitoring & Analysis Platform</p>"
                           "<p>Features:</p>"
                           "<ul>"
                           "<li>Real-time sensor data visualization</li>"
                           "<li>Statistical analysis (mean, std dev, percentiles)</li>"
                           "<li>Anomaly detection (Z-score, IQR, threshold)</li>"
                           "<li>Configurable alert thresholds</li>"
                           "</ul>"
                           "<p>Built with C++20, Qt5, and TDD methodology.</p>");
    });
}

void MainWindow::setupToolBar() {
    auto* toolbar = addToolBar("Main Toolbar");
    toolbar->setMovable(false);

    auto* startAction = toolbar->addAction(">> Start");

    startAction->setToolTip("Start monitoring (F5)");
    connect(startAction, &QAction::triggered, this, &MainWindow::onStartMonitoring);

    auto* stopAction = toolbar->addAction("[] Stop");

    stopAction->setToolTip("Stop monitoring (F6)");
    connect(stopAction, &QAction::triggered, this, &MainWindow::onStopMonitoring);

    toolbar->addSeparator();

    auto* clearAction = toolbar->addAction("X Clear");
    clearAction->setToolTip("Clear all data");
    connect(clearAction, &QAction::triggered, this, &MainWindow::onClearData);
}

void MainWindow::setupStatusBar() {
    statusLabel_ = new QLabel("Ready");
    statusBar()->addPermanentWidget(statusLabel_);
}

void MainWindow::setupSensors() {
    SensorConfig tempConfig;
    tempConfig.id = SensorId{"TEMP-001"};
    tempConfig.name = "Reactor Temperature";
    tempConfig.type = SensorType::Temperature;
    tempConfig.unit = Unit::Celsius;
    tempConfig.validRange = {-40.0, 150.0};
    tempConfig.thresholds = {0.0, 80.0, -20.0, 100.0};
    sensors_.push_back(std::make_unique<Sensor>(tempConfig));
    sensorData_.push_back({});

    SensorConfig pressConfig;
    pressConfig.id = SensorId{"PRES-001"};
    pressConfig.name = "Main Pressure";
    pressConfig.type = SensorType::Pressure;
    pressConfig.unit = Unit::Pascal;
    pressConfig.validRange = {0.0, 1000.0};
    pressConfig.thresholds = {100.0, 800.0, 50.0, 900.0};
    sensors_.push_back(std::make_unique<Sensor>(pressConfig));
    sensorData_.push_back({});

    SensorConfig flowConfig;
    flowConfig.id = SensorId{"FLOW-001"};
    flowConfig.name = "Coolant Flow";
    flowConfig.type = SensorType::FlowRate;
    flowConfig.unit = Unit::CubicMetersPerHour;
    flowConfig.validRange = {0.0, 100.0};
    flowConfig.thresholds = {10.0, 80.0, 5.0, 90.0};
    sensors_.push_back(std::make_unique<Sensor>(flowConfig));
    sensorData_.push_back({});

    SensorConfig vibConfig;
    vibConfig.id = SensorId{"VIB-001"};
    vibConfig.name = "Motor Vibration";
    vibConfig.type = SensorType::Vibration;
    vibConfig.unit = Unit::MetersPerSecondSquared;
    vibConfig.validRange = {0.0, 50.0};
    vibConfig.thresholds = {5.0, 25.0, 2.0, 35.0};
    sensors_.push_back(std::make_unique<Sensor>(vibConfig));
    sensorData_.push_back({});

    sensorWidget_->setSensors(sensors_);

    onSensorSelected(0);
}

void MainWindow::onSensorSelected(int index) {
    if (index < 0 || index >= static_cast<int>(sensors_.size()))
        return;

    currentSensorIndex_ = index;
    const auto& sensor = sensors_[index];

    chartWidget_->setTitle(QString::fromStdString(sensor->name()));
    chartWidget_->setThresholds(sensor->thresholds().warningLow, sensor->thresholds().warningHigh,
                                sensor->thresholds().criticalLow,
                                sensor->thresholds().criticalHigh);
    chartWidget_->setData(sensorData_[index]);

    if (!sensorData_[index].empty()) {
        auto stats = analyzer_.analyze(sensorData_[index]);
        chartWidget_->setStatistics(stats);
        statusPanel_->updateStatistics(stats);

        auto anomalyResult = detector_.detect(sensorData_[index]);
        chartWidget_->setAnomalies(anomalyResult.anomalies);
        statusPanel_->updateAnomalyStatus(anomalyResult);
    }
}

void MainWindow::onStartMonitoring() {
    if (!isMonitoring_) {
        isMonitoring_ = true;
        updateTimer_->start(100);
        statusLabel_->setText("Monitoring...");
    }
}

void MainWindow::onStopMonitoring() {
    if (isMonitoring_) {
        isMonitoring_ = false;
        updateTimer_->stop();
        statusLabel_->setText("Stopped");
    }
}

void MainWindow::onClearData() {
    for (auto& data : sensorData_) {
        data.clear();
    }
    chartWidget_->clear();
    tickCount_ = 0;
    statusLabel_->setText("Data cleared");
}

void MainWindow::updateSensorData() {
    generateSimulatedReading();
    tickCount_++;
}

void MainWindow::generateSimulatedReading() {
    static std::mt19937 gen(std::random_device{}());

    for (size_t i = 0; i < sensors_.size(); ++i) {
        const auto& sensor = sensors_[i];
        auto& data = sensorData_[i];

        double baseValue =
            (sensor->thresholds().warningLow + sensor->thresholds().warningHigh) / 2.0;
        double range = sensor->thresholds().warningHigh - sensor->thresholds().warningLow;

        std::normal_distribution<> noise(0, range * 0.05);
        std::normal_distribution<> trend(0, range * 0.01);

        double value = baseValue;
        if (!data.empty()) {
            value = data.back() + trend(gen);
        }
        value += noise(gen);

        std::uniform_real_distribution<> anomalyChance(0, 1);
        if (anomalyChance(gen) < 0.02) {
            std::uniform_real_distribution<> anomalyMag(1.5, 3.0);
            value = baseValue + (anomalyChance(gen) > 0.5 ? 1 : -1) * range * anomalyMag(gen) * 0.3;
        }

        value = std::clamp(value, sensor->validRange().min, sensor->validRange().max);

        data.push_back(value);

        if (data.size() > 500) {
            data.erase(data.begin());
        }

        auto result = sensor->validateReading(value);
        SensorState state = SensorState::Normal;
        if (result.status == ReadingStatus::Warning)
            state = SensorState::Warning;
        else if (result.status == ReadingStatus::Critical)
            state = SensorState::Critical;
        else if (result.status == ReadingStatus::Invalid)
            state = SensorState::Faulted;

        sensors_[i]->setState(state);
        sensorWidget_->updateSensorStatus(static_cast<int>(i), state, value);
    }

    const auto& currentData = sensorData_[currentSensorIndex_];
    if (!currentData.empty()) {
        double lastValue = currentData.back();
        chartWidget_->addDataPoint(lastValue);

        if (currentData.size() % 10 == 0 && currentData.size() >= 10) {
            auto stats = analyzer_.analyze(currentData);
            chartWidget_->setStatistics(stats);
            statusPanel_->updateStatistics(stats);

            auto anomalyResult = detector_.detect(currentData);
            chartWidget_->setAnomalies(anomalyResult.anomalies);
            statusPanel_->updateAnomalyStatus(anomalyResult);
        }

        auto result = sensors_[currentSensorIndex_]->validateReading(lastValue);
        QString status;
        switch (result.status) {
        case ReadingStatus::Normal:
            status = "Normal";
            break;
        case ReadingStatus::Warning:
            status = "Warning";
            break;
        case ReadingStatus::Critical:
            status = "CRITICAL";
            break;
        default:
            status = "Invalid";
            break;
        }
        statusPanel_->setCurrentValue(lastValue, status);
    }
}

}  