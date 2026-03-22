#include "SearchBox.h"
#include <QKeyEvent>
#include <QTimer>

SearchBox::SearchBox(QWidget *parent)
    : QLineEdit(parent)
{
    setPlaceholderText("Search clips...");
    setClearButtonEnabled(true);

    QTimer *debounce = new QTimer(this);
    debounce->setSingleShot(true);
    debounce->setInterval(150);

    connect(this, &QLineEdit::textChanged, [debounce]() { debounce->start(); });
    connect(debounce, &QTimer::timeout, [this]() { emit searchChanged(text()); });
}

void SearchBox::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit escapePressed();
        return;
    }
    if (event->key() == Qt::Key_Down) {
        emit arrowDownPressed();
        return;
    }
    if (event->key() == Qt::Key_Up) {
        emit arrowUpPressed();
        return;
    }
    QLineEdit::keyPressEvent(event);
}
