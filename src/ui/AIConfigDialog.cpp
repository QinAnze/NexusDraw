#include "AIConfigDialog.h"
#include "core/AppConfig.h"
#include "core/CodeExecutor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>

AIConfigDialog::AIConfigDialog(QWidget* parent) : QDialog(parent)
{
    setupUI();
    loadSettings();
    applyDarkTheme();
    setWindowTitle(QString::fromUtf8("AI 配置"));
    setMinimumWidth(500);
    setModal(true);
}

void AIConfigDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 16, 20, 16);

    auto* llmGroup = new QGroupBox(QString::fromUtf8("LLM API 配置 (OpenAI 格式)"));
    auto* llmLayout = new QFormLayout(llmGroup);
    llmLayout->setSpacing(10);

    m_baseURLEdit = new QLineEdit;
    m_baseURLEdit->setPlaceholderText("https://api.openai.com");
    m_baseURLEdit->setMinimumHeight(30);
    llmLayout->addRow(QString::fromUtf8("Base URL:"), m_baseURLEdit);

    m_apiKeyEdit = new QLineEdit;
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setPlaceholderText("sk-...");
    m_apiKeyEdit->setMinimumHeight(30);
    llmLayout->addRow(QString::fromUtf8("API Key:"), m_apiKeyEdit);

    m_modelEdit = new QLineEdit;
    m_modelEdit->setPlaceholderText("gpt-4o / deepseek-chat / claude-sonnet-5");
    m_modelEdit->setMinimumHeight(30);
    llmLayout->addRow(QString::fromUtf8("模型名称:"), m_modelEdit);

    m_testBtn = new QPushButton(QString::fromUtf8("测试连接"));
    m_testBtn->setMinimumHeight(30);
    llmLayout->addRow(QString(), m_testBtn);

    mainLayout->addWidget(llmGroup);

    auto* execGroup = new QGroupBox(QString::fromUtf8("代码执行环境"));
    auto* execLayout = new QFormLayout(execGroup);
    execLayout->setSpacing(10);

    auto* pythonRow = new QHBoxLayout;
    m_pythonPathEdit = new QLineEdit;
    m_pythonPathEdit->setPlaceholderText("python (或完整路径)");
    m_pythonPathEdit->setMinimumHeight(30);
    pythonRow->addWidget(m_pythonPathEdit);
    auto* browsePy = new QPushButton("...");
    browsePy->setFixedWidth(36);
    browsePy->setMinimumHeight(30);
    connect(browsePy, &QPushButton::clicked, this, &AIConfigDialog::onBrowsePython);
    pythonRow->addWidget(browsePy);
    execLayout->addRow(QString::fromUtf8("Python 路径:"), pythonRow);

    auto* rRow = new QHBoxLayout;
    m_rPathEdit = new QLineEdit;
    m_rPathEdit->setPlaceholderText("Rscript (或完整路径)");
    m_rPathEdit->setMinimumHeight(30);
    rRow->addWidget(m_rPathEdit);
    auto* browseR = new QPushButton("...");
    browseR->setFixedWidth(36);
    browseR->setMinimumHeight(30);
    connect(browseR, &QPushButton::clicked, this, &AIConfigDialog::onBrowseR);
    rRow->addWidget(browseR);
    execLayout->addRow(QString::fromUtf8("R 路径:"), rRow);

    mainLayout->addWidget(execGroup);

    m_statusLabel = new QLabel;
    m_statusLabel->setWordWrap(true);
    mainLayout->addWidget(m_statusLabel);
    mainLayout->addStretch();

    auto* btnLayout = new QHBoxLayout;
    m_saveBtn = new QPushButton(QString::fromUtf8("保存"));
    m_saveBtn->setMinimumHeight(32);
    m_saveBtn->setDefault(true);
    // Uses global QSS
    m_cancelBtn = new QPushButton(QString::fromUtf8("取消"));
    m_cancelBtn->setMinimumHeight(32);
    btnLayout->addStretch();
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_saveBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_saveBtn, &QPushButton::clicked, this, &AIConfigDialog::onSave);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_testBtn, &QPushButton::clicked, this, &AIConfigDialog::onTestConnection);
}

void AIConfigDialog::loadSettings()
{
    const AppConfig& cfg = AppConfig::instance();
    m_baseURLEdit->setText(cfg.aiBaseURL());
    m_apiKeyEdit->setText(cfg.aiApiKey());
    m_modelEdit->setText(cfg.aiModel());
    m_pythonPathEdit->setText(cfg.pythonPath() == "python" ? "" : cfg.pythonPath());
    m_rPathEdit->setText(cfg.rPath() == "Rscript" ? "" : cfg.rPath());
}

void AIConfigDialog::onSave()
{
    AppConfig& cfg = AppConfig::instance();
    QString url = m_baseURLEdit->text().trimmed();
    if (url.isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("验证"),
                             QString::fromUtf8("Base URL 不能为空。"));
        return;
    }
    cfg.setAiBaseURL(url);
    cfg.setAiApiKey(m_apiKeyEdit->text().trimmed());
    cfg.setAiModel(m_modelEdit->text().trimmed());
    QString pyPath = m_pythonPathEdit->text().trimmed();
    cfg.setPythonPath(pyPath.isEmpty() ? "python" : pyPath);
    QString rPath = m_rPathEdit->text().trimmed();
    cfg.setRPath(rPath.isEmpty() ? "Rscript" : rPath);
    cfg.save();
    accept();
}

void AIConfigDialog::onTestConnection()
{
    m_statusLabel->setText(QString::fromUtf8("正在检测..."));
    m_testBtn->setEnabled(false);
    QStringList checks;
    AppConfig& cfg = AppConfig::instance();

    if (cfg.aiApiKey().isEmpty())
        checks << QString::fromUtf8("! API Key 未设置");
    else
        checks << QString::fromUtf8("OK API Key: ...%1").arg(cfg.aiApiKey().right(4));
    checks << QString::fromUtf8("OK Base URL: %1").arg(cfg.aiBaseURL());
    checks << QString::fromUtf8("OK 模型: %1").arg(cfg.aiModel());

    if (CodeExecutor::isPythonAvailable())
        checks << QString::fromUtf8("OK Python: %1").arg(CodeExecutor::pythonVersion());
    else
        checks << QString::fromUtf8("! Python 未找到");

    if (CodeExecutor::isRAvailable())
        checks << QString::fromUtf8("OK R: %1").arg(CodeExecutor::rVersion());
    else
        checks << QString::fromUtf8("! R 未找到 (可选)");

    m_statusLabel->setText(checks.join("\n"));
    bool hasWarn = checks.join("").contains("!");
    m_statusLabel->setStyleSheet(hasWarn ? "color: #c0392b;" : "color: #27ae60;");
    QTimer::singleShot(8000, this, [this]() { m_testBtn->setEnabled(true); });
}

void AIConfigDialog::onBrowsePython() {
    QString p = QFileDialog::getOpenFileName(this, QString::fromUtf8("选择 Python 可执行文件"));
    if (!p.isEmpty()) m_pythonPathEdit->setText(p);
}
void AIConfigDialog::onBrowseR() {
    QString p = QFileDialog::getOpenFileName(this, QString::fromUtf8("选择 Rscript 可执行文件"));
    if (!p.isEmpty()) m_rPathEdit->setText(p);
}

void AIConfigDialog::applyDarkTheme()
{
    // Global stylesheet handles everything
}
