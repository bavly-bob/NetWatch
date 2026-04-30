#include "mainwindow.hpp"

// Qt
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFrame>
#include <QSplitter>
#include <QMetaObject>
#include <QListWidgetItem>
#include <QColor>

// Networking
#include <boost/asio.hpp>
#include "server.hpp"
#include "connection.hpp"

// Serialization
#include "json_serializer.hpp"
#include "config.hpp"

// ─── Constructor ──────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    isDemoMode = true;
    m_llmClient = new LlmClient(this);
    m_dbManager = new DbManager();
    if (!m_dbManager->initialize()) {
        delete m_dbManager;
        m_dbManager = nullptr;
    }
    setupStyles();
    setupUI();
    loadDemoData();
    setWindowTitle("NetWatch Central Command [PREVIEW MODE]");

    // Connect the cross-thread signals to their slots using QueuedConnection
    // so the networking thread can safely post data onto the Qt event loop.
    connect(this, &MainWindow::statsReceived,
            this, &MainWindow::onDataReceived,
            Qt::QueuedConnection);
    connect(this, &MainWindow::agentConnected,
            this, &MainWindow::onConnectionStatusChanged,
            Qt::QueuedConnection);
    connect(m_llmClient, &LlmClient::analysisReady,
            this, &MainWindow::onLlmAnalysisReady);
    connect(m_llmClient, &LlmClient::analysisFailed,
            this, &MainWindow::onLlmAnalysisFailed);

    startServer();
}

// ─── Destructor ───────────────────────────────────────────────────────────────
MainWindow::~MainWindow() {
    if (m_io) m_io->stop();
    if (m_ioThread.joinable()) m_ioThread.join();
    delete m_dbManager;
}

// ─── Start the Boost.Asio server ─────────────────────────────────────────────
void MainWindow::startServer() {
    m_io = std::make_shared<boost::asio::io_context>();

    m_server = std::make_shared<netwatch::networking::Server>(
        *m_io, netwatch::DEFAULT_PORT);

    // For every new connection accepted by the server, install a message handler
    // that parses the incoming JSON and emits the Qt signal into the GUI thread.
    //
    // We wrap the lambda in a QMetaObject::invokeMethod so the actual slot call
    // always runs on the Qt main thread even though this lambda runs on m_ioThread.

    // Override doAccept callback: we patch the existing Server by setting a
    // message handler on every connection it creates.
    // We use the server's getConnections() polling approach via a periodic Qt timer,
    // OR the cleaner approach: intercept via a custom connection handler.
    //
    // Since Server::doAccept() already calls conn->start() and we can set the
    // handler before start(), we subclass or wrap. The simplest compatible approach
    // is to poll getConnections() with a QTimer and set handlers on new connections.

    QTimer* poll = new QTimer(this);
    connect(poll, &QTimer::timeout, this, [this, poll]() {
        for (auto& conn : m_server->getConnections()) {
            // setMessageHandler is idempotent if we guard with a flag,
            // but Connection doesn't have one. We use a small workaround:
            // replace with a lambda that includes a shared "installed" flag.
            // The simplest safe approach: always overwrite — it's cheap.
            conn->setMessageHandler([this](const std::string& msg) {
                try {
                    SystemStats stats = netwatch::deserialize(msg);
                    emit statsReceived(stats);      // safe cross-thread signal
                } catch (...) {
                    // Malformed message — ignore
                }
            });
            conn->setDisconnectHandler([this]() {
                emit agentConnected(false);
            });
        }
    });
    poll->start(500); // check for new connections every 500 ms

    // Start the server (begins async_accept loop)
    m_server->start();
    statusBar()->showMessage(
        QString("Listening for agents on port %1...").arg(netwatch::DEFAULT_PORT));

    // Run Boost.Asio on a background thread
    m_ioThread = std::thread([this]() {
        m_io->run();
    });
}

// ─── Style ───────────────────────────────────────────────────────────────────
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

// ─── UI Layout ───────────────────────────────────────────────────────────────
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

    auto* incidentGroup = new QGroupBox("RECENT INCIDENTS");
    auto* incidentLayout = new QVBoxLayout(incidentGroup);
    incidentHistoryWidget = new QListWidget();
    incidentHistoryWidget->setToolTip("Live records read from SQLite");
    incidentLayout->addWidget(incidentHistoryWidget);
    detailLayout->addWidget(incidentGroup);

    auto* analysisGroup = new QGroupBox("AI INCIDENT ANALYSIS");
    auto* analysisLayout = new QVBoxLayout(analysisGroup);

    analyzeButton = new QPushButton("Analyze Selected Device");
    analyzeButton->setEnabled(m_llmClient && m_llmClient->isConfigured());
    connect(analyzeButton, &QPushButton::clicked, this, &MainWindow::onAnalyzeClicked);

    analysisStatusLabel = new QLabel();
    if (m_llmClient && !m_llmClient->isConfigured()) {
        analysisStatusLabel->setText("LLM not configured. Set OPENAI_API_KEY.");
    } else {
        analysisStatusLabel->setText("Ready");
    }

    analysisView = new QTextEdit();
    analysisView->setReadOnly(true);
    analysisView->setPlaceholderText("Run an analysis to get AI triage recommendations.");

    analysisLayout->addWidget(analyzeButton);
    analysisLayout->addWidget(analysisStatusLabel);
    analysisLayout->addWidget(analysisView);
    detailLayout->addWidget(analysisGroup);

    mainSplitter->addWidget(sidebar);
    mainSplitter->addWidget(detailView);
    setCentralWidget(mainSplitter);

    connect(deviceListWidget, &QListWidget::currentTextChanged, this, [this](const QString& name){
        currentSelectedDevice = name;
        refreshDisplay();
    });

    refreshIncidentHistory();
}

// ─── Slot: new stats packet arrived ──────────────────────────────────────────
void MainWindow::onDataReceived(const SystemStats& stats) {
    if (isDemoMode) {
        allDevicesData.clear();
        deviceListWidget->clear();
        isDemoMode = false;
        this->setWindowTitle("NetWatch Central Command [LIVE]");
    }

    QString name = QString::fromStdString(stats.hostname);
    allDevicesData[name] = stats;
    if (m_dbManager && m_dbManager->isReady()) {
        const qint64 incidentId = m_dbManager->upsertIncident(stats);
        if (incidentId >= 0) {
            incidentIdsByDevice[name] = incidentId;
        }
    }

    QList<QListWidgetItem*> items = deviceListWidget->findItems(name, Qt::MatchExactly);
    
    if (items.isEmpty()) {
        QListWidgetItem* newItem = new QListWidgetItem(name, deviceListWidget);
        newItem->setToolTip(QString("IP: %1 | CPU: %2%")
            .arg(QString::fromStdString(stats.ip_address))
            .arg(stats.cpu_total));

        newItem->setForeground(stats.cpu_total > 90 ? Qt::red : QColor("#64ffda"));

        if (deviceListWidget->count() == 1) {
            deviceListWidget->setCurrentRow(0);
            currentSelectedDevice = name;
        }
    } else {
        items[0]->setForeground(stats.cpu_total > 90 ? Qt::red : QColor("#64ffda"));
        items[0]->setToolTip(QString("IP: %1 | CPU: %2%")
            .arg(QString::fromStdString(stats.ip_address))
            .arg(stats.cpu_total));
        emit agentConnected(true);
    }

    if (currentSelectedDevice == name)
        refreshDisplay();

    refreshIncidentHistory();
}

void MainWindow::refreshDisplay() {
    if (allDevicesData.find(currentSelectedDevice) == allDevicesData.end()) return;

    const auto& data = allDevicesData[currentSelectedDevice];
    cpuBar->setValue(static_cast<int>(data.cpu_total));
    ramBar->setValue(data.ram_total_gb > 0.0
        ? static_cast<int>((data.ram_used_gb / data.ram_total_gb) * 100)
        : 0);

    cpuBar->setStyleSheet(data.cpu_total > 80.0
        ? "QProgressBar::chunk { background-color: #ff5555; }"
        : "QProgressBar::chunk { background-color: #64ffda; }");

    infoLabel->setText(QString("IP: %1 | Uptime: %2 | RAM: %3 / %4 GB")
        .arg(QString::fromStdString(data.ip_address))
        .arg(QString::fromStdString(data.uptime))
        .arg(data.ram_used_gb, 0, 'f', 1)
        .arg(data.ram_total_gb, 0, 'f', 1));

    processTable->updateProcesses(data.processes);
}

void MainWindow::refreshIncidentHistory() {
    if (!incidentHistoryWidget || !m_dbManager || !m_dbManager->isReady()) return;

    incidentHistoryWidget->clear();
    const auto recent = m_dbManager->fetchRecentIncidents(15);
    for (const auto& incident : recent) {
        const double ramPercent = incident.ramTotalGb > 0.0
            ? (incident.ramUsedGb / incident.ramTotalGb) * 100.0
            : 0.0;
        const QString label = QString("%1 [%2] CPU %3% | RAM %4% | %5")
            .arg(incident.hostname)
            .arg(incident.severity.toUpper())
            .arg(incident.cpuTotal, 0, 'f', 1)
            .arg(ramPercent, 0, 'f', 1)
            .arg(incident.updatedAt);

        auto* item = new QListWidgetItem(label, incidentHistoryWidget);
        item->setToolTip(QString("ID: %1 | IP: %2 | Status: %3")
            .arg(incident.id)
            .arg(incident.ipAddress)
            .arg(incident.status));
        if (incident.severity == "critical") item->setForeground(Qt::red);
        else if (incident.severity == "high") item->setForeground(QColor("#ffa500"));
        else item->setForeground(QColor("#64ffda"));
    }
}

QGroupBox* MainWindow::createGaugeGroup(QString title, QProgressBar* bar) {
    auto* group = new QGroupBox(title);
    auto* layout = new QVBoxLayout(group);
    layout->addWidget(bar);
    return group;
}

// ─── Slot: connection status change ──────────────────────────────────────────
void MainWindow::onConnectionStatusChanged(bool connected) {
    if (connected) {
        statusBar()->showMessage("Agent connected", 3000);
        this->setWindowTitle("NetWatch Central Command [ONLINE]");
    } else {
        statusBar()->showMessage("Agent disconnected", 0);
        this->setWindowTitle("NetWatch Central Command [WAITING]");
    }
}

void MainWindow::onAnalyzeClicked() {
    if (!m_llmClient) return;
    if (!m_llmClient->isConfigured()) {
        onLlmAnalysisFailed(m_llmClient->configurationHint());
        return;
    }
    if (allDevicesData.find(currentSelectedDevice) == allDevicesData.end()) {
        onLlmAnalysisFailed("Select a device before requesting analysis.");
        return;
    }

    const auto& selected = allDevicesData[currentSelectedDevice];
    analyzeButton->setEnabled(false);
    analysisStatusLabel->setText("Analyzing...");
    analysisView->setPlainText("Waiting for LLM response...");
    m_llmClient->requestAnalysis(selected);
}

void MainWindow::onLlmAnalysisReady(const QString& analysis) {
    if (analyzeButton) analyzeButton->setEnabled(true);
    if (analysisStatusLabel) analysisStatusLabel->setText("Analysis complete");
    if (analysisView) analysisView->setPlainText(analysis);

    if (m_dbManager && m_dbManager->isReady()) {
        const qint64 incidentId = incidentIdsByDevice.value(currentSelectedDevice, -1);
        if (incidentId >= 0) {
            m_dbManager->saveAnalysis(incidentId, m_llmClient ? m_llmClient->modelName() : "unknown", analysis);
        }
    }
    refreshIncidentHistory();
}

void MainWindow::onLlmAnalysisFailed(const QString& errorMessage) {
    if (analyzeButton) analyzeButton->setEnabled(true);
    if (analysisStatusLabel) analysisStatusLabel->setText("Analysis failed");
    if (analysisView) analysisView->setPlainText(errorMessage);
}

// ─── Demo data ───────────────────────────────────────────────────────────────
void MainWindow::loadDemoData() {
    SystemStats pi;
    pi.hostname = "Server-01";
    pi.ip_address = "192.168.1.10";
    pi.cpu_total = 12.4;
    pi.ram_used_gb = 0.5;
    pi.ram_total_gb = 4.0;
    pi.uptime = "15d 04h 22m";
    pi.processes = { {101, "thermal_daemon", 1.2, 0.1}, {102, "python_script", 5.4, 0.3} };

    SystemStats work;
    work.hostname = "Workstation-X";
    work.ip_address = "192.168.1.55";
    work.cpu_total = 88.7;
    work.ram_used_gb = 14.5;
    work.ram_total_gb = 16.0;
    work.uptime = "01d 02h 15m";
    work.processes = { {501, "Visual Studio", 45.0, 4.2}, {505, "Chrome", 32.1, 2.8} };

    SystemStats laptop;
    laptop.hostname = "Youanes-Laptop";
    laptop.ip_address = "192.168.1.102";
    laptop.cpu_total = 35.0;
    laptop.ram_used_gb = 4.2;
    laptop.ram_total_gb = 8.0;
    laptop.uptime = "00d 18h 45m";
    laptop.processes = { {901, "Zoom", 20.0, 1.5}, {905, "Spotify", 5.0, 0.4} };

    for (auto& s : {pi, work, laptop}) {
        QString name = QString::fromStdString(s.hostname);
        allDevicesData[name] = s;
        deviceListWidget->addItem(name);
    }
    deviceListWidget->item(1)->setForeground(Qt::red);
    deviceListWidget->setCurrentRow(0);
    currentSelectedDevice = QString::fromStdString(pi.hostname);
    refreshDisplay();
}
