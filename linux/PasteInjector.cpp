// PasteInjector — sets clipboard then injects Ctrl+V.
// On Wayland: uses XDG RemoteDesktop portal (one-time user approval).
// On X11: uses xdotool (optional, opt-in).
// Fallback: clipboard-set only, user pastes manually.

#include "PasteInjector.h"
#include "PortalInputInjector.h"
#include "ClipboardMonitor.h"
#include "FormatMapper.h"
#include "Settings.h"
#include "QittoApp.h"

#include "StdAfx.h"
#include "Clip.h"
#include "Misc.h"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTimer>

PasteInjector::PasteInjector(ClipboardMonitor *monitor, QObject *parent)
    : QObject(parent)
    , m_monitor(monitor)
{
    m_sessionType = QProcessEnvironment::systemEnvironment().value("XDG_SESSION_TYPE", "x11");

    if (m_sessionType == "wayland") {
        // Try RemoteDesktop portal for auto-paste on Wayland
        m_portalInjector = std::make_unique<PortalInputInjector>(this);
        if (m_portalInjector->init()) {
            QittoApp::dbg("PasteInjector: RemoteDesktop portal ready — auto-paste enabled");
        } else {
            QittoApp::dbg("PasteInjector: RemoteDesktop portal not available — clipboard-set only");
            m_portalInjector.reset();
        }
    } else if (m_sessionType == "x11") {
        m_autoPasteX11 = Settings::instance().autoPasteX11();
        QittoApp::dbg("PasteInjector: X11 auto-paste " + QString(m_autoPasteX11 ? "enabled" : "disabled"));
    }

    QittoApp::dbg("PasteInjector: session=" + m_sessionType);
}

PasteInjector::~PasteInjector() = default;

void PasteInjector::pasteClip(qint64 clipId)
{
    QittoApp::dbg("PasteInjector: clip id=" + QString::number(clipId));

    // Step 1: Load clip from DB
    CClip clip;
    clip.LoadMainTable((int)clipId);
    if (!clip.LoadFormats((int)clipId)) {
        QittoApp::dbg("PasteInjector: failed to load formats");
        return;
    }

    // Step 2: Record active window (X11 only)
    QString activeWid;
    if (m_autoPasteX11) {
        QProcess proc;
        proc.start("xdotool", {"getactivewindow"});
        proc.waitForFinished(1000);
        activeWid = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    }

    // Step 3: Build QMimeData
    QMimeData *mimeData = new QMimeData();
    FormatMapper &mapper = FormatMapper::instance();

    for (INT_PTR i = 0; i < clip.m_Formats.GetSize(); i++) {
        CClipFormat &cf = clip.m_Formats.ElementAt(i);
        CString dittoName = GetFormatName(cf.m_cfType);
        QString mime = mapper.dittoToMime(QString::fromUtf8((const char*)dittoName));

        void *data = GlobalLock(cf.m_hgData);
        int size = (int)GlobalSize(cf.m_hgData);
        if (data && size > 0)
            mimeData->setData(mime, QByteArray((const char*)data, size));
        GlobalUnlock(cf.m_hgData);
    }

    // Step 4: Set clipboard
    m_monitor->setSelfIgnore(true);
    QApplication::clipboard()->setMimeData(mimeData);
    QittoApp::dbg("PasteInjector: clipboard set");

    // Step 5: Auto-paste
    int delayMs = Settings::instance().pasteDelayMs();

    if (m_portalInjector && m_portalInjector->isReady()) {
        // Wayland — inject Ctrl+V via RemoteDesktop portal
        QTimer::singleShot(delayMs, this, [this]() {
            m_portalInjector->sendCtrlV();
        });
    } else if (m_autoPasteX11 && !activeWid.isEmpty()) {
        // X11 — inject via xdotool
        QTimer::singleShot(delayMs, this, [this, activeWid]() {
            autoPasteX11(activeWid);
        });
    } else {
        QittoApp::dbg("PasteInjector: clipboard set — user pastes with Ctrl+V");
    }
}

void PasteInjector::autoPasteX11(const QString &windowId)
{
    QProcess focus;
    focus.start("xdotool", {"windowactivate", "--sync", windowId});
    focus.waitForFinished(2000);

    QProcess paste;
    paste.start("xdotool", {"key", "--clearmodifiers", "ctrl+v"});
    paste.waitForFinished(2000);

    if (paste.exitCode() != 0)
        QittoApp::dbg("PasteInjector: xdotool failed: " + QString::fromUtf8(paste.readAllStandardError()));
    else
        QittoApp::dbg("PasteInjector: auto-paste sent via xdotool");
}
