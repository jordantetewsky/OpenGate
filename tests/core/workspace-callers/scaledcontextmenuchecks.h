// SPDX-License-Identifier: MIT
#pragma once
#include <QContextMenuEvent>
#include <QMenu>
#include <QScreen>
#include <QGraphicsRectItem>
#include <QGraphicsSceneContextMenuEvent>
#include <functional>
#include "../../../src/core/scalablecontentviewport.h"

namespace OpenGateTest {
class SceneMenuObserver : public QObject {
public:
    std::function<void(QPoint)> open;
    QPoint screen;
    QPointF point;
protected:
    bool eventFilter(QObject *, QEvent *event) override {
        if (event->type() != QEvent::GraphicsSceneContextMenu) return false;
        const auto menu = static_cast<QGraphicsSceneContextMenuEvent *>(event);
        screen = menu->screenPos(); point = menu->scenePos();
        if (open) open(screen);
        event->accept(); return true;
    }
};
// Exercise native QWidget menu handlers through the actual scaled viewport.
template <class Check> void scaledContextMenuChecks(Check check)
{
    QWidget host;
    host.setAttribute(Qt::WA_DontShowOnScreen);
    host.resize(720, 450);
    auto layout = new QVBoxLayout(&host);
    auto elsewhere = new QLineEdit;
    layout->addWidget(elsewhere);
    auto canvas = new QWidget;
    auto canvasLayout = new QVBoxLayout(canvas);
    auto bins = new QTreeWidget;
    bins->setHeaderHidden(true);
    bins->addTopLevelItem(new QTreeWidgetItem(QStringList{"Fixture bin"}));
    bins->setContextMenuPolicy(Qt::CustomContextMenu);
    canvasLayout->addWidget(bins);
    QGraphicsScene scene;
    SceneMenuObserver observer;
    scene.installEventFilter(&observer);
    auto item = scene.addRect(QRectF(0, 0, 160, 60));
    auto clips = new QGraphicsView(&scene);
    canvasLayout->addWidget(clips);
    auto view = new Unbound::ScalableContentViewport(canvas, &host, 600);
    layout->addWidget(view);
    host.show();
    auto settle = [] { for (int i = 0; i < 8; ++i) QApplication::processEvents(); };
    settle();
    auto proxy = canvas->graphicsProxyWidget();
    for (const double factor : {0.7, 1.0, 1.25, 1.47, 2.0, 3.0}) {
        view->setContentScale(factor);
        settle();
        for (const bool nestedScene : {false, true}) for (const bool embedded : {false, true}) {
            QMenu menu(embedded ? (nestedScene ? static_cast<QWidget *>(clips) : bins) : &host);
            menu.setAttribute(Qt::WA_DontShowOnScreen);
            menu.addAction("Rename");
            auto submenu = menu.addMenu("Clip Color");
            submenu->setAttribute(Qt::WA_DontShowOnScreen);
            auto orange = submenu->addAction("Orange");
            int invoked = 0;
            QObject::connect(orange, &QAction::triggered, [&] { ++invoked; });
            int received = 0;
            auto open = [&](QPoint global) { ++received; menu.popup(global); };
            const auto connection = QObject::connect(bins, &QWidget::customContextMenuRequested,
                &menu, [&](QPoint local) {
                    open(bins->viewport()->mapToGlobal(local));
                });
            observer.open = open;
            QApplication::setActiveWindow(&host);
            elsewhere->setFocus();
            settle();
            auto source = nestedScene ? clips->viewport() : bins->viewport();
            const auto local = nestedScene ? clips->mapFromScene(item->rect().center()) :
                                           bins->visualItemRect(bins->topLevelItem(0)).center();
            const auto scenePoint = proxy->mapToScene(source->mapTo(canvas, local));
            view->ensureVisible(QRectF(scenePoint, QSizeF(1, 1)), 0, 0);
            settle();
            const auto hit = view->mapFromScene(scenePoint);
            const auto global = view->viewport()->mapToGlobal(hit);
            QMouseEvent down(QEvent::MouseButtonPress, hit, global, Qt::RightButton,
                             Qt::RightButton, Qt::NoModifier);
            QApplication::sendEvent(view->viewport(), &down);
            QContextMenuEvent context(QContextMenuEvent::Mouse, hit, global);
            QApplication::sendEvent(view->viewport(), &context);
            settle();
            if (received != 1 || !menu.isVisible())
                std::fprintf(stderr, "menu scale=%.2f nested=%d embedded=%d received=%d visible=%d focus=%d hit=%d,%d view=%dx%d source=%dx%d local=%d,%d\n",
                             factor, nestedScene, embedded, received, menu.isVisible(), proxy->hasFocus(),
                             hit.x(),hit.y(),view->viewport()->width(),view->viewport()->height(),source->width(),source->height(),local.x(),local.y());
            if (nestedScene && received != 1)
                std::fprintf(stderr,"nested scene %.2f,%.2f screen=%d,%d mapped=%d,%d current=%d,%d itemAt=%d outer=%d\n",
                    observer.point.x(),observer.point.y(),observer.screen.x(),observer.screen.y(),source->mapFromGlobal(observer.screen).x(),source->mapFromGlobal(observer.screen).y(),clips->mapFromScene(item->rect().center()).x(),clips->mapFromScene(item->rect().center()).y(),clips->itemAt(local)==item,view->itemAt(hit)==proxy);
            check(received == 1 && menu.isVisible(),
                  "right click opens one native menu from another editor at every content scale");
            if (nestedScene) {
                // Both QWidget context events use integer pixels. Downscaling
                // can round one native pixel on each axis without a hit error.
                const auto delta = observer.point-item->rect().center();
                const auto tolerance = qMax(1.,1./factor);
                const bool correct = qAbs(delta.x()) <= tolerance && qAbs(delta.y()) <= tolerance &&
                                     (observer.screen-global).manhattanLength() < 2;
                if (!correct)
                    std::fprintf(stderr,"nested position scale=%.2f scene=%.2f,%.2f expected=%.2f,%.2f screen delta=%d,%d\n",
                        factor,observer.point.x(),observer.point.y(),item->rect().center().x(),item->rect().center().y(),observer.screen.x()-global.x(),observer.screen.y()-global.y());
                check(correct,
                      "nested scene receives the correct item and physical screen coordinates");
            }
            check(!menu.graphicsProxyWidget() && QApplication::activePopupWidget() == &menu,
                  "scaled native menu remains a tracked popup window outside the canvas");
            if (auto screen = QGuiApplication::screenAt(global))
                check(screen->availableGeometry().contains(menu.frameGeometry()),
                      "native menu stays inside screen bounds at every scale");
            menu.setActiveAction(submenu->menuAction());
            QKeyEvent right(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
            QApplication::sendEvent(&menu, &right);
            settle();
            check(submenu->isVisible() && !submenu->graphicsProxyWidget() &&
                  QApplication::activePopupWidget() == submenu,
                  "native submenu remains visible and tracked above the scaled canvas");
            submenu->setActiveAction(orange);
            QKeyEvent choose(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
            QApplication::sendEvent(submenu, &choose);
            settle();
            check(invoked == 1, "native submenu key selection executes its original action once");
            submenu->hide();
            menu.hide();
            observer.open = {};
            QObject::disconnect(connection);
            QMouseEvent up(QEvent::MouseButtonRelease, hit, global, Qt::RightButton,
                           Qt::NoButton, Qt::NoModifier);
            QApplication::sendEvent(view->viewport(), &up);
            settle();
        }
    }
    view->release(&host);
    check(!canvas->windowFlags().testFlag(Qt::BypassGraphicsProxyWidget),
          "canvas popup policy restores when scaling is released");
}
}
