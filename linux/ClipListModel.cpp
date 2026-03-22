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
    default:
        return {};
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
