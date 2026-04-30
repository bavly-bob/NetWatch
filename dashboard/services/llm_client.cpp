#include "llm_client.hpp"

#include "message_types.hpp"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QUrl>
#include <algorithm>

LlmClient::LlmClient(QObject* parent)
    : QObject(parent),
      m_network(new QNetworkAccessManager(this)),
      m_apiKey(qEnvironmentVariable("OPENAI_API_KEY")),
      m_endpoint(qEnvironmentVariable("NETWATCH_LLM_ENDPOINT", "https://api.openai.com/v1/chat/completions")),
      m_model(qEnvironmentVariable("NETWATCH_LLM_MODEL", "gpt-4o-mini"))
{
}

bool LlmClient::isConfigured() const {
    return !m_apiKey.trimmed().isEmpty();
}

QString LlmClient::configurationHint() const {
    return QString("Set OPENAI_API_KEY. Optional: NETWATCH_LLM_MODEL, NETWATCH_LLM_ENDPOINT.");
}

void LlmClient::requestAnalysis(const SystemStats& stats) {
    if (!isConfigured()) {
        emit analysisFailed(configurationHint());
        return;
    }

    QNetworkRequest request{QUrl(m_endpoint)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());

    const QString systemInstruction =
        "You are a senior network incident analyst. Given system metrics and processes, "
        "return a concise analysis with: (1) severity, (2) likely cause, (3) immediate actions.";
    const QString userPrompt = buildPrompt(stats);

    QJsonObject payload{
        {"model", m_model},
        {"temperature", 0.2},
        {"max_tokens", 300},
        {"messages", QJsonArray{
            QJsonObject{{"role", "system"}, {"content", systemInstruction}},
            QJsonObject{{"role", "user"}, {"content", userPrompt}}
        }}
    };

    QNetworkReply* reply = m_network->post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray body = reply->readAll();
        const auto error = reply->error();
        const QString errorString = reply->errorString();
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        reply->deleteLater();

        if (error != QNetworkReply::NoError) {
            emit analysisFailed(QString("LLM request failed (%1): %2").arg(statusCode).arg(errorString));
            return;
        }

        const QString analysis = parseAnalysisFromResponse(body);
        if (analysis.trimmed().isEmpty()) {
            emit analysisFailed("LLM response was empty or could not be parsed.");
            return;
        }

        emit analysisReady(analysis);
    });
}

QString LlmClient::buildPrompt(const SystemStats& stats) const {
    QStringList topProcesses;
    const std::size_t maxProcesses = std::min<std::size_t>(stats.processes.size(), 5);
    for (std::size_t i = 0; i < maxProcesses; ++i) {
        const auto& p = stats.processes[i];
        topProcesses << QString("%1 (pid=%2, cpu=%3%%, mem=%4 MB)")
                            .arg(QString::fromStdString(p.name))
                            .arg(p.pid)
                            .arg(p.cpu_usage, 0, 'f', 1)
                            .arg(p.mem_usage, 0, 'f', 1);
    }

    const double ramPercent = stats.ram_total_gb > 0.0
        ? (stats.ram_used_gb / stats.ram_total_gb) * 100.0
        : 0.0;

    return QString(
        "Hostname: %1\n"
        "IP: %2\n"
        "CPU total: %3%%\n"
        "RAM used: %4 / %5 GB (%6%%)\n"
        "Uptime: %7\n"
        "Top processes: %8\n"
        "Provide a concise incident analysis.")
        .arg(QString::fromStdString(stats.hostname))
        .arg(QString::fromStdString(stats.ip_address))
        .arg(stats.cpu_total, 0, 'f', 1)
        .arg(stats.ram_used_gb, 0, 'f', 1)
        .arg(stats.ram_total_gb, 0, 'f', 1)
        .arg(ramPercent, 0, 'f', 1)
        .arg(QString::fromStdString(stats.uptime))
        .arg(topProcesses.join("; "));
}

QString LlmClient::parseAnalysisFromResponse(const QByteArray& responseBody) const {
    const QJsonDocument document = QJsonDocument::fromJson(responseBody);
    if (!document.isObject()) return {};

    const QJsonObject root = document.object();
    const QJsonArray choices = root.value("choices").toArray();
    if (choices.isEmpty()) return {};

    const QJsonObject firstChoice = choices.first().toObject();
    const QJsonObject message = firstChoice.value("message").toObject();
    return message.value("content").toString().trimmed();
}
