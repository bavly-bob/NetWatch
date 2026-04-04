#pragma once
#include <QTableWidget>
#include <QLineEdit>
#include <QVBoxLayout>
#include <vector>
#include "message_types.hpp"
class ProcessTable : public QWidget {
    Q_OBJECT
public:
    explicit ProcessTable(QWidget *parent = nullptr);
    void updateProcesses(const std::vector<ProcessInfo>& processes);

private slots:
    void filterTable(const QString &text);

private:
    QLineEdit* searchBar;
    QTableWidget* table;
};
