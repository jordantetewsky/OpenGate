// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QTemporaryDir>
#include <QPainter>
#include <cstdio>
#include "nativeeditcommands.h"
class PaintedMenuRow : public QWidget {
public:
    explicit PaintedMenuRow(QString label):caption(std::move(label)) { resize(320,32); }
    QString caption;
protected:
    void paintEvent(QPaintEvent *) override { QPainter painter(this); painter.drawText(rect(),Qt::AlignCenter,caption); }
};
int main(int argc,char **argv) {
    QApplication app(argc,argv); QTemporaryDir logs; qputenv("UNBOUND_DIAGNOSTICS_DIR",logs.path().toUtf8());
    int failures=0; auto check=[&](bool ok,const char *message) { if (!ok) { ++failures; std::fprintf(stderr,"FAIL: %s\n",message); } };
    QWidget page; page.setAttribute(Qt::WA_DontShowOnScreen); auto layout=new QVBoxLayout(&page);
    auto inspector=new QWidget(&page); inspector->setObjectName("widgetEditInspector"); layout->addWidget(inspector);
    auto row=new QVBoxLayout(inspector); auto stabilize=new QPushButton("Stabilize",inspector); row->addWidget(stabilize);
    int clicks=0; QObject::connect(stabilize,&QPushButton::clicked,[&] { ++clicks; });
    Unbound::NativeEditCommands commands(nullptr,[&] { return &page; });
    QMenu workflow(&page);auto nativeStabilize=workflow.addAction("OpenGate - Edit - Stabilize");
    nativeStabilize->setObjectName("workspaceWIPlugins_17");int nativeStabilizations=0;
    QObject::connect(nativeStabilize,&QAction::triggered,[&] {++nativeStabilizations;});
    QTemporaryDir scripts;qputenv("OPENGATE_WORKFLOW_SCRIPT_DIRECTORY",scripts.path().toUtf8());
    const auto scriptPath=scripts.path()+"/OpenGate - Edit - Stabilize.py";
    auto writeScript=[&](QByteArray data) {QFile script(scriptPath);script.open(QIODevice::WriteOnly);script.write(data);};
    writeScript("# opengate-command-route: selected-timeline-api-v1\n");
    check(!commands.stabilize()&&nativeStabilizations==0,"inactive page cannot trigger analysis");
    page.show();inspector->hide();QApplication::processEvents();
    check(commands.stabilize()&&nativeStabilizations==1&&clicks==0,"hidden Inspector routes through original workflow action without clicking a button");
    nativeStabilize->setObjectName("workspaceWIPlugins_99");
    check(commands.stabilize()&&nativeStabilizations==2,"native workflow position can change without changing command identity");
    nativeStabilize->setEnabled(false);
    QString failureReason;commands.report=[&](QString reason){failureReason=reason;};
    check(!commands.stabilize()&&nativeStabilizations==2,"disabled native workflow command is respected");
    check(failureReason.contains("disabled")&&!nativeStabilize->isEnabled(),"disabled target is identified precisely without force-enabling");
    nativeStabilize->setEnabled(true);
    auto duplicate=workflow.addAction("OpenGate - Edit - Stabilize");duplicate->setObjectName("workspaceWIPlugins_duplicate");
    check(!commands.stabilize()&&nativeStabilizations==2,"ambiguous workflow actions are refused");delete duplicate;
    check(failureReason.contains("ambiguous"),"ambiguous target is distinguished from a disabled action");
    writeScript("# legacy wrapper posts registry requests\n");
    check(!commands.stabilize()&&nativeStabilizations==2,"legacy wrapper cannot create recursive stabilization requests");
    QFile::remove(scriptPath);
    check(!commands.stabilize()&&nativeStabilizations==2,"missing API wrapper cannot fall back to a visible button");
    writeScript("# opengate-command-route: selected-timeline-api-v1\n");
    check(commands.stabilize()&&nativeStabilizations==3&&clicks==0,"restored direct API wrapper routes once with Inspector hidden");
    int audioCalls=0;
    for(const auto &name:{QString("Previous Audio Channel — Skip Excluded Channels"),QString("Next Audio Channel — Skip Excluded Channels"),QString("Previous Audio Channel — All Source Channels"),QString("Next Audio Channel — All Source Channels")}) {
        QFile script(scripts.path()+"/OpenGate - "+name+".py");script.open(QIODevice::WriteOnly);
        script.write("# opengate-command-route: native-audio-plan-v2\n");script.close();
        auto action=workflow.addAction("OpenGate - "+name);action->setObjectName("workspaceWIPlugins_"+name);
        QObject::connect(action,&QAction::triggered,[&] {++audioCalls;});
    }
    check(!commands.audioChannel(-1)&&!commands.audioChannel(1)&&audioCalls==0,"both channel directions require a native selected Timeline item before planning");
    check(!commands.audioChannel(0)&&audioCalls==0,"invalid direction cannot invoke an audio command");
    check(!commands.audioChannel(-1,false)&&!commands.audioChannel(1,false)&&audioCalls==0,"all-source requires native Timeline selection and never falls back to API workflow");
    page.hide();QApplication::processEvents();
    check(!commands.audioChannel(1)&&audioCalls==0,"audio command respects inactive Edit page");
    page.show();QApplication::processEvents();
    QMenu unrelated; auto wrong=unrelated.addAction("Fixed Playhead"); wrong->setCheckable(true);
    check(!commands.fixedPlayhead() && !wrong->isChecked(),"unrelated same-label action is not invoked");
    QMenu timeline; timeline.addAction("Display Stacked Timelines"); auto fixed=timeline.addAction("Fixed Playhead"); fixed->setCheckable(true);
    check(commands.fixedPlayhead() && fixed->isChecked() && !wrong->isChecked(),"only native Timeline Options action is toggled");
    check(commands.fixedPlayhead() && !fixed->isChecked(),"second command reverses the same native state");
    // No selected clip or timeline focus is necessary for presentation toggling.
    stabilize->setEnabled(false);
    inspector->show();QLineEdit inspectorFocus(inspector);inspectorFocus.show();inspectorFocus.setFocus();QApplication::processEvents();
    check(commands.fixedPlayhead() && fixed->isChecked(),"Inspector focus and unavailable clip target do not block fixed playhead");
    check(commands.fixedPlayhead() && !fixed->isChecked(),"fixed playhead restores state independent of clip selection");
    page.hide();QApplication::processEvents();
    check(!commands.fixedPlayhead() && !fixed->isChecked(),"leaving Edit cannot toggle the timeline presentation");
    page.show();QApplication::processEvents();
    fixed->setEnabled(false); check(!commands.fixedPlayhead() && !fixed->isChecked(),"disabled timeline action is respected");
    timeline.removeAction(fixed);
    QWidget preferences; preferences.setObjectName("btnEditToolbarPreferences");
    QMenu embedded(&preferences); auto custom=new QWidgetAction(&embedded);
    auto content=new QWidget; auto contentLayout=new QVBoxLayout(content);
    contentLayout->addWidget(new QLabel("Fixed Playhead",content)); custom->setDefaultWidget(content);
    custom->setCheckable(true); embedded.addAction(custom);
    check(commands.fixedPlayhead() && custom->isChecked(),"blank native action is identified through its embedded row label");
    contentLayout->takeAt(0)->widget()->deleteLater();QApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
    content->setAccessibleName("Fixed Playhead");
    check(commands.fixedPlayhead()&&!custom->isChecked(),"custom-painted native row is identified by accessibility metadata without text children");
    check(commands.fixedPlayhead()&&custom->isChecked(),"semantic row retains both toggle states");
    custom->setEnabled(false);
    check(!commands.fixedPlayhead() && custom->isChecked(),"disabled embedded native action is respected");
    custom->setEnabled(true); embedded.setProperty("unboundOwnMenu",true);
    check(!commands.fixedPlayhead() && custom->isChecked(),"helper-owned menu cannot impersonate a native target");
    QAction nativeFixed(&page);nativeFixed.setObjectName("viewTimelineScrollingFixed");nativeFixed.setCheckable(true);
    check(!commands.fixedPlayhead()&&!nativeFixed.isChecked(),"global scrolling QAction cannot substitute for real Edit settings");
    embedded.setProperty("unboundOwnMenu",false);
    embedded.removeAction(custom);delete custom;
    custom=new QWidgetAction(&embedded);
    auto painted=new PaintedMenuRow("Fixed Playhead"); custom->setDefaultWidget(painted);custom->setCheckable(true);embedded.addAction(custom);
    bool actualMode=false;int nativeCalls=0;
    QObject::connect(custom,&QAction::triggered,[&] { actualMode=!actualMode;++nativeCalls; });
    page.hide();preferences.hide();embedded.hide();painted->hide();
    painted->resize(640,480);const auto unlaidOutSize=painted->size();
    bool editActive=true;commands.editActive=[&] {return editActive;};
    auto focusBefore=QApplication::focusWidget();
    const int beforeHiddenStabilize=nativeStabilizations;
    check(commands.stabilize()&&nativeStabilizations==beforeHiddenStabilize+1,"active Edit with hidden panel uses native scripting command");
    check(commands.fixedPlayhead()&&actualMode&&nativeCalls==1,"hidden custom-painted native row invokes real settings handler");
    check(painted->size()==unlaidOutSize,"caption reading preserves unlaid-out native geometry");
    check(!page.isVisible()&&!embedded.isVisible()&&!painted->isVisible(),"invocation does not show timeline, settings menu or its row");
    check(QApplication::focusWidget()==focusBefore,"invocation does not move focus");
    check(commands.fixedPlayhead()&&!actualMode&&nativeCalls==2,"same hidden handler switches Fixed Playhead off");
    QWidget editTimeline;editTimeline.setObjectName("frameConformTimelines");
    preferences.setParent(&editTimeline);
    QWidget otherPreferences;otherPreferences.setObjectName("btnEditToolbarPreferences");
    QMenu otherMenu(&otherPreferences);auto otherFixed=otherMenu.addAction("Fixed Playhead");otherFixed->setCheckable(true);
    check(commands.fixedPlayhead()&&actualMode&&nativeCalls==3&&!otherFixed->isChecked(),"hidden Edit-owned settings disambiguate an unrelated duplicate button");
    QWidget duplicateEditPreferences(&editTimeline);duplicateEditPreferences.setObjectName("btnEditToolbarPreferences");
    check(!commands.fixedPlayhead()&&nativeCalls==3,"two actual Edit owners fail closed instead of selecting by focus or visibility");
    duplicateEditPreferences.setObjectName("inactive-fixture");
    preferences.setParent(nullptr);otherPreferences.setObjectName("inactive-fixture");
    check(commands.fixedPlayhead()&&!actualMode&&nativeCalls==4,"same native handler restores state after scoped dispatch");
    editActive=false;
    check(!commands.fixedPlayhead()&&nativeCalls==4,"active page contract prevents invocation outside Edit");
    editActive=true;
    painted->caption="Display Audio Waveforms";
    const bool result=commands.fixedPlayhead();
    check(!result&&nativeCalls==4,"different painted caption cannot be treated as Fixed Playhead");
    std::printf("native edit command failures=%d\n",failures); return failures ? 1 : 0;
}
