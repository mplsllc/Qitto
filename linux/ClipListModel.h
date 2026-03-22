#pragma once

#include <QAbstractListModel>
#include <QVector>
#include <QString>
#include <QByteArray>
#include <cstdint>

struct ClipListItem {
    qint64 id;
    qint64 date;
    QString text;
    bool isGroup;
    double clipOrder;
    double stickyClipOrder;
    uint32_t crc;
    bool hasImage = false;
    QByteArray imageData;  // raw CF_DIB/PNG data for thumbnail
};

class ClipListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        DateRole,
        IsGroupRole,
        CrcRole,
        HasImageRole,
        ImageDataRole
    };

    explicit ClipListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void refresh(const QString &searchText = QString());
    void loadMore();
    bool canLoadMore() const;

    qint64 clipIdAt(int row) const;

private:
    QVector<ClipListItem> m_items;
    QString m_searchText;
    int m_pageSize = 50;
    bool m_hasMore = false;

    void loadImageData(ClipListItem &item);
};
