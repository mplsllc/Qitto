#pragma once

#include <QWidget>
#include <QListView>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>

class ClipListModel;
class ClipDelegate;
class SearchBox;
class ClipboardMonitor;

class MainWindow : public QWidget {
    Q_OBJECT

public:
    explicit MainWindow(ClipboardMonitor *monitor = nullptr, QWidget *parent = nullptr);

    void showPopup();
    void hidePopup();

    // Load custom QSS theme file
    void loadTheme(const QString &qssPath);

signals:
    void clipSelected(qint64 clipId);

protected:
    bool event(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onSearchChanged(const QString &text);
    void onItemActivated(const QModelIndex &index);
    void onClipCaptured(qint64 clipId);

private:
    SearchBox *m_searchBox;
    QListView *m_listView;
    ClipListModel *m_model;
    ClipDelegate *m_delegate;
    ClipboardMonitor *m_monitor;
    QPushButton *m_closeBtn;
    QLabel *m_brandLabel;

    void setupUi();
    void positionAtCursor();
};
