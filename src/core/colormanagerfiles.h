// SPDX-License-Identifier: MIT
#pragma once
#include <QFile>
#include <QImageReader>
#include <QSaveFile>
namespace Unbound {
// Bounded file I/O for explicit manager import/export and sample thumbnails.
// Dialogs, schema interpretation and palette state belong to their callers.
class ColorManagerFiles {
  public:
    static QString readPalette(QString path, QByteArray &bytes) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly) || file.size() > 256000)
            return "Cannot read palette or file exceeds 256 KB.";
        bytes = file.readAll();
        return {};
    }
    static QString writePalette(QString path, const QByteArray &bytes) {
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
            return "Cannot export palette.";
        return {};
    }
    static QString readThumbnail(QString path, QImage &image) {
        QImageReader reader(path);
        reader.setAutoTransform(true);
        const auto size = reader.size();
        if (!size.isValid() || size.width() > 8192 || size.height() > 8192 ||
            qint64(size.width()) * size.height() > 32000000)
            return "Choose a thumbnail up to 32 megapixels and 8192 pixels per side.";
        reader.setScaledSize(size.scaled(1280, 720, Qt::KeepAspectRatio));
        const auto result = reader.read();
        if (result.isNull())
            return "Cannot read thumbnail.";
        image = result;
        return {};
    }
};
} // namespace Unbound
