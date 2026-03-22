#pragma once

#include <QObject>

class ClipboardMonitor;

class PasteInjector : public QObject {
    Q_OBJECT

public:
    explicit PasteInjector(ClipboardMonitor *monitor, QObject *parent = nullptr);

public slots:
    void pasteClip(qint64 clipId);

private:
    ClipboardMonitor *m_monitor;
    QString m_sessionType;
    bool m_autoPasteX11 = false;  // optional: simulate Ctrl+V on X11 via xdotool

    void autoPasteX11(const QString &windowId);
};
