#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QIcon>

#include "ui/MainWindow.h"
#include "core/AppConfig.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName("NexusDraw");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("NexusDraw");

    // Set application icon
    app.setWindowIcon(QIcon(":/logo.png"));

    // Set default sans-serif font for the entire application
    QFont appFont("Microsoft YaHei", 10);
    appFont.setStyleHint(QFont::SansSerif);
    app.setFont(appFont);

    // Load global stylesheet
    QFile styleFile(":/style.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QString style = QString::fromUtf8(styleFile.readAll());
        app.setStyleSheet(style);
        styleFile.close();
    }

    // Initialize configuration
    AppConfig::instance();

    // Ensure temp directories exist
    QString tempDir = QDir::tempPath() + "/NexusDraw/output";
    QDir().mkpath(tempDir);

    // Create and show main window
    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
