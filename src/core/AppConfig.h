#pragma once

#include <QString>
#include <QSettings>
#include <QJsonObject>

/// Manages persistent application configuration via QSettings.
/// Stores LLM connection parameters and user preferences.
class AppConfig
{
public:
    static AppConfig& instance();

    // LLM Configuration
    QString aiBaseURL() const;
    void setAiBaseURL(const QString& url);

    QString aiApiKey() const;
    void setAiApiKey(const QString& key);

    QString aiModel() const;
    void setAiModel(const QString& model);

    // Code Execution
    QString pythonPath() const;
    void setPythonPath(const QString& path);

    /// Auto-detect the bundled Python environment path
    QString bundledPythonPath() const;

    QString rPath() const;
    void setRPath(const QString& path);
    QString rHomePath() const;

    QString preferredLanguage() const;  // "python" or "r"
    void setPreferredLanguage(const QString& lang);

    // Appearance
    bool darkTheme() const;
    void setDarkTheme(bool dark);

    // Load / Save
    void save();
    void load();

    // Build system prompt for the LLM with dataset context
    QString buildSystemPrompt(const QJsonObject& datasetInfo, const QString& language,
                               bool chatMode = false) const;

private:
    AppConfig();
    ~AppConfig() = default;
    AppConfig(const AppConfig&) = delete;
    AppConfig& operator=(const AppConfig&) = delete;

    QSettings m_settings;
    QString m_baseURL;
    QString m_apiKey;
    QString m_model;
    QString m_pythonPath;
    QString m_rPath;
    QString m_preferredLang;
    bool m_darkTheme;
};
