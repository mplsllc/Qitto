#pragma once

#include <QObject>
#include <QClipboard>
#include <cstdint>

class ClipboardMonitor : public QObject {
    Q_OBJECT

public:
    explicit ClipboardMonitor(QObject *parent = nullptr);

    void start();
    void stop();
    void setSelfIgnore(bool ignore);

signals:
    void clipCaptured(qint64 clipId);

private slots:
    void onClipboardChanged();

private:
    bool m_active = false;
    bool m_selfIgnore = false;
};
