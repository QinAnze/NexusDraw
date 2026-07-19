#include "AppConfig.h"
#include <QJsonArray>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

AppConfig& AppConfig::instance()
{
    static AppConfig config;
    return config;
}

AppConfig::AppConfig()
    : m_settings("NexusDraw", "NexusDraw")
    , m_darkTheme(true)
    , m_preferredLang("python")
{
    load();
}

// --- LLM Configuration ---

QString AppConfig::aiBaseURL() const { return m_baseURL; }
void AppConfig::setAiBaseURL(const QString& url) { m_baseURL = url; }

QString AppConfig::aiApiKey() const { return m_apiKey; }
void AppConfig::setAiApiKey(const QString& key) { m_apiKey = key; }

QString AppConfig::aiModel() const { return m_model; }
void AppConfig::setAiModel(const QString& model) { m_model = model; }

// --- Code Execution ---

QString AppConfig::bundledPythonPath() const
{
    // Use applicationFilePath for reliable exe location
    QString exePath = QCoreApplication::applicationFilePath();
    exePath.replace('\\', '/');
    int lastSlash = exePath.lastIndexOf('/');
    QString exeDir = exePath.left(lastSlash);

    // Try from exe dir (level 0: same dir; level 1-3: parent dirs)
    for (int level = 0; level <= 3; ++level) {
        QString base = exeDir;
        for (int i = 0; i < level; ++i) {
            int slash = base.lastIndexOf('/');
            if (slash < 0) break;
            base = base.left(slash);
        }
        QString py = base + "/portable_python/python.exe";
        if (QFile::exists(py)) return QDir::toNativeSeparators(py);
    }

    return QString();
}

QString AppConfig::pythonPath() const
{
    // 1. User-configured path if explicitly set
    if (!m_pythonPath.isEmpty()) {
        return m_pythonPath;
    }
    // 2. Auto-detect bundled Python environment
    QString bundled = bundledPythonPath();
    if (!bundled.isEmpty()) {
        return bundled;
    }
    // 3. Fall back to system PATH
    return "python";
}

void AppConfig::setPythonPath(const QString& path) { m_pythonPath = path; }

QString AppConfig::rPath() const
{
    if (!m_rPath.isEmpty()) return m_rPath;

    // First: use applicationFilePath to find exe location reliably
    QString exePath = QCoreApplication::applicationFilePath();
    exePath.replace('\\', '/');
    int lastSlash = exePath.lastIndexOf('/');
    QString exeDir = exePath.left(lastSlash);

    // Try from exe dir (level 0: same dir; level 1-3: parent dirs)
    for (int level = 0; level <= 3; ++level) {
        QString base = exeDir;
        for (int i = 0; i < level; ++i) {
            int slash = base.lastIndexOf('/');
            if (slash < 0) break;
            base = base.left(slash);
        }
        QString rp = base + "/portable_r/bin/Rscript.exe";
        if (QFile::exists(rp)) return QDir::toNativeSeparators(rp);
    }

    return "Rscript";
}

/// Get the R home directory (parent of bin/Rscript.exe)
QString AppConfig::rHomePath() const
{
    QString rp = rPath();
    if (rp == "Rscript" || rp.isEmpty()) return QString();
    QFileInfo fi(rp);
    QDir binDir = fi.absoluteDir();     // bin/
    binDir.cdUp();                       // R home
    return binDir.absolutePath();
}

void AppConfig::setRPath(const QString& path) { m_rPath = path; }

QString AppConfig::preferredLanguage() const { return m_preferredLang; }
void AppConfig::setPreferredLanguage(const QString& lang) { m_preferredLang = lang; }

// --- Appearance ---

bool AppConfig::darkTheme() const { return m_darkTheme; }
void AppConfig::setDarkTheme(bool dark) { m_darkTheme = dark; }

// --- Load / Save ---

void AppConfig::save()
{
    m_settings.setValue("ai/baseURL", m_baseURL);
    m_settings.setValue("ai/apiKey", m_apiKey);
    m_settings.setValue("ai/model", m_model);
    m_settings.setValue("exec/pythonPath", m_pythonPath);
    m_settings.setValue("exec/rPath", m_rPath);
    m_settings.setValue("exec/preferredLang", m_preferredLang);
    m_settings.setValue("ui/darkTheme", m_darkTheme);
    m_settings.sync();
}

void AppConfig::load()
{
    m_baseURL = m_settings.value("ai/baseURL", "https://api.openai.com").toString();
    m_apiKey = m_settings.value("ai/apiKey", "").toString();
    m_model = m_settings.value("ai/model", "gpt-4o").toString();
    m_pythonPath = m_settings.value("exec/pythonPath", "").toString();
    m_rPath = m_settings.value("exec/rPath", "").toString();
    m_preferredLang = m_settings.value("exec/preferredLang", "python").toString();
    m_darkTheme = m_settings.value("ui/darkTheme", true).toBool();
}

QString AppConfig::buildSystemPrompt(const QJsonObject& datasetInfo, const QString& language,
                                       bool chatMode) const
{
    // Chat mode: simple conversational prompt, no code generation
    if (chatMode) {
        QString prompt;
        prompt += "You are a knowledgeable data science expert. ";
        prompt += "You help users understand data analysis concepts, visualization methods, ";
        prompt += "statistical techniques, and best practices. ";
        prompt += "You are familiar with Python (pandas, matplotlib, seaborn, numpy) and R (ggplot2, dplyr).\n\n";
        prompt += "Rules:\n";
        prompt += "- Answer conversationally in the user's language.\n";
        prompt += "- Explain concepts clearly with examples when helpful.\n";
        prompt += "- Do NOT generate code scripts or markdown code blocks.\n";
        prompt += "- If the user wants a plot, ask them to switch to plotting mode.\n";
        prompt += "- Keep responses concise and helpful.\n";
        return prompt;
    }

    // Plot mode: full skill-based prompt
    // Load universal skill (always included)
    QFile universalFile(":/skills/universal.md");
    QString universalSkill;
    if (universalFile.open(QFile::ReadOnly | QFile::Text)) {
        universalSkill = QString::fromUtf8(universalFile.readAll());
        universalFile.close();
    }

    // Load language-specific skill
    QString langSkill;
    QString skillPath = (language == "python") ? ":/skills/py.md" : ":/skills/r.md";
    QFile langFile(skillPath);
    if (langFile.open(QFile::ReadOnly | QFile::Text)) {
        langSkill = QString::fromUtf8(langFile.readAll());
        langFile.close();
    }

    QString prompt;
    prompt += "## Workflow\n";
    prompt += "1. Analyze the dataset columns, types, and sample data below.\n";
    prompt += "2. Determine what type of plot the user is requesting.\n";
    prompt += "3. Identify the research domain (biology/physics/medicine/finance/engineering/social-science).\n";
    prompt += "4. Apply the domain-specific standards from the skill reference below.\n";
    prompt += "5. Generate the code.\n\n";
    prompt += universalSkill;
    prompt += "\n\n";
    prompt += langSkill;
    prompt += "\n\n";

    // Color scheme (if selected)
    QString colorScheme = datasetInfo["colorScheme"].toString();
    if (!colorScheme.isEmpty()) {
        QString colorPath = QString(":/skills/colors/%1.md").arg(colorScheme);
        QFile colorFile(colorPath);
        if (colorFile.open(QFile::ReadOnly | QFile::Text)) {
            prompt += QString::fromUtf8(colorFile.readAll());
            prompt += "\n\n";
            colorFile.close();
        }
    }

    // Dataset context
    prompt += "## Dataset\n";
    prompt += QString("- File: %1 (%2 rows, %3 columns)\n")
        .arg(datasetInfo["filename"].toString())
        .arg(datasetInfo["rowCount"].toInt())
        .arg(datasetInfo["columnCount"].toInt());
    prompt += "- Columns:\n";

    {
        QJsonArray columns = datasetInfo["columns"].toArray();
        QJsonObject stats = datasetInfo["stats"].toObject();
        for (const auto& col : columns) {
            QJsonObject colObj = col.toObject();
            prompt += QString("  %1 (%2)").arg(colObj["name"].toString(), colObj["type"].toString());
            if (stats.contains(colObj["name"].toString())) {
                QJsonObject s = stats[colObj["name"].toString()].toObject();
                if (!s.isEmpty())
                    prompt += QString(" [%1 - %2]").arg(s["min"].toDouble(), 0, 'f', 2).arg(s["max"].toDouble(), 0, 'f', 2);
            }
            prompt += "\n";
        }
    }

    prompt += "\n## Sample\n";
    prompt += datasetInfo["sampleData"].toString();
    prompt += "\n";

    // Technical: path variables + output format
    prompt += "## Code Requirements\n";
    prompt += "- Use DATASET_PATH (csv path) and OUTPUT_PATH (image path) as pre-defined variables.\n";
    if (language == "python") {
        prompt += "- Read: df = pd.read_csv(DATASET_PATH)\n";
        prompt += "- Save: plt.savefig(OUTPUT_PATH, dpi=300, bbox_inches='tight')\n";
        prompt += "- Do NOT call plt.show(), pip install, or matplotlib.use().\n";
        prompt += "```python\nimport pandas as pd; import matplotlib.pyplot as plt\n";
        prompt += "df = pd.read_csv(DATASET_PATH)\n# ...\nplt.savefig(OUTPUT_PATH, dpi=300, bbox_inches='tight')\n```\n";
    } else {
        prompt += "- Read: df <- read.csv(DATASET_PATH)\n";
        prompt += "- Save: ggsave(OUTPUT_PATH, width=10, height=6, dpi=300)\n";
        prompt += "```r\nlibrary(ggplot2)\ndf <- read.csv(DATASET_PATH)\n# ...\nggsave(OUTPUT_PATH, width=10, height=6, dpi=300)\n```\n";
    }
    prompt += "\nReturn ONLY the code block. No explanation.\n";

    return prompt;
}
