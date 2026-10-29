// SPDX-License-Identifier: MIT
#ifndef UNBOUND_PANESNAP_H
#define UNBOUND_PANESNAP_H
#include <QRect>
#include <QVector>
namespace Unbound {
// Native move loops can feed a previously snapped rectangle back to us. Derive
// new proposals from accumulated pointer travel, not that snapped position.
class PaneDragProposal
{
public:
    void reset() { m_valid = false; }
    QRect update(const QRect &suggested, const QPoint &cursor, bool left, bool right, bool top, bool bottom)
    {
        if (!m_valid) { m_start = suggested; m_cursor = cursor; m_valid = true; }
        const auto delta = cursor - m_cursor;
        const int x = m_start.x() + (left ? delta.x() : 0);
        const int y = m_start.y() + (top ? delta.y() : 0);
        return QRect(x, y, m_start.width() + (right ? delta.x() : 0) - (left ? delta.x() : 0),
            m_start.height() + (bottom ? delta.y() : 0) - (top ? delta.y() : 0));
    }
private:
    bool m_valid = false;
    QRect m_start;
    QPoint m_cursor;
};
// Rectangles use screen coordinates; compare exclusive right/bottom edges.
inline QPoint paneSnapOffset(const QRect &moving, const QVector<QRect> &targets, int distance,
    bool left = true, bool right = true, bool top = true, bool bottom = true,
    QSize minimum = QSize(), QSize maximum = QSize(16777215, 16777215))
{
    int bestX = distance + 1, bestY = distance + 1;
    auto consider = [distance](int delta, int &best) {
        if (qAbs(delta) <= distance && qAbs(delta) < qAbs(best)) best = delta;
    };
    for (const auto &other : targets) {
        if (moving.y() <= other.y() + other.height() + distance
            && moving.y() + moving.height() >= other.y() - distance) {
            for (int edge = 0; edge < 2; ++edge) {
                if (!(edge ? right : left)) continue;
                int from = edge ? moving.x() + moving.width() : moving.x();
                for (int to : {other.x(), other.x() + other.width()}) {
                    const int width = moving.width() + (edge ? to - from : from - to);
                    if (left == right || (width >= minimum.width() && width <= maximum.width())) consider(to - from, bestX);
                }
            }
        }
        if (moving.x() <= other.x() + other.width() + distance
            && moving.x() + moving.width() >= other.x() - distance) {
            for (int edge = 0; edge < 2; ++edge) {
                if (!(edge ? bottom : top)) continue;
                int from = edge ? moving.y() + moving.height() : moving.y();
                for (int to : {other.y(), other.y() + other.height()}) {
                    const int height = moving.height() + (edge ? to - from : from - to);
                    if (top == bottom || (height >= minimum.height() && height <= maximum.height())) consider(to - from, bestY);
                }
            }
        }
    }
    return QPoint(qAbs(bestX) <= distance ? bestX : 0, qAbs(bestY) <= distance ? bestY : 0);
}
}
#endif
