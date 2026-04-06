#pragma once
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

class DeviceCard : public QFrame {
    Q_OBJECT
public:
    explicit DeviceCard(QString name, QWidget *parent = nullptr);
    void updateStatus(double cpu, bool online);
    QString getName() const { return deviceName; }

private:
    QString deviceName;
    QLabel* nameLabel;
    QLabel* statusLabel;
};
