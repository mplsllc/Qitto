#include "FormatMapper.h"

FormatMapper &FormatMapper::instance()
{
    static FormatMapper fm;
    return fm;
}

FormatMapper::FormatMapper()
{
    auto add = [&](const QString &mime, const QString &ditto) {
        m_mimeToDitto[mime] = ditto;
        m_dittoToMime[ditto] = mime;
    };

    add("text/plain",    "CF_UNICODETEXT");
    add("text/html",     "HTML Format");
    add("text/rtf",      "Rich Text Format");
    add("image/png",     "PNG");
    add("image/bmp",     "CF_DIB");
    add("text/uri-list", "CF_HDROP");
}

QString FormatMapper::mimeToDitto(const QString &mime) const
{
    return m_mimeToDitto.value(mime, mime);
}

QString FormatMapper::dittoToMime(const QString &ditto) const
{
    return m_dittoToMime.value(ditto, ditto);
}

bool FormatMapper::isSupported(const QString &mime) const
{
    return m_mimeToDitto.contains(mime)
        || mime.startsWith("text/")
        || mime.startsWith("image/");
}
