#include "db_manager.hpp"

#include "message_types.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QVariant>
#include <QtDebug>

DbManager::DbManager()
    : m_connectionName("netwatch_main")
{
    const QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!appDataPath.isEmpty()) {
        m_dbPath = QDir(appDataPath).filePath("netwatch.db");
    } else {
        m_dbPath = QDir(QCoreApplication::applicationDirPath()).filePath("netwatch.db");
    }
}

DbManager::~DbManager() {
    if (QSqlDatabase::contains(m_connectionName)) {
        {
            QSqlDatabase db = QSqlDatabase::database(m_connectionName);
            if (db.isOpen()) db.close();
        }
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool DbManager::initialize() {
    QDir dbDir(QFileInfo(m_dbPath).absolutePath());
    if (!dbDir.exists() && !dbDir.mkpath(".")) {
        qWarning() << "Failed to create DB directory:" << dbDir.path();
        return false;
    }

    QSqlDatabase db = QSqlDatabase::contains(m_connectionName)
        ? QSqlDatabase::database(m_connectionName)
        : QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    db.setDatabaseName(m_dbPath);

    if (!db.open()) {
        qWarning() << "Failed to open database:" << db.lastError().text();
        return false;
    }

    QSqlQuery pragma(db);
    pragma.exec("PRAGMA foreign_keys = ON;");

    return createSchema();
}

bool DbManager::isReady() const {
    if (!QSqlDatabase::contains(m_connectionName)) return false;
    return QSqlDatabase::database(m_connectionName).isOpen();
}

QString DbManager::databasePath() const {
    return m_dbPath;
}

qint64 DbManager::upsertIncident(const SystemStats& stats) {
    if (!isReady()) return -1;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);

    const QString severity = inferSeverity(stats);
    QSqlQuery upsert(db);
    upsert.prepare(
        "INSERT INTO incidents (hostname, ip_address, severity, status, cpu_total, ram_used_gb, ram_total_gb, uptime) "
        "VALUES (:hostname, :ip, :severity, 'open', :cpu, :ram_used, :ram_total, :uptime) "
        "ON CONFLICT(hostname) DO UPDATE SET "
        "ip_address = excluded.ip_address, "
        "severity = excluded.severity, "
        "cpu_total = excluded.cpu_total, "
        "ram_used_gb = excluded.ram_used_gb, "
        "ram_total_gb = excluded.ram_total_gb, "
        "uptime = excluded.uptime, "
        "updated_at = CURRENT_TIMESTAMP;"
    );
    upsert.bindValue(":hostname", QString::fromStdString(stats.hostname));
    upsert.bindValue(":ip", QString::fromStdString(stats.ip_address));
    upsert.bindValue(":severity", severity);
    upsert.bindValue(":cpu", stats.cpu_total);
    upsert.bindValue(":ram_used", stats.ram_used_gb);
    upsert.bindValue(":ram_total", stats.ram_total_gb);
    upsert.bindValue(":uptime", QString::fromStdString(stats.uptime));

    if (!upsert.exec()) {
        qWarning() << "Failed to upsert incident:" << upsert.lastError().text();
        return -1;
    }

    QSqlQuery select(db);
    select.prepare("SELECT id FROM incidents WHERE hostname = :hostname;");
    select.bindValue(":hostname", QString::fromStdString(stats.hostname));
    if (!select.exec() || !select.next()) {
        qWarning() << "Failed to fetch incident id:" << select.lastError().text();
        return -1;
    }

    return select.value(0).toLongLong();
}

bool DbManager::saveAnalysis(qint64 incidentId, const QString& model, const QString& analysisText) {
    if (!isReady() || incidentId < 0) return false;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);

    QSqlQuery insert(db);
    insert.prepare(
        "INSERT INTO ai_analyses (incident_id, model, analysis_text) "
        "VALUES (:incident_id, :model, :analysis_text);"
    );
    insert.bindValue(":incident_id", incidentId);
    insert.bindValue(":model", model);
    insert.bindValue(":analysis_text", analysisText);
    if (!insert.exec()) {
        qWarning() << "Failed to save AI analysis:" << insert.lastError().text();
        return false;
    }
    return true;
}

QList<IncidentRecord> DbManager::fetchRecentIncidents(int limit) const {
    QList<IncidentRecord> items;
    if (!isReady()) return items;

    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);
    query.prepare(
        "SELECT id, hostname, ip_address, severity, status, cpu_total, ram_used_gb, ram_total_gb, updated_at "
        "FROM incidents ORDER BY updated_at DESC LIMIT :limit;"
    );
    query.bindValue(":limit", limit);
    if (!query.exec()) {
        qWarning() << "Failed to fetch incidents:" << query.lastError().text();
        return items;
    }

    while (query.next()) {
        IncidentRecord item;
        item.id = query.value(0).toLongLong();
        item.hostname = query.value(1).toString();
        item.ipAddress = query.value(2).toString();
        item.severity = query.value(3).toString();
        item.status = query.value(4).toString();
        item.cpuTotal = query.value(5).toDouble();
        item.ramUsedGb = query.value(6).toDouble();
        item.ramTotalGb = query.value(7).toDouble();
        item.updatedAt = query.value(8).toString();
        items.push_back(item);
    }
    return items;
}

bool DbManager::updateIncidentStatus(qint64 incidentId, const QString& newStatus) {
    if (!isReady() || incidentId < 0) return false;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery update(db);
    update.prepare(
        "UPDATE incidents SET status = :status, updated_at = CURRENT_TIMESTAMP WHERE id = :id;"
    );
    update.bindValue(":status", newStatus);
    update.bindValue(":id", incidentId);
    if (!update.exec()) {
        qWarning() << "Failed to update incident status:" << update.lastError().text();
        return false;
    }
    return true;
}

bool DbManager::deleteIncident(qint64 incidentId) {
    if (!isReady() || incidentId < 0) return false;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery del(db);
    del.prepare("DELETE FROM incidents WHERE id = :id;");
    del.bindValue(":id", incidentId);
    if (!del.exec()) {
        qWarning() << "Failed to delete incident:" << del.lastError().text();
        return false;
    }
    return true;
}

bool DbManager::createSchema() {
    if (!isReady()) return false;
    QSqlDatabase db = QSqlDatabase::database(m_connectionName);
    QSqlQuery query(db);

    const char* incidentsSql =
        "CREATE TABLE IF NOT EXISTS incidents ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "hostname TEXT NOT NULL UNIQUE,"
        "ip_address TEXT NOT NULL,"
        "severity TEXT NOT NULL,"
        "status TEXT NOT NULL DEFAULT 'open',"
        "cpu_total REAL NOT NULL,"
        "ram_used_gb REAL NOT NULL,"
        "ram_total_gb REAL NOT NULL,"
        "uptime TEXT,"
        "created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP"
        ");";

    const char* analysesSql =
        "CREATE TABLE IF NOT EXISTS ai_analyses ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "incident_id INTEGER NOT NULL,"
        "model TEXT NOT NULL,"
        "analysis_text TEXT NOT NULL,"
        "created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY(incident_id) REFERENCES incidents(id) ON DELETE CASCADE"
        ");";

    const char* incidentIndexUpdatedAtSql =
        "CREATE INDEX IF NOT EXISTS idx_incidents_updated_at ON incidents(updated_at);";
    const char* incidentIndexSeveritySql =
        "CREATE INDEX IF NOT EXISTS idx_incidents_severity ON incidents(severity);";
    const char* incidentIndexStatusSql =
        "CREATE INDEX IF NOT EXISTS idx_incidents_status ON incidents(status);";

    if (!query.exec(incidentsSql)) {
        qWarning() << "Failed to create incidents table:" << query.lastError().text();
        return false;
    }
    if (!query.exec(analysesSql)) {
        qWarning() << "Failed to create ai_analyses table:" << query.lastError().text();
        return false;
    }
    if (!query.exec(incidentIndexUpdatedAtSql) ||
        !query.exec(incidentIndexSeveritySql) ||
        !query.exec(incidentIndexStatusSql)) {
        qWarning() << "Failed to create incident indexes:" << query.lastError().text();
        return false;
    }
    return true;
}

QString DbManager::inferSeverity(const SystemStats& stats) const {
    const double ramPercent = stats.ram_total_gb > 0.0
        ? (stats.ram_used_gb / stats.ram_total_gb) * 100.0
        : 0.0;

    if (stats.cpu_total >= 90.0 || ramPercent >= 95.0) return "critical";
    if (stats.cpu_total >= 75.0 || ramPercent >= 85.0) return "high";
    if (stats.cpu_total >= 50.0 || ramPercent >= 70.0) return "medium";
    return "low";
}
