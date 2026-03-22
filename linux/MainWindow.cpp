#include "MainWindow.h"
#include "ClipListModel.h"
#include "ClipDelegate.h"
#include "SearchBox.h"
#include "ClipboardMonitor.h"
#include "Settings.h"
#include "QittoApp.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QScreen>
#include <QApplication>
#include <QScrollBar>
#include <QPainter>
#include <QFile>
#include <QStandardPaths>
#include <QToolButton>

MainWindow::MainWindow(ClipboardMonitor *monitor, QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool)
    , m_monitor(monitor)
{
    setObjectName("QittoPopup");
    setupUi();

    // Try loading custom theme
    QString themePath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                        + "/qitto/theme.qss";
    if (QFile::exists(themePath))
        loadTheme(themePath);

    if (m_monitor) {
        connect(m_monitor, &ClipboardMonitor::clipCaptured,
                this, &MainWindow::onClipCaptured);
    }
}

void MainWindow::setupUi()
{
    setFixedSize(380, 450);
    setAttribute(Qt::WA_ShowWithoutActivating, false);

    double opacity = Settings::instance().transparency();
    if (opacity < 1.0)
        setWindowOpacity(opacity);

    // Main horizontal layout: [clip list area] [brand strip]
    QHBoxLayout *outerLayout = new QHBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    // Left side: clip list + search bar
    QWidget *leftPanel = new QWidget(this);
    leftPanel->setObjectName("leftPanel");
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(2, 2, 0, 2);
    leftLayout->setSpacing(0);

    // Close button at top right of left panel
    QHBoxLayout *topBar = new QHBoxLayout;
    topBar->setContentsMargins(0, 0, 4, 0);
    topBar->addStretch();
    m_closeBtn = new QPushButton("X", this);
    m_closeBtn->setObjectName("closeBtn");
    m_closeBtn->setFixedSize(20, 20);
    m_closeBtn->setFlat(true);
    m_closeBtn->setFocusPolicy(Qt::NoFocus);
    connect(m_closeBtn, &QPushButton::clicked, this, &MainWindow::hidePopup);
    topBar->addWidget(m_closeBtn);
    leftLayout->addLayout(topBar);

    // Clip list
    m_listView = new QListView(this);
    m_listView->setObjectName("clipList");
    m_model = new ClipListModel(this);
    m_delegate = new ClipDelegate(this);
    m_delegate->setLinesPerRow(Settings::instance().linesPerRow());

    m_listView->setModel(m_model);
    m_listView->setItemDelegate(m_delegate);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setMouseTracking(true);
    m_listView->setFrameShape(QFrame::NoFrame);
    leftLayout->addWidget(m_listView, 1);

    // Single click pastes (like Ditto)
    connect(m_listView, &QListView::clicked, this, &MainWindow::onItemActivated);
    connect(m_listView, &QListView::activated, this, &MainWindow::onItemActivated);

    connect(m_listView->verticalScrollBar(), &QScrollBar::valueChanged, [this](int value) {
        QScrollBar *sb = m_listView->verticalScrollBar();
        if (value >= sb->maximum() - 10 && m_model->canLoadMore()) {
            m_model->loadMore();
        }
    });

    // Search bar at BOTTOM (matching Ditto layout)
    QHBoxLayout *searchBar = new QHBoxLayout;
    searchBar->setContentsMargins(4, 2, 4, 4);
    searchBar->setSpacing(4);

    m_searchBox = new SearchBox(this);
    m_searchBox->setObjectName("searchBox");
    searchBar->addWidget(m_searchBox, 1);

    connect(m_searchBox, &SearchBox::searchChanged, this, &MainWindow::onSearchChanged);
    connect(m_searchBox, &SearchBox::escapePressed, this, &MainWindow::hidePopup);
    connect(m_searchBox, &SearchBox::arrowUpPressed, [this]() {
        m_listView->setFocus();
        int last = m_model->rowCount() - 1;
        if (last >= 0)
            m_listView->setCurrentIndex(m_model->index(last));
    });

    // Menu button ("...")
    QToolButton *menuBtn = new QToolButton(this);
    menuBtn->setObjectName("menuBtn");
    menuBtn->setText("\u2026"); // ellipsis
    menuBtn->setFixedSize(28, 28);
    menuBtn->setFocusPolicy(Qt::NoFocus);
    searchBar->addWidget(menuBtn);

    leftLayout->addLayout(searchBar);
    outerLayout->addWidget(leftPanel, 1);

    // Right side: vertical brand label "Qitto"
    m_brandLabel = new QLabel(this);
    m_brandLabel->setObjectName("brandLabel");
    m_brandLabel->setText("Qitto");
    m_brandLabel->setFixedWidth(22);
    m_brandLabel->setAlignment(Qt::AlignCenter);
    // The vertical text is painted in paintEvent
    outerLayout->addWidget(m_brandLabel);
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    // Draw vertical "Qitto" text on the brand label area
    if (m_brandLabel) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        QRect brandRect = m_brandLabel->geometry();

        // Background for brand strip
        QColor brandBg = palette().color(QPalette::Highlight);
        painter.fillRect(brandRect, brandBg);

        // Vertical text
        painter.save();
        QFont font = this->font();
        font.setPointSize(10);
        font.setBold(true);
        painter.setFont(font);
        painter.setPen(palette().color(QPalette::HighlightedText));

        painter.translate(brandRect.center().x() + 5, brandRect.center().y() + 30);
        painter.rotate(-90);
        painter.drawText(0, 0, "Qitto");
        painter.restore();
    }
}

void MainWindow::loadTheme(const QString &qssPath)
{
    QFile f(qssPath);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStyleSheet(QString::fromUtf8(f.readAll()));
        QittoApp::dbg("Loaded theme: " + qssPath);
        f.close();
    }
}

void MainWindow::showPopup()
{
    m_searchBox->clear();
    m_model->refresh();
    positionAtCursor();
    show();
    raise();
    activateWindow();

    // Focus the list with first item selected (like Ditto)
    m_listView->setFocus();
    if (m_model->rowCount() > 0)
        m_listView->setCurrentIndex(m_model->index(0));

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
        int row = event->key() - Qt::Key_1;
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
