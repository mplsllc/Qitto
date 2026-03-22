#pragma once

#include <QObject>
#include <QDBusObjectPath>

/// Injects keyboard input via the XDG RemoteDesktop portal.
/// Works on all Wayland compositors that support the portal (KDE, GNOME, etc.)
/// Requires one-time user approval via a system dialog.
class PortalInputInjector : public QObject {
    Q_OBJECT

public:
    explicit PortalInputInjector(QObject *parent = nullptr);
    ~PortalInputInjector() override;

    bool init();
    bool isReady() const { return m_ready; }
    bool sendCtrlV();

private slots:
    void onCreateSessionResponse(uint response, const QVariantMap &results);
    void onSelectDevicesResponse(uint response, const QVariantMap &results);
    void onStartResponse(uint response, const QVariantMap &results);

private:
    QDBusObjectPath m_sessionHandle;
    QString m_requestToken;
    bool m_ready = false;
    int m_initStep = 0;

    QString nextRequestToken();
    void selectDevices();
    void startSession();
    void sendKey(uint32_t keycode, bool pressed);
};
