#include "ClipDelegate.h"
#include "ClipListModel.h"

#include <QPainter>
#include <QApplication>
#include <QPixmap>
#include <QImage>

ClipDelegate::ClipDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void ClipDelegate::setLinesPerRow(int lines) { m_linesPerRow = lines; }
void ClipDelegate::setSearchHighlight(const QString &text) { m_highlight = text; }

void ClipDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // Background — selected or hover
    if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, option.palette.highlight());
    else if (option.state & QStyle::State_MouseOver)
        painter->fillRect(option.rect, option.palette.midlight());

    int row = index.row();
    QString text = index.data(Qt::DisplayRole).toString();
    bool hasImage = index.data(ClipListModel::HasImageRole).toBool();
    QByteArray imageData = index.data(ClipListModel::ImageDataRole).toByteArray();

    QRect cellRect = option.rect.adjusted(2, 2, -2, -2);

    // Number label on the left (1-9)
    int numWidth = 0;
    if (row < 9) {
        numWidth = 22;
        QRect numRect(cellRect.left(), cellRect.top(), numWidth, cellRect.height());

        QFont numFont = option.font;
        numFont.setBold(true);
        numFont.setPointSize(numFont.pointSize() - 1);
        painter->setFont(numFont);

        if (option.state & QStyle::State_Selected)
            painter->setPen(option.palette.color(QPalette::HighlightedText));
        else
            painter->setPen(option.palette.color(QPalette::Disabled, QPalette::Text));

        painter->drawText(numRect, Qt::AlignCenter, QString::number(row + 1));
    }

    QRect contentRect = cellRect.adjusted(numWidth + 2, 0, 0, 0);

    // Set text color
    if (option.state & QStyle::State_Selected)
        painter->setPen(option.palette.color(QPalette::HighlightedText));
    else
        painter->setPen(option.palette.color(QPalette::Text));

    painter->setFont(option.font);

    if (hasImage && !imageData.isEmpty()) {
        // Image clip — show thumbnail + format label
        QImage img;
        img.loadFromData(imageData);

        if (!img.isNull()) {
            int thumbH = contentRect.height() - 4;
            int thumbW = qMin(contentRect.width() / 2, thumbH * img.width() / img.height());
            QRect thumbRect(contentRect.left(), contentRect.top() + 2, thumbW, thumbH);
            painter->drawImage(thumbRect, img);

            // Label to the right of thumbnail
            QRect labelRect = contentRect.adjusted(thumbW + 6, 0, 0, 0);
            QString label = text.isEmpty() ? "CF_DIB" : text;
            painter->drawText(labelRect, Qt::AlignVCenter | Qt::AlignLeft, label);
        } else {
            // Couldn't decode image — show format name
            painter->drawText(contentRect, Qt::AlignVCenter | Qt::AlignLeft,
                              text.isEmpty() ? "CF_DIB" : text);
        }
    } else {
        // Text clip — show preview lines
        QFontMetrics fm(option.font);
        QString elided;
        QStringList lines = text.split('\n');
        int maxLines = qMin(m_linesPerRow, lines.size());

        for (int i = 0; i < maxLines; i++) {
            if (i > 0) elided += '\n';
            elided += fm.elidedText(lines[i].trimmed(), Qt::ElideRight, contentRect.width());
        }

        painter->drawText(contentRect, Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap, elided);
    }

    // Bottom separator line
    painter->setPen(QPen(option.palette.color(QPalette::Mid), 1, Qt::SolidLine));
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());

    painter->restore();
}

QSize ClipDelegate::sizeHint(const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
    bool hasImage = index.data(ClipListModel::HasImageRole).toBool();

    int lineHeight = option.fontMetrics.lineSpacing();
    if (hasImage)
        return QSize(100, qMax(lineHeight * 3, 60)); // Taller for image thumbnails
    else
        return QSize(100, lineHeight * m_linesPerRow + 8);
}
