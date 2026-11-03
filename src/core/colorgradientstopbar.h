// SPDX-License-Identifier: MIT
#pragma once
#include "colorpreset.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QWidget>
#include <functional>
namespace Unbound {
// Gradient-stop display and local pointer/keyboard gestures. The preset editor
// owns selection and mutations; no palette state or global input hooks live here.
class ColorGradientStopBar : public QWidget {
  public:
    std::function<void(QString)> chosen, removeRequested;
    std::function<void(QString, double)> moved;
    std::function<void(double)> addRequested;
    explicit ColorGradientStopBar(QWidget *owner = nullptr) : QWidget(owner) {
        setObjectName("openGateGradientStopBar");
        setFixedHeight(64);
        setFocusPolicy(Qt::StrongFocus);
        setAccessibleName("Gradient color stops");
        setAccessibleDescription("Select a handle and drag, or use arrow keys to move it. Control "
                                 "and arrow selects another stop.");
    }
    void setStops(QVector<ColorStop> value, QString selected) {
        stops = std::move(value);
        active = std::move(selected);
        update();
    }

  protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.fillRect(track(), QColor(66, 66, 72));
        QLinearGradient gradient(track().topLeft(), track().topRight());
        for (const auto &s : stops) {
            auto color = s.color;
            color.setAlphaF(s.opacity);
            gradient.setColorAt(s.position, color);
        }
        p.fillRect(track(), gradient);
        for (const auto &s : stops) {
            p.setPen(
                QPen(s.id == active ? Qt::white : QColor(120, 120, 130), s.id == active ? 2 : 1));
            p.setBrush(s.color);
            p.drawRect(QRectF(x(s.position) - 6, 38, 12, 18));
        }
    }
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() != Qt::LeftButton)
            return;
        setFocus();
        QString hit;
        double distance = 13;
        for (const auto &s : stops) {
            const auto d = std::abs(event->localPos().x() - x(s.position));
            if (event->localPos().y() >= 32 && d < distance) {
                distance = d;
                hit = s.id;
            }
        }
        if (!hit.isEmpty()) {
            dragged = hit;
            if (chosen)
                chosen(hit);
        } else if (track().contains(event->localPos()) && stops.size() < 5 && addRequested)
            addRequested(position(event->localPos().x()));
        event->accept();
    }
    void mouseMoveEvent(QMouseEvent *event) override {
        if (!dragged.isEmpty() && event->buttons().testFlag(Qt::LeftButton) && moved)
            moved(dragged, position(event->localPos().x()));
    }
    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton)
            dragged.clear();
    }
    void keyPressEvent(QKeyEvent *event) override {
        int index = -1;
        for (int i = 0; i < stops.size(); ++i)
            if (stops[i].id == active)
                index = i;
        if (index < 0) {
            QWidget::keyPressEvent(event);
            return;
        }
        const auto key = event->key();
        if (key == Qt::Key_Delete || key == Qt::Key_Backspace) {
            if (stops.size() > 2 && removeRequested)
                removeRequested(active);
            event->accept();
            return;
        }
        if (key != Qt::Key_Left && key != Qt::Key_Right && key != Qt::Key_Home &&
            key != Qt::Key_End) {
            QWidget::keyPressEvent(event);
            return;
        }
        if (event->modifiers().testFlag(Qt::ControlModifier)) {
            if (chosen)
                chosen(
                    stops[qBound(0, index + (key == Qt::Key_Left ? -1 : 1), stops.size() - 1)].id);
        } else if (moved) {
            const auto step = event->modifiers().testFlag(Qt::ShiftModifier) ? .1 : .01;
            const auto next = key == Qt::Key_Home ? 0.
                              : key == Qt::Key_End
                                  ? 1.
                                  : stops[index].position + (key == Qt::Key_Left ? -step : step);
            moved(active, qBound(0., next, 1.));
        }
        event->accept();
    }

  private:
    QVector<ColorStop> stops;
    QString active, dragged;
    QRectF track() const {
        return {12, 6, double(qMax(1, width() - 24)), 24};
    }
    double x(double position) const {
        return track().left() + position * track().width();
    }
    double position(double x) const {
        return qBound(0., (x - track().left()) / track().width(), 1.);
    }
};
} // namespace Unbound
