#pragma once

#include <QStyledItemDelegate>

class ClipDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit ClipDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

    void setLinesPerRow(int lines);
    void setSearchHighlight(const QString &text);

private:
    int m_linesPerRow = 4;
    QString m_highlight;
};
