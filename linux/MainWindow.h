#pragma once

#include <QWidget>
#include <QListView>

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

signals:
    void clipSelected(qint64 clipId);

protected:
    bool event(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

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

    void setupUi();
    void positionAtCursor();
};
