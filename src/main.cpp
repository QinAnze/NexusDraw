#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QIcon>

#include <QSplashScreen>
#include <QTimer>
#include <QPainter>
#include <QProgressBar>
#include <QThread>
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

    // Splash screen
    QFile splashFile(":/NEXUS.md");
    QString splashText;
    if (splashFile.open(QFile::ReadOnly | QFile::Text))
        splashText = QString::fromUtf8(splashFile.readAll());

    QPixmap splashPixmap(760, 370);
    splashPixmap.fill(Qt::white);
    QPainter painter(&splashPixmap);
    QFont monoFont("Consolas", 10);
    monoFont.setStyleHint(QFont::Monospace);
    painter.setFont(monoFont);
    painter.setPen(Qt::black);
    QRect textRect(0, 0, 760, 340);
    painter.drawText(textRect, Qt::AlignCenter, splashText);
    // Draw progress bar background
    painter.fillRect(QRect(80, 345, 600, 18), QColor("#e0e0e0"));
    painter.end();

    QSplashScreen splash(splashPixmap);
    QProgressBar* splashProgress = new QProgressBar(&splash);
    splashProgress->setGeometry(80, 345, 600, 18);
    splashProgress->setRange(0, 100);
    splashProgress->setValue(0);
    splashProgress->setTextVisible(false);
    splashProgress->setStyleSheet("QProgressBar { background: #e0e0e0; border: 2px solid #000; } QProgressBar::chunk { background: #000; }");

    splash.show();
    app.processEvents();

    // Animate progress
    for (int i = 0; i <= 100; i += 5) {
        splashProgress->setValue(i);
        app.processEvents();
        QThread::msleep(30);
    }

    // Create and show main window
    MainWindow mainWindow;
    splash.finish(&mainWindow);
    mainWindow.show();

    return app.exec();
}
