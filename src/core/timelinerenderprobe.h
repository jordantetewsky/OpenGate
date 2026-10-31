// SPDX-License-Identifier: MIT
#pragma once
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QImage>
#include <QJsonArray>
#include <QPainter>
#include <QPointer>
#include <QtMath>
#include "diagnostics.h"

namespace Unbound {
// Opt-in visual evidence for native scene rendering at backing-store scale.
// It never replaces a viewport, handles input or applies a user preference.
class TimelineRenderProbe {
public:
    static QImage render(QGraphicsScene *scene, const QRectF &visible, double factor) {
        if (!scene || visible.isEmpty() || !qIsFinite(factor) || factor < 1. || factor > 3.)
            return {};
        if (!qIsFinite(visible.x()) || !qIsFinite(visible.y()) ||
            !qIsFinite(visible.width()) || !qIsFinite(visible.height()) ||
            visible.width() > 4096 || visible.height() > 4096)
            return {};
        const QSize pixels(qCeil(visible.width()), qCeil(visible.height()));

        // Keep world coordinates at 1:1. Device pixel ratio enlarges glyphs and
        // positions together, including drawing that bypasses world transforms.
        const QRectF source(visible.topLeft(), visible.size() / factor);
        QImage image(pixels, QImage::Format_ARGB32_Premultiplied);
        if (image.isNull())
            return {};
        image.setDevicePixelRatio(factor);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        scene->render(&painter, QRectF(QPointF(), source.size()), source, Qt::IgnoreAspectRatio);
        return image;
    }

    void capture(QWidget *timeline) {
        if (!timeline || qEnvironmentVariableIntValue("OPENGATE_TIMELINE_RENDER_PROBE") != 1)
            return;
        for (auto view : timeline->findChildren<QGraphicsView *>("m_TiTimelineView")) {
            if (captured.size() >= 2)
                return;
            if (!view->isVisible() || !view->scene() || view->scene()->items().isEmpty() || captured.contains(view))
                continue;
            captured.append(view);
            const auto visible = view->mapToScene(view->viewport()->rect()).boundingRect();
            const auto directory = diagnosticDirectory() + "/TimelineRenderProbe/" + diagnosticSessionId();
            QDir().mkpath(directory);
            QJsonArray images;
            for (double factor : {1., 2.}) {
                const auto image = render(view->scene(), visible, factor);
                const auto path = directory + QString("/view-%1-%2x.png").arg(captured.size()).arg(factor);
                const bool saved = !image.isNull() && image.save(path);
                images.append(QJsonObject{{"factor", factor}, {"saved", saved}, {"path", path}});
            }
            logEvent("timeline.render.probe", {{"images", images}, {"input_changed", false},
                {"ownership_changed", false}, {"native_live_verified", false}});
        }
    }

private:
    QList<QPointer<QGraphicsView>> captured;
};
}
