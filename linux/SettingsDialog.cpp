#include "SettingsDialog.h"
#include "Settings.h"
#include "QittoApp.h"

#include <QTabWidget>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QDir>
#include <QStandardPaths>
#include <QFile>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Qitto Settings");
    setMinimumSize(400, 350);
    setupUi();
    loadSettings();
}

void SettingsDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QTabWidget *tabs = new QTabWidget(this);

    // General
    QWidget *generalTab = new QWidget;
    QFormLayout *generalLayout = new QFormLayout(generalTab);
    m_autostart = new QCheckBox("Start Qitto on login");
    generalLayout->addRow(m_autostart);
    m_allowDuplicates = new QCheckBox("Allow duplicate clips");
    generalLayout->addRow(m_allowDuplicates);
    tabs->addTab(generalTab, "General");

    // Database
    QWidget *dbTab = new QWidget;
    QFormLayout *dbLayout = new QFormLayout(dbTab);
    m_maxEntries = new QSpinBox;
    m_maxEntries->setRange(10, 100000);
    m_maxEntries->setSuffix(" clips");
    dbLayout->addRow("Maximum clips:", m_maxEntries);
    m_autoDeleteDays = new QSpinBox;
    m_autoDeleteDays->setRange(0, 3650);
    m_autoDeleteDays->setSpecialValueText("Never");
    m_autoDeleteDays->setSuffix(" days");
    dbLayout->addRow("Auto-delete after:", m_autoDeleteDays);
    tabs->addTab(dbTab, "Database");

    // Hotkeys
    QWidget *hotkeyTab = new QWidget;
    QFormLayout *hotkeyLayout = new QFormLayout(hotkeyTab);
    m_showPopupHotkey = new QLineEdit;
    m_showPopupHotkey->setPlaceholderText("e.g. Ctrl+`");
    hotkeyLayout->addRow("Show popup:", m_showPopupHotkey);
    hotkeyLayout->addRow(new QLabel("Restart Qitto for hotkey changes to take effect."));
    tabs->addTab(hotkeyTab, "Hotkeys");

    // Display
    QWidget *displayTab = new QWidget;
    QFormLayout *displayLayout = new QFormLayout(displayTab);
    m_linesPerRow = new QSpinBox;
    m_linesPerRow->setRange(1, 20);
    displayLayout->addRow("Lines per clip:", m_linesPerRow);
    m_transparency = new QDoubleSpinBox;
    m_transparency->setRange(0.1, 1.0);
    m_transparency->setSingleStep(0.05);
    m_transparency->setDecimals(2);
    displayLayout->addRow("Window opacity:", m_transparency);
    m_pasteDelay = new QSpinBox;
    m_pasteDelay->setRange(10, 1000);
    m_pasteDelay->setSuffix(" ms");
    displayLayout->addRow("Paste delay:", m_pasteDelay);
    displayLayout->addRow(new QLabel("Increase if paste injection is unreliable (50-200ms typical)."));
    tabs->addTab(displayTab, "Display");

    mainLayout->addWidget(tabs);

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);
}

void SettingsDialog::loadSettings()
{
    Settings &s = Settings::instance();
    m_maxEntries->setValue(s.maxEntries());
    m_autoDeleteDays->setValue(s.autoDeleteDays());
    m_allowDuplicates->setChecked(s.allowDuplicates());
    m_showPopupHotkey->setText(s.showPopupHotkey());
    m_linesPerRow->setValue(s.linesPerRow());
    m_transparency->setValue(s.transparency());
    m_pasteDelay->setValue(s.pasteDelayMs());

    QString autostartPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                            + "/autostart/qitto.desktop";
    m_autostart->setChecked(QFile::exists(autostartPath));
}

void SettingsDialog::accept()
{
    Settings &s = Settings::instance();
    s.setPasteDelayMs(m_pasteDelay->value());
    s.setShowPopupHotkey(m_showPopupHotkey->text());

    QSettings qs(QSettings::IniFormat, QSettings::UserScope, "qitto", "qitto");
    qs.setValue("database/maxEntries", m_maxEntries->value());
    qs.setValue("database/autoDeleteDays", m_autoDeleteDays->value());
    qs.setValue("clipboard/allowDuplicates", m_allowDuplicates->isChecked());
    qs.setValue("ui/linesPerRow", m_linesPerRow->value());
    qs.setValue("ui/transparency", m_transparency->value());
    qs.sync();

    // Autostart
    QString autostartDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart";
    QString autostartPath = autostartDir + "/qitto.desktop";

    if (m_autostart->isChecked()) {
        QDir().mkpath(autostartDir);
        QFile f(autostartPath);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write("[Desktop Entry]\n"
                    "Name=Qitto\n"
                    "Comment=Clipboard Manager\n"
                    "Exec=qitto\n"
                    "Icon=qitto\n"
                    "Terminal=false\n"
                    "Type=Application\n"
                    "Categories=Utility;\n"
                    "StartupNotify=false\n"
                    "X-GNOME-Autostart-enabled=true\n");
            f.close();
        }
    } else {
        QFile::remove(autostartPath);
    }

    QittoApp::dbg("Settings saved");
    QDialog::accept();
}
