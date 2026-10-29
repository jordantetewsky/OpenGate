// SPDX-License-Identifier: MIT
#pragma once
#include "nativepanelposition.h"
#include "paneloverflow.h"
namespace Unbound {
// Borrow only the tools row. Resolve retains its Timeline viewport and its own
// scrolling/painting; there is never a second scroll area around the tracks.
class TimelineToolbarViewport {
public:
    ~TimelineToolbarViewport(){restore();}
    void attach(QWidget *timeline) {
        if(position.active() || !timeline || timeline->objectName()!="frameConformTimelines")return;
        auto toolbar=timeline->findChild<QWidget *>("frameEditToolbarTop");
        if(!toolbar)return;
        const int height=qMax(24,qMin(120,toolbar->height()));
        const auto error=position.acquire(toolbar,timeline);
        if(!error.isEmpty()){logEvent("timeline.toolbar.overflow.unavailable",{{"reason",error}});return;}
        auto slot=position.surface();auto layout=new QVBoxLayout(slot);layout->setContentsMargins(0,0,0,0);
        auto area=qobject_cast<QScrollArea *>(overflow.wrap(toolbar,slot,true));layout->addWidget(area);
        QPointer<QWidget> alive=slot;QPointer<QScrollArea> viewport=area;
        auto fit=[alive,viewport,height]{if(alive&&viewport){auto bar=viewport->horizontalScrollBar();alive->setFixedHeight(height+(bar->maximum()>bar->minimum()?bar->sizeHint().height():0));}};
        QObject::connect(area->horizontalScrollBar(),&QScrollBar::rangeChanged,slot,fit);fit();
        toolbar->show();slot->show();
        logEvent("timeline.toolbar.overflow.attached",{{"body_wrapped",false}});
    }
    bool restore() {
        if(!position.active())return true;
        if(!overflow.owns(position.content()))return false;
        auto slot=position.surface();overflow.releaseInto(position.contentParking());
        delete slot->layout();return position.restore();
    }
private:NativePanelPosition position;PanelOverflow overflow;
};
}
