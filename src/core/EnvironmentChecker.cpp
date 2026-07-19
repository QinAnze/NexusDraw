#include "EnvironmentChecker.h"
#include "AppConfig.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
#include <QDir>
#include <QFile>

EnvironmentChecker::EnvironmentChecker(QObject* parent)
    : QObject(parent)
{
    const AppConfig& cfg = AppConfig::instance();
    m_pythonPath = cfg.pythonPath();
    m_rPath = cfg.rPath();
}

QString EnvironmentChecker::runCommand(const QString& program, const QStringList& args, int timeoutMs)
{
    QProcess proc;
    proc.start(program, args);
    if (!proc.waitForFinished(timeoutMs)) {
        proc.kill();
        return QString();
    }
    QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());
    if (output.isEmpty()) {
        output = QString::fromLocal8Bit(proc.readAllStandardError());
    }
    return output.trimmed();
}

PackageStatus EnvironmentChecker::checkPython()
{
    PackageStatus status;
    status.name = "Python";

    // Try ALL known locations — bundled, configured, system installs, then PATH
    QString bundled = AppConfig::instance().bundledPythonPath();
    QStringList candidates = {
        bundled,       // Bundled Python (highest priority)
        m_pythonPath,  // User-configured path
        // System Python installations (Windows) - newest first
        "C:/Users/15205/AppData/Local/Programs/Python/Python313/python.exe",
        "C:/Users/15205/AppData/Local/Programs/Python/Python312/python.exe",
        "C:/Users/15205/AppData/Local/Programs/Python/Python311/python.exe",
        "C:/Program Files/Python313/python.exe",
        "C:/Program Files/Python312/python.exe",
        "C:/Python313/python.exe",
        // PATH fallbacks (last resort)
        "python",
        "python3",
    };

    // Also search AppData for any Python version (reverse = newest first)
    QDir appDataPython("C:/Users/15205/AppData/Local/Programs/Python");
    if (appDataPython.exists()) {
        QStringList versions = appDataPython.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (int i = versions.size() - 1; i >= 0; --i) {
            QString candidate = appDataPython.absoluteFilePath(versions[i] + "/python.exe");
            if (QFile::exists(candidate) && !candidates.contains(candidate)) {
                candidates.append(candidate);
            }
        }
    }

    for (const QString& py : candidates) {
        if (py.isEmpty()) continue;
        QString ver = runCommand(py, {"--version"}, 5000);
        if (!ver.isEmpty() && (ver.contains("Python") || ver.contains("python"))) {
            status.installed = true;
            status.version = ver;
            m_pythonPath = py;
            // Update AppConfig so CodeExecutor uses the correct path
            AppConfig::instance().setPythonPath(py);
            return status;
        }
    }

    status.installed = false;
    status.errorMessage = tr("未找到 Python。请安装 Python 3.8+ 并添加到 PATH。");
    return status;
}

PackageStatus EnvironmentChecker::checkR()
{
    PackageStatus status;
    status.name = "R";

    // Direct path checks (QDir scanning fails on MinGW)
    QStringList candidates = {
        m_rPath,
        "D:/testworld/NexusDraw/portable_r/bin/Rscript.exe",
        "D:\\testworld\\NexusDraw\\portable_r\\bin\\Rscript.exe",
        "C:/Program Files/R/R-4.6.1/bin/Rscript.exe",
        "C:/Program Files/R/R-4.6.0/bin/Rscript.exe",
        "Rscript",
    };

    for (const QString& r : candidates) {
        if (r.isEmpty()) continue;
        if (!QFile::exists(r) && r != "Rscript" && r != "R") continue;
        QString ver = runCommand(r, {"--version"}, 5000);
        if (!ver.isEmpty() && (ver.contains("R scripting") || ver.contains("R version"))) {
            status.installed = true;
            status.version = ver;
            m_rPath = r;
            AppConfig::instance().setRPath(r);
            return status;
        }
    }

    status.installed = false;
    status.errorMessage = tr("未找到 R。请安装 R 4.0+ 并添加到 PATH。");
    return status;
}

QList<PackageStatus> EnvironmentChecker::checkPythonPackages()
{
    // Check ALL packages in ONE Python process (more reliable than separate QProcess calls)
    QList<QPair<QString, QString>> pkgs = {
        {"pandas",     "pandas"},
        {"numpy",      "numpy"},
        {"matplotlib", "matplotlib"},
        {"seaborn",    "seaborn"},
    };

    // Build a single script that checks all packages and outputs JSON
    QString script = "import importlib, json, sys; r={};";
    for (const auto& pkg : pkgs) {
        script += QString(
            "try:\n"
            "  m=importlib.import_module('%1')\n"
            "  r['%1']=getattr(m,'__version__','ok')\n"
            "except Exception:\n"
            "  r['%1']=None\n").arg(pkg.second);
    }
    script += "print(json.dumps(r)); sys.exit(0);";

    QProcess proc;
    proc.start(m_pythonPath, {"-c", script});
    bool finished = proc.waitForFinished(10000);

    m_pythonPkgs.clear();

    if (!finished || proc.exitCode() != 0) {
        // Process failed entirely - mark all as not installed
        for (const auto& pkg : pkgs) {
            PackageStatus st;
            st.name = pkg.first;
            st.installed = false;
            st.errorMessage = tr("无法检测 %1").arg(pkg.first);
            m_pythonPkgs.append(st);
        }
        return m_pythonPkgs;
    }

    // Parse JSON output
    QString output = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    QJsonObject result = doc.object();

    for (const auto& pkg : pkgs) {
        PackageStatus st;
        st.name = pkg.first;
        if (result.contains(pkg.second) && !result[pkg.second].isNull()) {
            st.installed = true;
            st.version = result[pkg.second].toString();
        } else {
            st.installed = false;
            st.errorMessage = tr("未安装 %1").arg(pkg.first);
        }
        m_pythonPkgs.append(st);
    }

    return m_pythonPkgs;
}

PackageStatus EnvironmentChecker::checkRPackage(const QString& packageName)
{
    PackageStatus status;
    status.name = packageName;

    QString script = QString(
        ".libPaths(c(Sys.getenv('R_LIBS_USER'), .libPaths())); "
        "suppressPackageStartupMessages({"
        "  if (require('%1', quietly=TRUE, character.only=TRUE)) {"
        "    cat(as.character(packageVersion('%1'))); quit(status=0);"
        "  } else {"
        "    quit(status=1);"
        "  }"
        "})").arg(packageName);

    QProcess proc;
    proc.start(m_rPath, {"--no-save", "--no-restore", "-e", script});
    bool finished = proc.waitForFinished(10000);

    if (!finished) {
        proc.kill();
        status.installed = false;
        status.errorMessage = tr("R 进程无响应");
        return status;
    }

    if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
        status.installed = false;
        status.errorMessage = tr("未安装 %1").arg(packageName);
    } else {
        status.installed = true;
        status.version = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
    }

    return status;
}

QList<PackageStatus> EnvironmentChecker::checkRPackages()
{
    QStringList pkgs = {"ggplot2", "dplyr", "readr"};
    m_rPkgs.clear();
    for (const auto& pkg : pkgs) {
        PackageStatus st = checkRPackage(pkg);
        m_rPkgs.append(st);
    }
    return m_rPkgs;
}

void EnvironmentChecker::runFullCheck()
{
    emit statusMessage(tr("正在检测环境..."));

    QJsonObject result;

    // Check Python
    PackageStatus py = checkPython();
    QJsonObject pyObj;
    pyObj["installed"] = py.installed;
    pyObj["version"] = py.version;
    pyObj["error"] = py.errorMessage;
    result["python"] = pyObj;

    // Check Python packages
    if (py.installed) {
        QList<PackageStatus> pyPkgs = checkPythonPackages();
        QJsonArray pyArr;
        for (const auto& p : pyPkgs) {
            QJsonObject pObj;
            pObj["name"] = p.name;
            pObj["installed"] = p.installed;
            pObj["version"] = p.version;
            pyArr.append(pObj);
        }
        result["pythonPackages"] = pyArr;
    }

    // Check R
    PackageStatus r = checkR();
    QJsonObject rObj;
    rObj["installed"] = r.installed;
    rObj["version"] = r.version;
    rObj["error"] = r.errorMessage;
    result["r"] = rObj;

    // Check R packages
    if (r.installed) {
        QList<PackageStatus> rPkgs = checkRPackages();
        QJsonArray rArr;
        for (const auto& p : rPkgs) {
            QJsonObject pObj;
            pObj["name"] = p.name;
            pObj["installed"] = p.installed;
            pObj["version"] = p.version;
            rArr.append(pObj);
        }
        result["rPackages"] = rArr;
    }

    emit checkCompleted(result);
}

bool EnvironmentChecker::isPythonReady() const
{
    for (const auto& p : m_pythonPkgs) {
        if (!p.installed) return false;
    }
    return !m_pythonPkgs.isEmpty();
}

bool EnvironmentChecker::isRReady() const
{
    for (const auto& p : m_rPkgs) {
        if (!p.installed) return false;
    }
    return !m_rPkgs.isEmpty();
}

void EnvironmentChecker::installPythonPackages(const QStringList& packages)
{
    if (packages.isEmpty()) {
        emit installFinished(true, tr("所有包已安装。"));
        return;
    }

    emit installProgress(tr("正在安装: %1 ...").arg(packages.join(", ")));

    QStringList args;
    args << "-m" << "pip" << "install";
    for (const auto& pkg : packages) {
        args << pkg;
    }

    QProcess* proc = new QProcess(this);
    connect(proc, &QProcess::readyReadStandardOutput, this, [this, proc]() {
        QString out = QString::fromUtf8(proc->readAllStandardOutput()).trimmed();
        if (!out.isEmpty()) emit installProgress(out);
    });
    connect(proc, &QProcess::readyReadStandardError, this, [this, proc]() {
        QString err = QString::fromUtf8(proc->readAllStandardError()).trimmed();
        if (!err.isEmpty()) emit installProgress(err);
    });

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc, packages](int exitCode, QProcess::ExitStatus) {
        if (exitCode == 0) {
            emit installFinished(true, tr("Python 包安装成功: %1").arg(packages.join(", ")));
        } else {
            QString err = QString::fromUtf8(proc->readAllStandardError());
            emit installFinished(false, tr("安装失败: %1").arg(err));
        }
        proc->deleteLater();
    });

    proc->start(m_pythonPath, args);
}

void EnvironmentChecker::installRPackages(const QStringList& packages)
{
    if (packages.isEmpty()) {
        emit installFinished(true, tr("所有包已安装。"));
        return;
    }

    emit installProgress(tr("正在安装 R 包: %1 ...").arg(packages.join(", ")));

    // Build R script to install packages
    QString pkgsStr;
    for (const auto& pkg : packages) {
        pkgsStr += QString("install.packages('%1', repos='https://cloud.r-project.org', quiet=TRUE); ").arg(pkg);
    }

    QStringList args;
    args << "--no-save" << "--no-restore" << "-e" << pkgsStr;

    QProcess* proc = new QProcess(this);
    connect(proc, &QProcess::readyReadStandardOutput, this, [this, proc]() {
        QString out = QString::fromUtf8(proc->readAllStandardOutput()).trimmed();
        if (!out.isEmpty()) emit installProgress(out);
    });

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc, packages](int exitCode, QProcess::ExitStatus) {
        if (exitCode == 0) {
            emit installFinished(true, tr("R 包安装成功: %1").arg(packages.join(", ")));
        } else {
            emit installFinished(false, tr("R 包安装失败，请手动安装。"));
        }
        proc->deleteLater();
    });

    proc->start(m_rPath, args);
}
