#include "GlobalHotkey.h"
#include "QittoApp.h"

#include <QApplication>
#include <QProcessEnvironment>
#include <QGuiApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <QAction>

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>

static xcb_connection_t *getXcbConnection()
{
    auto *x11app = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    if (!x11app) return nullptr;
    return x11app->connection();
}

GlobalHotkey::GlobalHotkey(QObject *parent)
    : QObject(parent)
{
    m_sessionType = QProcessEnvironment::systemEnvironment().value("XDG_SESSION_TYPE", "x11");
    QittoApp::dbg("Session type: " + m_sessionType);
    QApplication::instance()->installNativeEventFilter(this);

    // Register D-Bus object so KGlobalAccel can call our Toggle slot
    QDBusConnection::sessionBus().registerObject(
        "/qitto", this, QDBusConnection::ExportScriptableSlots);
    QDBusConnection::sessionBus().registerService("com.qitto.app");
}

GlobalHotkey::~GlobalHotkey()
{
    unregisterHotkey();
    QApplication::instance()->removeNativeEventFilter(this);
}

bool GlobalHotkey::registerHotkey(const QKeySequence &keySequence)
{
    if (keySequence.isEmpty()) return false;
    unregisterHotkey();

    // Try KGlobalAccel first (works on KDE X11 + Wayland)
    if (registerKGlobalAccel(keySequence))
        return true;

    // Fall back to xcb_grab_key (X11 only)
    if (m_sessionType == "x11" || m_sessionType == "tty") {
        QKeyCombination combo = keySequence[0];
        return registerX11(combo.key(), combo.keyboardModifiers());
    }

    QittoApp::dbg("WARNING: No hotkey method available for this session type");
    return false;
}

void GlobalHotkey::unregisterHotkey()
{
    if (!m_registered) return;
    if (!m_usingKGlobalAccel && (m_sessionType == "x11" || m_sessionType == "tty"))
        unregisterX11();
    m_registered = false;
    m_usingKGlobalAccel = false;
}

// ============================================================================
// KGlobalAccel — works on KDE Plasma (X11 + Wayland)
// ============================================================================

bool GlobalHotkey::registerKGlobalAccel(const QKeySequence &keySequence)
{
    QDBusConnection bus = QDBusConnection::sessionBus();

    // Check if kglobalaccel is available
    QDBusMessage ping = QDBusMessage::createMethodCall(
        "org.kde.kglobalaccel", "/kglobalaccel",
        "org.freedesktop.DBus.Peer", "Ping");
    QDBusReply<void> pingReply = bus.call(ping, QDBus::Block, 500);
    if (!pingReply.isValid()) {
        QittoApp::dbg("KGlobalAccel not available, will try xcb fallback");
        return false;
    }

    // Register the shortcut with KGlobalAccel via the component/shortcut D-Bus API
    // Component ID: "qitto", Friendly name: "Qitto Clipboard Manager"
    // Shortcut ID: "toggle_popup", Friendly name: "Show/Hide Qitto"

    // Step 1: Register component
    // setShortcut(componentUnique, componentFriendly, shortcutUnique, shortcutFriendly, defaultKeys, activeKeys)
    // This uses the org.kde.KGlobalAccel interface

    // The KGlobalAccel D-Bus API expects:
    // doRegister(QStringList{componentUnique, componentFriendly, shortcutUnique, shortcutFriendly})
    // setShortcut(QStringList{...}, QList<int>{keySequence}, SetFlag)

    QStringList actionId;
    actionId << "qitto"                    // component unique name
             << "Qitto Clipboard Manager"  // component friendly name
             << "toggle_popup"             // shortcut unique name
             << "Show/Hide Qitto";         // shortcut friendly name

    // Convert key sequence to int list for KGlobalAccel
    QList<int> keys;
    keys << keySequence[0].toCombined();

    QList<int> defaultKeys = keys;

    // Call setShortcut
    QDBusMessage msg = QDBusMessage::createMethodCall(
        "org.kde.kglobalaccel",
        "/kglobalaccel",
        "org.kde.KGlobalAccel",
        "setShortcutKeys"
    );
    msg << QVariant::fromValue(actionId)
        << QVariant::fromValue(keys)
        << uint(0x02);  // SetPresent flag = autoloading

    QDBusReply<QList<int>> reply = bus.call(msg, QDBus::Block, 2000);

    if (!reply.isValid()) {
        // Try the older API: setShortcut with different signature
        QittoApp::dbg("KGlobalAccel setShortcutKeys failed: " + reply.error().message());
        QittoApp::dbg("Trying alternative registration...");

        // Alternative: use setForeignShortcut
        msg = QDBusMessage::createMethodCall(
            "org.kde.kglobalaccel",
            "/kglobalaccel",
            "org.kde.KGlobalAccel",
            "setForeignShortcutKeys"
        );
        msg << QVariant::fromValue(actionId)
            << QVariant::fromValue(keys);

        QDBusReply<void> reply2 = bus.call(msg, QDBus::Block, 2000);
        if (!reply2.isValid()) {
            QittoApp::dbg("KGlobalAccel registration failed: " + reply2.error().message());
            return false;
        }
    }

    // Connect to the KGlobalAccel notification signal
    bus.connect(
        "org.kde.kglobalaccel",
        "/kglobalaccel",
        "org.kde.KGlobalAccel",
        "yourShortcutGotChanged",
        this,
        SLOT(Toggle())
    );

    // Also listen for the invokeShortcut signal
    bus.connect(
        "org.kde.kglobalaccel",
        "/kglobalaccel",
        "org.kde.KGlobalAccel",
        "invokedShortcut",
        this,
        SLOT(Toggle())
    );

    m_registered = true;
    m_usingKGlobalAccel = true;
    QittoApp::dbg("KGlobalAccel: registered shortcut " + keySequence.toString());
    return true;
}

void GlobalHotkey::Toggle()
{
    QittoApp::dbg("Toggle() called via D-Bus/KGlobalAccel");
    emit activated();
}

// ============================================================================
// X11 fallback — xcb_grab_key
// ============================================================================

bool GlobalHotkey::registerX11(int qtKey, Qt::KeyboardModifiers qtMods)
{
    auto *conn = getXcbConnection();
    if (!conn) {
        QittoApp::dbg("Failed to get xcb connection");
        return false;
    }

    xcb_key_symbols_t *keysyms = xcb_key_symbols_alloc(conn);
    if (!keysyms) {
        QittoApp::dbg("Failed to allocate key symbols");
        return false;
    }

    xcb_keysym_t keysym = 0;
    if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z)
        keysym = XK_a + (qtKey - Qt::Key_A);
    else if (qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9)
        keysym = XK_0 + (qtKey - Qt::Key_0);
    else if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F12)
        keysym = XK_F1 + (qtKey - Qt::Key_F1);
    else if (qtKey == Qt::Key_QuoteLeft || qtKey == Qt::Key_Dead_Grave)
        keysym = XK_grave;
    else if (qtKey == Qt::Key_Space)
        keysym = XK_space;
    else {
        QittoApp::dbg("Unsupported key for X11 hotkey: " + QString::number(qtKey));
        xcb_key_symbols_free(keysyms);
        return false;
    }

    xcb_keycode_t *keycodes = xcb_key_symbols_get_keycode(keysyms, keysym);
    xcb_key_symbols_free(keysyms);

    if (!keycodes || keycodes[0] == XCB_NO_SYMBOL) {
        QittoApp::dbg("Failed to map keysym to keycode");
        free(keycodes);
        return false;
    }

    m_nativeKeycode = keycodes[0];
    free(keycodes);
    m_nativeModifiers = qtModsToX11(qtMods);

    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    xcb_window_t root = screen->root;

    uint16_t modVariants[] = {
        static_cast<uint16_t>(m_nativeModifiers),
        static_cast<uint16_t>(m_nativeModifiers | XCB_MOD_MASK_LOCK),
        static_cast<uint16_t>(m_nativeModifiers | XCB_MOD_MASK_2),
        static_cast<uint16_t>(m_nativeModifiers | XCB_MOD_MASK_LOCK | XCB_MOD_MASK_2),
    };

    for (auto mod : modVariants) {
        xcb_grab_key(conn, 1, root, mod, m_nativeKeycode,
                     XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    }

    xcb_flush(conn);
    m_registered = true;

    QittoApp::dbg("X11 hotkey registered: keycode=" + QString::number(m_nativeKeycode)
                   + " mods=0x" + QString::number(m_nativeModifiers, 16));
    return true;
}

void GlobalHotkey::unregisterX11()
{
    auto *conn = getXcbConnection();
    if (!conn) return;

    xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    xcb_window_t root = screen->root;

    uint16_t modVariants[] = {
        static_cast<uint16_t>(m_nativeModifiers),
        static_cast<uint16_t>(m_nativeModifiers | XCB_MOD_MASK_LOCK),
        static_cast<uint16_t>(m_nativeModifiers | XCB_MOD_MASK_2),
        static_cast<uint16_t>(m_nativeModifiers | XCB_MOD_MASK_LOCK | XCB_MOD_MASK_2),
    };

    for (auto mod : modVariants)
        xcb_ungrab_key(conn, m_nativeKeycode, root, mod);

    xcb_flush(conn);
    QittoApp::dbg("X11 hotkey unregistered");
}

uint GlobalHotkey::qtModsToX11(Qt::KeyboardModifiers mods) const
{
    uint x11mods = 0;
    if (mods & Qt::ControlModifier) x11mods |= XCB_MOD_MASK_CONTROL;
    if (mods & Qt::AltModifier)     x11mods |= XCB_MOD_MASK_1;
    if (mods & Qt::ShiftModifier)   x11mods |= XCB_MOD_MASK_SHIFT;
    if (mods & Qt::MetaModifier)    x11mods |= XCB_MOD_MASK_4;
    return x11mods;
}

bool GlobalHotkey::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *)
{
    if (!m_registered || m_usingKGlobalAccel) return false;
    if (eventType != "xcb_generic_event_t") return false;

    auto *event = static_cast<xcb_generic_event_t *>(message);
    if ((event->response_type & ~0x80) == XCB_KEY_PRESS) {
        auto *keyEvent = reinterpret_cast<xcb_key_press_event_t *>(event);
        uint cleanMods = keyEvent->state & ~(XCB_MOD_MASK_LOCK | XCB_MOD_MASK_2);

        if (keyEvent->detail == m_nativeKeycode && cleanMods == m_nativeModifiers) {
            QittoApp::dbg("X11 hotkey activated");
            emit activated();
            return true;
        }
    }

    return false;
}
