#pragma once

#include <QLineEdit>

class SearchBox : public QLineEdit {
    Q_OBJECT

public:
    explicit SearchBox(QWidget *parent = nullptr);

signals:
    void searchChanged(const QString &text);
    void escapePressed();
    void arrowDownPressed();

protected:
    void keyPressEvent(QKeyEvent *event) override;
};
