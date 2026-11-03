// SPDX-License-Identifier: MIT
#include <QTemporaryDir>
#include <cstdio>
#include "../../../src/core/timelineclipcolorpreview.h"
#include "../../core/workspace-callers/timelineviewportfixtures.h"
#include "../../core/workspace-callers/clipnativefillprobe.h"
int main(int argc,char **argv) {
    QApplication app(argc,argv);QTemporaryDir tmp;qputenv("UNBOUND_DIAGNOSTICS_DIR",tmp.path().toUtf8());
    int failures=0;
    for(double factor:{.7,1.,1.47,2.13,3.}) {
        OpenGateTest::TimelineViewportFixture fixture;fixture.view->clip->hide();
        auto proxy=fixture.view->tracksProxy;
        auto tracks=dynamic_cast<BatchedClipSceneView *>(proxy->widget());tracks->clipBackground=true;
        auto clip=new UiTiClipContainer;clip->setFlag(QGraphicsItem::ItemHasNoContents);clip->setSelected(true);
        fixture.view->tracksScene->addItem(clip);
        auto details=new UiTiClipContainer;details->paintBody=false;details->setParentItem(clip);
        details->setFlag(QGraphicsItem::ItemIsSelectable,false);
        auto other=new UiTiClipContainer;other->setFlag(QGraphicsItem::ItemHasNoContents);other->setPos(0,110);
        fixture.view->tracksScene->addItem(other);OpenGateTest::TimelineViewportFixture::settle();
        auto frame=[&]{
            const auto bounds=fixture.view->sceneRect();QImage image((bounds.size()*factor*2).toSize(),QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(2);image.fill(Qt::transparent);QPainter p(&image);p.scale(factor,factor);
            fixture.view->scene()->render(&p,bounds,bounds);return image;
        };
        const auto before=frame();Unbound::TimelineClipColorPreview preview(fixture.view,&fixture.timeline);
        preview.applySelected(Qt::red);int changed=0;
        proxy->setGraphicsEffect(new OpenGateTest::ClipNativeFillProbe(fixture.view,proxy,
            [&](const Unbound::ClipFillPaintDevice::Result &result){changed=result.changed;}));
        const auto after=frame();
        auto pixel=[&](const QImage &image,QPointF pos){return image.pixelColor((proxy->mapToScene(pos)*factor*2).toPoint());};
        auto check=[&](bool good,const char *why){if(!good){++failures;std::fprintf(stderr,"FAIL %.2f %s\n",factor,why);}};
        check(changed>0 && pixel(after,{200,20})==QColor(Qt::red),"native proxy background fill reaches paint adapter");
        check(pixel(after,{200,130})==other->fill,"neighbor in shared batch retains native fill");
        check(pixel(after,{20,15})==pixel(before,{20,15}),"thumbnail pixels remain native");
        proxy->setGraphicsEffect(nullptr);preview.clear();check(frame()==before,"probe cleanup restores exact native frame");
    }
    std::fprintf(stderr,"native fill probe failures=%d\n",failures);return failures?1:0;
}
