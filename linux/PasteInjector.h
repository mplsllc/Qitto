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

    QString getActiveWindowId();
    void activateWindow(const QString &windowId);
    void simulatePaste();
    void simulatePasteX11();
    void simulatePasteWayland();
};
