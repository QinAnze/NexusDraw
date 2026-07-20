#include "AIManager.h"
#include "AppConfig.h"

#include <QJsonDocument>
#include <QNetworkRequest>
#include <QRegularExpression>

AIManager::AIManager(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &AIManager::onReplyFinished);
}

bool AIManager::isBusy() const
{
    return m_activeReply != nullptr;
}

QString AIManager::buildApiUrl(const QString& baseUrl) const
{
    QString url = baseUrl;
    // Remove trailing slash
    while (url.endsWith('/')) url.chop(1);

    // If URL already ends with /chat/completions, use as-is
    if (url.endsWith("/chat/completions")) return url;

    // If URL ends with /v1, append /chat/completions
    if (url.endsWith("/v1")) return url + "/chat/completions";

    // Otherwise append /v1/chat/completions
    return url + "/v1/chat/completions";
}

QJsonObject AIManager::buildRequestBody(const QString& userMessage,
                                         const QJsonObject& datasetInfo,
                                         const QString& language) const
{
    const AppConfig& cfg = AppConfig::instance();
    QString systemPrompt = cfg.buildSystemPrompt(datasetInfo, language, m_chatMode);

    // Build user message
    QString fullUserMessage = userMessage;
    if (!m_chatMode) {
        fullUserMessage += QString("\n\n[DATASET_PATH: %1]\n[OUTPUT_PATH: %2]")
            .arg(datasetInfo["filePath"].toString(),
                 datasetInfo["outputPath"].toString());
    }

    QJsonArray messages;
    QJsonObject sysMsg;
    sysMsg["role"] = QString("system");
    sysMsg["content"] = systemPrompt;
    messages.append(sysMsg);

    QJsonObject userMsg;
    userMsg["role"] = QString("user");
    userMsg["content"] = fullUserMessage;
    messages.append(userMsg);

    QJsonObject body;
    body["model"] = cfg.aiModel();
    body["messages"] = messages;
    body["temperature"] = 0.3;
    body["max_tokens"] = 4096;

    return body;
}

void AIManager::sendRequest(const QString& userMessage, const QJsonObject& datasetInfo,
                             const QString& language, bool chatMode)
{
    if (isBusy()) {
        emit errorOccurred(tr("A request is already in progress. Please wait."));
        return;
    }

    m_chatMode = chatMode;
    const AppConfig& cfg = AppConfig::instance();

    if (cfg.aiApiKey().isEmpty()) {
        emit errorOccurred(tr("API Key is not configured. Please set it in AI Settings."));
        return;
    }

    m_expectedLanguage = language;

    QString apiUrl = buildApiUrl(cfg.aiBaseURL());
    QJsonObject body = buildRequestBody(userMessage, datasetInfo, language);
    QJsonDocument doc(body);

    QUrl url(apiUrl);
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(cfg.aiApiKey()).toUtf8());
    request.setRawHeader("HTTP-Referer", "NexusDraw/1.0");
    request.setTransferTimeout(120000);  // 2 minute timeout

    emit statusMessage(tr("Sending request to %1...").arg(cfg.aiModel()));

    m_activeReply = m_networkManager->post(request, doc.toJson());
}

void AIManager::fixCode(const QString& brokenCode, const QString& error, const QString& language)
{
    if (isBusy()) return;

    const AppConfig& cfg = AppConfig::instance();
    m_expectedLanguage = language;

    // Build a fix-it prompt demanding complete code
    QString prompt = QString(
        "The following %1 code failed. Return the COMPLETE fixed code (not just the fix).\n\n"
        "Error:\n%2\n\n"
        "Broken code:\n```%1\n%3\n```\n\n"
        "CRITICAL: Return ONLY the complete corrected code in a ```%1 block.\n"
        "Do NOT explain the fix. Do NOT return a diff or partial code. "
        "The code MUST be complete and runnable.\n"
    ).arg(language, error, brokenCode);

    QJsonArray messages;
    QJsonObject userMsg;
    userMsg["role"] = QString("user");
    userMsg["content"] = prompt;
    messages.append(userMsg);

    QJsonObject body;
    body["model"] = cfg.aiModel();
    body["messages"] = messages;
    body["temperature"] = 0.1;
    body["max_tokens"] = 4096;

    QJsonDocument doc(body);
    QString apiUrl = buildApiUrl(cfg.aiBaseURL());
    QUrl url(apiUrl);
    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(cfg.aiApiKey()).toUtf8());
    request.setTransferTimeout(120000);

    m_activeReply = m_networkManager->post(request, doc.toJson());
}

void AIManager::cancelRequest()
{
    if (m_activeReply) {
        m_activeReply->abort();
        m_activeReply->deleteLater();
        m_activeReply = nullptr;
        emit statusMessage(tr("Request cancelled."));
    }
}

void AIManager::onReplyFinished(QNetworkReply* reply)
{
    if (reply != m_activeReply) {
        reply->deleteLater();
        return;
    }

    m_activeReply = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        QString errMsg;
        if (reply->error() == QNetworkReply::OperationCanceledError) {
            errMsg = tr("Request was cancelled.");
        } else if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
            errMsg = tr("Authentication failed. Please check your API Key.");
        } else if (reply->error() == QNetworkReply::TimeoutError) {
            errMsg = tr("Request timed out. Please try again.");
        } else {
            // Try to get error details from response body
            QByteArray respBody = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(respBody);
            if (doc.isObject() && doc.object().contains("error")) {
                QJsonObject err = doc.object()["error"].toObject();
                errMsg = err["message"].toString();
            } else {
                errMsg = reply->errorString();
            }
        }
        emit errorOccurred(errMsg);
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (!doc.isObject()) {
        emit errorOccurred(tr("Invalid response from API."));
        return;
    }

    QJsonObject responseObj = doc.object();
    QJsonArray choices = responseObj["choices"].toArray();

    if (choices.isEmpty()) {
        emit errorOccurred(tr("API returned no choices in the response."));
        return;
    }

    QString content = choices[0].toObject()["message"].toObject()["content"].toString();

    if (content.isEmpty()) {
        emit errorOccurred(tr("API returned empty content."));
        return;
    }

    // Chat mode: return raw text directly, no code extraction
    if (m_chatMode) {
        emit statusMessage(tr("Chat response received."));
        emit chatResponse(content.trimmed());
        return;
    }

    // Plot mode: extract code block from the response
    QString code = extractCodeBlock(content, m_expectedLanguage);

    if (code.isEmpty()) {
        // If no code block found, maybe the entire response is code
        // Let's check if it looks like code
        if (content.contains("import ") || content.contains("library(") ||
            content.contains("plt.") || content.contains("ggplot")) {
            code = content;
        } else {
            emit errorOccurred(tr("Could not extract code from the AI response.\n\nResponse:\n%1")
                .arg(content.left(500)));
            return;
        }
    }

    emit statusMessage(tr("Code generated successfully!"));
    emit codeGenerated(code.trimmed(), m_expectedLanguage);
}

QString AIManager::extractCodeBlock(const QString& responseText, const QString& language) const
{
    // Try to find a code block with the specific language marker
    QString pattern = QString("```%1\\s*\\n(.*?)```").arg(language);
    QRegularExpression re(pattern, QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = re.match(responseText);

    if (match.hasMatch()) {
        return match.captured(1).trimmed();
    }

    // Try generic code block (no language marker)
    QRegularExpression reGeneric("```\\s*\\n(.*?)```",
                                  QRegularExpression::DotMatchesEverythingOption);
    match = reGeneric.match(responseText);
    if (match.hasMatch()) {
        return match.captured(1).trimmed();
    }

    // No code block found
    return QString();
}
