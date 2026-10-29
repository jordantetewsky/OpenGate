// SPDX-License-Identifier: MIT
#pragma once
#include "diagnostics.h"
#include "nativepanelposition.h"
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>

namespace Unbound {
// Attach real native content to an owner's layout while preserving its original
// slot. Qt parentage makes it travel with the owner; no geometry-follow timer.
class PanelAccessoryLease : public QObject {
public:
    explicit PanelAccessoryLease(QObject *owner) : QObject(owner) {}
    ~PanelAccessoryLease() override { release(); }
    bool active() const { return position.active() && viewport; }
    QWidget *ownerSurface() const { return destination; }
    QRect visibleBounds() const {
        return viewport && viewport->isVisible()?QRect(viewport->mapToGlobal(QPoint()),viewport->size()):QRect();
    }
    QString attach(QWidget *accessory,QWidget *nativeRoot,QWidget *owner, Qt::Edge edge=Qt::BottomEdge,bool resizeContent=false,bool allowParked=false)
    {
        if (active()) return destination==owner ? QString() : QStringLiteral("Detach the accessory before changing its owner.");
        if (viewport || position.content()) return QStringLiteral("The previous accessory position changed; restoration is required.");
        auto layout=owner ? qobject_cast<QBoxLayout *>(owner->layout()) : nullptr;
        if ((edge!=Qt::TopEdge && edge!=Qt::BottomEdge) || !layout || layout->direction()!=QBoxLayout::TopToBottom || !accessory || accessory==owner || accessory->isAncestorOf(owner))
            return QStringLiteral("This panel does not have a supported reserved-band layout.");
        logEvent("accessory.attach.request",{{"panel",accessory->objectName()},{"owner",owner->objectName()},{"edge",edge==Qt::TopEdge?"top":"bottom"}});
        // QStackedWidget::sizeHint includes inactive native pages, some of
        // which still have construction-sized geometry. Size the visible row.
        auto row=accessory;
        if(auto stack=qobject_cast<QStackedWidget *>(accessory))if(stack->currentWidget())row=stack->currentWidget();
        const int height=qMax(32,row->sizeHint().height());
        const auto error=position.acquire(accessory,nativeRoot,allowParked);
        if (!error.isEmpty()) return error;
        content=accessory; destination=owner; destinationLayout=layout;
        originalFill=accessory->autoFillBackground();
        position.surface()->hide();
        viewport=new QScrollArea(owner); viewport->setObjectName("unboundAttachedAccessory_"+accessory->objectName());
        viewport->setFrameShape(QFrame::NoFrame); viewport->setWidgetResizable(resizeContent);
        viewport->setMinimumWidth(0); viewport->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        viewport->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        viewport->setWidget(accessory); accessory->show(); if(edge==Qt::TopEdge)layout->insertWidget(0,viewport,0);else layout->addWidget(viewport,0);
        rowHeight=height;
        connect(viewport->horizontalScrollBar(),&QScrollBar::rangeChanged,this,[this](int,int) { updateHeight(); });
        ownerDestroyed=connect(owner,&QObject::destroyed,this,[this] { releaseImpl(true); });
        updateHeight(); viewport->show();
        logEvent("accessory.attach",{{"panel",accessory->objectName()},{"owner",owner->objectName()},{"edge",edge==Qt::TopEdge?"top":"bottom"}});
        return {};
    }
    bool release() { return releaseImpl(false); }
private:
    bool releaseImpl(bool ownerDying)
    {
        // If the native slot rejected a previous restore, retain the content in
        // its original parking owner and allow an explicit safe retry.
        if (!viewport) {
            if (!position.content()) return true;
            if (!content || content!=position.content() || content->parentWidget()!=position.contentParking())return false;
            return finishRestore();
        }
        if (!content || viewport->widget()!=content || content->parentWidget()!=viewport->viewport() ||
            (!ownerDying && (!destination || viewport->parentWidget()!=destination || !destinationLayout || destinationLayout->indexOf(viewport)<0))) {
            logEvent("accessory.release.rejected",{{"reason","attachment-ownership-changed"}}); return false;
        }
        logEvent("accessory.release.request",{{"panel",content->objectName()},{"owner",destination?destination->objectName():QString()},{"owner_dying",ownerDying}});
        disconnect(ownerDestroyed);
        // Qt5 QScrollArea::takeWidget calls setParent(nullptr), which creates a
        // temporary top-level native window. Move directly between retained
        // parents instead. Destroy the wrapper before restoring native geometry;
        // it must no longer observe or lay out the borrowed native widget.
        content->removeEventFilter(viewport);
        content->hide();content->setParent(position.contentParking());
        if (destinationLayout) destinationLayout->removeWidget(viewport);
        delete viewport.data();viewport=nullptr;
        return finishRestore();
    }
    bool finishRestore()
    {
        QPointer<QWidget> widget=content;
        if(!position.restore())return false;
        if(widget)widget->setAutoFillBackground(originalFill);
        destination=nullptr;destinationLayout=nullptr;content=nullptr;
        logEvent("accessory.release",{{"panel",widget?widget->objectName():QString()}});
        return true;
    }

public:
    void setHidden(bool hidden) { if (viewport) viewport->setVisible(!hidden); }
    bool hidden() const { return viewport && viewport->isHidden(); }
private:
    void updateHeight()
    {
        if (!viewport) return;
        const auto bar=viewport->horizontalScrollBar();
        viewport->setFixedHeight(rowHeight+(bar->maximum()>bar->minimum() ? bar->sizeHint().height() : 0));
    }
    NativePanelPosition position;
    QPointer<QWidget> content,destination;
    QPointer<QBoxLayout> destinationLayout;
    QPointer<QScrollArea> viewport;
    QMetaObject::Connection ownerDestroyed;
    bool originalFill=false;
    int rowHeight=32;
};
}
