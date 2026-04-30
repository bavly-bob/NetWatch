#pragma once
#include <thread>
#include <memory>
#include <QMainWindow>
#include <QProgressBar>
#include <QLabel>
#include <QListWidget>
#include <QMap>
#include <QGroupBox>
#include <QTimer>
#include <QPushButton>
#include <QTextEdit>

#include "widgets/process_table.hpp"
#include "message_types.hpp"
#include "services/llm_client.hpp"
#include "services/db_manager.hpp"

// Forward-declare so we don't pull Boost headers into every Qt translation unit
namespace netwatch::networking { class Server; }
namespace boost::asio { class io_context; }

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    // Emitted from the networking thread; connected to the slot below via Qt::QueuedConnection
    void statsReceived(SystemStats stats);
    void agentConnected(bool connected);

private slots:
    void onDataReceived(const SystemStats& stats);
    void onConnectionStatusChanged(bool connected);
    void onAnalyzeClicked();
    void onLlmAnalysisReady(const QString& analysis);
    void onLlmAnalysisFailed(const QString& errorMessage);

private:
    void setupUI();
    void setupStyles();
    void refreshDisplay();
    void refreshIncidentHistory();
    void loadDemoData();
    void startServer();
    QGroupBox* createGaugeGroup(QString title, QProgressBar* bar);

    // UI widgets
    QProgressBar *cpuBar  = nullptr;
    QProgressBar *ramBar  = nullptr;
    QLabel       *infoLabel   = nullptr;
    QLabel       *statusLabel = nullptr;
    QListWidget  *deviceListWidget = nullptr;
    ProcessTable *processTable     = nullptr;
    QPushButton  *analyzeButton    = nullptr;
    QTextEdit    *analysisView     = nullptr;
    QLabel       *analysisStatusLabel = nullptr;
    QListWidget  *incidentHistoryWidget = nullptr;

    // Data
    QMap<QString, SystemStats> allDevicesData;
    QMap<QString, qint64> incidentIdsByDevice;
    QString currentSelectedDevice;
    bool    isDemoMode = true;

    // Networking (owned by MainWindow so they outlive the window)
    std::shared_ptr<boost::asio::io_context>        m_io;
    std::shared_ptr<netwatch::networking::Server>   m_server;
    std::thread m_ioThread;
    LlmClient* m_llmClient = nullptr;
    DbManager* m_dbManager = nullptr;
};
