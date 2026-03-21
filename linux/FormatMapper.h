#pragma once

#include <QString>
#include <QHash>

class FormatMapper {
public:
    static FormatMapper &instance();

    QString mimeToDitto(const QString &mime) const;
    QString dittoToMime(const QString &ditto) const;
    bool isSupported(const QString &mime) const;

private:
    FormatMapper();
    QHash<QString, QString> m_mimeToDitto;
    QHash<QString, QString> m_dittoToMime;
};
