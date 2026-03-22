#include "Settings.h"
#include <QDir>

Settings &Settings::instance()
{
    static Settings s;
    return s;
}

Settings::Settings()
    : m_settings(QSettings::IniFormat, QSettings::UserScope, "qitto", "qitto")
{
}

QString Settings::databasePath() const
{
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                          + "/Ditto.db";
    return m_settings.value("database/path", defaultPath).toString();
}

int Settings::maxEntries() const { return m_settings.value("database/maxEntries", 500).toInt(); }
int Settings::autoDeleteDays() const { return m_settings.value("database/autoDeleteDays", 0).toInt(); }
bool Settings::allowDuplicates() const { return m_settings.value("clipboard/allowDuplicates", false).toBool(); }
bool Settings::allowBackToBackDuplicates() const { return m_settings.value("clipboard/allowBackToBackDuplicates", false).toBool(); }

QString Settings::showPopupHotkey() const { return m_settings.value("hotkeys/showPopup", "Meta+V").toString(); }
void Settings::setShowPopupHotkey(const QString &hotkey) { m_settings.setValue("hotkeys/showPopup", hotkey); }

int Settings::pasteDelayMs() const { return m_settings.value("paste/delayMs", 50).toInt(); }
void Settings::setPasteDelayMs(int ms) { m_settings.setValue("paste/delayMs", ms); }

int Settings::linesPerRow() const { return m_settings.value("ui/linesPerRow", 4).toInt(); }
double Settings::transparency() const { return m_settings.value("ui/transparency", 1.0).toDouble(); }
int Settings::descTextSize() const { return m_settings.value("ui/descTextSize", 200).toInt(); }

void Settings::sync() { m_settings.sync(); }
