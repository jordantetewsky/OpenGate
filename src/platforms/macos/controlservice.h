// SPDX-License-Identifier: MIT
#pragma once
#include "../../core/nativeaudioactionexperiment.h"
#include <QFileSystemWatcher>
#include <QSaveFile>
#include "panelcontrols.h"
#include "crashsession.h"
#include "interactiondiagnostics.h"
#include "activationbridge.h"
#include "responsiveness.h"
#include "surfacetrace.h"
#include "clipcolorprobe.h"
#include "keyboardprobe.h"
#include "boundarydiagnostics.h"
#include "pagepanelinventory.h"
#include "workflowcommandbridge.h"
#include "workflowcommandresults.h"
#include "timelinetabscrolling.h"
#include "../../core/controllaunchrequests.h"

namespace Unbound {
class ControlService : public QObject
{
public:
    explicit ControlService(QObject *parent) : QObject(parent)
    {
        setObjectName(QStringLiteral("unboundControlService"));
        journal=new CrashSession(this);
        const auto permit=qEnvironmentVariable("OPENGATE_AUDIO_ACTION_PERMIT");
        experiment=permit.isEmpty()?nullptr:new Unbound::NativeAudioActionExperiment(this,permit);
        auto diagnostics=new InteractionDiagnostics(this);
        QPointer<Unbound::NativeAudioActionExperiment> retained=experiment;
        if(retained)diagnostics->audioExperimentObserver=[retained](QObject *object,QEvent *event){if(retained)retained->observe(object,event);};
        new ResolveActivationBridge(this);
        new ResponsivenessProbe(this);
        new SurfaceTrace(this);
        new ClipColorProbe(this);
        new KeyboardProbe(this);
        const auto tabScrolling=qEnvironmentVariable("OPENGATE_TIMELINE_TAB_SCROLLING");
        if(tabScrolling.isEmpty() || tabScrolling=="1" || tabScrolling=="smooth")new TimelineTabScrolling(this);
        new BoundaryDiagnostics(this);
        new PagePanelInventory(this);
        directory = qEnvironmentVariable("UNBOUND_LAUNCH_DIRECTORY",
            QDir::homePath() + QStringLiteral("/Library/Logs/OpenGate"));
        launches=new ControlLaunchRequests(this,directory,[this]{showControls();});
        #ifdef UNBOUND_FEATURE_MANIFEST
        logEvent("helper.feature-manifest", {{"manifest", QJsonDocument::fromJson(QByteArray(UNBOUND_FEATURE_MANIFEST)).object()}});
#endif
        logEvent(QStringLiteral("helper.start"), {{"executable", QCoreApplication::applicationFilePath()}});
        // Initialize menus, native decorations and saved workspace restoration
        // without putting a second UI in front of Resolve on every launch.
        controls = new PanelControls;
        if(retained)controls->audioExperiment=[retained](int direction,bool skip){return retained && retained->invoke(direction,skip);};
        logEvent("controls.ready",{{"visible",controls->isVisible()},{"startup","native-workspace"}});
        new WorkflowCommandBridge(this,directory,[this](QString id) {
            return controls && controls->invokeWorkflowCommand(id);
        });
        new WorkflowCommandResults(this,directory);
        launches->consume(true);
        connect(qApp,&QCoreApplication::aboutToQuit,this,[this] { journal->markClean(); });
    }
    ~ControlService() override { delete controls.data(); }
    void showControls()
    {
        if (!controls) {
            controls = new PanelControls;
            QPointer<NativeAudioActionExperiment> retained=experiment;
            if(retained)controls->audioExperiment=[retained](int direction,bool skip){return retained && retained->invoke(direction,skip);};
        }
        controls->showNormal();
        keepOnDisplay(controls);
        controls->raise();
        controls->activateWindow();
        logEvent(QStringLiteral("controls.show"));
    }
private:
    NativeAudioActionExperiment *experiment=nullptr;
    CrashSession *journal=nullptr;
    ControlLaunchRequests *launches=nullptr;
    QPointer<PanelControls> controls;
    QString directory;
};
}
