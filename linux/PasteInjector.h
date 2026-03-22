#pragma once

#include <QObject>
#include <memory>

class ClipboardMonitor;
class PortalInputInjector;

class PasteInjector : public QObject {
    Q_OBJECT

public:
    explicit PasteInjector(ClipboardMonitor *monitor, QObject *parent = nullptr);
    ~PasteInjector() override;

public slots:
    void pasteClip(qint64 clipId);

private:
    ClipboardMonitor *m_monitor;
    QString m_sessionType;
    std::unique_ptr<PortalInputInjector> m_portalInjector;
    bool m_autoPasteX11 = false;

    void autoPasteX11(const QString &windowId);
};
