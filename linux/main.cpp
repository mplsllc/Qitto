// main.cpp — Qitto entry point for Linux
// Initializes DB via Ditto's real CreateDB/OpenDatabase, starts clipboard
// monitoring, and shows system tray icon.

#include "QittoApp.h"
#include "Settings.h"
#include "MainWindow.h"
#include "ClipboardMonitor.h"
#include "HotkeyManager.h"
#include "PasteInjector.h"

// Ditto headers — the real ported code
#include "StdAfx.h"
#include "DatabaseUtilities.h"

#include <QDir>
#include <QStandardPaths>

int main(int argc, char *argv[])
{
    QittoApp app(argc, argv);

    // Ensure data directory exists
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dataDir);

    Settings &settings = Settings::instance();

    // Open database using Ditto's real DatabaseUtilities
    CString dbPath = settings.databasePath().toUtf8().constData();
    QittoApp::dbg("Opening database: " + settings.databasePath());

    if (!CreateDB(dbPath)) {
        QittoApp::dbg("FATAL: CreateDB failed");
        return 1;
    }

    if (!OpenDatabase(dbPath)) {
        QittoApp::dbg("FATAL: OpenDatabase failed");
        return 1;
    }

    if (!ValidDB(dbPath, TRUE)) {
        QittoApp::dbg("FATAL: ValidDB failed");
        return 1;
    }

    QittoApp::dbg("Database ready at " + settings.databasePath());

    // Start clipboard monitoring
    ClipboardMonitor monitor;
    monitor.start();

    // Main popup window
    MainWindow mainWindow(&monitor);

    // Paste injection
    PasteInjector pasteInjector(&monitor);
    QObject::connect(&mainWindow, &MainWindow::clipSelected,
                     &pasteInjector, &PasteInjector::pasteClip);

    // Global hotkey
    HotkeyManager hotkeyManager(&mainWindow);
    hotkeyManager.registerAll();

    // System tray
    app.setupTrayIcon(&mainWindow);

    QittoApp::dbg("Qitto running — press " + settings.showPopupHotkey() + " to toggle popup");

    int ret = app.exec();
    hotkeyManager.unregisterAll();
    monitor.stop();
    return ret;
}
