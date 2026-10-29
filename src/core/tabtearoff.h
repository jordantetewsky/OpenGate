// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QMouseEvent>
#include <QPointer>
#include <QTabBar>
#include <QTabWidget>
#include <QTimer>
#include <functional>
namespace Unbound {
// Leave ordinary tab clicks intact. Detach only after dragging outside the tab strip.
class TabTearOff : public QObject
{
public:
    TabTearOff(QTabWidget *tabs, QObject *owner, std::function<void(QWidget *, QPoint)> detach)
        : QObject(owner), tabs(tabs), detach(std::move(detach))
    { tabs->tabBar()->installEventFilter(this); }
protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (!tabs || object != tabs->tabBar()) return false;
        auto bar = tabs->tabBar();
        if (event->type() == QEvent::MouseButtonPress) {
            auto mouse = static_cast<QMouseEvent *>(event);
            if (mouse->button() == Qt::LeftButton) {
                ++generation;
                const int index = bar->tabAt(mouse->pos());
                selected = index >= 0 ? tabs->widget(index) : nullptr;
                press = mouse->globalPos();
            }
        } else if (event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::Hide) { selected = nullptr; ++generation; }
        else if (event->type() == QEvent::MouseMove && selected) {
            auto mouse = static_cast<QMouseEvent *>(event);
            if (!mouse->buttons().testFlag(Qt::LeftButton)) { selected = nullptr; return false; }
            if ((mouse->globalPos()-press).manhattanLength() < QApplication::startDragDistance() ||
                bar->rect().adjusted(-6,-6,6,6).contains(mouse->pos())) return false;
            const auto panel = selected; const auto position = mouse->globalPos(); selected = nullptr;
            // Rebuilding the group may retire this tab bar; never do that inside its event dispatch.
            const auto request = generation;
            QTimer::singleShot(0, this, [this, panel, position, request] {
                if (!panel || request != generation) return;
                auto callback = detach;
                callback(panel, position);
            });
            return true;
        }
        return false;
    }
private:
    QPointer<QTabWidget> tabs;
    QPointer<QWidget> selected;
    QPoint press;
    unsigned generation = 0;
    std::function<void(QWidget *, QPoint)> detach;
};
}
