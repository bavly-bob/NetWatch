#include "mainwindow.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUI();
    setWindowTitle("NetWatch Dashboard");
    resize(800, 600);
}

void MainWindow::setupUI() {
    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new  QVBoxLayout(centralWidget);

    statusLabel = new QLabel("Status: Disconnected");
    statusLabel->setStyleSheet("color: red; font-weight: bold;");
    mainLayout->addWidget(statusLabel);

       auto* gaugeLayout = new QHBoxLayout();
    
    auto* cpuGroup = new QGroupBox("CPU Usage");
    auto* cpuLayout = new QVBoxLayout(cpuGroup);
    cpuBar = new QProgressBar();
    cpuLayout->addWidget(cpuBar);
    
    auto* ramGroup = new QGroupBox("RAM Usage");
    auto* ramLayout = new  QVBoxLayout(ramGroup);
    ramBar = new QProgressBar();
    ramLayout->addWidget(ramBar);

    gaugeLayout->addWidget(cpuGroup);
    gaugeLayout->addWidget(ramGroup);
    mainLayout->addLayout(gaugeLayout);

    uptimeLabel = new QLabel("Uptime: --:--:--");
    mainLayout->addWidget(uptimeLabel);

    processTable = new ProcessTable();
    mainLayout->addWidget(processTable);

    setCentralWidget(centralWidget);
}

void MainWindow::onDataReceived(const SystemStats& stats) {
    cpuBar->setValue(static_cast<int>(stats.cpu_total));
    ramBar->setValue(static_cast<int>((stats.ram_used_gb / stats.ram_total_gb) * 100));
    uptimeLabel->setText(QString("Uptime: %1").arg(QString::fromStdString(stats.uptime)));
    processTable->updateProcesses(stats.processes);
}

void MainWindow::onConnectionStatusChanged(bool connected) {
    if (connected) {
        statusLabel->setText("Status: Connected");
        statusLabel->setStyleSheet("color: green; font-weight: bold;");
    } else {
        statusLabel->setText("Status: Connection Lost!");
        statusLabel->setStyleSheet("color: red; font-weight: bold;");
    }
}
