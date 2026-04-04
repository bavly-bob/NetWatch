#pragma once
#include <QMainWindow>
#include <QProgressBar>
#include <QLabel>
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
    
    QProgressBar *cpuBar;
    QProgressBar *ramBar;
    QLabel *uptimeLabel;
    QLabel *statusLabel;
    ProcessTable *processTable;
};
