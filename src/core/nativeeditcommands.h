// SPDX-License-Identifier: MIT
#pragma once
#include "../platform/resolveintegration.h"
#include <QAbstractButton>
#include <QActionGroup>
#include <QPointer>
#include <QMenu>
#include <QSet>
#include "panelcommands.h"
#include "nativeeditmenutarget.h"
#include "editcontrolinspection.h"
#include "nativeworkflowoperation.h"
namespace Unbound {
// Dispatch the actual native action/signal; never hardcode an internal address,
// mouse coordinate, keyboard mapping or a second implementation of stabilization.
class NativeEditCommands : public QObject {
public:
    NativeEditCommands(QObject *owner,std::function<QWidget *()> page) : QObject(owner),editPage(std::move(page)) {}
    std::function<void(QString)> report;
    std::function<bool()> editActive;
    std::function<bool(int,bool)> audioExperiment;
    void addTo(PanelCommands *commands) {
        commands->add({"edit.timeline.fixed-playhead","Edit Timeline / Toggle Fixed Playhead",[this] { return ready(); },[this] { fixedPlayhead(); }});
        commands->add({"edit.inspector.stabilize","Edit Inspector / Stabilize",[this] { return ready(); },[this] { stabilize(); }});
        commands->add({"edit.audio.previous-channel","Previous Audio Channel — Skip Excluded Channels",[this] { return ready(); },[this] { audioChannel(-1); }});
        commands->add({"edit.audio.next-channel","Next Audio Channel — Skip Excluded Channels",[this] { return ready(); },[this] { audioChannel(1); }});
        commands->add({"edit.audio.previous-channel-all-source","Previous Audio Channel — All Source Channels",[this] { return ready(); },[this] { audioChannel(-1,false); }});
        commands->add({"edit.audio.next-channel-all-source","Next Audio Channel — All Source Channels",[this] { return ready(); },[this] { audioChannel(1,false); }});
    }
    // Edit-page presentation command: no clip-selection or focused-panel requirement.
    bool fixedPlayhead() {
        if(!ready())return unavailable("fixed-playhead",0,"Fixed Playhead requires the Edit page.");
        // Native UiMenuButton prepares its menu through this signal before
        // displaying it. Invoke only that preparation phase: no click, popup,
        // timeline focus, native key mapping or visible-panel prerequisite.
        QList<QPointer<QWidget>> buttons,editButtons;
        for(auto widget:QApplication::allWidgets())
            if(widget->objectName()=="btnEditToolbarPreferences") {
                buttons.append(widget);
                // Page and timeline ownership survive hiding and reparenting
                // into a floating group; visibility/focus must not pick a mode.
                for(auto parent=widget->parentWidget();parent;parent=parent->parentWidget())
                    if(parent==(editPage?editPage():nullptr) || parent->objectName()=="frameConformTimelines") {
                        editButtons.append(widget);break;
                    }
            }
        if(!editButtons.isEmpty())buttons=editButtons;
        if(buttons.size()>1)return unavailable("fixed-playhead",buttons.size(),"Edit timeline settings owner is ambiguous.");
        if(buttons.size()==1 && buttons.front()) {
            auto button=buttons.front();
            if(button->metaObject()->indexOfSignal("MenuAboutToShow()")>=0)
                QMetaObject::invokeMethod(button,"MenuAboutToShow",Qt::DirectConnection);
        }
        QSet<QAction *> matches;
        // Prefer the native Timeline Options row that owns the actual setting.
        // A different View action's Qt checked flag changed without the
        // custom-painted popup's mode changing; it is not acceptance evidence.
        if(ready())for(auto widget:QApplication::allWidgets())if(auto menu=qobject_cast<QMenu *>(widget)) {
            if(!nativeTimelineOptionsMenu(menu))continue;
            if(!editButtons.isEmpty() && menu->parentWidget()!=buttons.front())continue;
            for(auto action:menu->actions())if(nativeEditActionLabel(action,"Fixed Playhead",true))matches.insert(action);
        }
        if (matches.size()!=1 || !(*matches.begin())->isEnabled()) return unavailable("fixed-playhead",matches.size(),"Edit's native Fixed Playhead action is unavailable or ambiguous; no scrolling state was changed.");
        auto action=*matches.begin();
        logEvent("command.edit-target.invoke",{{"command","fixed-playhead"},{"object",action->objectName()},
            {"checkable",action->isCheckable()},{"checked_before",action->isChecked()}});
        QAction *target=action;
        QPointer<QAction> retained(action);const auto targetName=target->objectName();
        target->trigger();
        logEvent("command.edit-target.result",{{"command","fixed-playhead"},{"success",true},
            {"checked_after",retained ? retained->isChecked():false},{"action_retained",bool(retained)},{"native_target",targetName},{"completion","native-action-triggered"},{"native_mode_verified",false}});
        return true;
    }
    bool audioChannel(int direction,bool skipExcluded=true) {
        if(direction!=-1 && direction!=1)return unavailable("audio-channel",0,"Invalid channel direction.");
        if(!ready())return unavailable("audio-channel",0,"Audio channel cycling requires the Edit page.");
        if(audioExperiment && audioExperiment(direction,skipExcluded))return true;
        return unavailable("audio-channel",0,"Audio channel execution is unarmed or its diagnostic safety checks refused the request; no action was invoked.");
    }
    bool stabilize() {
        if(!ready())return unavailable("stabilize",0,"Stabilize requires the Edit page.");
        return NativeWorkflowOperation::invoke("stabilize","OpenGate - Edit - Stabilize",
            "# opengate-command-route: selected-timeline-api-v1",
            [this](int matches,const QString &message) { return unavailable("stabilize",matches,message); });
    }
private:
    std::function<QWidget *()> editPage;
    bool ready() const { if(editActive)return editActive(); auto page=editPage ? editPage() : nullptr; return page && page->isVisible(); }
    bool unavailable(const QString &command,int matches,const QString &message) {
        logEvent("command.edit-target.unavailable",{{"command",command},{"matches",matches},{"reason",message}});
        logEvent("command.edit-target.result",{{"command",command},{"success",false},{"reason",message}});
        if (report) report(message); return false;
    }
};
}
