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
namespace Unbound {
// Dispatch the actual native action/signal; never hardcode an internal address,
// mouse coordinate, keyboard mapping or a second implementation of stabilization.
class NativeEditCommands : public QObject {
public:
    NativeEditCommands(QObject *owner,std::function<QWidget *()> page) : QObject(owner),editPage(std::move(page)) {}
    std::function<void(QString)> report;
    std::function<bool()> editActive;
    void addTo(PanelCommands *commands) {
        commands->add({"edit.timeline.fixed-playhead","Edit Timeline / Toggle Fixed Playhead",[this] { return ready(); },[this] { fixedPlayhead(); },"frameConformTimelines"});
        commands->add({"edit.inspector.stabilize","Edit Inspector / Stabilize",[this] { return ready(); },[this] { stabilize(); },"frameConformTimelines"});
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
    bool stabilize() {
        if(!ready())return unavailable("stabilize",0,"Stabilize requires the Edit page.");
        QString directory=nativeWorkflowDirectory();
#ifdef UNBOUND_NATIVE_TESTS
        directory=qEnvironmentVariable("OPENGATE_WORKFLOW_SCRIPT_DIRECTORY",directory);
#endif
        if(directory.isEmpty())return unavailable("stabilize",0,"Resolve's Workflow Integration directory is unavailable.");
        QFile script(directory+"/OpenGate - Edit - Stabilize.py");
        if(!script.open(QIODevice::ReadOnly) || !script.read(8192).contains("# opengate-command-route: selected-timeline-api-v1"))
            return unavailable("stabilize",0,"The matching selected-clip Stabilize script must be installed with this helper; no legacy button fallback was used.");
        QSet<QAction *> matches;
        for(auto widget:QApplication::allWidgets()) {
            auto actions=widget->actions();actions.append(widget->findChildren<QAction *>(QString(),Qt::FindDirectChildrenOnly));
            for(auto action:actions)
                if(action->objectName().startsWith("workspaceWIPlugins_") && label(action->text())=="OpenGate - Edit - Stabilize")matches.insert(action);
        }
        if(matches.size()!=1 || !(*matches.begin())->isEnabled())
            return unavailable("stabilize",matches.size(),"Resolve's native OpenGate Stabilize workflow command is unavailable or ambiguous.");
        auto action=*matches.begin();
        logEvent("command.edit-target.invoke",{{"command","stabilize"},{"object",action->objectName()},{"method","native-workflow-selected-timeline-api"}});
        action->trigger();
        // Completion comes from the API script, not from a Qt checked flag or
        // merely observing that QAction::trigger() returned.
        return true;
    }
private:
    std::function<QWidget *()> editPage;
    bool ready() const { if(editActive)return editActive(); auto page=editPage ? editPage() : nullptr; return page && page->isVisible(); }
    static QString label(QString text) { text=text.section('\t',0,0); text.remove('&'); return text.trimmed(); }
    bool unavailable(const QString &command,int matches,const QString &message) {
        logEvent("command.edit-target.unavailable",{{"command",command},{"matches",matches},{"reason",message}});
        logEvent("command.edit-target.result",{{"command",command},{"success",false},{"reason",message}});
        if (report) report(message); return false;
    }
};
}
