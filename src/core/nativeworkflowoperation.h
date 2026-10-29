// SPDX-License-Identifier: MIT
#pragma once
#include "../platform/resolveintegration.h"
#include <QAction>
#include <QApplication>
#include "diagnostics.h"
#include <QFile>
#include <QSet>
namespace Unbound {
// Validates and invokes one native workflow action. Owns no selection or UI state.
class NativeWorkflowOperation {
public:
    static bool invoke(const QString &command, const QString &nativeLabel,
                       const QByteArray &routeMarker,
                       const std::function<bool(int,const QString &)> &fail) {
        QString directory=nativeWorkflowDirectory();
#ifdef UNBOUND_NATIVE_TESTS
        directory=qEnvironmentVariable("OPENGATE_WORKFLOW_SCRIPT_DIRECTORY",directory);
#endif
        if(directory.isEmpty())return fail(0,"Resolve's Workflow Integration directory is unavailable.");
        QFile script(directory+"/"+nativeLabel+".py");
        if(!script.open(QIODevice::ReadOnly) || !script.read(8192).contains(routeMarker))
            return fail(0,"The matching operation script must be installed with this helper; no legacy fallback was used.");
        QSet<QAction *> matches;
        for(auto widget:QApplication::allWidgets()) {
            auto actions=widget->actions();actions.append(widget->findChildren<QAction *>(QString(),Qt::FindDirectChildrenOnly));
            for(auto action:actions)
                if(action->objectName().startsWith("workspaceWIPlugins_") && label(action->text())==nativeLabel)matches.insert(action);
        }
        if(matches.isEmpty())return fail(0,"Resolve's native workflow command was not found.");
        if(matches.size()!=1)return fail(matches.size(),"Resolve's native workflow command is ambiguous.");
        if(!(*matches.begin())->isEnabled())
            return fail(1,"Resolve's native workflow command is disabled; the assigned shortcut was recognized, but the operation did not run.");
        auto action=*matches.begin();
        logEvent("command.edit-target.invoke",{{"command",command},{"object",action->objectName()},{"method","native-workflow-selected-timeline-api"}});
        action->trigger();
        // Completion comes from the API script, not from a Qt checked flag or
        // merely observing that QAction::trigger() returned.
        return true;
    }
private:
    static QString label(QString text) { text=text.section('\t',0,0); text.remove('&'); return text.trimmed(); }
};
}
