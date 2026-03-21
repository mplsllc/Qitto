#include "HotkeyManager.h"
#include "GlobalHotkey.h"
#include "MainWindow.h"
#include "Settings.h"
#include "QittoApp.h"

#include <QKeySequence>

HotkeyManager::HotkeyManager(MainWindow *mainWindow, QObject *parent)
    : QObject(parent)
    , m_showPopupHotkey(new GlobalHotkey(this))
    , m_mainWindow(mainWindow)
{
    connect(m_showPopupHotkey, &GlobalHotkey::activated, [this]() {
        if (m_mainWindow->isVisible())
            m_mainWindow->hidePopup();
        else
            m_mainWindow->showPopup();
    });
}

HotkeyManager::~HotkeyManager()
{
    unregisterAll();
}

void HotkeyManager::registerAll()
{
    QString hotkeyStr = Settings::instance().showPopupHotkey();
    QKeySequence seq(hotkeyStr);

    if (seq.isEmpty()) {
        QittoApp::dbg("No hotkey configured");
        return;
    }

    if (m_showPopupHotkey->registerHotkey(seq))
        QittoApp::dbg("Registered show popup hotkey: " + hotkeyStr);
    else
        QittoApp::dbg("Failed to register hotkey: " + hotkeyStr);
}

void HotkeyManager::unregisterAll()
{
    m_showPopupHotkey->unregisterHotkey();
}
