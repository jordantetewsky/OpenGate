// SPDX-License-Identifier: MIT
#pragma once
#include <QImage>
#include <QPainterPath>
#include <QRegion>
namespace Unbound {
// Opaque chromatic coverage of an already verified monochrome body skin.
// Neutral shadows and translucent antialiasing remain native pixels.
inline QPainterPath clipOpaqueMask(const QImage &image) {
    QRegion runs;
    for (int y = 0; y < image.height(); ++y) {
        int start = -1;
        for (int x = 0; x <= image.width(); ++x) {
            const auto color = x < image.width() ? image.pixelColor(x, y) : QColor(Qt::transparent);
            const bool opaque = color.alpha() == 255 && color.hslSaturationF() > .1;
            if (opaque && start < 0)
                start = x;
            if (!opaque && start >= 0) {
                runs += QRect(start, y, x - start, 1);
                start = -1;
            }
        }
    }
    QPainterPath path;
    path.addRegion(runs);
    return path;
}
} // namespace Unbound
