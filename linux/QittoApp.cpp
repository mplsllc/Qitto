#include "QittoApp.h"
#include "MainWindow.h"
#include "SettingsDialog.h"

#include <QDateTime>
#include <QIcon>
#include <QMessageBox>
#include <QPixmap>
#include <iostream>

QittoApp::QittoApp(int &argc, char **argv)
    : QApplication(argc, argv)
{
    setApplicationName("Qitto");
    setApplicationVersion("0.1.0");
    setOrganizationName("MPLS LLC");
    setQuitOnLastWindowClosed(false);

    parseArgs();

    if (m_debug)
        dbg("Qitto starting in debug mode");
}

QittoApp::~QittoApp()
{
    delete m_trayMenu;
}

QittoApp *QittoApp::instance()
{
    return qittoApp;
}

void QittoApp::parseArgs()
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Qitto — Clipboard Manager");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption debugOption(
        QStringList() << "d" << "debug",
        "Enable debug logging to stdout");
    parser.addOption(debugOption);

    parser.process(*this);
    m_debug = parser.isSet(debugOption);
}

void QittoApp::dbg(const QString &msg)
{
    if (!qittoApp || !qittoApp->isDebug())
        return;
    std::cout << "[" << QDateTime::currentDateTime().toString("hh:mm:ss.zzz").toStdString()
              << "] " << msg.toStdString() << std::endl;
}

void QittoApp::setupTrayIcon(MainWindow *mainWindow)
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        dbg("System tray not available");
        return;
    }

    m_trayIcon = new QSystemTrayIcon(this);

    QIcon icon = QIcon::fromTheme("edit-paste", QIcon::fromTheme("clipboard"));
    if (icon.isNull()) {
        QPixmap pix(32, 32);
        pix.fill(QColor(0x4a, 0x9e, 0xd6));
        icon = QIcon(pix);
    }
    m_trayIcon->setIcon(icon);
    m_trayIcon->setToolTip("Qitto — Clipboard Manager");

    m_trayMenu = new QMenu;

    QAction *showAction = m_trayMenu->addAction("Show Clipboard");
    connect(showAction, &QAction::triggered, mainWindow, &MainWindow::showPopup);

    m_trayMenu->addSeparator();

    QAction *settingsAction = m_trayMenu->addAction("Settings...");
    connect(settingsAction, &QAction::triggered, []() {
        SettingsDialog dlg;
        dlg.exec();
    });

    QAction *aboutAction = m_trayMenu->addAction("About Qitto");
    connect(aboutAction, &QAction::triggered, []() {
        QMessageBox::about(nullptr, "About Qitto",
            "<h3>Qitto v0.1.0</h3>"
            "<p>Clipboard manager for Linux — Qt6 port of Ditto.</p>"
            "<p>&copy; MPLS LLC</p>");
    });

    m_trayMenu->addSeparator();

    QAction *quitAction = m_trayMenu->addAction("Quit");
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_trayIcon->setContextMenu(m_trayMenu);

    connect(m_trayIcon, &QSystemTrayIcon::activated,
            [mainWindow](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            if (mainWindow->isVisible())
                mainWindow->hidePopup();
            else
                mainWindow->showPopup();
        }
    });

    m_trayIcon->show();
    dbg("System tray icon active");
}
