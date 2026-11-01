// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsView>
#include <QGraphicsProxyWidget>
#include <QGraphicsObject>
#include <QVector>
#include <QtMath>

namespace Unbound {
// Discover native header/button geometry and map one presented point back to its
// original viewport. It owns no native widgets, input handlers or preferences.
struct TimelineTrackHeaderLayout {
    struct Row { double top, bottom; };
    struct Slice { QRectF source, target; };
    double nativeWidth = 0.;
    double pinnedWidth = 0.;
    double width = 0.;
    double offset = 0.;
    double minimumBodyWidth = 32.;
    double minimumHeight = 0.;
    bool pinned = true;
    QRectF horizontalBar;
    QSizeF sourceSize;
    QSizeF targetSize;
    QVector<Row> buttonRows;

    static bool isType(QGraphicsItem *item, const char *type) {
        auto object = item ? item->toGraphicsObject() : nullptr;
        return object && object->inherits(type);
    }
    static TimelineTrackHeaderLayout discover(QGraphicsView *view) {
        TimelineTrackHeaderLayout result;
        if (!view || !view->scene())
            return result;
        result.sourceSize = view->viewport()->size();
        double firstTrackTop = view->viewport()->height(), trackMinimum = 0., separators = 0.;
        for (auto item : view->scene()->items()) {
            if(!item->isVisible())continue;
            if(isType(item,"UiTiSequenceSceneSplitterHandle"))separators+=item->boundingRect().height();
            if (isType(item, "UiTiScrollBarItem")) {
                const auto bounds = view->viewportTransform().mapRect(item->sceneBoundingRect());
                if (bounds.width() > bounds.height() * 4 && bounds.bottom() >= view->viewport()->height() - 24)
                    result.horizontalBar = bounds;
            }
            auto proxy = qgraphicsitem_cast<QGraphicsProxyWidget *>(item);
            auto controls = proxy ? qobject_cast<QGraphicsView *>(proxy->widget()) : nullptr;
            if(controls && controls->objectName().endsWith("TracksView") && proxy->size().height()>0.) {
                const auto minimum=proxy->effectiveSizeHint(Qt::MinimumSize);
                result.minimumBodyWidth=qMax(result.minimumBodyWidth,minimum.width());
                trackMinimum+=qMax(32.,minimum.height());
                firstTrackTop=qMin(firstTrackTop,view->viewportTransform().mapRect(proxy->sceneBoundingRect()).top());
            }
            if (!controls || !controls->scene() || !controls->objectName().endsWith("TrackControlsView"))
                continue;
            const auto bounds = view->viewportTransform().mapRect(proxy->sceneBoundingRect());
            if (qAbs(bounds.left()) > 2 || bounds.width() < 40)
                continue;
            result.nativeWidth = qMax(result.nativeWidth, double(bounds.width()));
            for (auto child : controls->scene()->items()) {
                if (!isType(child, "UiTiTrackLayerItem") || !child->isVisible())
                    continue;
                const auto local = child->sceneBoundingRect();
                // The two compact leftmost layer/patch buttons share a row.
                // Track names and audio-channel badges are outside this band.
                if (local.width() > local.height() * 2.5 || local.left() > bounds.width() * .36)
                    continue;
                auto rectangle = controls->viewportTransform().mapRect(local);
                rectangle.translate(controls->viewport()->pos());
                const auto sceneBounds = proxy->mapRectToScene(rectangle);
                const auto button = view->viewportTransform().mapRect(sceneBounds).intersected(bounds);
                if (button.isEmpty())
                    continue;
                result.pinnedWidth = qMax(result.pinnedWidth, double(button.right() + 5));
                result.buttonRows.append({double(button.top() - 3), double(button.bottom() + 3)});
            }
        }
        if(trackMinimum>0.)result.minimumHeight=firstTrackTop+trackMinimum+separators+result.horizontalBar.height();
        return result;
    }
    void configure(QSizeF target, double requestedWidth, double requestedOffset, bool pin) {
        targetSize = target;
        pinned = pin;
        const double minimum = pinned ? pinnedWidth + 24. : 40.;
        const double maximum=qMax(0.,qMin(nativeWidth,target.width()-minimumBodyWidth));
        width = qBound(qMin(minimum, maximum), requestedWidth > 0. ? requestedWidth : nativeWidth, maximum);
        offset = qBound(0., requestedOffset, maximumOffset());
    }
    double maximumOffset() const { return qMax(0., nativeWidth - width); }
    bool buttonRow(double y) const {
        if (!pinned)
            return false;
        for (const auto &row : buttonRows)
            if (y >= row.top && y <= row.bottom)
                return true;
        return false;
    }
    QPointF toNative(QPointF point) const {
        if (!horizontalBar.isEmpty() && point.y() >= horizontalBar.top()) {
            if (targetSize.width() > width)
                point.setX(horizontalBar.left() + (point.x() - width) * horizontalBar.width() / (targetSize.width() - width));
            return point;
        }
        if (point.x() >= width)
            point.rx() += nativeWidth - width;
        else if (!(buttonRow(point.y()) && point.x() < pinnedWidth))
            point.rx() += offset;
        return point;
    }
    QVector<Slice> slices() const {
        QVector<Slice> result;
        const double bottom = horizontalBar.isEmpty() ? targetSize.height() : horizontalBar.top();
        result.append({QRectF(offset, 0, width, bottom), QRectF(0, 0, width, bottom)});
        if (pinned && pinnedWidth > 0.) {
            // Redraw whole button rows in two bands. Names below the buttons
            // scroll as whole labels, so no glyph is split at the pinned edge.
            for (const auto &row : buttonRows) {
                const double top = qMax(0., row.top), end = qMin(bottom, row.bottom);
                if (end <= top)
                    continue;
                result.append({QRectF(0, top, pinnedWidth, end - top), QRectF(0, top, pinnedWidth, end - top)});
                result.append({QRectF(pinnedWidth + offset, top, width - pinnedWidth, end - top),
                               QRectF(pinnedWidth, top, width - pinnedWidth, end - top)});
            }
        }
        result.append({QRectF(nativeWidth, 0, qMax(0., targetSize.width() - width), bottom),
                       QRectF(width, 0, qMax(0., targetSize.width() - width), bottom)});
        if (!horizontalBar.isEmpty())
            result.append({horizontalBar, QRectF(width, horizontalBar.top(), qMax(0., targetSize.width() - width), horizontalBar.height())});
        return result;
    }
};
}
