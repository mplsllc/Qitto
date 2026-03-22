#pragma once

#include <QSettings>
#include <QString>
#include <QStandardPaths>

class Settings {
public:
    static Settings &instance();

    QString databasePath() const;
    int maxEntries() const;
    int autoDeleteDays() const;
    bool allowDuplicates() const;
    bool allowBackToBackDuplicates() const;

    QString showPopupHotkey() const;
    void setShowPopupHotkey(const QString &hotkey);

    int pasteDelayMs() const;
    void setPasteDelayMs(int ms);

    bool autoPasteX11() const;
    void setAutoPasteX11(bool enabled);

    int linesPerRow() const;
    double transparency() const;

    int descTextSize() const;

    void sync();

private:
    Settings();
    QSettings m_settings;
};
