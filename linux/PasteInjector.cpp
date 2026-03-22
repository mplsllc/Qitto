// PasteInjector — loads clip formats from DB via real CClip::LoadFormats,
// sets QClipboard, then simulates Ctrl+V via xdotool/ydotool/wtype.

#include "PasteInjector.h"
#include "ClipboardMonitor.h"
#include "FormatMapper.h"
#include "Settings.h"
#include "QittoApp.h"

// Ditto headers
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

    // Detect available paste tools
    if (m_sessionType == "wayland") {
        // Check for wtype first (best Wayland support), then ydotool
        QProcess check;
        check.start("which", {"wtype"});
        check.waitForFinished(1000);
        if (check.exitCode() == 0) {
            m_pasteTool = "wtype";
        } else {
            check.start("which", {"ydotool"});
            check.waitForFinished(1000);
            m_pasteTool = (check.exitCode() == 0) ? "ydotool" : "none";
        }
    } else {
        QProcess check;
        check.start("which", {"xdotool"});
        check.waitForFinished(1000);
        m_pasteTool = (check.exitCode() == 0) ? "xdotool" : "none";
    }
    QittoApp::dbg("PasteInjector: session=" + m_sessionType + " tool=" + m_pasteTool);
}

void PasteInjector::pasteClip(qint64 clipId)
{
    QittoApp::dbg("PasteInjector: starting paste for clip id=" + QString::number(clipId));

    // Step 1: Record active window (X11 only)
    QString activeWid;
    if (m_sessionType != "wayland") {
        activeWid = getActiveWindowId();
        QittoApp::dbg("PasteInjector: active window=" + activeWid);
    }

    // Step 2: Load clip from DB using Ditto's real LoadFormats
    CClip clip;
    clip.LoadMainTable((int)clipId);
    if (!clip.LoadFormats((int)clipId)) {
        QittoApp::dbg("PasteInjector: failed to load formats for clip " + QString::number(clipId));
        return;
    }

    // Update last paste date
    clip.m_lastPasteDate = CTime::GetCurrentTime();
    // TODO: persist lastPasteDate update

    // Step 3: Build QMimeData from loaded CClipFormats
    QMimeData *mimeData = new QMimeData();
    FormatMapper &mapper = FormatMapper::instance();
    QString plainText;

    for (INT_PTR i = 0; i < clip.m_Formats.GetSize(); i++) {
        CClipFormat &cf = clip.m_Formats.ElementAt(i);
        CString dittoName = GetFormatName(cf.m_cfType);
        QString mime = mapper.dittoToMime(QString::fromUtf8((const char*)dittoName));

        void *data = GlobalLock(cf.m_hgData);
        int size = (int)GlobalSize(cf.m_hgData);
        if (data && size > 0) {
            QByteArray ba((const char*)data, size);
            mimeData->setData(mime, ba);

            // Track plain text for wtype fallback
            if (mime == "text/plain" && plainText.isEmpty())
                plainText = QString::fromUtf8(ba);

            QittoApp::dbg("PasteInjector: set " + mime + " (" + QString::number(size) + " bytes)");
        }
        GlobalUnlock(cf.m_hgData);
    }

    // Step 4: Set clipboard with self-ignore
    m_monitor->setSelfIgnore(true);
    QApplication::clipboard()->setMimeData(mimeData);
    QittoApp::dbg("PasteInjector: clipboard set");

    // Step 5: Simulate paste
    int delayMs = Settings::instance().pasteDelayMs();

    if (m_pasteTool == "none") {
        // No paste tool available — clipboard is set, user pastes manually
        QittoApp::dbg("PasteInjector: no paste tool, clipboard set — user pastes with Ctrl+V");
        return;
    }

    QittoApp::dbg("PasteInjector: will paste in " + QString::number(delayMs) + "ms via " + m_pasteTool);

    // Save state for the lambda
    QString tool = m_pasteTool;
    QString session = m_sessionType;

    QTimer::singleShot(delayMs, this, [this, activeWid, tool, session, plainText]() {
        // Restore focus on X11
        if (session != "wayland" && !activeWid.isEmpty())
            activateWindow(activeWid);

        QTimer::singleShot(50, this, [this, tool, plainText]() {
            if (tool == "xdotool")
                simulatePasteXdotool();
            else if (tool == "wtype")
                simulatePasteWtype(plainText);
            else if (tool == "ydotool")
                simulatePasteYdotool();

            QittoApp::dbg("PasteInjector: paste complete");
        });
    });
}

QString PasteInjector::getActiveWindowId()
{
    QProcess proc;
    proc.start("xdotool", {"getactivewindow"});
    proc.waitForFinished(1000);
    return QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
}

void PasteInjector::activateWindow(const QString &windowId)
{
    if (windowId.isEmpty()) return;

    QProcess proc;
    proc.start("xdotool", {"windowactivate", "--sync", windowId});
    proc.waitForFinished(2000);
    QittoApp::dbg("PasteInjector: activated window " + windowId);
}

void PasteInjector::simulatePasteXdotool()
{
    QProcess proc;
    proc.start("xdotool", {"key", "--clearmodifiers", "ctrl+v"});
    proc.waitForFinished(2000);

    if (proc.exitCode() != 0)
        QittoApp::dbg("PasteInjector: xdotool failed: " + QString::fromUtf8(proc.readAllStandardError()));
    else
        QittoApp::dbg("PasteInjector: xdotool ctrl+v sent");
}

void PasteInjector::simulatePasteWtype(const QString &text)
{
    // wtype can either type text directly or simulate key combos
    // Simulate Ctrl+V to paste from clipboard
    QProcess proc;
    proc.start("wtype", {"-M", "ctrl", "-P", "v", "-p", "v", "-m", "ctrl"});
    proc.waitForFinished(2000);

    if (proc.exitCode() != 0) {
        QittoApp::dbg("PasteInjector: wtype key sim failed, trying direct type");
        // Fallback: type text directly (only works for plain text)
        if (!text.isEmpty()) {
            QProcess proc2;
            proc2.start("wtype", {text});
            proc2.waitForFinished(2000);
        }
    } else {
        QittoApp::dbg("PasteInjector: wtype ctrl+v sent");
    }
}

void PasteInjector::simulatePasteYdotool()
{
    // ydotool scancodes: 29=LCtrl, 47=V
    QProcess proc;
    proc.start("ydotool", {"key", "29:1", "47:1", "47:0", "29:0"});
    proc.waitForFinished(2000);

    if (proc.exitCode() != 0)
        QittoApp::dbg("PasteInjector: ydotool failed: " + QString::fromUtf8(proc.readAllStandardError()));
    else
        QittoApp::dbg("PasteInjector: ydotool ctrl+v sent");
}
