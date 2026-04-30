#pragma once

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QByteArray;

struct SystemStats;

class LlmClient : public QObject {
    Q_OBJECT
public:
    explicit LlmClient(QObject* parent = nullptr);

    bool isConfigured() const;
    QString configurationHint() const;
    QString modelName() const;

    void requestAnalysis(const SystemStats& stats);

signals:
    void analysisReady(const QString& analysis);
    void analysisFailed(const QString& errorMessage);

private:
    QString buildPrompt(const SystemStats& stats) const;
    QString parseAnalysisFromResponse(const QByteArray& responseBody) const;

private:
    QNetworkAccessManager* m_network = nullptr;
    QString m_apiKey;
    QString m_endpoint;
    QString m_model;
};
