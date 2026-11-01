// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/panelcontentscale.h"
#include "../../../src/core/panelcommandcontext.h"
#include <QTreeWidget>
#include <QTabWidget>
#include <QLineEdit>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QMouseEvent>
#include "nativepaintfixtures.h"
namespace OpenGateTest {
// Represents native custom painting that deliberately ignores QWidget fonts.
class FixedPaintDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *p,const QStyleOptionViewItem &option,const QModelIndex &) const override {
        p->fillRect(option.rect,Qt::black);
        p->fillRect(QRect(option.rect.x()+4,option.rect.y()+4,10,10),Qt::red);
        const auto point=p->worldTransform().map(QPointF(option.rect.x()+30,option.rect.y()+16));
        p->save();p->setWorldTransform(QTransform());
        QFont font;font.setPixelSize(10);p->setFont(font);p->setPen(Qt::green);p->drawText(point,"HH");p->restore();
    }
    QSize sizeHint(const QStyleOptionViewItem &,const QModelIndex &) const override {return {250,24};}
};
template<class Check> void mediaPoolContentScaleChecks(Check check) {
    QTabWidget tabs;tabs.setAttribute(Qt::WA_DontShowOnScreen);tabs.resize(650,350);
    QWidget root;root.setObjectName("frameEditPageMediaPool");tabs.addTab(&root,"Media Pool");
    auto layout=new QVBoxLayout(&root);auto toolbar=new QPushButton("Native toolbar");toolbar->setFixedHeight(31);layout->addWidget(toolbar);
    auto canvas=new QSplitter;canvas->setObjectName("mediaPoolViewSplitter");auto bins=new QTreeWidget;auto clips=new QTreeWidget;
    bins->setHeaderHidden(true);clips->setHeaderHidden(true);bins->setRootIsDecorated(false);clips->setRootIsDecorated(false);
    canvas->addWidget(bins);canvas->addWidget(clips);layout->addWidget(canvas);
    bins->addTopLevelItem(new QTreeWidgetItem(QStringList{"Bins"}));clips->addTopLevelItem(new QTreeWidgetItem(QStringList{"Clip"}));
    clips->setItemDelegate(new FixedPaintDelegate(clips));tabs.show();QApplication::setActiveWindow(&tabs);
    for(int i=0;i<4;++i)QApplication::processEvents();
    const auto originalParent=canvas->parentWidget();const auto toolbarFont=toolbar->font();
    const int baselineGlyph=paintedInkHeight(clips->viewport()->grab().toImage(),Qt::green);
    Unbound::PanelContentScale scale;scale.setValue(3.);
    check(scale.apply(&root),"Media Pool actual content canvas mounts");
    for(int i=0;i<8;++i)QApplication::processEvents();
    auto viewport=dynamic_cast<Unbound::ScalableContentViewport *>(root.findChild<QGraphicsView *>("openGateScalableContentViewport"));
    check(viewport&&viewport->horizontalScrollBar()->maximum()>0,"scaled content offers a draggable horizontal scrollbar");
    tabs.resize(230,350);for(int i=0;i<8;++i)QApplication::processEvents();
    check(root.width()<=230&&viewport&&viewport->width()<240&&viewport->horizontalScrollBar()->maximum()>500,"panel narrows while scaled canvas remains scrollable");
    check(toolbar->height()==31&&toolbar->font()==toolbarFont,"Media Pool toolbar remains compact");
    if(viewport) {
        viewport->horizontalScrollBar()->setValue(viewport->horizontalScrollBar()->maximum());
        for(int i=0;i<4;++i)QApplication::processEvents();
        auto nativeProxy=canvas->graphicsProxyWidget();
        const auto paintPoint=clips->viewport()->mapTo(canvas,QPoint(4,4));
        viewport->ensureVisible(nativeProxy->mapRectToScene(QRectF(paintPoint,QSize(12,12))),4,4);
        for(int i=0;i<4;++i)QApplication::processEvents();
        const auto image=viewport->viewport()->grab().toImage();int red=0;
        for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x) {
            const auto c=image.pixelColor(x,y);if(c.red()>220&&c.green()<30&&c.blue()<30)++red;
        }
        check(red>=600,"fixed-size native delegate drawing becomes larger, not only heading fonts");
        check(baselineGlyph>0&&paintedInkHeight(image,Qt::green)>=baselineGlyph*2.4,
              "Media Pool glyphs enlarge with positions even when native delegate painting bypasses world transforms");
        QApplication::setActiveWindow(&tabs);viewport->setFocus();viewport->scene()->setFocus(Qt::ActiveWindowFocusReason);
        const auto target=clips->visualItemRect(clips->topLevelItem(0)).topLeft()+QPoint(8,8);
        check(clips->itemAt(target)==clips->topLevelItem(0),"native input test point belongs to the clip item");
        auto proxy=canvas->graphicsProxyWidget();const auto local=clips->viewport()->mapTo(canvas,target);
        const auto point=viewport->mapFromScene(proxy->mapToScene(local));
        viewport->ensureVisible(proxy->mapRectToScene(QRectF(local-QPoint(3,3),QSize(6,6))),0,0);
        for(int i=0;i<4;++i)QApplication::processEvents();
        const auto hit=viewport->mapFromScene(proxy->mapToScene(local));
        const auto global=viewport->viewport()->mapToGlobal(hit);
        QMouseEvent down(QEvent::MouseButtonPress,hit,global,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
        QMouseEvent up(QEvent::MouseButtonRelease,hit,global,Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(viewport->viewport(),&down);QApplication::sendEvent(viewport->viewport(),&up);
        if(clips->currentItem()!=clips->topLevelItem(0))std::fprintf(stderr,"scaled hit %d,%d viewport %dx%d item=%d current=%p focus=%s\n",hit.x(),hit.y(),viewport->viewport()->width(),viewport->viewport()->height(),viewport->itemAt(hit)==proxy,static_cast<void *>(clips->currentItem()),QApplication::focusWidget()?QApplication::focusWidget()->metaObject()->className():"none");
        check(clips->currentItem()==clips->topLevelItem(0),"scaled pointer input selects the original native clip item");
        (void)point;
        QApplication::setActiveWindow(&tabs);clips->setFocus();QApplication::processEvents();
        check(Unbound::PanelCommandContext::capture({"frameEditPageMediaPool"}).panel=="frameEditPageMediaPool","focused commands retain panel identity across embedding");
        clips->topLevelItem(0)->setFlags(clips->topLevelItem(0)->flags()|Qt::ItemIsEditable);
        clips->editItem(clips->topLevelItem(0));for(int i=0;i<4;++i)QApplication::processEvents();
        check(Unbound::textEditor(QApplication::focusWidget()),"inline rename remains protected inside the scaled canvas");
        const QPoint wanted(180,0);scale.restoreScrollOffset(wanted);for(int i=0;i<4;++i)QApplication::processEvents();
        const auto state=scale.snapshot();
        scale.suspend();check(scale.scrollOffset().x()==180,"suspension remembers content scroll state");
        scale.apply(&root);for(int i=0;i<8;++i)QApplication::processEvents();
        check(scale.scrollOffset().x()==180,"reattachment restores scroll state after layout");
        scale.suspend();for(int i=0;i<4;++i)QApplication::processEvents();
        Unbound::PanelContentScale restored;restored.restoreState(state);restored.apply(&root);
        for(int i=0;i<8;++i)QApplication::processEvents();
        check(restored.value()==3.&&restored.snapshot()["content_canvas_width"]==state["content_canvas_width"]&&restored.scrollOffset().x()==180,"serialized content scale/canvas/scroll restore into a fresh state owner");
        auto reset=state;reset["interface_scale"]=1.;reset["content_scroll"]=QJsonArray{0,0};
        restored.restoreState(reset);restored.apply(&root);
        check(restored.scrollOffset().isNull(),"workspace reset preserves its requested zero scroll instead of recapturing old live offset");
        restored.suspend();
    }
    scale.suspend();
    QWidget other;QLineEdit editing(&other);
    tabs.addTab(&other,"Other");tabs.show();tabs.setCurrentWidget(&other);
    editing.show();QApplication::setActiveWindow(&tabs);editing.setFocus();QApplication::processEvents();
    const auto focused=QApplication::focusWidget();
    check(scale.apply(&root),"covered Media Pool content can prepare its scale without selecting its tab");
    for(int i=0;i<4;++i)QApplication::processEvents();
    check(tabs.currentWidget()==&other&&QApplication::focusWidget()==focused,"covered content preparation preserves current tab and editor focus");
    scale.suspend();
    check(canvas->parentWidget()==originalParent&&layout->indexOf(canvas)==1&&!canvas->graphicsProxyWidget(),"native owner and layout restore exactly");
}
}
