// SPDX-License-Identifier: MIT
#pragma once
#include <QColor>
#include <QVector>
#include <cmath>
namespace Unbound {
// Select safe label foreground/scrim using worst-case contrast, never a single
// gradient endpoint. Native text geometry and editing state belong to callers.
struct ColorLabelStyle {
    QColor text, scrim;
    double contrast = 0;
};
inline double colorLuminance(QColor c) {
    const auto channel = [](double v) {
        return v <= .04045 ? v / 12.92 : std::pow((v + .055) / 1.055, 2.4);
    };
    return .2126 * channel(c.redF()) + .7152 * channel(c.greenF()) + .0722 * channel(c.blueF());
}
inline double colorContrast(QColor a, QColor b) {
    const auto x = colorLuminance(a), y = colorLuminance(b);
    return (qMax(x, y) + .05) / (qMin(x, y) + .05);
}
inline ColorLabelStyle colorBackingForInk(QColor ink) {
    const QColor dark(24, 24, 29);
    if (colorContrast(ink, dark) >= 4.5)
        return {ink, dark, colorContrast(ink, dark)};
    const auto white = colorContrast(ink, Qt::white), black = colorContrast(ink, Qt::black);
    return white >= black ? ColorLabelStyle{ink, Qt::white, white}
                          : ColorLabelStyle{ink, Qt::black, black};
}
inline ColorLabelStyle colorLabelStyle(const QVector<QColor> &backgrounds, bool patterned = false) {
    double black = 21, white = 21;
    for (auto c : backgrounds) {
        black = qMin(black, colorContrast(Qt::black, c));
        white = qMin(white, colorContrast(Qt::white, c));
    }
    // A pattern's extreme pixels can sit directly under a small glyph. A solid
    // neutral label strip makes the guarantee independent of texture phase.
    if (patterned || qMax(black, white) < 4.5 || backgrounds.isEmpty()) {
        const QColor plate(24, 24, 29);
        return {Qt::white, plate, colorContrast(Qt::white, plate)};
    }
    return white >= black ? ColorLabelStyle{Qt::white, {}, white}
                          : ColorLabelStyle{Qt::black, {}, black};
}
} // namespace Unbound
