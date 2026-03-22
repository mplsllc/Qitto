// PortalInputInjector — uses org.freedesktop.portal.RemoteDesktop
// to inject Ctrl+V on Wayland. Requires one-time user approval.

#include "PortalInputInjector.h"
#include "QittoApp.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusPendingCallWatcher>
#include <QCoreApplication>
#include <QThread>
#include <QMessageBox>

static int s_tokenCounter = 0;

PortalInputInjector::PortalInputInjector(QObject *parent)
    : QObject(parent)
{
}

PortalInputInjector::~PortalInputInjector()
{
    // Close the session if we have one
    if (!m_sessionHandle.path().isEmpty()) {
        QDBusMessage msg = QDBusMessage::createMethodCall(
            "org.freedesktop.portal.Desktop",
            m_sessionHandle.path(),
            "org.freedesktop.portal.Session",
            "Close");
        QDBusConnection::sessionBus().call(msg, QDBus::NoBlock);
    }
}

QString PortalInputInjector::nextRequestToken()
{
    return QString("qitto_%1").arg(++s_tokenCounter);
}

bool PortalInputInjector::init()
{
    QittoApp::dbg("PortalInputInjector: initializing...");

    // Explain to the user why we need this permission
    QMessageBox::information(nullptr, "Qitto — Auto-Paste Setup",
        "Qitto needs keyboard input permission to automatically paste "
        "clips into your apps.\n\n"
        "A system dialog will appear next asking to allow \"Remote Desktop\" "
        "access. This is just keyboard simulation (like pressing Ctrl+V for you) "
        "— Qitto does not record your screen or input.\n\n"
        "This is a one-time approval per session.");
    m_initStep = 0;

    // Step 1: CreateSession
    QString token = nextRequestToken();
    QString senderName = QDBusConnection::sessionBus().baseService().mid(1).replace('.', '_');
    QString requestPath = "/org/freedesktop/portal/desktop/request/" + senderName + "/" + token;

    // Connect to the Response signal BEFORE making the call
    QDBusConnection::sessionBus().connect(
        "org.freedesktop.portal.Desktop",
        requestPath,
        "org.freedesktop.portal.Request",
        "Response",
        this,
        SLOT(onCreateSessionResponse(uint, QVariantMap)));

    QVariantMap options;
    options["handle_token"] = token;
    options["session_handle_token"] = "qitto_session";

    QDBusMessage msg = QDBusMessage::createMethodCall(
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.RemoteDesktop",
        "CreateSession");
    msg << options;

    QDBusMessage reply = QDBusConnection::sessionBus().call(msg, QDBus::Block, 5000);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        QittoApp::dbg("PortalInputInjector: CreateSession failed: " + reply.errorMessage());
        return false;
    }

    QittoApp::dbg("PortalInputInjector: CreateSession called, waiting for response...");

    // Process events to receive the Response signal.
    // The user needs time to click "Allow" in the system dialog —
    // give up to 60 seconds for the full flow.
    for (int i = 0; i < 1200 && !m_ready; i++) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        if (!m_ready)
            QThread::msleep(50);
    }

    if (!m_ready) {
        QittoApp::dbg("PortalInputInjector: init timed out or user denied access");
        return false;
    }

    return true;
}

void PortalInputInjector::onCreateSessionResponse(uint response, const QVariantMap &results)
{
    if (response != 0) {
        QittoApp::dbg("PortalInputInjector: CreateSession denied (response=" + QString::number(response) + ")");
        return;
    }

    m_sessionHandle = QDBusObjectPath(results.value("session_handle").toString());
    QittoApp::dbg("PortalInputInjector: session created: " + m_sessionHandle.path());
    m_initStep = 1;

    selectDevices();
}

void PortalInputInjector::selectDevices()
{
    QString token = nextRequestToken();
    QString senderName = QDBusConnection::sessionBus().baseService().mid(1).replace('.', '_');
    QString requestPath = "/org/freedesktop/portal/desktop/request/" + senderName + "/" + token;

    QDBusConnection::sessionBus().connect(
        "org.freedesktop.portal.Desktop",
        requestPath,
        "org.freedesktop.portal.Request",
        "Response",
        this,
        SLOT(onSelectDevicesResponse(uint, QVariantMap)));

    QVariantMap options;
    options["handle_token"] = token;
    options["types"] = uint(1);  // 1 = keyboard

    QDBusMessage msg = QDBusMessage::createMethodCall(
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.RemoteDesktop",
        "SelectDevices");
    msg << QVariant::fromValue(m_sessionHandle) << options;

    QDBusConnection::sessionBus().call(msg, QDBus::Block, 5000);
    QittoApp::dbg("PortalInputInjector: SelectDevices called");
}

void PortalInputInjector::onSelectDevicesResponse(uint response, const QVariantMap &results)
{
    Q_UNUSED(results);
    if (response != 0) {
        QittoApp::dbg("PortalInputInjector: SelectDevices denied");
        return;
    }

    QittoApp::dbg("PortalInputInjector: devices selected");
    m_initStep = 2;

    startSession();
}

void PortalInputInjector::startSession()
{
    QString token = nextRequestToken();
    QString senderName = QDBusConnection::sessionBus().baseService().mid(1).replace('.', '_');
    QString requestPath = "/org/freedesktop/portal/desktop/request/" + senderName + "/" + token;

    QDBusConnection::sessionBus().connect(
        "org.freedesktop.portal.Desktop",
        requestPath,
        "org.freedesktop.portal.Request",
        "Response",
        this,
        SLOT(onStartResponse(uint, QVariantMap)));

    QVariantMap options;
    options["handle_token"] = token;

    QDBusMessage msg = QDBusMessage::createMethodCall(
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.RemoteDesktop",
        "Start");
    msg << QVariant::fromValue(m_sessionHandle)
        << QString("")  // parent window identifier (empty = no parent)
        << options;

    QDBusConnection::sessionBus().call(msg, QDBus::Block, 30000); // Long timeout — user prompt
    QittoApp::dbg("PortalInputInjector: Start called (waiting for user approval)");
}

void PortalInputInjector::onStartResponse(uint response, const QVariantMap &results)
{
    Q_UNUSED(results);
    if (response != 0) {
        QittoApp::dbg("PortalInputInjector: Start denied by user (response=" + QString::number(response) + ")");
        return;
    }

    m_ready = true;
    m_initStep = 3;
    QittoApp::dbg("PortalInputInjector: ready — input injection approved");
}

void PortalInputInjector::sendKey(uint32_t keycode, bool pressed)
{
    QVariantMap options;
    QDBusMessage msg = QDBusMessage::createMethodCall(
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.RemoteDesktop",
        "NotifyKeyboardKeycode");
    msg << QVariant::fromValue(m_sessionHandle)
        << options
        << int(keycode)
        << uint(pressed ? 1 : 0);  // 1 = pressed, 0 = released

    QDBusConnection::sessionBus().call(msg, QDBus::NoBlock);
}

bool PortalInputInjector::sendCtrlV()
{
    if (!m_ready) {
        QittoApp::dbg("PortalInputInjector: not ready, can't send Ctrl+V");
        return false;
    }

    // evdev keycodes: KEY_LEFTCTRL=29, KEY_V=47
    sendKey(29, true);   // Ctrl down
    sendKey(47, true);   // V down
    sendKey(47, false);  // V up
    sendKey(29, false);  // Ctrl up

    QittoApp::dbg("PortalInputInjector: Ctrl+V sent");
    return true;
}
