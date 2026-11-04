// SPDX-License-Identifier: MIT
#pragma once
#include "colorpresetpaint.h"
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QTimer>

namespace Unbound {
// A native action's right-side swatch only. Native QMenu retains row painting,
// hit testing, accessibility and activation; the overlay never receives input.
class MenuActionColorSwatch : public QObject {
  public:
    MenuActionColorSwatch(QMenu *menu, QAction *action, QColor color)
        : MenuActionColorSwatch(menu, action, solid(color)) {}
    MenuActionColorSwatch(QMenu *menu, QAction *action, ColorPreset preset)
        : QObject(action), menu(menu), action(action), dot(new Dot(menu, std::move(preset))) {
        menu->installEventFilter(this);
        connect(action, &QAction::changed, this, [this] { queue(); });
        queue();
    }
    ~MenuActionColorSwatch() override {
        delete dot;
    }

  protected:
    bool eventFilter(QObject *, QEvent *event) override {
        switch (event->type()) {
        case QEvent::Paint:
        case QEvent::Resize:
        case QEvent::Show:
        case QEvent::Wheel:
            queue();
            break;
        case QEvent::Hide:
            if (dot)
                dot->hide();
            break;
        default:
            break;
        }
        return false;
    }

  private:
    class Dot : public QWidget {
      public:
        Dot(QWidget *owner, ColorPreset preset) : QWidget(owner), preset(std::move(preset)) {
            setObjectName("openGateMenuColorSwatch");
            setAttribute(Qt::WA_TransparentForMouseEvents);
            setAttribute(Qt::WA_NoSystemBackground);
            setFocusPolicy(Qt::NoFocus);
            hide();
        }

      protected:
        void paintEvent(QPaintEvent *) override {
            QPainter p(this);
            p.setRenderHint(QPainter::Antialiasing);
            const double size = qBound(6., height() / 3., 14.);
            const bool pill = !preset.stops.isEmpty();
            const QRectF bounds((width() - (pill ? size * 1.6 : size)) / 2., (height() - size) / 2.,
                                pill ? size * 1.6 : size, size);
            QPainterPath shape;
            if (pill)
                shape.addRoundedRect(bounds, size / 2, size / 2);
            else
                shape.addEllipse(bounds);
            p.save();
            p.setClipPath(shape);
            paint.fill(p, bounds, preset);
            p.restore();
            p.setPen(QPen(Qt::black, 1));
            p.setBrush(Qt::NoBrush);
            p.drawPath(shape);
        }

      private:
        ColorPreset preset;
        ColorPresetPaint paint;
    };
    QPointer<QMenu> menu;
    QPointer<QAction> action;
    QPointer<Dot> dot;
    bool queued = false;
    static ColorPreset solid(QColor color) {
        ColorPreset p;
        p.color = color;
        return p;
    }
    void queue() {
        if (queued)
            return;
        queued = true;
        QTimer::singleShot(0, this, [this] {
            queued = false;
            if (!menu || !action || !dot)
                return;
            const auto row = menu->actionGeometry(action);
            dot->setGeometry(row.right() - 20 - row.height() / 2, row.top(), row.height(),
                             row.height());
            dot->setVisible(menu->isVisible() && action->isVisible() &&
                            row.intersects(menu->rect()));
            dot->raise();
        });
    }
};
} // namespace Unbound
