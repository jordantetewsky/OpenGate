// SPDX-License-Identifier: MIT
#pragma once
#include <QAbstractButton>
#include <QHBoxLayout>
#include <QMenu>
#include <QPointer>
#include <QSignalBlocker>
#include <QTimer>
#include <QToolButton>
#include "diagnostics.h"
#include "panelplacementcontrol.h"
#include "nativepaneldecoration.h"
#include "../platform/workspaceuiconventions.h"
namespace Unbound {
class DoorGlyph : public QIconEngine {
public:
    explicit DoorGlyph(QString id):id(std::move(id)){}
    QIconEngine *clone()const override{return new DoorGlyph(id);}
    void paint(QPainter *p,const QRect &r,QIcon::Mode mode,QIcon::State)override{
        p->save();p->setRenderHint(QPainter::Antialiasing);p->translate(r.topLeft());p->scale(r.width()/20.,r.height()/20.);
        p->setPen(QPen(mode==QIcon::Active?QColor(235,235,235):QColor(175,175,175),1.5));
        if(id.contains("Timeline")){p->drawRect(QRectF(2,3,16,14));p->drawLine(2,7,18,7);p->drawLine(6,7,6,17);p->drawLine(8,10,15,10);p->drawLine(8,14,12,14);}
        else if(id.contains("navigation",Qt::CaseInsensitive)){for(int x:{3,8,13})p->drawRect(QRectF(x,7,3,6));}
        else {p->drawRect(QRectF(2,3,16,12));p->drawLine(7,18,13,18);p->drawLine(10,15,10,18);
            if(id.contains("Source")){p->drawLine(6,7,10,7);p->drawLine(6,7,6,11);}else{p->drawLine(10,7,14,9);p->drawLine(14,9,10,11);p->drawLine(10,11,10,7);}}
        p->restore();
    }
    QPixmap pixmap(const QSize &s,QIcon::Mode m,QIcon::State state)override{QPixmap out(s);out.fill(Qt::transparent);QPainter p(&out);paint(&p,QRect(QPoint(),s),m,state);return out;}
private:QString id;
};
// Only OpenGate's accessory container is constrained. Native controls, title,
// layout slots, size policies and ownership are never modified.
class ToolbarDoorStrip : public QWidget {
public:
    explicit ToolbarDoorStrip(QWidget *parent):QWidget(parent) {}
    // Accessory labels must never enlarge the native row into the centered
    // project title. Consume only space allocated after the native controls.
    QSize sizeHint() const override {return QSize(0,QWidget::sizeHint().height());}
    QSize minimumSizeHint() const override {return QSize(0,QWidget::minimumSizeHint().height());}
};
class ToolbarDoors : public QObject {
public:
    struct Door{QString id,title;std::function<bool()> shown;std::function<void()> toggle;std::function<void(QPoint)> drag={};};
    explicit ToolbarDoors(QObject *owner):QObject(owner){timer.setSingleShot(true);timer.setInterval(50);connect(&timer,&QTimer::timeout,this,[this]{fit();});qApp->installEventFilter(this);}
    ~ToolbarDoors()override{qApp->removeEventFilter(this);for(auto strip:strips)if(strip)delete strip;}
    void add(Door door){doors.append(std::move(door));}
    void wire(QWidget *surface){
        if(!surface)return;
        for(auto row:surface->findChildren<QWidget *>())if(row->objectName().startsWith("frameEditTopBarLeft")&&row->layout()){
            QWidget *strip=nullptr;
            for(auto existing:strips)if(existing && existing->property("openGateNativeToolbarRow").value<QObject *>()==row){strip=existing;break;}
            if(!strip){
                strip=new ToolbarDoorStrip(row);strip->setProperty("openGateNativeToolbarRow",QVariant::fromValue<QObject *>(row));strip->setObjectName("openGateToolbarDoors");strip->setMinimumWidth(0);strip->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
                auto slot=new QHBoxLayout(strip);slot->setSizeConstraint(QLayout::SetNoConstraint);slot->setContentsMargins(0,0,0,0);slot->setSpacing(0);
                auto controls=new QWidget(strip);controls->setObjectName("openGateDoorControls");controls->setMinimumWidth(0);controls->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
                auto layout=new QHBoxLayout(controls);layout->setContentsMargins(0,0,0,0);layout->setSpacing(2);slot->addWidget(controls);slot->addStretch();
                for(const auto &door:doors){
                    auto button=new NativeUndockButton(controls);button->drag=door.drag;button->setObjectName("openGateToolbarDoor_"+door.id);button->setText(door.title);
                    button->setIcon(QIcon(new DoorGlyph(door.id)));button->setIconSize(QSize(18,18));button->setAutoRaise(true);button->setCheckable(true);button->setFocusPolicy(Qt::NoFocus);
                    button->setToolTip("Show or hide "+door.title+" in its remembered location"+(door.drag?QStringLiteral("; drag to move"):QString()));button->setAccessibleName(door.title);layout->addWidget(button);
                    connect(button,&QToolButton::clicked,this,[this,door]{QTimer::singleShot(0,this,[door]{logEvent("toolbar.panel-door",{{"panel",door.id}});if(door.toggle)door.toggle();});});
                }
                auto more=new QToolButton(controls);more->setObjectName("openGateToolbarOverflow");more->setText(QString::fromUtf8("⋯"));more->setToolTip("More OpenGate panels");more->setAccessibleName(more->toolTip());more->setAutoRaise(true);more->setPopupMode(QToolButton::InstantPopup);
                auto menu=new QMenu(more);menu->setProperty("unboundOwnMenu",true);more->setMenu(menu);
                for(const auto &door:doors){auto action=menu->addAction(door.title);action->setCheckable(true);connect(action,&QAction::triggered,this,[this,door]{QTimer::singleShot(0,this,[door]{if(door.toggle)door.toggle();});});}
                layout->addWidget(more);
                // Resolve bounds its native left button row to the native buttons.
                // Reuse the adjacent slot in that same toolbar's box layout;
                // putting our callers inside the bounded row gives them zero space.
                auto parentRow=row->parentWidget();auto box=parentRow?qobject_cast<QBoxLayout *>(parentRow->layout()):nullptr;
                if(box && box->indexOf(row)>=0 && (box->direction()==QBoxLayout::LeftToRight || box->direction()==QBoxLayout::RightToLeft))
                    box->insertWidget(box->indexOf(row)+1,strip,1);
                else row->layout()->addWidget(strip);
                strips.append(strip);
            }
            for(int i=0;i<doors.size();++i){const auto &door=doors[i];auto button=strip->findChild<QToolButton *>("openGateToolbarDoor_"+door.id);QSignalBlocker block(button);button->setChecked(door.shown&&door.shown());
                auto more=strip->findChild<QToolButton *>("openGateToolbarOverflow");if(i<more->menu()->actions().size())more->menu()->actions()[i]->setChecked(button->isChecked());}
        }
        schedule();
    }
protected:
    bool eventFilter(QObject *object,QEvent *event)override{
        if(event->type()==QEvent::Resize||event->type()==QEvent::LayoutRequest||event->type()==QEvent::Show||event->type()==QEvent::FontChange||event->type()==QEvent::Move||event->type()==QEvent::Hide){
            auto widget=qobject_cast<QWidget *>(object);
            if(widget)for(auto strip:strips)if(strip&&(widget==strip->parentWidget()||widget==strip->window()||widget->objectName()=="btnProjectsMainBarProjectList")){schedule();break;}
        }return false;
    }
private:
    void schedule(){if(!timer.isActive())timer.start();}
    void fit(){for(auto strip:strips)if(strip){
        auto row=strip->parentWidget();int start=0;
        for(int i=0;i<row->layout()->count();++i){auto item=row->layout()->itemAt(i);auto widget=item->widget();if(widget&&widget!=strip&&widget->isVisible()&&widget->geometry().right()<strip->geometry().left())start=qMax(start,widget->geometry().right()+1+row->layout()->spacing());}
        // Use the accessory's actual slot, not the ideal position inferred
        // from sibling hints. Native stretch/alignment can leave a later origin.
        start=qMax(start,strip->geometry().left());
        int available=qMax(0,qMin(strip->width(),row->width()-start));
        for(auto title:row->window()->findChildren<QAbstractButton *>("btnProjectsMainBarProjectList"))if(title->isVisible()){
            const QRect titleRect(title->mapToGlobal(QPoint()),title->size());
            const QRect rowRect(row->mapToGlobal(QPoint()),row->size());
            if(titleRect.bottom()<rowRect.top() || titleRect.top()>rowRect.bottom())continue;
            const int origin=row->mapToGlobal(QPoint(start,0)).x();available=qMin(available,qMax(0,titleRect.left()-origin-nativeWorkspaceTitleGap(row)));
        }
        const auto buttons=strip->findChildren<QToolButton *>();
        int labeled=0;for(auto button:buttons)if(button->objectName()!="openGateToolbarOverflow")labeled+=button->fontMetrics().horizontalAdvance(button->text())+30;
        const int icons=doors.size()*28+qMax(0,doors.size()-1)*2;
        const QString old=strip->property("openGateDoorMode").toString();
        const QString mode=available>=labeled+(old=="labels"?0:24)?"labels":available>=icons+(old=="icons"?0:12)?"icons":"overflow";
        const int width=mode=="labels"?labeled:mode=="icons"?icons:28;
        auto controls=strip->findChild<QWidget *>("openGateDoorControls");controls->setFixedWidth(qMin(qMin(available,strip->width()),width));controls->setVisible(available>=28);
        for(auto button:buttons){const bool more=button->objectName()=="openGateToolbarOverflow";button->setVisible(more?mode=="overflow":mode!="overflow");if(!more)button->setToolButtonStyle(mode=="labels"?Qt::ToolButtonTextBesideIcon:Qt::ToolButtonIconOnly);}
        if(old!=mode || strip->property("openGateDoorAvailable").toInt()!=available){
            strip->setProperty("openGateDoorMode",mode);strip->setProperty("openGateDoorAvailable",available);
            logEvent("header.controls.mode",{{"mode",mode},{"available",available},{"slot_width",strip->width()},
                {"slot_left",strip->geometry().left()},{"row_width",row->width()},{"start",start},
                {"controls_width",controls->width()},{"controls_visible",controls->isVisible()}});
        }
    }}
    QList<Door> doors;QList<QPointer<QWidget>> strips;QTimer timer;
};
}
