#include "process_table.hpp"
#include <QHeaderView>

ProcessTable::ProcessTable(QWidget *parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    
    searchBar = new QLineEdit();
    searchBar->setPlaceholderText("Search processes...");
    
    table = new QTableWidget(0, 3);
    table->setHorizontalHeaderLabels({"PID", "Name", "CPU %"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setSortingEnabled(true);

    layout->addWidget(searchBar);
    layout->addWidget(table);

    connect(searchBar, &QLineEdit::textChanged, this, &ProcessTable::filterTable);
}

void ProcessTable::updateProcesses(const std::vector<ProcessInfo>& processes) {
    table->setSortingEnabled(false);
    table->setRowCount(0);
    for (const auto& proc : processes) {
        int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(QString::number(proc.pid)));
        table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(proc.name)));
        table->setItem(row, 2, new QTableWidgetItem(QString::number(proc.cpu_usage, 'f', 1) + "%"));
    }
    table->setSortingEnabled(true);
}

void ProcessTable::filterTable(const QString &text) {
    for (int i = 0; i < table->rowCount(); ++i) {
        bool match = table->item(i, 1)->text().contains(text, Qt::CaseInsensitive);
        table->setRowHidden(i, !match);
    }
}
