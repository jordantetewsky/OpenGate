// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QTemporaryDir>
#include <QLineEdit>
#include <cstdio>
#include "../../../src/core/mediapoolclipcolorpreview.h"
#include "../../../src/core/mediapoolcontentscale.h"
#include "../../core/workspace-callers/clipcolorpoolfixture.h"
using namespace OpenGateTest;
static void settle(){for(int i=0;i<15;++i)QApplication::processEvents();}
int main(int argc,char **argv) {
    QApplication app(argc,argv);QTemporaryDir files;qputenv("UNBOUND_DIAGNOSTICS_DIR",files.path().toUtf8());
    int failures=0;auto check=[&](bool value,const char *why){if(!value){++failures;std::fprintf(stderr,"FAIL: %s\n",why);}};
    for(bool rowSpecific:{false,true}) {
        ClipColorPoolFixture fixture;NativeClipDelegate rowDelegate(&fixture.view);
        if(rowSpecific)fixture.view.setItemDelegateForRow(0,&rowDelegate);
        auto original=fixture.view.itemDelegateForRow(0);auto source=rowSpecific?&rowDelegate:&fixture.delegate;
        settle();auto index=fixture.model.index(0,0);const auto nativeColor=index.data(Qt::UserRole);
        int writes=0;QObject::connect(&fixture.model,&QAbstractItemModel::dataChanged,&fixture.root,[&]{++writes;});
        auto capture=[&]{settle();return fixture.view.viewport()->grab().toImage();};
        auto pixel=[&](const QImage &im,int x,int y){return im.pixelColor((QPointF(x,y)*im.devicePixelRatioF()).toPoint());};
        const auto before=capture();
        Unbound::MediaPoolClipColorPreview preview(&fixture.root);
        check(preview.applySelected(&fixture.root,Qt::red)==1,"only the selected native Media Pool model index is previewed");
        const auto red=capture();
        check(pixel(red,200,20)==QColor(Qt::red) && pixel(red,200,90)==QColor(30,130,180),"selected fill changes; adjacent clip retains its native color");
        check(pixel(red,15,15)==pixel(before,15,15) && pixel(red,1,1)==pixel(before,1,1),"thumbnail pixels and native selection highlight are preserved");
        check(writes==0 && index.data(Qt::UserRole)==nativeColor && fixture.model.rowCount()==2,"preview writes no model/color data and retains clip rows");
        fixture.view.edit(index);settle();auto editor=fixture.view.findChild<QLineEdit *>();
        check(editor!=nullptr,"native delegate still creates the real rename editor");
        if(editor){editor->setText("Renamed native clip");QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);QApplication::sendEvent(editor,&enter);settle();}
        check(index.data().toString()=="Renamed native clip" && source->commits==1,"native Return commits renaming exactly once through default and per-row delegates");
        check(index.data(Qt::UserRole)==nativeColor,"rename leaves the real native color unchanged");
        preview.clear(&fixture.root);check(fixture.view.itemDelegateForRow(0)==original,"clear restores the exact prior row delegate slot");
        for(double factor:{.7,1.47,3.}) {
            Unbound::MediaPoolContentScale scale;check(scale.apply(&fixture.root,factor),"native Media Pool scaling fixture uses its production viewport");settle();
            fixture.view.setCurrentIndex(index);preview.applySelected(&fixture.root,Qt::red);settle();
            const auto image=fixture.root.grab().toImage();int redPixels=0;
            for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)redPixels+=image.pixelColor(x,y)==QColor(Qt::red);
            check(redPixels>20,"Media Pool preview reaches actual scaled presentation pixels");preview.clear(&fixture.root);scale.restore();settle();
        }
        fixture.view.setCurrentIndex(index);preview.applySelected(&fixture.root,Qt::red);
        fixture.model.removeRow(0);settle();check(pixel(capture(),200,20)==QColor(30,130,180),"deleted persistent index never recolors the clip that takes its row");
        auto foreign=new NativeClipDelegate(&fixture.view);fixture.view.setItemDelegateForRow(0,foreign);preview.clear(&fixture.root);
        check(fixture.view.itemDelegateForRow(0)==foreign,"clearing a stale lease never removes a replacement native delegate");
    }
    std::printf("media pool red preview failures=%d\n",failures);return failures?1:0;
}
