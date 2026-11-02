// SPDX-License-Identifier: MIT
#pragma once
#include <QElapsedTimer>
#include <QThread>
#include "timelineviewportfixtures.h"
#include "../../../src/core/timelinescenerenderer.h"
#include "../../../src/core/timelinesceneupdates.h"

namespace OpenGateTest {
class CountedGlyph : public FixedGlyphItem {
public:
    int paints=0;
    void paint(QPainter *p,const QStyleOptionGraphicsItem *option,QWidget *widget) override {
        ++paints;FixedGlyphItem::paint(p,option,widget);
    }
};
template<class Check> void timelineRenderCacheChecks(Check check) {
    TimelineViewportFixture fixture;auto view=fixture.view;
    auto glyph=new CountedGlyph;glyph->setPos(220,110);view->tracksScene->addItem(glyph);
    auto playhead=new QGraphicsRectItem(0,0,2,view->viewport()->height());
    playhead->setBrush(Qt::red);playhead->setPen(Qt::NoPen);playhead->setZValue(1000);view->sequence->addItem(playhead);
    const auto flags=view->tracksProxy->flags();
    Unbound::TimelineSceneRenderer renderer;
    const auto geometry=Unbound::TimelineTrackHeaderLayout::discover(view);
    const auto source=view->mapToScene(view->viewport()->rect()).boundingRect();
    auto snapshot=[&](double backing) {
        QImage image(QSize(qCeil(source.width()*backing),qCeil(source.height()*backing)),QImage::Format_ARGB32_Premultiplied);
        image.setDevicePixelRatio(backing);image.fill(Qt::transparent);
        QPainter painter(&image);renderer.draw(painter,QRectF(QPointF(),source.size()),QRectF(QPointF(),source.size()));
        return image;
    };
    auto equivalent=[&](const QImage &a,const QImage &b,double backing,const char *kind) {
        if(a==b)return true;
        int count=0;QRect bounds;
        for(int y=0;y<qMin(a.height(),b.height());++y)for(int x=0;x<qMin(a.width(),b.width());++x)
            if(a.pixel(x,y)!=b.pixel(x,y)) {
                if(count<3)std::fprintf(stderr,"pixel %s backing %.2f at %d,%d %08x/%08x\n",kind,backing,x,y,a.pixel(x,y),b.pixel(x,y));
                ++count;bounds|=QRect(x,y,1,1);
            }
        std::fprintf(stderr,"pixel difference %s backing %.2f count %d bounds %d,%d %dx%d\n",kind,backing,count,bounds.x(),bounds.y(),bounds.width(),bounds.height());
        return false;
    };
    for(double backing:{1.4,2.,2.94,4.26,6.}) {
        playhead->setX(500.);renderer.invalidateAll();
        const bool first=renderer.render(view->sequence,source,backing,geometry.sectionTops);
        check(first,"cache renders a valid native image at fractional backing resolution");
        const auto bytes=renderer.rasterBytes();const auto revision=renderer.revision();const int count=glyph->paints;
        for(int i=0;i<10;++i)renderer.render(view->sequence,source,backing,geometry.sectionTops);
        check(glyph->paints==count && renderer.rasterBytes()==bytes,"unchanged exposure reuses pixels without drawing native clip content or detaching its image");
        check(renderer.revision()==revision,"unchanged exposure cannot claim a fresh native raster revision");
        const auto old=playhead->sceneBoundingRect();playhead->setX(505.3);
        renderer.invalidate(old);renderer.invalidate(playhead->sceneBoundingRect());
        renderer.render(view->sequence,source,backing,geometry.sectionTops);const auto incremental=snapshot(backing);
        renderer.invalidateAll();renderer.render(view->sequence,source,backing,geometry.sectionTops);const auto fresh=snapshot(backing);
        check(equivalent(incremental,fresh,backing,"playhead"),"moving playhead damage is pixel-identical to a full fresh render without trails");
        const auto before=view->tracksProxy->mapRectToScene(glyph->sceneBoundingRect());glyph->moveBy(.3,2.7);
        renderer.invalidate(before);renderer.invalidate(view->tracksProxy->mapRectToScene(glyph->sceneBoundingRect()));
        renderer.render(view->sequence,source,backing,geometry.sectionTops);const auto moved=snapshot(backing);
        renderer.invalidateAll();
        renderer.render(view->sequence,source,backing,geometry.sectionTops);
        check(equivalent(moved,snapshot(backing),backing,"glyph"),"native glyph movement remains coherent and pixel-identical under incremental painting");
    }
    const auto old=view->tracksProxy->mapRectToScene(glyph->sceneBoundingRect());delete glyph;renderer.invalidate(old);
    renderer.render(view->sequence,source,6.,geometry.sectionTops);const auto removed=snapshot(6.);renderer.invalidateAll();
    renderer.render(view->sequence,source,6.,geometry.sectionTops);
    check(removed==snapshot(6.),"deleted content leaves no stale cached pixels");
    renderer.releasePixels();check(renderer.rasterBytes()==0,"hidden presentations release raster memory without releasing native model ownership");
    check(renderer.render(view->sequence,source,6.,geometry.sectionTops),"presentation pixels rebuild from current native content after becoming visible");
    renderer.reset();check(view->tracksProxy->flags()==flags,"native exposure flags restore exactly with the renderer lease");
    Unbound::TimelineSceneUpdates updates(view,&fixture.timeline);
    int layouts=0,damages=0;
    updates.layoutChanged=[&]{++layouts;updates.geometryObserved();};
    updates.damaged=[&](const QList<QRectF> &){++damages;};updates.refresh();TimelineViewportFixture::settle();
    layouts=damages=0;
    for(int i=0;i<30;++i) {playhead->setX(510+i);TimelineViewportFixture::settle();}
    check(layouts==0 && damages>0,"playhead content updates invalidate pixels without rebuilding the track/header layout");
    layouts=damages=0;QElapsedTimer idle;idle.start();
    while(idle.elapsed()<80){QApplication::processEvents();QThread::usleep(500);}
    check(layouts==0 && damages==0,"idle presentation creates no redraw or layout polling work");
    view->clip->setBrush(Qt::green);TimelineViewportFixture::settle();
    check(layouts==0 && damages>0,"nested native clip-scene changes invalidate content without rebuilding header geometry");
    view->blue->moveBy(0,1);TimelineViewportFixture::settle();
    check(layouts>0,"real native header geometry still requests layout reconciliation");
    fixture.timeline.hide();TimelineViewportFixture::settle();layouts=0;
    view->blue->moveBy(0,1);TimelineViewportFixture::settle();
    check(layouts==0,"hidden Timeline content does not rebuild its layout on background native changes");
    fixture.timeline.show();TimelineViewportFixture::settle();
    check(layouts>0,"showing the Timeline reconciles retained geometry after hidden changes");
    updates.stop();
}
}
