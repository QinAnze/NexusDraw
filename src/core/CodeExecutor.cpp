#include "CodeExecutor.h"
#include "AppConfig.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTemporaryFile>
#include <QRegularExpression>
#include <QDebug>
#include <QCoreApplication>
#include <QProcessEnvironment>
#include <QTimer>

CodeExecutor::CodeExecutor(QObject* parent)
    : QObject(parent)
{
}

CodeExecutor::~CodeExecutor()
{
    cancel();
}

bool CodeExecutor::isRunning() const
{
    return m_running;
}

void CodeExecutor::cancel()
{
    if (m_process && m_running) {
        m_process->kill();
        m_process->waitForFinished(3000);
        m_running = false;
        emit statusMessage(tr("Execution cancelled."));
    }
}

bool CodeExecutor::isPythonAvailable()
{
    QProcess proc;
    proc.start(AppConfig::instance().pythonPath(), {"--version"});
    proc.waitForFinished(5000);
    return proc.exitCode() == 0;
}

bool CodeExecutor::isRAvailable()
{
    QProcess proc;
    proc.start(AppConfig::instance().rPath(), {"--version"});
    proc.waitForFinished(5000);
    return proc.exitCode() == 0;
}

QString CodeExecutor::pythonVersion()
{
    QProcess proc;
    proc.start(AppConfig::instance().pythonPath(), {"--version"});
    proc.waitForFinished(5000);
    return QString::fromUtf8(proc.readAllStandardError()).trimmed(); // Python outputs version to stderr
}

QString CodeExecutor::rVersion()
{
    QProcess proc;
    proc.start(AppConfig::instance().rPath(), {"--version"});
    proc.waitForFinished(5000);
    return QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
}

QString CodeExecutor::wrapPythonCode(const QString& userCode, const QString& dataPath,
                                      const QString& outputPath) const
{
    QString code;
    code += "# -*- coding: utf-8 -*-\n";
    code += "# NexusDraw auto-generated wrapper\n";
    code += "import sys, os, warnings\n";
    code += "warnings.filterwarnings('ignore')\n";
    code += "import matplotlib\n";
    code += "matplotlib.use('Agg')\n\n";

    // Pre-define paths as Python variables so AI code can use them
    // Use escaped double quotes for safety with any path characters
    QString dp = dataPath;
    QString op = outputPath;
    code += QString("DATASET_PATH = \"%1\"\n").arg(dp.replace("\\", "\\\\"));
    code += QString("OUTPUT_PATH  = \"%1\"\n\n").arg(op.replace("\\", "\\\\"));

    // Ensure output directory
    code += "os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)\n\n";

    // Insert user code
    code += "# === User-generated code below ===\n";
    code += userCode;
    code += "\n# === End user code ===\n\n";

    // Verify
    code += QString("if os.path.exists(OUTPUT_PATH):\n");
    code += "    print('OUTPUT_IMAGE_CREATED: ' + OUTPUT_PATH)\n";
    code += "else:\n";
    code += "    print('ERROR: No output image created. Check that savefig/ggsave was called with OUTPUT_PATH.')\n";
    code += "    sys.exit(1)\n";

    return code;
}

QString CodeExecutor::wrapRCode(const QString& userCode, const QString& dataPath,
                                 const QString& outputPath) const
{
    QString code;
    code += "# NexusDraw auto-generated wrapper\n";
    code += "options(warn=-1)\n";
    code += "suppressPackageStartupMessages({\n";
    code += "  library(ggplot2)\n";
    code += "  library(dplyr)\n";
    code += "  library(tidyr)\n";
    code += "  library(readr)\n";
    code += "})\n\n";

    // Pre-define paths as R variables (R uses forward slashes)
    QString dp = dataPath;
    QString op = outputPath;
    code += QString("DATASET_PATH <- \"%1\"\n").arg(dp.replace("\\", "/"));
    code += QString("OUTPUT_PATH  <- \"%1\"\n\n").arg(op.replace("\\", "/"));

    code += QString("dir.create(dirname(OUTPUT_PATH), showWarnings=FALSE, recursive=TRUE)\n\n");

    code += "# === User-generated code below ===\n";
    code += userCode;
    code += "\n# === End user code ===\n";

    code += QString("if (file.exists(OUTPUT_PATH)) {\n");
    code += "  cat('OUTPUT_IMAGE_CREATED: ', OUTPUT_PATH, '\\n')\n";
    code += "} else {\n";
    code += "  cat('ERROR: No output image created.\\n')\n";
    code += "  quit(status=1)\n";
    code += "}\n";

    return code;
}

QString CodeExecutor::createTempFile(const QString& content, const QString& extension) const
{
    QString tempDir = QDir::tempPath() + "/NexusDraw";
    QDir().mkpath(tempDir);

    QString fileName = tempDir + "/nexus_script_" +
                       QString::number(QCoreApplication::applicationPid()) + "." + extension;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(content.toUtf8());
        file.close();
    }

    return fileName;
}

void CodeExecutor::execute(const QString& code, const QString& language,
                            const QString& dataPath, const QString& outputImagePath)
{
    if (m_running) {
        emit executionError(tr("已有代码在运行中。"), QString());
        return;
    }

    const AppConfig& cfg = AppConfig::instance();
    m_outputImagePath = outputImagePath;
    m_stdoutBuffer.clear();
    m_stderrBuffer.clear();

    // Delete old output image if it exists
    if (QFile::exists(outputImagePath)) {
        QFile::remove(outputImagePath);
    }

    QString wrappedCode;
    QString extension;
    QString executable;
    QStringList args;

    if (language == "python") {
        wrappedCode = wrapPythonCode(code, dataPath, outputImagePath);
        extension = "py";
        executable = QDir::toNativeSeparators(cfg.pythonPath());
        args << "-u";
    } else {
        wrappedCode = wrapRCode(code, dataPath, outputImagePath);
        extension = "r";
        executable = QDir::toNativeSeparators(cfg.rPath());
        // Verify R exists before trying to run
        if (!QFile::exists(executable) && executable != "Rscript") {
            m_running = false;
            emit executionError(
                tr("R 未找到: %1\n请在设置中配置 R 路径。").arg(executable), QString());
            return;
        }
        args << "--no-save" << "--no-restore";
    }

    // Create QProcess and set environment
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    // Set R_HOME for portable R (packages now bundled in portable_r/library)
    if (language == "r") {
        QString rHome = cfg.rHomePath();
        if (!rHome.isEmpty()) {
            QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
            env.insert("R_HOME", QDir::toNativeSeparators(rHome));
            m_process->setProcessEnvironment(env);
        }
    }

    QString scriptPath = createTempFile(wrappedCode, extension);
    args << scriptPath;

    // Diagnostic logging
    emit statusMessage(tr("Python: %1").arg(executable));
    emit statusMessage(tr("脚本: %1 (%2 字节)")
        .arg(scriptPath).arg(QFileInfo(scriptPath).size()));
    emit statusMessage(tr("输出: %1").arg(outputImagePath));

    // Verify temp file was created and has content
    QFile verifyFile(scriptPath);
    if (!verifyFile.exists() || verifyFile.size() == 0) {
        m_running = false;
        emit executionError(
            tr("临时脚本文件创建失败或为空。\n路径: %1\n大小: %2")
                .arg(scriptPath).arg(verifyFile.size()), QString());
        return;
    }

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &CodeExecutor::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &CodeExecutor::onProcessError);
    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &CodeExecutor::onReadyReadStdout);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &CodeExecutor::onReadyReadStderr);

    // 30 second timeout
    QTimer* timeout = new QTimer(this);
    timeout->setSingleShot(true);
    connect(timeout, &QTimer::timeout, this, [this]() {
        if (m_process && m_running) {
            m_process->kill();
            emit statusMessage(tr("执行超时 (30s)，已终止。"));
        }
    });
    timeout->start(30000);

    m_running = true;
    m_process->start(executable, args);

    if (!m_process->waitForStarted(10000)) {
        m_running = false;
        QString err = m_process->errorString();
        emit executionError(
            tr("无法启动 %1。\n路径: %2\n错误: %3\n请检查 Settings -> AI Configuration 中的路径。")
                .arg(language, executable, err), QString());
        m_process->deleteLater();
        m_process = nullptr;
        QFile::remove(scriptPath);
    }
}

void CodeExecutor::onReadyReadStdout()
{
    if (m_process) {
        QString output = QString::fromLocal8Bit(m_process->readAllStandardOutput());
        m_stdoutBuffer += output;
        if (!output.trimmed().isEmpty())
            qDebug() << "[stdout]" << output.trimmed();
    }
}

void CodeExecutor::onReadyReadStderr()
{
    if (m_process) {
        QString output = QString::fromLocal8Bit(m_process->readAllStandardError());
        m_stderrBuffer += output;
        if (!output.trimmed().isEmpty())
            qDebug() << "[stderr]" << output.trimmed();
    }
}

void CodeExecutor::onProcessError(QProcess::ProcessError error)
{
    if (!m_process) return;

    QString errorMsg;
    switch (error) {
    case QProcess::FailedToStart:
        errorMsg = tr("Failed to start the interpreter. Please check your Python/R installation path.");
        break;
    case QProcess::Timedout:
        errorMsg = tr("Code execution timed out.");
        break;
    default:
        errorMsg = tr("Process error: %1").arg(m_process->errorString());
        break;
    }

    m_running = false;
    emit executionError(errorMsg, m_stderrBuffer);

    m_process->deleteLater();
    m_process = nullptr;
}

void CodeExecutor::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (!m_process) return;

    m_running = false;

    // Check if output image was created
    bool imageCreated = QFile::exists(m_outputImagePath);

    if (exitStatus == QProcess::CrashExit) {
        emit executionError(tr("代码执行崩溃\n\nStderr:\n%1").arg(m_stderrBuffer), m_stderrBuffer);
    } else if (exitCode != 0 || !imageCreated) {
        QString errMsg;
        if (!imageCreated) {
            errMsg = tr("代码已运行但未生成图表。\n\n"
                        "Stdout: %1\n\nStderr: %2\n\n"
                        "提示: 请检查代码是否正确使用了 DATASET_PATH 和 OUTPUT_PATH 变量。")
                .arg(m_stdoutBuffer.isEmpty() ? QString::fromUtf8("(空)") : m_stdoutBuffer,
                     m_stderrBuffer.isEmpty() ? QString::fromUtf8("(空)") : m_stderrBuffer);
        } else {
            errMsg = tr("代码退出码 %1\n\nStderr:\n%2")
                .arg(exitCode).arg(m_stderrBuffer);
        }
        emit executionError(errMsg, m_stderrBuffer);
    } else {
        emit statusMessage(tr("Plot generated successfully!"));
        emit executionFinished(m_outputImagePath, m_stdoutBuffer);
    }

    m_process->deleteLater();
    m_process = nullptr;
}
