#pragma once
#include <QMainWindow>
#include <QProgressBar>
#include <QLabel>
#include <QListWidget>
#include <QMap>
#include <QGroupBox>
#include "widgets/process_table.hpp"
#include "../networking/include/client.hpp"
#include "message_types.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void onDataReceived(const SystemStats& stats);
    void onConnectionStatusChanged(bool connected);

private:
    void setupUI();
    void setupStyles();
    void refreshDisplay();
    bool isDemoMode = true;
    void loadDemoData();
    QGroupBox* createGaugeGroup(QString title, QProgressBar* bar);
    QProgressBar *cpuBar;
    QProgressBar *ramBar;
    QLabel *infoLabel;
    QLabel *statusLabel;
    QListWidget *deviceListWidget;
    ProcessTable *processTable;

    QMap<QString, SystemStats> allDevicesData;
    QString currentSelectedDevice;
};
