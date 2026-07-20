#include "AppConfig.h"
#include <QJsonArray>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QMap>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QSysInfo>

// Simple XOR encrypt/decrypt with machine-specific key
static QByteArray configKey() {
    QByteArray seed = QSysInfo::machineUniqueId() + QByteArray("NexusDrawSalt");
    return QCryptographicHash::hash(seed, QCryptographicHash::Sha256);
}

static QByteArray encryptConfig(const QString& plain) {
    QByteArray key = configKey();
    QByteArray data = plain.toUtf8();
    for (int i = 0; i < data.size(); ++i) data[i] ^= key[i % key.size()];
    return data.toBase64();
}

static QString decryptConfig(const QString& b64) {
    QByteArray key = configKey();
    QByteArray data = QByteArray::fromBase64(b64.toUtf8());
    for (int i = 0; i < data.size(); ++i) data[i] ^= key[i % key.size()];
    return QString::fromUtf8(data);
}

AppConfig& AppConfig::instance()
{
    static AppConfig config;
    return config;
}

AppConfig::AppConfig()
    : m_settings(QSettings::IniFormat, QSettings::UserScope, "NexusDraw", "NexusDraw")
    , m_darkTheme(true)
    , m_preferredLang("python")
{
    // Migrate from old registry settings on first run
    if (!QFile::exists(m_settings.fileName())) {
        QSettings oldReg("NexusDraw", "NexusDraw");
        if (oldReg.contains("ai/apiKey")) {
            m_settings.setValue("ai/baseURL", oldReg.value("ai/baseURL").toString());
            m_settings.setValue("ai/apiKey", encryptConfig(oldReg.value("ai/apiKey").toString()));
            m_settings.setValue("ai/model", oldReg.value("ai/model").toString());
            m_settings.setValue("exec/preferredLang", oldReg.value("exec/preferredLang").toString());
            m_settings.sync();
        }
    }
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
    m_settings.setValue("ai/apiKey", encryptConfig(m_apiKey));
    m_settings.setValue("ai/model", m_model);
    m_settings.setValue("exec/pythonPath", m_pythonPath);
    m_settings.setValue("exec/rPath", m_rPath);
    m_settings.setValue("ui/darkTheme", m_darkTheme);
    m_settings.sync();
}

void AppConfig::load()
{
    m_baseURL = m_settings.value("ai/baseURL", "https://api.openai.com").toString();
    QString encryptedKey = m_settings.value("ai/apiKey", "").toString();
    m_apiKey = encryptedKey.isEmpty() ? "" : decryptConfig(encryptedKey);
    m_model = m_settings.value("ai/model", "gpt-4o").toString();
    m_pythonPath = m_settings.value("exec/pythonPath", "").toString();
    m_rPath = m_settings.value("exec/rPath", "").toString();
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
    bool isFlowchart = (datasetInfo["mode"].toString() == "flowchart");
    QString effectiveLang = isFlowchart ? "xml" : language;

    QString langSkill;
    QString skillPath;
    if (isFlowchart) {
        skillPath = ":/skills/flowchart.md";
    } else {
        skillPath = (language == "python") ? ":/skills/py.md" : ":/skills/r.md";
    }
    QFile langFile(skillPath);
    if (langFile.open(QFile::ReadOnly | QFile::Text)) {
        langSkill = QString::fromUtf8(langFile.readAll());
        langFile.close();
    }

    QString prompt;
    prompt += universalSkill;
    prompt += "\n";
    prompt += langSkill;
    prompt += "\n";

    // Static color palettes — built once
    static QMap<QString, QStringList> palettes;
    if (palettes.isEmpty()) {
        palettes["viridis"]    = {"#440154","#482878","#3E4989","#31688E","#26828E","#1F9E89","#35B779","#6ECE58","#B5DE2B","#FDE725"};
        palettes["plasma"]     = {"#0D0887","#46039F","#7201A8","#9C179E","#BD3786","#D8576B","#ED7953","#FB9F3A","#FDCA26","#F0F921"};
        palettes["inferno"]    = {"#000004","#1B0C41","#4A0C6B","#781C6D","#A52C60","#CF4446","#ED6925","#FB9B06","#F7D13D","#FCFFA4"};
        palettes["magma"]      = {"#000004","#180F3D","#440F76","#721F81","#9E2F7F","#CD4071","#F1605D","#FD9567","#FECA8D","#FCFDBF"};
        palettes["cividis"]    = {"#00224E","#123570","#3B496C","#575D6D","#707173","#8A8678","#A59C74","#C3B369","#E1CC55","#FFE838"};
        palettes["deep"]       = {"#4C72B0","#DD8452","#55A868","#C44E52","#8172B3","#937860","#DA8BC3","#8C8C8C","#CCB974","#64B5CD"};
        palettes["muted"]      = {"#4878D0","#EE854A","#6ACC64","#D65F5F","#956CB4","#8C613C","#DC7EC0","#797979","#D5BB67","#82C6E2"};
        palettes["colorblind"] = {"#0173B2","#DE8F05","#029E73","#D55E00","#CC78BC","#CA9161","#FBAFE4","#949494","#ECE133","#56B4E9"};
        palettes["bright"]     = {"#023EFF","#FF7C00","#1AC938","#E8000B","#8B2BE2","#9F4800","#F14CC1","#A3A3A3","#FFC400","#00D7FF"};
        palettes["dark"]       = {"#001C7F","#B1400D","#12711C","#8C0800","#591E71","#592F0D","#A23582","#3C3C3C","#B8850A","#006374"};
        palettes["Set1"]       = {"#E41A1C","#377EB8","#4DAF4A","#984EA3","#FF7F00","#FFFF33","#A65628","#F781BF","#999999"};
        palettes["Set2"]       = {"#66C2A5","#FC8D62","#8DA0CB","#E78AC3","#A6D854","#FFD92F","#E5C494","#B3B3B3"};
        palettes["Set3"]       = {"#8DD3C7","#FFFFB3","#BEBADA","#FB8072","#80B1D3","#FDB462","#B3DE69","#FCCDE5","#D9D9D9","#BC80BD","#CCEBC5","#FFED6F"};
        palettes["ggplot2"]    = {"#F8766D","#7CAE00","#00BFC4","#C77CFF"};
        palettes["okabe-ito"]  = {"#000000","#E69F00","#56B4E9","#009E73","#F0E442","#0072B2","#D55E00","#CC79A7"};
        palettes["nature"]     = {"#2D6A4F","#40916C","#52B788","#74C69D","#95D5B2","#B7E4C7","#D8F3DC","#081C15","#1B4332"};
        palettes["bio"]        = {"#1B9E77","#D95F02","#7570B3","#E7298A","#66A61E","#E6AB02","#A6761D","#666666"};
        palettes["finance"]    = {"#003F5C","#2F4B7C","#665191","#A05195","#D45087","#F95D6A","#FF7C43","#FFA600"};
        palettes["BWR"]        = {"#053061","#2166AC","#4393C3","#92C5DE","#D1E5F0","#F7F7F7","#FDDBC7","#F4A582","#D6604D","#B2182B","#67001F"};
        palettes["BWR2"]       = {"#0000FF","#4444FF","#8888FF","#CCCCFF","#FFFFFF","#FFCCCC","#FF8888","#FF4444","#FF0000"};
        palettes["flow-1"]     = {"#3498db","#e74c3c","#2ecc71","#f39c12","#9b59b6","#1abc9c","#e67e22","#34495e"};
        palettes["flow-2"]     = {"#2c3e50","#c0392b","#27ae60","#f1c40f","#8e44ad","#16a085","#d35400","#2980b9"};
        palettes["flow-3"]     = {"#1B9E77","#D95F02","#7570B3","#E7298A","#66A61E","#E6AB02","#A6761D","#666666"};
        palettes["flow-4"]     = {"#4C72B0","#DD8452","#55A868","#C44E52","#8172B3","#937860","#DA8BC3","#8C8C8C"};
        palettes["flow-5"]     = {"#FF6B6B","#4ECDC4","#45B7D1","#96CEB4","#FFEAA7","#DDA0DD","#98D8C8","#F7DC6F"};
    }

    QString colorScheme = datasetInfo["colorScheme"].toString();
    if (colorScheme == "auto") {
        prompt += "## Color Palette\nPick the best palette for this data. Use hex codes directly (NOT named palettes like 'viridis'):\n";
        for (auto it = palettes.begin(); it != palettes.end(); ++it) {
            prompt += QString("- %1: %2\n").arg(it.key(), it.value().join(", "));
        }
        if (language == "python") {
            prompt += "\nApply: palette = ['#hex1','#hex2',...]; sns.set_palette(palette)\n";
        } else {
            prompt += "\nApply: scale_color_manual(values=c('#hex1','#hex2',...))\n";
        }
        prompt += "Do NOT use scale_color_viridis() or any named palette function. Use hex codes only.\n\n";
    } else if (palettes.contains(colorScheme)) {
        const QStringList& hex = palettes[colorScheme];
        prompt += QString("## Color Palette — %1\nHex codes: %2\n").arg(colorScheme, hex.join(", "));
        QStringList quoted;
        for (const QString& c : hex) quoted.append("'" + c + "'");
        if (language == "python") {
            prompt += "Apply: custom_palette = [" + quoted.join(",") + "]\n";
            prompt += "  sns.set_palette(custom_palette) or plt.rcParams['axes.prop_cycle'] = plt.cycler(color=custom_palette)\n";
        } else {
            prompt += "Apply: scale_color_manual(values=c(" + quoted.join(",") + "))\n";
        }
        prompt += "Use these exact hex codes. Do NOT use named palette functions.\n\n";
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
    if (isFlowchart) {
        prompt += "- Generate a complete SVG flowchart (standalone XML, no external dependencies).\n";
        prompt += "- Use `<svg>` root with viewBox, `<rect>`, `<ellipse>`, `<line>`, `<text>` elements.\n";
        prompt += "- Use DATASET_PATH to understand the data structure.\n";
        prompt += "- Apply the color palette directly to SVG fill/stroke attributes.\n";
        prompt += "Return ONLY the SVG code in a ```xml code block. No explanation.\n";
    } else {
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
    }  // end isFlowchart else

    prompt += "\nReturn ONLY the code block. No explanation.\n";

    return prompt;
}
