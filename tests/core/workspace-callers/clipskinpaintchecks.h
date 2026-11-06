// SPDX-License-Identifier: MIT
#pragma once
#include "timelineviewportfixtures.h"
#include "../../../src/core/panelcontentscale.h"
#include "../../../src/core/timelineclipcolorpreview.h"
#include "../../../src/core/clipskinpaint.h"
#include "../../../src/core/clippainttrace.h"
namespace OpenGateTest {
template<class Check> void clipSkinPaintChecks(Check check) {
    QImage skinImage(12,12,QImage::Format_ARGB32);skinImage.fill(QColor(30,130,180));
    skinImage.setPixelColor(0,0,QColor(0,0,0,100));skinImage.setPixelColor(1,0,QColor(30,130,180,90));
    const auto recolored=Unbound::clipSkinPalette(skinImage,Qt::red);
    check(recolored.pixelColor(6,6)==QColor(Qt::red) && recolored.pixelColor(0,0)==skinImage.pixelColor(0,0) && recolored.pixelColor(1,0).alpha()==90,
          "skin palette changes the body hue and preserves neutral shadows and alpha");
    auto mixed=skinImage;mixed.setPixelColor(5,5,Qt::green);
    check(Unbound::clipSkinPalette(mixed,Qt::red).isNull(),"mixed-hue image is not accepted as a monochrome clip skin");
    {
        // Actual native trace: 244x164 physical pixels at DPR 2. A preceding
        // neighbor's right edge overlaps the selected clip's shadow margin.
        QPixmap bitmap(244,164);bitmap.fill(QColor(67,118,161));bitmap.setDevicePixelRatio(2);
        Unbound::ClipSkinPaint paint;paint.begin({{QRectF(670.905548,173,144.746224,72),Qt::red}});
        paint.map({667,174,4,5},bitmap,{236,0,8,10});paint.map({667,242,4,4},bitmap,{236,156,8,8});paint.map({667,179,4,63},bitmap,{236,10,8,146});
        const QVector<Unbound::ClipSkinTile> tiles{
            {{670,174,4,5},{0,0,8,10}},{{812,174,4,5},{236,0,8,10}},
            {{670,242,4,4},{0,156,8,8}},{{812,242,4,4},{236,156,8,8}},
            {{674,174,138,5},{8,0,228,10}},{{674,242,138,4},{8,156,228,8}},
            {{670,179,4,63},{0,10,8,146}},{{812,179,4,63},{236,10,8,146}},{{674,179,138,63},{8,10,228,146}}};
        QPixmap result;for(const auto &tile:tiles)result=paint.map(tile.target,bitmap,tile.source);
        check(paint.learned() && !result.isNull() && result.toImage().pixelColor(122,82)==QColor(Qt::red),
            "observed native DPR-2 skin and adjacent shadow fragments identify only the selected background");
    }
    {
        const auto pixmap=QPixmap::fromImage(skinImage);const QRectF bounds(10,10,240,95);
        const auto tiles=BatchedClipSceneView::skinTiles(bounds,pixmap.size());Unbound::ClipSkinPaint paint;
        paint.begin({{bounds,Qt::red}});check(paint.map(bounds,pixmap,QRectF(QPointF(),pixmap.size())).isNull(),"a single image covering a clip is not a nine-piece skin");
        paint.begin({{bounds,Qt::red}});QPixmap result;
        for(const auto &tile:tiles)result=paint.map(tile.first,pixmap,tile.second);
        check(paint.learned() && !result.isNull() && result.toImage().pixelColor(6,6)==QColor(Qt::red),"a complete nine-piece native background becomes a verified recolored skin");
        check(paint.mapBodyFill(bounds,skinImage.pixelColor(6,6))==QColor(Qt::red),"complete native skin establishes its source palette for this capture");
        paint.begin({{bounds,Qt::red}});const auto cached=paint.map(tiles.front().first,pixmap,tiles.front().second);
        check(!paint.learned() && cached.cacheKey()==result.cacheKey(),"later frames reuse recolored bitmap pixels without another palette transform");
        paint.begin({{bounds,Qt::red}});
        check(!paint.mapBodyFill(bounds,skinImage.pixelColor(6,6)).isValid(),"new capture discards prior clip-to-palette association");
        paint.map(tiles.front().first,pixmap,tiles.front().second);
        check(paint.mapBodyFill(bounds,skinImage.pixelColor(6,6))==QColor(Qt::red),"cached native skin reestablishes current palette ownership");
        check(!paint.mapBodyFill(bounds,Qt::green).isValid(),"unknown solid ink cannot borrow a verified body's palette");
    }
    {
        const QRectF bounds(10,10,240,95);
        QPixmap transparent(226,228);transparent.fill(Qt::transparent);
        const auto tiles=BatchedClipSceneView::skinTiles(bounds,transparent.size());
        Unbound::ClipSkinPaint paint;
        const auto capture=[&](const QPixmap &image,QColor target){
            paint.begin({{bounds,target}});QPixmap result;
            for(const auto &tile:tiles)result=paint.map(tile.first,image,tile.second);
            return result;
        };
        check(capture(transparent,Qt::red).isNull() && capture(transparent,Qt::blue).isNull(),
              "transparent-center skin stays native across targets and repeated captures");
        int reports=0;paint.begin({{bounds,Qt::red}},[&](QRectF,QColor center,QSize size,qint64){
            ++reports;check(center.alpha()==0 && size==transparent.size(),
                            "cached rejection retains center metadata without inventing a body color");});
        for(const auto &tile:tiles)paint.map(tile.first,transparent,tile.second);
        check(reports==1 && !paint.mapBodyFill(bounds,Qt::transparent).isValid() &&
                  paint.opaqueMask(transparent).isEmpty(),
              "cached rejection can be inspected later without creating paint coverage");
        paint.begin({{bounds,Qt::red},{bounds,Qt::blue}});
        QPixmap ambiguous;for(const auto &tile:tiles)ambiguous=paint.map(tile.first,transparent,tile.second);
        check(ambiguous.isNull(),"negative classification never bypasses current owner ambiguity");
        const auto rejectedKey=transparent.cacheKey();transparent.fill(QColor(30,130,180));
        const auto repaired=capture(transparent,Qt::red);
        check(transparent.cacheKey()!=rejectedKey && !repaired.isNull() &&
                  repaired.toImage().pixelColor(113,114)==QColor(Qt::red),
              "pixmap mutation changes its key and reclassifies newly valid source pixels");
        QPixmap fresh(226,228);fresh.fill(QColor(30,130,180));
        check(capture(fresh,QColor()).isNull() && !capture(fresh,Qt::red).isNull(),
              "invalid target color cannot poison an otherwise valid native skin");
    }
    {
        const QRectF bounds(10,10,240,95);QImage gray(12,12,QImage::Format_ARGB32);gray.fill(Qt::gray);
        const auto tiles=BatchedClipSceneView::skinTiles(bounds,gray.size());
        Unbound::ClipSkinPaint paint;int rejected=0;
        const auto report=[&](QRectF r,QColor center,QSize size,qint64){
            ++rejected;check(r==bounds && center==QColor(Qt::gray) && size==gray.size(),"rejected skin metadata identifies verified geometry and center only");};
        const auto first=QPixmap::fromImage(gray);paint.begin({{bounds,Qt::red}},report);
        paint.map(tiles.front().first,first,tiles.front().second);
        check(rejected==0,"unverified image fragments do not emit palette rejection receipts");
        for(int n=0;n<20;++n){const auto image=QPixmap::fromImage(gray);paint.begin({{bounds,Qt::red}},report);
            for(const auto &tile:tiles)paint.map(tile.first,image,tile.second);}
        check(rejected==16,"opt-in rejected skin receipts are bounded by distinct image keys");
        auto style=QSharedPointer<Unbound::ColorPreset>::create();style->id="trace-fixture";
        QJsonObject receipt;Unbound::ClipPaintTrace trace(true,[&](const QJsonObject &r){receipt=r;});
        trace.begin({{bounds,Qt::red,{},style}},QTransform(),3);
        trace.rejectedSkin(bounds,Qt::gray,gray.size(),first.cacheKey());
        check(receipt["complete_nine_patch_verified"].toBool() && receipt["read_only"].toBool() &&
                  !receipt["pixel_content_captured"].toBool() && receipt["source_color"]=="#ffa0a0a4",
              "palette rejection trace preserves its read-only metadata boundary");
    }
    for(double factor:{.7,1.,1.47,2.13,3.}) {
        TimelineViewportFixture fixture;fixture.view->clip->hide();auto tracks=dynamic_cast<BatchedClipSceneView *>(fixture.view->tracksProxy->widget());
        tracks->clipBackground=true;tracks->clipSkins=true;tracks->skin=QPixmap::fromImage(skinImage);
        auto clip=new UiTiClipContainer;clip->setFlag(QGraphicsItem::ItemHasNoContents);clip->setSelected(true);fixture.view->tracksScene->addItem(clip);
        auto details=new UiTiClipContainer;details->paintBody=false;details->setParentItem(clip);details->setFlag(QGraphicsItem::ItemIsSelectable,false);
        auto other=new UiTiClipContainer;other->setFlag(QGraphicsItem::ItemHasNoContents);other->setPos(0,110);fixture.view->tracksScene->addItem(other);
        Unbound::PanelContentScale scale;scale.restoreState({{"interface_scale",factor},{"timeline_header_width",160.},{"timeline_header_scale",.7}});scale.apply(&fixture.timeline);
        TimelineViewportFixture::settle();auto content=dynamic_cast<Unbound::TimelineContentViewport *>(fixture.timeline.findChild<QWidget *>("openGateTimelineContentViewport"));
        auto frame=[&]{tracks->viewport()->update();TimelineViewportFixture::settle();return content->presentation()->grab().toImage();};
        auto pixel=[&](const QImage &im,QPointF point){return im.pixelColor((QPointF((content->headerWidth()+point.x())*factor,(40+point.y())*factor)*im.devicePixelRatioF()).toPoint());};
        const auto before=frame();Unbound::TimelineClipColorPreview preview(fixture.view,content);preview.applySelected(Qt::red);frame();const auto after=frame();
        check(pixel(after,{200,20})==QColor(Qt::red),"native nine-piece body background paints red at every content scale");
        check(pixel(after,{200,130})==pixel(before,{200,130}),"unselected neighboring clip using the identical skin remains native");
        check(pixel(after,{20,15})==pixel(before,{20,15}) && pixel(after,{20,45})==pixel(before,{20,45}),"skin painting preserves thumbnails and waveform foreground");
        preview.applySelected(QColor("#e15da8"));frame();check(pixel(frame(),{200,20})==QColor("#e15da8"),"the same native skin supports an arbitrary RGB body color");
        preview.clear();check(frame()==before,"clearing the skin binding restores exact native pixels");
    }
}
}
