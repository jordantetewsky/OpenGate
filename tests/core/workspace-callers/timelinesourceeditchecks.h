// SPDX-License-Identifier: MIT
#pragma once
#include "timelineviewportfixtures.h"
#include "../../../src/core/panelcontentscale.h"
#include <QAction>
#include <QShortcut>
#include <QShortcutEvent>
namespace OpenGateTest {
inline void sourceEditAfterTurns(QObject *owner,int turns,std::function<void()> apply) {
    if(!turns){apply();return;}
    QTimer::singleShot(0,owner,[owner,turns,apply]{sourceEditAfterTurns(owner,turns-1,apply);});
}
// Native Qt transport/model fixtures, not calls into Resolve's private edit API.
template<class Check> void timelineSourceEditChecks(Check check) {
    for(double factor:{.7,1.,1.47,2.13,3.})for(int delay:{0,3,7}) {
        QWidget main;main.setAttribute(Qt::WA_DontShowOnScreen);main.resize(1200,800);
        TimelineViewportFixture fixture(&main);fixture.view->clip->hide();
        QWidget source(&main);source.setObjectName("widgetEditSourcePlayer");source.setFocusPolicy(Qt::StrongFocus);
        source.setGeometry(950,0,200,200);source.show();main.show();
        auto scene=fixture.view->tracksScene;
        auto original=scene->addRect(0,80,160,45,QPen(Qt::NoPen),QBrush(Qt::yellow));original->setZValue(1000);
        Unbound::PanelContentScale scale;scale.restoreState({{"interface_scale",factor},{"timeline_header_width",160.},{"timeline_header_scale",.7}});
        scale.apply(&fixture.timeline);TimelineViewportFixture::settle();
        auto content=dynamic_cast<Unbound::TimelineContentViewport *>(fixture.timeline.findChild<QWidget *>("openGateTimelineContentViewport"));
        int inserts=0,onTops=0;QGraphicsRectItem *inserted=nullptr,*onTop=nullptr;
        QAction insert("Insert",&source),place("Place on Top",&source);source.addAction(&insert);source.addAction(&place);
        QObject::connect(&insert,&QAction::triggered,&source,[&]{sourceEditAfterTurns(&source,delay,[&]{
            ++inserts;original->moveBy(40,0);inserted=scene->addRect(0,80,40,45,QPen(Qt::NoPen),QBrush(QColor(255,140,0)));inserted->setZValue(2000);
        });});
        QObject::connect(&place,&QAction::triggered,&source,[&]{sourceEditAfterTurns(&source,delay,[&]{
            ++onTops;onTop=scene->addRect(0,10,140,45,QPen(Qt::NoPen),QBrush(Qt::green));onTop->setZValue(2000);
        });});
        QShortcut insertKey(QKeySequence(Qt::Key_F9),&source),topKey(QKeySequence(Qt::Key_F12),&source);
        QObject::connect(&insertKey,&QShortcut::activated,&insert,&QAction::trigger);
        QObject::connect(&topKey,&QShortcut::activated,&place,&QAction::trigger);
        TimelineViewportFixture::settle();QApplication::setActiveWindow(&main);source.setFocus();
        auto pixel=[&](int y){const auto image=content->presentation()->grab().toImage();return image.pixelColor(
            (QPointF((content->headerWidth()+20)*factor,y*factor)*image.devicePixelRatioF()).toPoint());};
        pixel(140);const auto count=scene->items().size();
        QShortcutEvent a(insertKey.key(),insertKey.id(),false);QApplication::sendEvent(&insertKey,&a);TimelineViewportFixture::settle();
        check(inserts==1 && inserted && original->x()==40 && scene->items().size()==count+1,
              "Source-focused native Insert shortcut edits the actual scene once after synchronous or queued work");
        check(pixel(140)==QColor(255,140,0),"Source Insert paints its new native clip without resize, zoom or a Timeline click");
        QShortcutEvent b(topKey.key(),topKey.id(),false);QApplication::sendEvent(&topKey,&b);TimelineViewportFixture::settle();
        check(onTops==1 && onTop && onTop->rect().top()<original->rect().top() && scene->items().size()==count+2,
              "Source-focused native Place on Top creates the upper native clip exactly once");
        check(pixel(70)==QColor(Qt::green),"Source Place on Top paints the upper clip immediately at every content scale");
        check(QApplication::focusWidget()==&source,"native source editing leaves focus in Source Monitor");
    }
}
}
