#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QProgressBar>
#include <QAction>

class ChatPanel;
class DataPreviewPanel;
class CodeViewPanel;
class PlotViewPanel;
class TerminalPanel;
class DataManager;
class AIManager;
class CodeExecutor;
/// 主窗口 — 串联整个工作流: 上传 -> 对话 -> AI生成代码 -> 执行 -> 绘图 -> 导出
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onUploadDataset();
    void onAIConfig();
    void onUserMessage(const QString& message, bool isPlotMode, const QString& language, const QString& colorScheme);
    void onCodeGenerated(const QString& code, const QString& language);
    void onRunCode();
    void onExecutionFinished(const QString& imagePath, const QString& stdOut);
    void onExecutionError(const QString& error, const QString& stdErr);
    void onExportPlot(const QString& imagePath);
    void onAIError(const QString& error);
    void onAIStatus(const QString& status);

private:
    void setupUI();
    void setupMenuBar();
    void setupStatusBar();
    void setupConnections();
    void applyTheme();
    void setWorkflowEnabled(bool enabled);
    QString generateOutputPath() const;
    void logToTerminal(const QString& text);
    // 核心组件
    DataManager* m_dataManager;
    AIManager* m_aiManager;
    CodeExecutor* m_codeExecutor;
    // 界面面板
    ChatPanel* m_chatPanel;
    DataPreviewPanel* m_dataPreviewPanel;
    CodeViewPanel* m_codeViewPanel;
    PlotViewPanel* m_plotViewPanel;
    TerminalPanel* m_terminalPanel;

    // 布局
    QSplitter* m_mainSplitter;
    QSplitter* m_leftSplitter;
    QSplitter* m_rightSplitter;
    QSplitter* m_verticalSplitter;

    // 状态栏
    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;
    QLabel* m_langIndicator;

    // 菜单动作
    QAction* m_uploadAction;
    QAction* m_configAction;
    QAction* m_runAction;
    QAction* m_terminalAction;

    // 状态
    QString m_currentLanguage;
    bool m_isPlotMode = true;
};
