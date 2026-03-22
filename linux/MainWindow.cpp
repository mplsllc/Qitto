#include "MainWindow.h"
#include "ClipListModel.h"
#include "ClipDelegate.h"
#include "SearchBox.h"
#include "ClipboardMonitor.h"
#include "Settings.h"
#include "QittoApp.h"

#include <QVBoxLayout>
#include <QKeyEvent>
#include <QScreen>
#include <QApplication>
#include <QScrollBar>

MainWindow::MainWindow(ClipboardMonitor *monitor, QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
    , m_monitor(monitor)
{
    setupUi();

    if (m_monitor) {
        connect(m_monitor, &ClipboardMonitor::clipCaptured,
                this, &MainWindow::onClipCaptured);
    }
}

void MainWindow::setupUi()
{
    setFixedSize(400, 500);
    setAttribute(Qt::WA_ShowWithoutActivating, false);

    double opacity = Settings::instance().transparency();
    if (opacity < 1.0)
        setWindowOpacity(opacity);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(2);

    m_searchBox = new SearchBox(this);
    layout->addWidget(m_searchBox);

    connect(m_searchBox, &SearchBox::searchChanged, this, &MainWindow::onSearchChanged);
    connect(m_searchBox, &SearchBox::escapePressed, this, &MainWindow::hidePopup);
    connect(m_searchBox, &SearchBox::arrowDownPressed, [this]() {
        m_listView->setFocus();
        if (m_model->rowCount() > 0)
            m_listView->setCurrentIndex(m_model->index(0));
    });

    m_listView = new QListView(this);
    m_model = new ClipListModel(this);
    m_delegate = new ClipDelegate(this);
    m_delegate->setLinesPerRow(Settings::instance().linesPerRow());

    m_listView->setModel(m_model);
    m_listView->setItemDelegate(m_delegate);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_listView->setMouseTracking(true);
    layout->addWidget(m_listView);

    connect(m_listView, &QListView::activated, this, &MainWindow::onItemActivated);

    connect(m_listView->verticalScrollBar(), &QScrollBar::valueChanged, [this](int value) {
        QScrollBar *sb = m_listView->verticalScrollBar();
        if (value >= sb->maximum() - 10 && m_model->canLoadMore()) {
            m_model->loadMore();
        }
    });
}

void MainWindow::showPopup()
{
    m_searchBox->clear();
    m_model->refresh();
    positionAtCursor();
    show();
    raise();
    activateWindow();
    m_searchBox->setFocus();
    QittoApp::dbg("Popup shown with " + QString::number(m_model->rowCount()) + " clips");
}

void MainWindow::hidePopup()
{
    hide();
    QittoApp::dbg("Popup hidden");
}

void MainWindow::positionAtCursor()
{
    QPoint cursor = QCursor::pos();
    QScreen *screen = QApplication::screenAt(cursor);
    if (!screen)
        screen = QApplication::primaryScreen();

    QRect screenGeom = screen->availableGeometry();
    int x = cursor.x();
    int y = cursor.y();

    if (x + width() > screenGeom.right())
        x = screenGeom.right() - width();
    if (y + height() > screenGeom.bottom())
        y = cursor.y() - height();
    if (x < screenGeom.left())
        x = screenGeom.left();
    if (y < screenGeom.top())
        y = screenGeom.top();

    move(x, y);
}

void MainWindow::onSearchChanged(const QString &text)
{
    m_model->refresh(text);
    m_delegate->setSearchHighlight(text);
    m_listView->viewport()->update();
}

void MainWindow::onItemActivated(const QModelIndex &index)
{
    qint64 clipId = m_model->clipIdAt(index.row());
    if (clipId >= 0) {
        QittoApp::dbg("Clip selected: id=" + QString::number(clipId));
        emit clipSelected(clipId);
        hidePopup();
    }
}

void MainWindow::onClipCaptured(qint64 clipId)
{
    Q_UNUSED(clipId);
    if (isVisible())
        m_model->refresh(m_searchBox->text());
}

bool MainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::WindowDeactivate) {
        hidePopup();
        return true;
    }
    return QWidget::event(event);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        hidePopup();
        return;
    }

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QModelIndex idx = m_listView->currentIndex();
        if (idx.isValid())
            onItemActivated(idx);
        return;
    }

    // Number shortcuts: 1-9 paste the Nth clip (only when search box is empty)
    if (m_searchBox->text().isEmpty() && event->key() >= Qt::Key_1 && event->key() <= Qt::Key_9) {
        int row = event->key() - Qt::Key_1;  // 0-indexed
        if (row < m_model->rowCount()) {
            QModelIndex idx = m_model->index(row);
            onItemActivated(idx);
            return;
        }
    }

    // Forward typing to search box if list has focus
    if (m_listView->hasFocus() && !event->text().isEmpty()
        && event->key() != Qt::Key_Up && event->key() != Qt::Key_Down) {
        m_searchBox->setFocus();
        m_searchBox->setText(m_searchBox->text() + event->text());
        return;
    }

    QWidget::keyPressEvent(event);
}
