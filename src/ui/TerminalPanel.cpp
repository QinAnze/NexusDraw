#include "TerminalPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QScrollBar>
#include <QDir>
#include <QTimer>

TerminalPanel::TerminalPanel(QWidget* parent) : QWidget(parent)
{
    setupUI();
}

void TerminalPanel::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* headerBar = new QHBoxLayout;
    headerBar->setContentsMargins(10, 3, 6, 3);
    m_headerLabel = new QLabel(QString::fromUtf8("终端"));
    m_headerLabel->setStyleSheet("font-weight: bold; font-size: 12px; color: #333;");
    headerBar->addWidget(m_headerLabel);
    headerBar->addStretch();
    m_clearBtn = new QPushButton(QString::fromUtf8("清空"));
    m_clearBtn->setMinimumWidth(44);
    m_clearBtn->setMaximumHeight(22);
    m_clearBtn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    connect(m_clearBtn, &QPushButton::clicked, this, &TerminalPanel::clear);
    headerBar->addWidget(m_clearBtn);
    mainLayout->addLayout(headerBar);

    auto* sep = new QWidget; sep->setFixedHeight(1);
    sep->setStyleSheet("background-color: #d0d0d0;");
    mainLayout->addWidget(sep);

    m_output = new QPlainTextEdit;
    m_output->setReadOnly(true);
    m_output->setUndoRedoEnabled(false);
    QFont mono("Consolas", 11); mono.setStyleHint(QFont::Monospace);
    m_output->setFont(mono);
    m_output->setStyleSheet("QPlainTextEdit{background:#f5f5f5;color:#1a1a1a;border:none;padding:6px 10px;}");
    mainLayout->addWidget(m_output, 1);

    auto* inputRow = new QHBoxLayout;
    inputRow->setContentsMargins(6, 4, 6, 6);
    auto* promptLabel = new QLabel(">");
    promptLabel->setStyleSheet("color:#333;font-weight:bold;font-size:13px;font-family:Consolas,monospace;");
    promptLabel->setFixedWidth(16);
    inputRow->addWidget(promptLabel);
    m_inputEdit = new QLineEdit;
    m_inputEdit->setStyleSheet("QLineEdit{background:#fff;border:1px solid #d0d0d0;border-radius:3px;padding:4px 8px;color:#1a1a1a;font-family:Consolas,monospace;font-size:13px;}QLineEdit:focus{border-color:#333;}");
    m_inputEdit->setPlaceholderText(QString::fromUtf8("输入命令..."));
    connect(m_inputEdit, &QLineEdit::returnPressed, this, &TerminalPanel::onSendCommand);
    inputRow->addWidget(m_inputEdit, 1);
    mainLayout->addLayout(inputRow);
    setStyleSheet("TerminalPanel{background-color:#fff;}");
}

void TerminalPanel::onSendCommand()
{
    QString cmd = m_inputEdit->text().trimmed();
    if (cmd.isEmpty()) return;
    m_inputEdit->clear();

    m_output->appendHtml(QString("<span style='color:#333;font-weight:bold'>%1&gt; %2</span>")
                         .arg(m_workDir.toHtmlEscaped(), cmd.toHtmlEscaped()));

    if (cmd.startsWith("cd ", Qt::CaseInsensitive) || cmd == "cd") {
        QString target = cmd.mid(3).trimmed();
        if (target.isEmpty()) { m_output->appendPlainText(m_workDir); }
        else {
            QDir dir(m_workDir);
            QString newDir = QDir::cleanPath(dir.absoluteFilePath(target));
            if (QDir(newDir).exists()) m_workDir = newDir;
            else m_output->appendPlainText(QString::fromUtf8("目录不存在: %1").arg(newDir));
        }
        scrollToBottom();
        return;
    }

    auto* proc = new QProcess(this);
    proc->setWorkingDirectory(m_workDir);
    connect(proc, &QProcess::readyReadStandardOutput, this, [this, proc]() {
        m_output->moveCursor(QTextCursor::End);
        m_output->insertPlainText(QString::fromLocal8Bit(proc->readAllStandardOutput()));
        scrollToBottom();
    });
    connect(proc, &QProcess::readyReadStandardError, this, [this, proc]() {
        m_output->moveCursor(QTextCursor::End);
        m_output->insertPlainText(QString::fromLocal8Bit(proc->readAllStandardError()));
        scrollToBottom();
    });
    proc->start("cmd.exe", QStringList() << "/c" << cmd);
}

void TerminalPanel::runCommand(const QString& cmd)
{
    m_output->appendHtml(QString("<span style='color:#333;font-weight:bold'>$ %1</span>")
                         .arg(cmd.toHtmlEscaped()));
    auto* proc = new QProcess(this);
    proc->setWorkingDirectory(m_workDir);
    connect(proc, &QProcess::readyReadStandardOutput, this, [this, proc]() {
        m_output->moveCursor(QTextCursor::End);
        m_output->insertPlainText(QString::fromLocal8Bit(proc->readAllStandardOutput()));
        scrollToBottom();
    });
    connect(proc, &QProcess::readyReadStandardError, this, [this, proc]() {
        m_output->moveCursor(QTextCursor::End);
        m_output->insertPlainText(QString::fromLocal8Bit(proc->readAllStandardError()));
        scrollToBottom();
    });
    proc->start("cmd.exe", QStringList() << "/c" << cmd);
}

void TerminalPanel::appendLog(const QString& text) {
    m_output->appendHtml(QString("<span style='color:#555'>%1</span>").arg(text.toHtmlEscaped()));
    scrollToBottom();
}
void TerminalPanel::appendError(const QString& text) {
    m_output->appendHtml(QString("<span style='color:#c0392b'>[错误] %1</span>").arg(text.toHtmlEscaped()));
    scrollToBottom();
}
void TerminalPanel::appendSuccess(const QString& text) {
    m_output->appendHtml(QString("<span style='color:#27ae60'>[完成] %1</span>").arg(text.toHtmlEscaped()));
    scrollToBottom();
}
void TerminalPanel::clear() { m_output->clear(); }
void TerminalPanel::setTerminalVisible(bool v) { setVisible(v); }
void TerminalPanel::scrollToBottom() { m_output->verticalScrollBar()->setValue(m_output->verticalScrollBar()->maximum()); }
