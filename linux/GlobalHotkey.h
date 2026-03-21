#pragma once

#include <QObject>
#include <QKeySequence>
#include <QAbstractNativeEventFilter>

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

    bool registerX11(int qtKey, Qt::KeyboardModifiers qtMods);
    void unregisterX11();
    uint qtModsToX11(Qt::KeyboardModifiers mods) const;
};
