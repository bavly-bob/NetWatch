#include "device_card.hpp"

DeviceCard::DeviceCard(QString name, QWidget *parent) 
    : QFrame(parent), deviceName(name) 
{
    // Make it look like a nice card
    setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    setLineWidth(1);
    
    auto* layout = new QVBoxLayout(this);

    nameLabel = new QLabel(name);
    nameLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #64ffda;");

    statusLabel = new QLabel("Connecting...");
    statusLabel->setStyleSheet("font-size: 11px; color: #8892b0;");

    layout->addWidget(nameLabel);
    layout->addWidget(statusLabel);
}

void DeviceCard::updateStatus(double cpu, bool online) {
    if (online) {
        statusLabel->setText(QString("CPU Usage: %1%").arg(cpu, 0, 'f', 1));
        statusLabel->setStyleSheet("color: #8892b0;");
    } else {
        statusLabel->setText("OFFLINE");
        statusLabel->setStyleSheet("color: #ff5555; font-weight: bold;");
    }
}
