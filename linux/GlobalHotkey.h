#pragma once

#include <QObject>
#include <QKeySequence>
#include <QAbstractNativeEventFilter>

/// Global hotkey — uses KGlobalAccel D-Bus on KDE (works on both X11 and Wayland),
/// falls back to xcb_grab_key on plain X11 without KDE.
class GlobalHotkey : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT

public:
    explicit GlobalHotkey(QObject *parent = nullptr);
    ~GlobalHotkey() override;

    bool registerHotkey(const QKeySequence &keySequence);
    void unregisterHotkey();
    bool isRegistered() const { return m_registered; }

signals:
    void activated();

protected:
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    bool m_registered = false;
    QString m_sessionType;
    uint m_nativeKeycode = 0;
    uint m_nativeModifiers = 0;

    // KGlobalAccel (works on KDE X11 + Wayland)
    bool registerKGlobalAccel(const QKeySequence &keySequence);

    // xcb fallback (X11 only, non-KDE)
    bool registerX11(int qtKey, Qt::KeyboardModifiers qtMods);
    void unregisterX11();
    uint qtModsToX11(Qt::KeyboardModifiers mods) const;

    bool m_usingKGlobalAccel = false;

public slots:
    Q_SCRIPTABLE void Toggle();
    void onShortcutPressed(const QStringList &actionId);
};
