#include "ClipDelegate.h"
#include "ClipListModel.h"

#include <QPainter>
#include <QApplication>

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

    if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, option.palette.highlight());
    else if (option.state & QStyle::State_MouseOver)
        painter->fillRect(option.rect, option.palette.midlight());

    QRect textRect = option.rect.adjusted(8, 4, -8, -4);
    QString text = index.data(Qt::DisplayRole).toString();
    QString dateStr = index.data(ClipListModel::DateRole).toString();

    // Date on right, small
    QFont smallFont = option.font;
    smallFont.setPointSize(smallFont.pointSize() - 2);
    QFontMetrics smallFm(smallFont);
    int dateWidth = smallFm.horizontalAdvance(dateStr) + 8;

    painter->setFont(smallFont);
    painter->setPen(option.palette.color(QPalette::Disabled, QPalette::Text));
    painter->drawText(textRect.adjusted(textRect.width() - dateWidth, 0, 0, 0),
                      Qt::AlignTop | Qt::AlignRight, dateStr);

    // Main text
    QRect mainRect = textRect.adjusted(0, 0, -dateWidth, 0);
    painter->setFont(option.font);

    if (option.state & QStyle::State_Selected)
        painter->setPen(option.palette.color(QPalette::HighlightedText));
    else
        painter->setPen(option.palette.color(QPalette::Text));

    QFontMetrics fm(option.font);
    QString elided;
    QStringList lines = text.split('\n');
    for (int i = 0; i < qMin(m_linesPerRow, lines.size()); i++) {
        if (i > 0) elided += '\n';
        elided += fm.elidedText(lines[i], Qt::ElideRight, mainRect.width());
    }

    painter->drawText(mainRect, Qt::AlignTop | Qt::AlignLeft | Qt::TextWordWrap, elided);

    // Separator line
    painter->setPen(QPen(option.palette.color(QPalette::Midlight), 1));
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());

    painter->restore();
}

QSize ClipDelegate::sizeHint(const QStyleOptionViewItem &option,
                              const QModelIndex &) const
{
    int lineHeight = option.fontMetrics.lineSpacing();
    return QSize(100, lineHeight * m_linesPerRow + 12);
}
