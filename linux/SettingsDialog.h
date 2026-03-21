#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QDoubleSpinBox>

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

private slots:
    void accept() override;

private:
    void setupUi();
    void loadSettings();

    QCheckBox *m_autostart;
    QCheckBox *m_allowDuplicates;
    QSpinBox *m_maxEntries;
    QSpinBox *m_autoDeleteDays;
    QLineEdit *m_showPopupHotkey;
    QSpinBox *m_linesPerRow;
    QDoubleSpinBox *m_transparency;
    QSpinBox *m_pasteDelay;
};
