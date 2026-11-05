#include "TextFile.h"
#include <QFile>
#include <QSaveFile>

static const QByteArray kBom("\xEF\xBB\xBF");

bool TextFile::read(const QString& path, QString* error) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = file.errorString();
        return false;
    }
    QByteArray data = file.readAll();
    bom = data.startsWith(kBom);
    text = QString::fromUtf8(QByteArrayView(data).sliced(bom ? kBom.size() : 0));
    data.clear();
    crlf = text.contains(QLatin1String("\r\n"));
    if (crlf) text.replace(QLatin1String("\r\n"), QLatin1String("\n"));
    return true;
}

bool TextFile::write(const QString& path, QString* error) const {
    QSaveFile file(path);
    QByteArray data = bom ? kBom : QByteArray();
    data += crlf ? QString(text).replace('\n', QLatin1String("\r\n")).toUtf8() : text.toUtf8();
    if (file.open(QIODevice::WriteOnly) && file.write(data) == data.size() && file.commit()) return true;
    if (error) *error = file.errorString();
    return false;
}
