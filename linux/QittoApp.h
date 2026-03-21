#pragma once

#include <QApplication>
#include <QCommandLineParser>
#include <QSystemTrayIcon>
#include <QMenu>

class MainWindow;

class QittoApp : public QApplication {
    Q_OBJECT

public:
    QittoApp(int &argc, char **argv);
    ~QittoApp() override;

    static QittoApp *instance();
    bool isDebug() const { return m_debug; }

    static void dbg(const QString &msg);

    void setupTrayIcon(MainWindow *mainWindow);

private:
    void parseArgs();
    bool m_debug = false;
    QSystemTrayIcon *m_trayIcon = nullptr;
    QMenu *m_trayMenu = nullptr;
};

#define qittoApp (static_cast<QittoApp *>(QCoreApplication::instance()))
