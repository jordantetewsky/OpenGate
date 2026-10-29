// SPDX-License-Identifier: MIT
#pragma once
#include <QLabel>
#include "nativepaneldecoration.h"
namespace Unbound {
// Timeline owns this control, independently of the detachable tools row. Group
// cells already provide their own Detach/header-drag surface.
class TimelineControl : public QObject {
public:
    explicit TimelineControl(QObject *owner) : QObject(owner) {}
    ~TimelineControl() override { release(); }
    void wire(QWidget *timeline,std::function<void()> toggle,std::function<void(QPoint)> drag) {
        if (!timeline) return;
        if (timeline->window()->objectName().startsWith("unboundFloating_")) { if (row) row->hide(); return; }
        if (!row || !timeline->isAncestorOf(row)) {
            release();
            auto layout=qobject_cast<QBoxLayout *>(timeline->layout());
            if (!layout || (layout->direction()!=QBoxLayout::TopToBottom && layout->count()!=1)) {
                logEvent("timeline.header.unavailable",{{"layout",timeline->layout() ? timeline->layout()->metaObject()->className() : "none"}});
                return;
            }
            originalLayout=layout; originalDirection=layout->direction();
            // With one native content item, orientation has no visual meaning.
            // Lease a vertical row and restore the original direction on teardown.
            if (layout->direction()!=QBoxLayout::TopToBottom) layout->setDirection(QBoxLayout::TopToBottom);
            row=new QWidget(timeline); row->setObjectName("unboundTimelineHeader");
            auto tools=new QHBoxLayout(row); tools->setContentsMargins(4,0,4,0); tools->setSpacing(4);
            tools->addWidget(new QLabel("Timeline",row)); tools->addStretch();
            button=new NativeUndockButton(row); button->setObjectName("unboundTimelineUndock");
            button->setIcon(QIcon(new PanelUndockGlyph)); button->setIconSize(QSize(18,18));
            button->setFixedSize(24,24); button->setAutoRaise(true); button->setFocusPolicy(Qt::NoFocus);
            button->setToolTip("Undock Timeline; drag to detach and move"); button->setAccessibleName(button->toolTip());
            tools->addWidget(button); layout->insertWidget(0,row);
            connect(button,&QToolButton::clicked,this,[this] {
                logEvent("timeline.header.undock",{{"origin","timeline-header"}});
                QTimer::singleShot(0,this,[this] { if (invoke) invoke(); });
            });
        }
        invoke=std::move(toggle); button->drag=std::move(drag);
        bool grouped=false;
        for (auto parent=timeline;parent;parent=parent->parentWidget())
            grouped |= parent->objectName()=="unboundGroupCell_frameConformTimelines";
        row->setVisible(!grouped && !timeline->window()->objectName().startsWith("unboundFloating_"));
    }
private:
    void release() {
        delete row.data(); row=nullptr;
        if (originalLayout && originalLayout->direction()!=originalDirection) originalLayout->setDirection(originalDirection);
        originalLayout=nullptr;
    }
    QPointer<QBoxLayout> originalLayout;
    QBoxLayout::Direction originalDirection=QBoxLayout::TopToBottom;
    QPointer<QWidget> row;
    QPointer<NativeUndockButton> button;
    std::function<void()> invoke;
};
}
