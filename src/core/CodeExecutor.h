#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

/// Executes Python or R code via QProcess and captures the output.
/// Generated plots are saved as images and loaded back into the application.
class CodeExecutor : public QObject
{
    Q_OBJECT

public:
    explicit CodeExecutor(QObject* parent = nullptr);
    ~CodeExecutor();

    /// Execute the given code with the specified language.
    /// @param code      The Python or R source code
    /// @param language  "python" or "r"
    /// @param dataPath  Path to the dataset CSV file
    /// @param outputImagePath  Where to save the generated plot image
    void execute(const QString& code, const QString& language,
                 const QString& dataPath, const QString& outputImagePath);

    /// Check if execution is currently in progress
    bool isRunning() const;

    /// Cancel the running process
    void cancel();

    /// Check if Python is available on the system
    static bool isPythonAvailable();

    /// Check if R is available on the system
    static bool isRAvailable();

    /// Get Python version string
    static QString pythonVersion();

    /// Get R version string
    static QString rVersion();

signals:
    /// Emitted when code execution completes successfully with the output image path
    void executionFinished(const QString& outputImagePath, const QString& stdOut);

    /// Emitted when execution fails
    void executionError(const QString& errorMessage, const QString& stdErr);

    /// Emitted for progress updates
    void statusMessage(const QString& message);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onReadyReadStdout();
    void onReadyReadStderr();

private:
    /// Wrap user code with necessary imports and save logic for Python
    QString wrapPythonCode(const QString& userCode, const QString& dataPath, const QString& outputPath) const;

    /// Wrap user code with necessary imports and save logic for R
    QString wrapRCode(const QString& userCode, const QString& dataPath, const QString& outputPath) const;

    /// Create a temporary file with the given content
    QString createTempFile(const QString& content, const QString& extension) const;

    QProcess* m_process = nullptr;
    QString m_outputImagePath;
    QString m_stdoutBuffer;
    QString m_stderrBuffer;
    bool m_running = false;
};
