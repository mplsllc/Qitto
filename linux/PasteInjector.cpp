// PasteInjector — loads clip formats from DB via real CClip::LoadFormats,
// sets QClipboard, then simulates Ctrl+V via xdotool/ydotool.

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
}

void PasteInjector::pasteClip(qint64 clipId)
{
    QittoApp::dbg("PasteInjector: starting paste for clip id=" + QString::number(clipId));

    // Step 1: Record active window
    QString activeWid = getActiveWindowId();
    QittoApp::dbg("PasteInjector: active window=" + activeWid);

    // Step 2: Load clip from DB using Ditto's real LoadFormats
    CClip clip;
    clip.LoadMainTable((int)clipId);
    if (!clip.LoadFormats((int)clipId)) {
        QittoApp::dbg("PasteInjector: failed to load formats for clip " + QString::number(clipId));
        return;
    }

    // Step 3: Build QMimeData from loaded CClipFormats
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

    // Step 4: Set clipboard with self-ignore
    m_monitor->setSelfIgnore(true);
    QApplication::clipboard()->setMimeData(mimeData);

    // Step 5: Paste after delay
    int delayMs = Settings::instance().pasteDelayMs();
    QittoApp::dbg("PasteInjector: paste delay=" + QString::number(delayMs) + "ms");

    QTimer::singleShot(delayMs, this, [this, activeWid]() {
        if (!activeWid.isEmpty())
            activateWindow(activeWid);

        QTimer::singleShot(30, this, [this]() {
            simulatePaste();
            QittoApp::dbg("PasteInjector: paste complete");
        });
    });
}

QString PasteInjector::getActiveWindowId()
{
    if (m_sessionType == "wayland")
        return QString();

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

void PasteInjector::simulatePaste()
{
    if (m_sessionType == "wayland")
        simulatePasteWayland();
    else
        simulatePasteX11();
}

void PasteInjector::simulatePasteX11()
{
    QProcess proc;
    proc.start("xdotool", {"key", "--clearmodifiers", "ctrl+v"});
    proc.waitForFinished(2000);

    if (proc.exitCode() != 0)
        QittoApp::dbg("PasteInjector: xdotool failed: " + QString::fromUtf8(proc.readAllStandardError()));
    else
        QittoApp::dbg("PasteInjector: xdotool ctrl+v sent");
}

void PasteInjector::simulatePasteWayland()
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
