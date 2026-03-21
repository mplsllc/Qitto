#pragma once

#include <QObject>

class GlobalHotkey;
class MainWindow;

class HotkeyManager : public QObject {
    Q_OBJECT

public:
    explicit HotkeyManager(MainWindow *mainWindow, QObject *parent = nullptr);
    ~HotkeyManager() override;

    void registerAll();
    void unregisterAll();

private:
    GlobalHotkey *m_showPopupHotkey;
    MainWindow *m_mainWindow;
};
