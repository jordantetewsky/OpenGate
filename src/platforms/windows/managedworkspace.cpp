// SPDX-License-Identifier: MIT
#include "../../core/nativeaudioactionexperiment.h"
#include <QApplication>
#include <QPointer>
#include <atomic>
#include "../../core/controller.h"
#include "../../core/panelcontrols.h"
#include "../../core/crashsession.h"
#include "../../core/interactiondiagnostics.h"
#include "../../core/surfacetrace.h"
#include "../../core/pagepanelinventory.h"
#include "../../core/workflowcommandbridge.h"
#include "../../core/workflowcommandresults.h"
#include "../../core/controllaunchrequests.h"
#include "../../core/bundledpresetseed.h"
namespace {
// Windows bootstrap/lifetime only. The accepted shared PanelControls owns all
// layout, panel commands, native return and persistence; no second controller.
class ManagedWorkspaceService : public QObject {
public:
    explicit ManagedWorkspaceService(QObject *app):QObject(app) {
        setObjectName("openGateManagedWorkspaceService");
        journal=new Unbound::CrashSession(this);
        const auto permit=qEnvironmentVariable("OPENGATE_AUDIO_ACTION_PERMIT");
        auto experiment=permit.isEmpty()?nullptr:new Unbound::NativeAudioActionExperiment(this,permit);
        auto diagnostics=new Unbound::InteractionDiagnostics(this);
        QPointer<Unbound::NativeAudioActionExperiment> retained=experiment;
        if(retained)diagnostics->audioExperimentObserver=[retained](QObject *object,QEvent *event){if(retained)retained->observe(object,event);};
        new Unbound::SurfaceTrace(this);
        auto windows=new Unbound::WindowDiagnostics(this);qApp->installEventFilter(windows);
        new Unbound::PagePanelInventory(this);
        new Unbound::PagePanelInventory(this,Unbound::PagePanelInventory::Scope::MainWindow);
        Unbound::logEvent("helper.start",{{"backend","shared-managed-workspace"},{"platform","windows"},
            {"live_accepted",false},{"qt",qVersion()}});
        const auto directory=qEnvironmentVariable("UNBOUND_LAUNCH_DIRECTORY",Unbound::diagnosticDirectory());
        auto launches=new Unbound::ControlLaunchRequests(this,directory,[this] {
            if(!controls)return;
            controls->showNormal();Unbound::keepOnDisplay(controls);controls->raise();controls->activateWindow();
            Unbound::logEvent("controls.show");
        });
        const auto seedError=Unbound::seedBundledPresets(Unbound::settingsProfilePath("Arrangements.json"),":/OpenGate/BundledArrangements.json");
        if(!seedError.isEmpty())Unbound::logEvent("workspace.preset.seed.failed",{{"error",seedError}});
        controls=new PanelControls;
        if(retained)controls->audioExperiment=[retained](int direction,bool skip){return retained && retained->invoke(direction,skip);};
        Unbound::logEvent("controls.ready",{{"visible",controls->isVisible()},{"startup","managed-workspace"}});
        new Unbound::WorkflowCommandBridge(this,directory,[this](QString id) {
            return controls&&controls->invokeWorkflowCommand(id);
        });
        new Unbound::WorkflowCommandResults(this,directory);
        launches->consume(true);
        connect(qApp,&QCoreApplication::aboutToQuit,this,[this] {
            delete controls.data();journal->markClean();Unbound::flushDiagnostics();
        });
    }
    ~ManagedWorkspaceService() override {delete controls.data();}
private:
    QPointer<PanelControls> controls;Unbound::CrashSession *journal=nullptr;
};
std::atomic<bool> started{false};
}
// Preserve the existing idempotent GUI-thread handoff used by the launcher.
bool Unbound::startController() {
    auto app=qobject_cast<QApplication *>(QCoreApplication::instance());
    if(!app||QCoreApplication::closingDown())return false;
    if(started.exchange(true))return true;
    if(!QMetaObject::invokeMethod(app,[app]{new ManagedWorkspaceService(app);},Qt::QueuedConnection)) {
        started=false;return false;
    }
    return true;
}
