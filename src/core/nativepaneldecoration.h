// SPDX-License-Identifier: MIT
#pragma once
#include <QAbstractButton>
#include <QBoxLayout>
#include <QHash>
#include <QIconEngine>
#include <QPainter>
#include <QPointer>
#include <QMouseEvent>
#include <QToolButton>
#include <functional>
#include "diagnostics.h"
#include "panelplacementcontrol.h"
#include <QApplication>
#include "nativeplacementcaller.h"
namespace Unbound {
// Dedicated native-header button; never replaces or overlays Resolve's options.
using PanelUndockGlyph = PanelPlacementGlyph;
class NativeUndockButton : public QToolButton {
public:
    using QToolButton::QToolButton;
    std::function<void(QPoint)> drag;
protected:
    void mousePressEvent(QMouseEvent *event) override {
        armed=event->button()==Qt::LeftButton; moved=false; press=event->globalPos();
        QToolButton::mousePressEvent(event);
    }
    void mouseMoveEvent(QMouseEvent *event) override {
        if (armed && drag && event->buttons().testFlag(Qt::LeftButton) &&
            (event->globalPos()-press).manhattanLength()>=QApplication::startDragDistance()) {
            armed=false; moved=true; setDown(false);
            if (QWidget::mouseGrabber()==this) releaseMouse();
            drag(event->globalPos()); event->accept(); return;
        }
        if (!moved) QToolButton::mouseMoveEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent *event) override {
        armed=false;
        if (moved) { setDown(false); event->accept(); return; }
        QToolButton::mouseReleaseEvent(event);
    }
private:
    QPoint press; bool armed=false,moved=false;
};
class NativePanelDecoration : public QObject {
public:
    explicit NativePanelDecoration(QObject *owner) : QObject(owner) {}
    ~NativePanelDecoration() override {
        for (const auto &button : buttons) delete button.data();
    }
    void wire(QWidget *panel,const QString &id,const QString &title,bool floating,std::function<void()> toggle,std::function<void(QPoint)> drag = {}) {
        if (!panel) return;
        // Edit Keyframes already supplies a placement control. Route it through
        // the same controller as our other panel headers, keeping its native row.
        if(id=="frameEditPageKeyframeEditor")if(auto native=panel->findChild<QAbstractButton *>("btnUndock")) {
            auto caller=nativeCallers.value(native);
            if(!caller){caller=new NativePlacementCaller(native,this);nativeCallers[native]=caller;
                connect(native,&QObject::destroyed,this,[this,native]{delete nativeCallers.take(native);});}
            caller->activate=std::move(toggle);caller->drag=std::move(drag);return;
        }
        auto button = buttons.value(id);
        bool grouped=false;
        for(auto parent=panel;parent&&!grouped;parent=parent->parentWidget())grouped=parent->objectName()=="unboundGroupCell_"+id;
        if(grouped){if(button)button->hide();return;}
        if (!button) {
            const QStringList anchors{"btnMediaPoolOptions","btnTitleOptions","btnAudioMixerOptions", "btnOptions",
                "m_BtnDisplayOptions","btnEdlOptions","tbtnPlayerOptions","metadataSelectionMenuBtn","btnCurveSettingsOptions"};
            QAbstractButton *anchor = nullptr; QBoxLayout *layout = nullptr;
            for (auto candidate : panel->findChildren<QAbstractButton *>()) {
                if (!anchors.contains(candidate->objectName())) continue;
                auto row = qobject_cast<QBoxLayout *>(candidate->parentWidget()->layout());
                if (row && row->direction() == QBoxLayout::LeftToRight && row->indexOf(candidate) >= 0) { anchor=candidate; layout=row; break; }
            }
            if (!anchor) return; // No fallback strip or guessed overlay over native input.
            button = new NativeUndockButton(anchor->parentWidget()); button->setObjectName("unboundNativeUndock_"+id);
            button->setIcon(QIcon(new PanelUndockGlyph)); button->setIconSize(QSize(20,20)); button->setFixedSize(24,24);
            button->setAutoRaise(true); button->setFocusPolicy(Qt::NoFocus);
            button->setProperty("unboundPanelId",id);
            layout->insertWidget(layout->indexOf(anchor),button,0,Qt::AlignVCenter);
            connect(button,&QToolButton::clicked,this,[id,toggle] {
                logEvent("panel.header.toggle",{{"panel",id},{"origin","native-header"}}); toggle();
            });
            buttons[id]=button;
        }
        button->drag=std::move(drag);
        setPlacementControl(button,floating,(floating ? "Attach " : "Float ")+title+(button->drag ? "; drag to place" : ""));
        button->setAccessibleName(button->toolTip()); button->show();
    }
private:
    QHash<QString,QPointer<NativeUndockButton>> buttons;
    QHash<QAbstractButton *,NativePlacementCaller *> nativeCallers;
};
}
