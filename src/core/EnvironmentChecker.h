#pragma once

#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QJsonObject>

/// Result of checking a specific package/tool
struct PackageStatus {
    QString name;
    bool installed = false;
    QString version;
    QString errorMessage;
};

/// Detects Python, R, and required plotting libraries.
/// Offers to auto-install missing dependencies via pip / install.packages.
class EnvironmentChecker : public QObject
{
    Q_OBJECT

public:
    explicit EnvironmentChecker(QObject* parent = nullptr);
    ~EnvironmentChecker() = default;

    /// Run full environment check (Python + R + libraries)
    void runFullCheck();

    /// Check Python availability
    PackageStatus checkPython();

    /// Check R availability
    PackageStatus checkR();

    /// Check Python packages (matplotlib, seaborn, pandas, numpy) — single QProcess
    QList<PackageStatus> checkPythonPackages();

    /// Check R packages (ggplot2, dplyr, readr)
    QList<PackageStatus> checkRPackages();

    /// Install missing Python packages via pip
    void installPythonPackages(const QStringList& packages);

    /// Install missing R packages via install.packages
    void installRPackages(const QStringList& packages);

    /// Returns true if Python is fully ready for plotting
    bool isPythonReady() const;

    /// Returns true if R is fully ready for plotting
    bool isRReady() const;

signals:
    /// Emitted after full check completes
    void checkCompleted(const QJsonObject& result);

    /// Emitted during installation for progress updates
    void installProgress(const QString& message);

    /// Emitted when installation finishes
    void installFinished(bool success, const QString& summary);

    /// Emitted on any status change
    void statusMessage(const QString& message);

private:
    /// Run a process and capture output
    QString runCommand(const QString& program, const QStringList& args, int timeoutMs = 15000);

    /// Check a single R package
    PackageStatus checkRPackage(const QString& packageName);

    QString m_pythonPath;
    QString m_rPath;
    QList<PackageStatus> m_pythonPkgs;
    QList<PackageStatus> m_rPkgs;
};
