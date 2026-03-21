// ClipboardMonitor — captures clipboard changes via QClipboard and writes
// to Ditto's database using the real CClip::AddToDB path.

#include "ClipboardMonitor.h"
#include "FormatMapper.h"
#include "QittoApp.h"

// Ditto headers
#include "StdAfx.h"
#include "Clip.h"
#include "Misc.h"

#include <QApplication>
#include <QMimeData>

ClipboardMonitor::ClipboardMonitor(QObject *parent)
    : QObject(parent)
{
}

void ClipboardMonitor::start()
{
    m_active = true;
    connect(QApplication::clipboard(), &QClipboard::dataChanged,
            this, &ClipboardMonitor::onClipboardChanged);
    QittoApp::dbg("Clipboard monitor started");
}

void ClipboardMonitor::stop()
{
    m_active = false;
    disconnect(QApplication::clipboard(), &QClipboard::dataChanged,
               this, &ClipboardMonitor::onClipboardChanged);
    QittoApp::dbg("Clipboard monitor stopped");
}

void ClipboardMonitor::setSelfIgnore(bool ignore)
{
    m_selfIgnore = ignore;
}

void ClipboardMonitor::onClipboardChanged()
{
    if (!m_active) return;

    if (m_selfIgnore) {
        m_selfIgnore = false;
        QittoApp::dbg("Clipboard change ignored (self-paste)");
        return;
    }

    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    if (!mimeData) return;

    FormatMapper &mapper = FormatMapper::instance();
    QittoApp::dbg("Clipboard changed — formats: " + mimeData->formats().join(", "));

    CClip clip;

    // Set description from plain text if available
    if (mimeData->hasText()) {
        QString text = mimeData->text();
        if (text.length() > 200)
            text = text.left(200);
        clip.m_Desc = text.toUtf8().constData();
    }

    // Capture each supported format into CClipFormat via the real AddFormat path
    for (const QString &mime : mimeData->formats()) {
        if (!mapper.isSupported(mime))
            continue;

        QByteArray data = mimeData->data(mime);
        if (data.isEmpty())
            continue;

        // Map MIME to Ditto format name, then to CLIPFORMAT ID
        QString dittoName = mapper.mimeToDitto(mime);
        CLIPFORMAT cfType = GetFormatID(dittoName.toUtf8().constData());

        // Use CClip::AddFormat — allocates HGLOBAL and appends to m_Formats
        clip.AddFormat(cfType, (void*)data.constData(), data.size(), false);

        QittoApp::dbg("  Captured: " + mime + " -> " + dittoName
                       + " (" + QString::number(data.size()) + " bytes)");
    }

    if (clip.m_Formats.GetSize() == 0) {
        QittoApp::dbg("  No supported formats, skipping");
        return;
    }

    // Set description from first text format if not already set
    if (clip.m_Desc.IsEmpty())
        clip.SetDescFromType();

    // Use Ditto's real AddToDB with duplicate checking
    if (clip.AddToDB(true)) {
        QittoApp::dbg("  Clip saved: id=" + QString::number(clip.m_id));
        emit clipCaptured(clip.m_id);
    } else {
        QittoApp::dbg("  AddToDB returned false");
    }
}
