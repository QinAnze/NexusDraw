#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>

/// Handles communication with OpenAI-format LLM APIs.
/// Sends chat completion requests and parses streaming/non-streaming responses.
class AIManager : public QObject
{
    Q_OBJECT

public:
    explicit AIManager(QObject* parent = nullptr);
    ~AIManager() = default;

    /// Send a chat completion request to the LLM.
    /// @param userMessage  The user's natural language request
    /// @param datasetInfo  JSON object describing the dataset (columns, types, sample)
    /// @param language     "python" or "r"
    /// @param chatMode     If true, skip code extraction and return raw text
    void sendRequest(const QString& userMessage, const QJsonObject& datasetInfo,
                     const QString& language, bool chatMode = false);

    /// Cancel the current request if one is in-flight
    void cancelRequest();

    /// Returns true if a request is currently in progress
    bool isBusy() const;

signals:
    /// Emitted when the LLM returns code successfully (plot mode)
    void codeGenerated(const QString& code, const QString& language);

    /// Emitted when the LLM returns a chat response (chat mode, no code extraction)
    void chatResponse(const QString& text);

    /// Emitted when an error occurs during API communication
    void errorOccurred(const QString& errorMessage);

    /// Emitted for progress/status updates
    void statusMessage(const QString& message);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    /// Build the full API endpoint URL from the base URL
    QString buildApiUrl(const QString& baseUrl) const;

    /// Build the JSON request body
    QJsonObject buildRequestBody(const QString& userMessage, const QJsonObject& datasetInfo,
                                  const QString& language) const;

    /// Extract code from the LLM's response text (between ``` markers)
    QString extractCodeBlock(const QString& responseText, const QString& language) const;

    QNetworkAccessManager* m_networkManager;
    QNetworkReply* m_activeReply = nullptr;
    QString m_expectedLanguage;
    bool m_chatMode = false;
};
