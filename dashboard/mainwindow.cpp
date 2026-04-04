#include "mainwindow.hpp"
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFrame>
#include <QSplitter>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    isDemoMode = true;
    setupStyles();
    setupUI();
    loadDemoData();
    setWindowTitle("NetWatch Central Command [PREVIEW MODE]");
}

void MainWindow::setupStyles() {
    this->setStyleSheet(R"(
        QMainWindow { background-color: #121212; }
        QFrame#Sidebar { background-color: #1e1e2e; border-right: 1px solid #333; }
        QGroupBox { color: #8892b0; font-weight: bold; border: 1px solid #333; margin-top: 10px; }
        QLabel { color: #ccd6f6; }
        QProgressBar { 
            border: 1px solid #333; border-radius: 5px; text-align: center; color: white; 
            background-color: #121212;
        }
        QProgressBar::chunk { background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #64ffda, stop:1 #48b09b); }
        QTableWidget { background-color: #1e1e2e; color: #ccd6f6; gridline-color: #333; border: none; }
        QHeaderView::section { background-color: #233554; color: white; border: 1px solid #121212; }
    )");
}

void MainWindow::setupUI() {
    auto* mainSplitter = new QSplitter(Qt::Horizontal, this);

    auto* sidebar = new QFrame();
    sidebar->setObjectName("Sidebar");
    sidebar->setMinimumWidth(250);
    auto* sideLayout = new QVBoxLayout(sidebar);
    
    auto* listTitle = new QLabel("REMOTE NODES");
    listTitle->setStyleSheet("font-size: 10px; color: #64ffda; font-weight: bold; letter-spacing: 2px;");
    sideLayout->addWidget(listTitle);

    deviceListWidget = new QListWidget();
    sideLayout->addWidget(deviceListWidget);

    auto* detailView = new QWidget();
    auto* detailLayout = new QVBoxLayout(detailView);
    auto* gaugeLayout = new QHBoxLayout();
    cpuBar = new QProgressBar();
    ramBar = new QProgressBar();
    gaugeLayout->addWidget(createGaugeGroup("CPU LOAD", cpuBar));
    gaugeLayout->addWidget(createGaugeGroup("MEMORY USAGE", ramBar));
    detailLayout->addLayout(gaugeLayout);


    infoLabel = new QLabel("Select a device to see diagnostics...");
    detailLayout->addWidget(infoLabel);

    processTable = new ProcessTable();
    detailLayout->addWidget(processTable);

    mainSplitter->addWidget(sidebar);
    mainSplitter->addWidget(detailView);
    setCentralWidget(mainSplitter);

    connect(deviceListWidget, &QListWidget::currentTextChanged, this, [this](const QString& name){
        currentSelectedDevice = name;
        refreshDisplay();
    });
}

void MainWindow::onDataReceived(const SystemStats& stats) {

    if (isDemoMode) {
        allDevicesData.clear();
        deviceListWidget->clear();
        isDemoMode = false;         
        this->setWindowTitle("NetWatch Central Command [LIVE]");
    }

    QString name = QString::fromStdString(stats.hostname);
    
    allDevicesData[name] = stats;

    QList<QListWidgetItem*> items = deviceListWidget->findItems(name, Qt::MatchExactly);
    
    if (items.isEmpty()) {
        QListWidgetItem* newItem = new QListWidgetItem(name, deviceListWidget);

        newItem->setToolTip(QString("IP: %1 | CPU: %2%").arg(
            QString::fromStdString(stats.ip_address)).arg(stats.cpu_total));

        if (stats.cpu_total > 90) {
            newItem->setForeground(Qt::red);
        } else {
            newItem->setForeground(QColor("#64ffda")); 
        }


        if (deviceListWidget->count() == 1) {
            deviceListWidget->setCurrentRow(0);
            currentSelectedDevice = name;
        }
    } else {

        if (stats.cpu_total > 90) {
            items[0]->setForeground(Qt::red);
        } else {
            items[0]->setForeground(QColor("#64ffda"));
        }
        
        items[0]->setToolTip(QString("IP: %1 | CPU: %2%").arg(
            QString::fromStdString(stats.ip_address)).arg(stats.cpu_total));
    }


    if (currentSelectedDevice == name) {
        refreshDisplay();
    }
}

void MainWindow::refreshDisplay() {
    if (allDevicesData.find(currentSelectedDevice) == allDevicesData.end()) return;

    const auto& data = allDevicesData[currentSelectedDevice];
    cpuBar->setValue(static_cast<int>(data.cpu_total));
    ramBar->setValue(static_cast<int>((data.ram_used_gb / data.ram_total_gb) * 100));

    if (data.cpu_total > 80.0) {
        cpuBar->setStyleSheet("QProgressBar::chunk { background-color: #ff5555; }");
    } else {
        cpuBar->setStyleSheet("QProgressBar::chunk { background-color: #64ffda; }");
    }

    infoLabel->setText(QString("IP: %1 | Uptime: %2")
                       .arg(QString::fromStdString(data.ip_address))
                       .arg(QString::fromStdString(data.uptime)));
                       
    processTable->updateProcesses(data.processes);
}

QGroupBox* MainWindow::createGaugeGroup(QString title, QProgressBar* bar) {
    auto* group = new QGroupBox(title);
    auto* layout = new QVBoxLayout(group);
    layout->addWidget(bar);
    return group;
}
void MainWindow::onConnectionStatusChanged(bool connected) {
    if (connected) {
        statusBar()->showMessage("Connected to Agent", 3000);
        this->setWindowTitle("NetWatch Central Command [ONLINE]");
    } else {
        statusBar()->showMessage("CONNECTION LOST", 0);
        this->setWindowTitle("NetWatch Central Command [OFFLINE]");
        cpuBar->setValue(0);
        ramBar->setValue(0);
        infoLabel->setText("Searching for agents...");    }
}

void MainWindow::loadDemoData() {
    SystemStats pi;
    pi.hostname = "Server-01";
    pi.ip_address = "192.168.1.10";
    pi.cpu_total = 12.4; // Low usage
    pi.ram_used_gb = 0.5;
    pi.ram_total_gb = 4.0;
    pi.uptime = "15d 04h 22m";
    pi.processes = {
        {101, "thermal_daemon", 1.2, 0.1},
        {102, "python_script", 5.4, 0.3}
    };

    SystemStats work;
    work.hostname = "Workstation-X";
    work.ip_address = "192.168.1.55";
    work.cpu_total = 88.7; // HIGH usage (Should trigger Red color!)
    work.ram_used_gb = 14.5;
    work.ram_total_gb = 16.0;
    work.uptime = "01d 02h 15m";
    work.processes = {
        {501, "Visual Studio", 45.0, 4.2},
        {505, "Chrome", 32.1, 2.8},
        {510, "Docker", 10.5, 3.1}
    };


    SystemStats laptop;
    laptop.hostname = "Youanes-Laptop";
    laptop.ip_address = "192.168.1.102";
    laptop.cpu_total = 35.0; // Medium usage
    laptop.ram_used_gb = 4.2;
    laptop.ram_total_gb = 8.0;
    laptop.uptime = "00d 18h 45m";
    laptop.processes = {
        {901, "Zoom", 20.0, 1.5},
        {905, "Spotify", 5.0, 0.4}
    };

    allDevicesData[QString::fromStdString(pi.hostname)] = pi;
    allDevicesData[QString::fromStdString(work.hostname)] = work;
    allDevicesData[QString::fromStdString(laptop.hostname)] = laptop;

    deviceListWidget->addItem(QString::fromStdString(pi.hostname));
    deviceListWidget->addItem(QString::fromStdString(work.hostname));
    deviceListWidget->addItem(QString::fromStdString(laptop.hostname));

    deviceListWidget->item(1)->setForeground(Qt::red);


    deviceListWidget->setCurrentRow(0);
}
