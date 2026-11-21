// ============================================================================
//  main.cpp  –  ParkNexa Qt 6 Application Entry Point
// ============================================================================
#include <QApplication>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <cstdlib>
#include <ctime>

#include "services/AppContext.h"
#include "ui/LoginWindow.h"
#include "ui/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("ParkNexa");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("ParkNexa");

    // Seed random (used by salt generation in AuthService)
    srand(static_cast<unsigned>(time(nullptr)));

    // ── Resolve data directory ──────────────────────────────────────────────
    // Use the directory where the executable lives so data files are portable
    QString exeDir = QCoreApplication::applicationDirPath();
    // Walk up to find the 'data' folder (works from build/Release too)
    QString dataDir;
    QDir d(exeDir);
    // Try exe dir first, then parent dirs
    for (int i = 0; i < 4; i++) {
        if (QDir(d.filePath("data")).exists()) {
            dataDir = d.filePath("data");
            break;
        }
        d.cdUp();
    }
    if (dataDir.isEmpty()) {
        // Fallback: create data folder next to the exe
        dataDir = exeDir + "/data";
        QDir().mkpath(dataDir);
    }

    // ── Load global stylesheet ───────────────────────────────────────────────
    QString qssPath;
    QDir resDir(exeDir);
    for (int i = 0; i < 4; i++) {
        QString candidate = resDir.filePath("resources/styles/app.qss");
        if (QFile::exists(candidate)) { qssPath = candidate; break; }
        resDir.cdUp();
    }
    if (!qssPath.isEmpty()) {
        QFile qss(qssPath);
        if (qss.open(QFile::ReadOnly)) {
            app.setStyleSheet(QString::fromUtf8(qss.readAll()));
        }
    }

    // ── Create shared application context ────────────────────────────────────
    AppContext ctx(dataDir);

    // ── Create windows ────────────────────────────────────────────────────────
    LoginWindow  loginWin(&ctx);
    MainWindow*  mainWin  = nullptr;

    // When login succeeds, hide login window and show main window
    QObject::connect(&loginWin, &LoginWindow::loginSuccess, [&](){
        loginWin.hide();
        if (!mainWin) {
            mainWin = new MainWindow(&ctx);
            // When user logs out, go back to login
            QObject::connect(mainWin, &MainWindow::logoutRequested, [&](){
                mainWin->hide();
                loginWin.show();
            });
        } else {
            // Refresh all pages with the newly logged-in user
            mainWin->refreshAll();
        }
        mainWin->show();
        mainWin->raise();
        mainWin->activateWindow();
    });

    loginWin.show();
    return app.exec();
}
