#include "MainWindow.h"
#include "ChatPanel.h"
#include "DataPreviewPanel.h"
#include "CodeViewPanel.h"
#include "PlotViewPanel.h"
#include "TerminalPanel.h"
#include "FavoritesPanel.h"
#include "AIConfigDialog.h"
#include "ExportDialog.h"
#include "core/DataManager.h"
#include "core/AIManager.h"
#include "core/CodeExecutor.h"
#include "core/AppConfig.h"
#include <QMenuBar>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QDir>
#include <QDateTime>
#include <QTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QStandardPaths>

static QIcon svgIcon(const QString& name) {
    return QIcon(QString(":/icons/%1.svg").arg(name));
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_dataManager(new DataManager(this))
    , m_aiManager(new AIManager(this))
    , m_codeExecutor(new CodeExecutor(this))
{
    setupUI();
    setupMenuBar();
    setupStatusBar();
    setupConnections();
    applyTheme();

    setWindowIcon(QIcon(":/logo.png"));
    setWindowTitle(QString::fromUtf8("NexusDraw — AI 数据科学绘图"));
    resize(1400, 900);
    setMinimumSize(1000, 600);
    logToTerminal(QString::fromUtf8("Python: %1").arg(AppConfig::instance().pythonPath()));
    logToTerminal(QString::fromUtf8("R: %1").arg(AppConfig::instance().rPath()));
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI()
{
    m_rightSplitter = new QSplitter(Qt::Vertical);
    m_codeViewPanel = new CodeViewPanel;
    m_plotViewPanel = new PlotViewPanel;
    m_rightSplitter->addWidget(m_codeViewPanel);
    m_favoritesPanel = new FavoritesPanel;
    m_rightSplitter->addWidget(m_plotViewPanel);
    m_rightSplitter->addWidget(m_favoritesPanel);
    m_favoritesPanel->setVisible(false);
    m_rightSplitter->setStretchFactor(0, 2);
    m_rightSplitter->setStretchFactor(1, 2);
    m_rightSplitter->setStretchFactor(2, 1);

    m_leftSplitter = new QSplitter(Qt::Vertical);
    m_chatPanel = new ChatPanel;
    m_dataPreviewPanel = new DataPreviewPanel;
    m_leftSplitter->addWidget(m_chatPanel);
    m_leftSplitter->addWidget(m_dataPreviewPanel);
    m_leftSplitter->setStretchFactor(0, 3);
    m_leftSplitter->setStretchFactor(1, 2);

    m_mainSplitter = new QSplitter(Qt::Horizontal);
    m_mainSplitter->addWidget(m_leftSplitter);
    m_mainSplitter->addWidget(m_rightSplitter);
    m_mainSplitter->setStretchFactor(0, 2);
    m_mainSplitter->setStretchFactor(1, 3);

    m_terminalPanel = new TerminalPanel;

    m_verticalSplitter = new QSplitter(Qt::Vertical);
    m_verticalSplitter->addWidget(m_mainSplitter);
    m_verticalSplitter->addWidget(m_terminalPanel);
    m_verticalSplitter->setStretchFactor(0, 4);
    m_verticalSplitter->setStretchFactor(1, 1);
    m_terminalPanel->setVisible(false);

    setCentralWidget(m_verticalSplitter);
}

void MainWindow::setupMenuBar()
{
    QMenu* fileMenu = menuBar()->addMenu(QString::fromUtf8("文件(&F)"));

    m_uploadAction = fileMenu->addAction(svgIcon("upload"), QString::fromUtf8("上传数据集..."));
    m_uploadAction->setShortcut(QKeySequence("Ctrl+O"));
    connect(m_uploadAction, &QAction::triggered, this, &MainWindow::onUploadDataset);

    fileMenu->addSeparator();

    QAction* exportAction = fileMenu->addAction(svgIcon("export"), QString::fromUtf8("导出图表..."));
    exportAction->setShortcut(QKeySequence("Ctrl+S"));
    connect(exportAction, &QAction::triggered, this, [this]() {
        if (m_plotViewPanel->hasPlot()) {
            onExportPlot(m_plotViewPanel->currentImagePath());
        } else {
            QMessageBox::information(this, QString::fromUtf8("无图表"),
                QString::fromUtf8("请先生成图表再导出。"));
        }
    });

    fileMenu->addSeparator();
    QAction* quitAction = fileMenu->addAction(QString::fromUtf8("退出(&Q)"));
    quitAction->setShortcut(QKeySequence("Ctrl+Q"));
    connect(quitAction, &QAction::triggered, this, &QMainWindow::close);

    QMenu* settingsMenu = menuBar()->addMenu(QString::fromUtf8("设置(&S)"));
    m_configAction = settingsMenu->addAction(svgIcon("settings"), QString::fromUtf8("AI 配置..."));
    m_configAction->setShortcut(QKeySequence("Ctrl+,"));
    connect(m_configAction, &QAction::triggered, this, &MainWindow::onAIConfig);

    QMenu* runMenu = menuBar()->addMenu(QString::fromUtf8("运行(&R)"));
    m_runAction = runMenu->addAction(svgIcon("run"), QString::fromUtf8("执行代码"));
    m_runAction->setShortcut(QKeySequence("F5"));
    m_runAction->setEnabled(false);
    connect(m_runAction, &QAction::triggered, this, &MainWindow::onRunCode);

    QMenu* viewMenu = menuBar()->addMenu(QString::fromUtf8("视图(&V)"));
    m_terminalAction = viewMenu->addAction(svgIcon("terminal"), QString::fromUtf8("终端面板"));
    m_terminalAction->setCheckable(true);
    m_terminalAction->setShortcut(QKeySequence("Ctrl+`"));
    connect(m_terminalAction, &QAction::toggled, this, [this](bool checked) {
        m_terminalPanel->setTerminalVisible(checked);
    });

    QMenu* helpMenu = menuBar()->addMenu(QString::fromUtf8("帮助(&H)"));
    QAction* aboutAction = helpMenu->addAction(QString::fromUtf8("关于 NexusDraw"));
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, QString::fromUtf8("关于 NexusDraw"),
            QString::fromUtf8("<h2>NexusDraw v1.0</h2>"
               "<p>AI 驱动的数据可视化平台</p>"
               "<p>连接任意 OpenAI 兼容的 LLM API，用自然语言描述"
               "可视化需求，由 AI 自动生成绘图代码并执行。</p>"
               "<p><b>工作流：</b>上传数据 → 对话描述 → AI 生成代码 → "
               "自动执行 → 查看图表 → 导出保存</p>"));
    });
}

void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel(QString::fromUtf8("就绪 — 请上传数据集开始"));
    m_statusLabel->setMinimumWidth(260);

    m_progressBar = new QProgressBar;
    m_progressBar->setMaximumWidth(120);
    m_progressBar->setMaximumHeight(14);
    m_progressBar->setRange(0, 0);
    m_progressBar->setVisible(false);

    m_langIndicator = new QLabel;

    statusBar()->addWidget(m_statusLabel, 1);
    statusBar()->addWidget(m_langIndicator);
    statusBar()->addWidget(m_progressBar);
}

void MainWindow::setupConnections()
{
    // Chat -> AI
    connect(m_chatPanel, &ChatPanel::messageSent, this, &MainWindow::onUserMessage);
    connect(m_chatPanel, &ChatPanel::modeChanged, this, [this](const QString& mode) {
        m_codeViewPanel->setEditorReadOnly(mode == "flowchart");
    });
    // AI -> Code or Chat
    connect(m_aiManager, &AIManager::codeGenerated, this, &MainWindow::onCodeGenerated);
    connect(m_aiManager, &AIManager::chatResponse, this, [this](const QString& text) {
        m_chatPanel->addMessage(text, false);  // Show as AI bubble
        setWorkflowEnabled(true);
        m_statusLabel->setText(QString::fromUtf8("就绪"));
    });
    connect(m_aiManager, &AIManager::errorOccurred, this, &MainWindow::onAIError);
    connect(m_aiManager, &AIManager::statusMessage, this, &MainWindow::onAIStatus);
    // Code View -> Run / Stop
    connect(m_codeViewPanel, &CodeViewPanel::runRequested, this, &MainWindow::onRunCode);
    connect(m_codeViewPanel, &CodeViewPanel::stopRequested, this, [this]() {
        if (m_codeExecutor->isRunning()) m_codeExecutor->cancel();
        if (m_aiManager->isBusy()) m_aiManager->cancelRequest();
        m_retryCount = 0;
        m_codeFromAI = false;
        m_codeViewPanel->setRunEnabled(true);
        m_codeViewPanel->setStopEnabled(false);
        setWorkflowEnabled(true);
        m_statusLabel->setText(QString::fromUtf8("已停止"));
    });
    // Executor -> Plot
    connect(m_codeExecutor, &CodeExecutor::executionFinished, this, &MainWindow::onExecutionFinished);
    connect(m_codeExecutor, &CodeExecutor::executionError, this, &MainWindow::onExecutionError);
    connect(m_codeExecutor, &CodeExecutor::statusMessage, this, &MainWindow::onAIStatus);
    // Data Preview -> Upload
    connect(m_dataPreviewPanel, &DataPreviewPanel::uploadRequested, this, &MainWindow::onUploadDataset);
    // Plot -> Export / Favorite
    connect(m_plotViewPanel, &PlotViewPanel::exportRequested, this, &MainWindow::onExportPlot);
    connect(m_plotViewPanel, &PlotViewPanel::favoriteRequested, this, [this](const QString& path) {
        m_favoritesPanel->addFavorite(path);
        m_favoritesPanel->setVisible(true);
        logToTerminal(QString::fromUtf8("已收藏: %1").arg(path));
    });

    // Data Manager
    connect(m_dataManager, &DataManager::dataLoaded, this, [this](int rows, int cols) {
        m_dataPreviewPanel->loadFromDataManager(m_dataManager);
        m_statusLabel->setText(QString::fromUtf8("数据集已加载: %1 行, %2 列").arg(rows).arg(cols));
        m_chatPanel->addSystemMessage(
            QString::fromUtf8("已加载数据集 '%1': %2 行, %3 列。现在可以描述您想要的图表了。")
                .arg(m_dataManager->fileName()).arg(rows).arg(cols));
        logToTerminal(QString::fromUtf8("加载数据集: %1 (%2 x %3)")
            .arg(m_dataManager->fileName()).arg(rows).arg(cols));
    });

    connect(m_dataManager, &DataManager::loadError, this, [this](const QString& err) {
        QMessageBox::warning(this, QString::fromUtf8("加载失败"), err);
        m_statusLabel->setText(QString::fromUtf8("数据集加载失败"));
        logToTerminal(QString::fromUtf8("[错误] 加载失败: %1").arg(err));
    });
}

void MainWindow::applyTheme() {}

// ==================== 辅助函数 ====================

QString MainWindow::generateOutputPath() const
{
    QString tempDir = QDir::tempPath() + "/NexusDraw/output";
    QDir().mkpath(tempDir);
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    return tempDir + "/plot_" + timestamp + ".png";
}

void MainWindow::logToTerminal(const QString& text)
{
    m_terminalPanel->appendLog(text);
}

void MainWindow::setWorkflowEnabled(bool enabled)
{
    m_chatPanel->setInputEnabled(enabled);
    m_uploadAction->setEnabled(enabled);
    m_progressBar->setVisible(!enabled);
}

// ==================== 工作流槽函数 ====================

void MainWindow::onUploadDataset()
{
    QString filter = QString::fromUtf8(
        "数据文件 (*.csv *.tsv *.txt *.dat *.md);;"
        "CSV 文件 (*.csv);;"
        "TSV 文件 (*.tsv);;"
        "Markdown 文件 (*.md);;"
        "所有文件 (*.*)");

    QString path = QFileDialog::getOpenFileName(
        this, QString::fromUtf8("打开数据集"), QString(), filter);
    if (path.isEmpty()) return;

    m_statusLabel->setText(QString::fromUtf8("正在加载数据集..."));
    logToTerminal(QString::fromUtf8("打开文件: %1").arg(path));

    if (m_dataManager->loadFile(path)) {
        m_chatPanel->setInputEnabled(true);
    }
}

void MainWindow::onAIConfig()
{
    AIConfigDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        const AppConfig& cfg = AppConfig::instance();
        QString langName = (cfg.preferredLanguage() == "python") ? "Python" : "R";
        m_langIndicator->setText(langName);
        m_statusLabel->setText(
            QString::fromUtf8("AI 配置已保存: %1").arg(cfg.aiModel()));
        m_chatPanel->addSystemMessage(
            QString::fromUtf8("AI 已配置: %1 (%2)")
                .arg(cfg.aiModel(), langName));
        logToTerminal(QString::fromUtf8("AI 配置: %1 @ %2 [%3]")
            .arg(cfg.aiModel(), cfg.aiBaseURL(), langName));
    }
}

void MainWindow::onUserMessage(const QString& message, const QString& mode, const QString& language, const QString& colorScheme)
{
    m_isPlotMode = (mode != "chat");
    m_currentMode = mode;
    m_currentLanguage = language;
    m_langIndicator->setText(mode == "chat" ? "..." : language.toUpper());

    bool needsData = (mode == "plot" || mode == "flowchart");
    if (needsData && !m_dataManager->isLoaded()) {
        m_chatPanel->addSystemMessage(QString::fromUtf8("请先上传数据集。"));
        QMessageBox::information(this, QString::fromUtf8("无数据集"), QString::fromUtf8("请先上传 CSV/TSV 数据集文件。"));
        return;
    }

    const AppConfig& cfg = AppConfig::instance();
    if (cfg.aiApiKey().isEmpty()) {
        m_chatPanel->addSystemMessage(QString::fromUtf8("请先配置 AI API Key（设置 -> AI 配置）。"));
        onAIConfig();
        if (cfg.aiApiKey().isEmpty()) return;
    }

    setWorkflowEnabled(false);
    m_statusLabel->setText(QString::fromUtf8("正在连接 AI: %1...").arg(cfg.aiModel()));

    // Lock code editor during flowchart AI generation
    if (mode == "flowchart") {
        m_codeViewPanel->setEditorReadOnly(true);
    }

    if (mode == "chat") {
        logToTerminal(QString::fromUtf8("对话请求 -> %1").arg(cfg.aiModel()));
        QJsonObject emptyInfo;
        m_aiManager->sendRequest(message, emptyInfo, language, true);
    } else {
        QString typeLabel = (mode == "flowchart") ? QString::fromUtf8("流程图") : QString::fromUtf8("绘图");
        logToTerminal(QString::fromUtf8("%1请求 -> %2 (%3) 配色:%4")
            .arg(typeLabel, cfg.aiModel(), language, colorScheme.isEmpty() ? QString::fromUtf8("默认") : colorScheme));
        QString outputPath = generateOutputPath();
        if (mode == "flowchart") outputPath.replace(".png", ".svg");
        QJsonObject datasetInfo = m_dataManager->buildDatasetInfo(outputPath);
        datasetInfo["colorScheme"] = colorScheme;
        datasetInfo["mode"] = mode;
        m_aiManager->sendRequest(message, datasetInfo, language, false);
    }
}

void MainWindow::onCodeGenerated(const QString& code, const QString& language)
{
    m_codeFromAI = true;
    m_codeViewPanel->setEditorReadOnly(false);  // Unlock after AI response
    m_currentLanguage = language;
    m_codeViewPanel->setCode(code, language);
    m_runAction->setEnabled(true);
    m_statusLabel->setText(QString::fromUtf8("代码已生成, 自动运行中..."));
    logToTerminal(QString::fromUtf8("AI 返回 %1 代码 (%2 字符)")
        .arg(language).arg(code.length()));
    setWorkflowEnabled(true);
    if (m_isPlotMode) {
        onRunCode();
    } else {
        m_chatPanel->addSystemMessage(QString::fromUtf8("AI 已回复（对话模式）。切换到绘图模式以执行代码。"));
    }
}

void MainWindow::onRunCode()
{
    QString code = m_codeViewPanel->code();
    if (code.isEmpty()) {
        QMessageBox::information(this, QString::fromUtf8("无代码"),
            QString::fromUtf8("请先生成代码再运行。"));
        return;
    }

    if (m_currentLanguage.isEmpty()) {
        m_currentLanguage = "python";
    }

    setWorkflowEnabled(false);
    m_codeViewPanel->setRunEnabled(false);
    m_codeViewPanel->setStopEnabled(true);
    m_runAction->setEnabled(false);

    QString outputPath = generateOutputPath();
    if (m_currentMode == "flowchart" || m_currentLanguage == "xml") {
        outputPath.replace(".png", ".svg");
    }

    m_statusLabel->setText(
        QString::fromUtf8("正在执行 %1 代码...").arg(m_currentLanguage));

    QString dataPath = m_dataManager->isLoaded() ? m_dataManager->filePath() : "";

    logToTerminal(QString::fromUtf8("执行 %1 脚本 -> %2")
        .arg(m_currentLanguage, outputPath));
    m_codeExecutor->execute(code, m_currentLanguage, dataPath, outputPath);
}

void MainWindow::onExecutionFinished(const QString& imagePath, const QString& stdOut)
{
    m_retryCount = 0;
    m_codeViewPanel->setRunEnabled(true);
    m_runAction->setEnabled(true);
    setWorkflowEnabled(true);

    if (!stdOut.isEmpty()) logToTerminal(stdOut.trimmed());

    if (QFile::exists(imagePath)) {
        m_plotViewPanel->loadPlot(imagePath);
        m_statusLabel->setText(QString::fromUtf8("图表生成成功"));
        logToTerminal(QString::fromUtf8("图表: %1").arg(imagePath));
    } else {
        m_statusLabel->setText(QString::fromUtf8("运行完成"));
    }
}

void MainWindow::onExecutionError(const QString& error, const QString& stdErr)
{
    setWorkflowEnabled(true);
    m_statusLabel->setText(QString::fromUtf8("执行出错"));
    logToTerminal(QString::fromUtf8("错误: %1").arg(error));
    m_terminalPanel->appendError(error);

    QString combined = error + "\n" + stdErr;
    bool isEnvError = combined.contains("ModuleNotFoundError") ||
                      combined.contains("No module named") ||
                      combined.contains("there is no package called");
    bool isDataError = combined.contains("FileNotFoundError") ||
                       combined.contains("No such file") ||
                       combined.contains("cannot open") ||
                       (combined.contains("KeyError") && combined.contains("DATASET_PATH"));

    if (isEnvError) {
        QString msg = QString::fromUtf8("环境配置问题：缺少必要的库。\n%1").arg(error);
        m_chatPanel->addSystemMessage(msg);
        m_codeViewPanel->setRunEnabled(true);
        m_runAction->setEnabled(true);
        return;
    }
    if (isDataError) {
        QString msg = QString::fromUtf8("数据集问题，请检查数据格式。\n%1").arg(error);
        m_chatPanel->addSystemMessage(msg);
        m_codeViewPanel->setRunEnabled(true);
        m_runAction->setEnabled(true);
        return;
    }

    // AI auto-fix only for AI-generated code
    if (m_codeFromAI) {
        m_retryCount++;
        if (m_retryCount <= 3) {
            m_chatPanel->addSystemMessage(
                QString::fromUtf8("代码报错，AI 正在修复 (第 %1/3 次)...").arg(m_retryCount));
            m_statusLabel->setText(QString::fromUtf8("AI 修复中..."));
            m_aiManager->fixCode(m_codeViewPanel->code(), combined, m_currentLanguage);
        } else {
            m_chatPanel->addSystemMessage(
                QString::fromUtf8("AI 修复 3 次后仍失败，请手动修改代码。"));
            m_codeViewPanel->setRunEnabled(true);
            m_runAction->setEnabled(true);
            m_retryCount = 0;
        }
        return;
    }

    // Manual code: just show error in terminal
    m_codeViewPanel->setRunEnabled(true);
    m_runAction->setEnabled(true);
    m_statusLabel->setText(QString::fromUtf8("运行出错，见终端"));
}

void MainWindow::onExportPlot(const QString& imagePath)
{
    ExportDialog dialog(imagePath, this);
    dialog.exec();
}

void MainWindow::onAIError(const QString& error)
{
    setWorkflowEnabled(true);
    m_statusLabel->setText(QString::fromUtf8("AI 请求失败"));
    m_terminalPanel->appendError(
        QString::fromUtf8("AI 错误: %1").arg(error));
    QMessageBox::warning(this, QString::fromUtf8("AI 错误"), error);
}

void MainWindow::onAIStatus(const QString& status)
{
    m_statusLabel->setText(status);
    logToTerminal(status);
}

