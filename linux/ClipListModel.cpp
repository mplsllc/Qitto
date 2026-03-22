// ClipListModel — reads from Ditto's real Main table via CppSQLite3

#include "ClipListModel.h"
#include "QittoApp.h"

#include "StdAfx.h"
#include "DatabaseUtilities.h"
#include "sqlite/CppSQLite3.h"
#include "Misc.h"

#include <QDateTime>

ClipListModel::ClipListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ClipListModel::rowCount(const QModelIndex &) const
{
    return m_items.size();
}

QVariant ClipListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};

    const auto &item = m_items[index.row()];

    switch (role) {
    case Qt::DisplayRole:
        return item.text.left(500);
    case IdRole:
        return item.id;
    case DateRole:
        return QDateTime::fromSecsSinceEpoch(item.date).toString("yyyy-MM-dd hh:mm");
    case IsGroupRole:
        return item.isGroup;
    case CrcRole:
        return item.crc;
    case HasImageRole:
        return item.hasImage;
    case ImageDataRole:
        return item.imageData;
    default:
        return {};
    }
}

void ClipListModel::loadImageData(ClipListItem &item)
{
    // Check if this clip has an image format (CF_DIB or PNG)
    try {
        CppSQLite3Query q = GetDittoDB().execQueryEx(
            "SELECT strClipBoardFormat, ooData FROM Data WHERE lParentID = %d "
            "AND (strClipBoardFormat = 'CF_DIB' OR strClipBoardFormat = 'PNG' "
            "OR strClipBoardFormat = 'CF_BITMAP') LIMIT 1",
            (int)item.id);

        if (q.eof() == false) {
            int nDataLen = 0;
            const unsigned char *cData = q.getBlobField("ooData", nDataLen);
            if (cData && nDataLen > 0) {
                item.hasImage = true;
                item.imageData = QByteArray(reinterpret_cast<const char*>(cData), nDataLen);
            }
        }
    }
    catch (CppSQLite3Exception &) {
        // Not an image clip, that's fine
    }
}

void ClipListModel::refresh(const QString &searchText)
{
    beginResetModel();
    m_items.clear();
    m_searchText = searchText;

    try {
        CString sql;
        if (m_searchText.isEmpty()) {
            sql.Format("SELECT lID, lDate, mText, CRC, bIsGroup, clipOrder, stickyClipOrder "
                       "FROM Main WHERE lParentID = -1 "
                       "ORDER BY stickyClipOrder DESC, bIsGroup ASC, clipOrder DESC "
                       "LIMIT %d", m_pageSize + 1);
        } else {
            CString search = m_searchText.toUtf8().constData();
            search.Replace("'", "''");
            sql.Format("SELECT lID, lDate, mText, CRC, bIsGroup, clipOrder, stickyClipOrder "
                       "FROM Main WHERE lParentID = -1 AND mText LIKE '%%%s%%' "
                       "ORDER BY stickyClipOrder DESC, bIsGroup ASC, clipOrder DESC "
                       "LIMIT %d", (const char*)search, m_pageSize + 1);
        }

        CppSQLite3Query q = GetDittoDB().execQuery(sql);

        while (q.eof() == false) {
            ClipListItem item;
            item.id = q.getIntField("lID");
            item.date = q.getInt64Field("lDate");
            item.text = QString::fromUtf8(q.getStringField("mText"));
            item.crc = q.getIntField("CRC");
            item.isGroup = q.getIntField("bIsGroup") != 0;
            item.clipOrder = q.getFloatField("clipOrder");
            item.stickyClipOrder = q.getFloatField("stickyClipOrder");

            // Check for image data if text is empty or looks like a format name
            if (item.text.isEmpty() || item.text.startsWith("CF_"))
                loadImageData(item);

            m_items.append(item);
            q.nextRow();
        }
    }
    catch (CppSQLite3Exception &e) {
        QittoApp::dbg("ClipListModel query failed: " + QString::fromUtf8(e.errorMessage()));
    }

    m_hasMore = m_items.size() > m_pageSize;
    if (m_hasMore)
        m_items.removeLast();

    endResetModel();
    QittoApp::dbg("Model refreshed: " + QString::number(m_items.size()) + " items");
}

void ClipListModel::loadMore()
{
    if (!m_hasMore) return;

    try {
        CString sql;
        int offset = m_items.size();
        if (m_searchText.isEmpty()) {
            sql.Format("SELECT lID, lDate, mText, CRC, bIsGroup, clipOrder, stickyClipOrder "
                       "FROM Main WHERE lParentID = -1 "
                       "ORDER BY stickyClipOrder DESC, bIsGroup ASC, clipOrder DESC "
                       "LIMIT %d OFFSET %d", m_pageSize + 1, offset);
        } else {
            CString search = m_searchText.toUtf8().constData();
            search.Replace("'", "''");
            sql.Format("SELECT lID, lDate, mText, CRC, bIsGroup, clipOrder, stickyClipOrder "
                       "FROM Main WHERE lParentID = -1 AND mText LIKE '%%%s%%' "
                       "ORDER BY stickyClipOrder DESC, bIsGroup ASC, clipOrder DESC "
                       "LIMIT %d OFFSET %d", (const char*)search, m_pageSize + 1, offset);
        }

        CppSQLite3Query q = GetDittoDB().execQuery(sql);

        QVector<ClipListItem> newItems;
        while (q.eof() == false) {
            ClipListItem item;
            item.id = q.getIntField("lID");
            item.date = q.getInt64Field("lDate");
            item.text = QString::fromUtf8(q.getStringField("mText"));
            item.crc = q.getIntField("CRC");
            item.isGroup = q.getIntField("bIsGroup") != 0;
            item.clipOrder = q.getFloatField("clipOrder");
            item.stickyClipOrder = q.getFloatField("stickyClipOrder");

            if (item.text.isEmpty() || item.text.startsWith("CF_"))
                loadImageData(item);

            newItems.append(item);
            q.nextRow();
        }

        m_hasMore = newItems.size() > m_pageSize;
        if (m_hasMore)
            newItems.removeLast();

        if (!newItems.isEmpty()) {
            beginInsertRows(QModelIndex(), m_items.size(), m_items.size() + newItems.size() - 1);
            m_items.append(newItems);
            endInsertRows();
        }
    }
    catch (CppSQLite3Exception &e) {
        QittoApp::dbg("ClipListModel loadMore failed: " + QString::fromUtf8(e.errorMessage()));
    }
}

bool ClipListModel::canLoadMore() const { return m_hasMore; }

qint64 ClipListModel::clipIdAt(int row) const
{
    if (row >= 0 && row < m_items.size())
        return m_items[row].id;
    return -1;
}
