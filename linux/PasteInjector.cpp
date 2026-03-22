// PasteInjector — Klipper-style: set clipboard, hide popup, user pastes.
// On X11, optionally simulates Ctrl+V via xdotool (opt-in setting).

#include "PasteInjector.h"
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

    // Auto-paste only available on X11 (xdotool works reliably there)
    // Disabled by default — user enables in Settings if they want it
    if (m_sessionType == "x11") {
        m_autoPasteX11 = Settings::instance().autoPasteX11();
        QittoApp::dbg("PasteInjector: X11 auto-paste " + QString(m_autoPasteX11 ? "enabled" : "disabled"));
    }

    QittoApp::dbg("PasteInjector: session=" + m_sessionType + " mode="
                   + (m_autoPasteX11 ? "auto-paste" : "clipboard-set"));
}

void PasteInjector::pasteClip(qint64 clipId)
{
    QittoApp::dbg("PasteInjector: clip id=" + QString::number(clipId));

    // Step 1: Load clip from DB
    CClip clip;
    clip.LoadMainTable((int)clipId);
    if (!clip.LoadFormats((int)clipId)) {
        QittoApp::dbg("PasteInjector: failed to load formats for clip " + QString::number(clipId));
        return;
    }

    // Step 2: Record active window (X11 auto-paste only)
    QString activeWid;
    if (m_autoPasteX11) {
        QProcess proc;
        proc.start("xdotool", {"getactivewindow"});
        proc.waitForFinished(1000);
        activeWid = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        QittoApp::dbg("PasteInjector: active window=" + activeWid);
    }

    // Step 3: Build QMimeData from CClipFormats
    QMimeData *mimeData = new QMimeData();
    FormatMapper &mapper = FormatMapper::instance();

    for (INT_PTR i = 0; i < clip.m_Formats.GetSize(); i++) {
        CClipFormat &cf = clip.m_Formats.ElementAt(i);
        CString dittoName = GetFormatName(cf.m_cfType);
        QString mime = mapper.dittoToMime(QString::fromUtf8((const char*)dittoName));

        void *data = GlobalLock(cf.m_hgData);
        int size = (int)GlobalSize(cf.m_hgData);
        if (data && size > 0) {
            mimeData->setData(mime, QByteArray((const char*)data, size));
            QittoApp::dbg("PasteInjector: set " + mime + " (" + QString::number(size) + " bytes)");
        }
        GlobalUnlock(cf.m_hgData);
    }

    // Step 4: Set clipboard (self-ignore so monitor doesn't re-capture)
    m_monitor->setSelfIgnore(true);
    QApplication::clipboard()->setMimeData(mimeData);
    QittoApp::dbg("PasteInjector: clipboard set — "
                   + QString(m_autoPasteX11 ? "will auto-paste" : "user pastes with Ctrl+V"));

    // Step 5: On X11 with auto-paste, simulate Ctrl+V after a short delay
    if (m_autoPasteX11 && !activeWid.isEmpty()) {
        int delayMs = Settings::instance().pasteDelayMs();
        QTimer::singleShot(delayMs, this, [this, activeWid]() {
            autoPasteX11(activeWid);
        });
    }
}

void PasteInjector::autoPasteX11(const QString &windowId)
{
    // Restore focus to original window
    QProcess focus;
    focus.start("xdotool", {"windowactivate", "--sync", windowId});
    focus.waitForFinished(2000);

    // Simulate Ctrl+V
    QProcess paste;
    paste.start("xdotool", {"key", "--clearmodifiers", "ctrl+v"});
    paste.waitForFinished(2000);

    if (paste.exitCode() != 0)
        QittoApp::dbg("PasteInjector: xdotool failed: " + QString::fromUtf8(paste.readAllStandardError()));
    else
        QittoApp::dbg("PasteInjector: auto-paste sent via xdotool");
}
