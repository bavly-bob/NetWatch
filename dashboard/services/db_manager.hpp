#pragma once

#include <QList>
#include <QString>
#include <QtGlobal>

struct SystemStats;

struct IncidentRecord {
    qint64 id = -1;
    QString hostname;
    QString ipAddress;
    QString severity;
    QString status;
    double cpuTotal = 0.0;
    double ramUsedGb = 0.0;
    double ramTotalGb = 0.0;
    QString updatedAt;
};

class DbManager {
public:
    DbManager();
    ~DbManager();

    bool initialize();
    bool isReady() const;
    QString databasePath() const;

    qint64 upsertIncident(const SystemStats& stats);
    bool saveAnalysis(qint64 incidentId, const QString& model, const QString& analysisText);
    QList<IncidentRecord> fetchRecentIncidents(int limit = 20) const;

    bool updateIncidentStatus(qint64 incidentId, const QString& newStatus);
    bool deleteIncident(qint64 incidentId);

private:
    bool createSchema();
    QString inferSeverity(const SystemStats& stats) const;

private:
    QString m_connectionName;
    QString m_dbPath;
};
