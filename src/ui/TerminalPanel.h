#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QProcess>

/// Interactive terminal panel using one-shot cmd.exe /c per command.
class TerminalPanel : public QWidget
{
    Q_OBJECT

public:
    explicit TerminalPanel(QWidget* parent = nullptr);

    void appendLog(const QString& text);
    void appendError(const QString& text);
    void appendSuccess(const QString& text);
    void clear();
    void setTerminalVisible(bool visible);
    void runCommand(const QString& cmd);

signals:
    void commandExecuted(const QString& command);

private slots:
    void onSendCommand();

private:
    void setupUI();
    void scrollToBottom();

    QLabel* m_headerLabel;
    QPlainTextEdit* m_output;
    QLineEdit* m_inputEdit;
    QPushButton* m_clearBtn;
    QString m_workDir = "C:\\";
};
