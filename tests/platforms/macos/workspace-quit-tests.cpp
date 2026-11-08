// SPDX-License-Identifier: MIT
#include "native-fixture-start.h"
#include <QApplication>
#include <QElapsedTimer>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPlatformSurfaceEvent>
#include <QTemporaryDir>
#include <QThread>
#include <cstdio>
#include "panelcontrols.h"

namespace {
int failures=0;
void check(bool pass,const char *message) {
    if(!pass){++failures;std::fprintf(stderr,"FAIL: %s\n",message);}
}
void settle(int ms=100) {
    QElapsedTimer time;time.start();
    while(time.elapsed()<ms){QApplication::processEvents();QThread::msleep(1);}
    QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
}
template<class T>T *component(QObject *owner) {
    for(auto child:owner->children())if(auto value=dynamic_cast<T *>(child))return value;
    return nullptr;
}
QJsonObject savedWorkspace() {
    QFile file(qEnvironmentVariable("UNBOUND_WORKSPACE_FILE"));
    if(!file.open(QIODevice::ReadOnly))return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}
// Record creation/movement, not ordinary SurfaceAboutToBeDestroyed during Qt
// owner deletion. Never call winId(): the observer must not create a window.
class QuitMutations : public QObject {
public:
    bool armed=false;int mutations=0;
    const char *phase="quit";
    QSet<QWidget *> baseline;
    void begin() {
        baseline.clear();for(auto widget:QApplication::allWidgets())baseline.insert(widget);
        armed=true;
    }
    bool eventFilter(QObject *object,QEvent *event) override {
        if(!armed)return false;
        const auto type=event->type();
        const bool created=type==QEvent::PlatformSurface &&
            static_cast<QPlatformSurfaceEvent *>(event)->surfaceEventType()==QPlatformSurfaceEvent::SurfaceCreated;
        if(type==QEvent::ParentChange || type==QEvent::Show || type==QEvent::WinIdChange || created) {
            // WinIdChange to zero is normal surface destruction.
            auto widget=qobject_cast<QWidget *>(object);
            if(type==QEvent::WinIdChange && widget && !widget->testAttribute(Qt::WA_WState_Created))return false;
            ++mutations;
            std::fprintf(stderr,"QUIT MUTATION (%s): %s/%s event=%d\n",phase,object->metaObject()->className(),qPrintable(object->objectName()),int(type));
        }
        return false;
    }
    void noNewWidgets() {
        for(auto widget:QApplication::allWidgets())check(baseline.contains(widget),"quit did not create another widget/host");
    }
};
}
int main(int argc,char **argv) {
    QApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);
    qputenv("UNBOUND_TEST_HIDDEN_HOSTS","1");
    QTemporaryDir files;
    qputenv("UNBOUND_DIAGNOSTICS_DIR",files.path().toUtf8());
    qputenv("UNBOUND_WORKSPACE_FILE",(files.path()+"/Workspace.json").toUtf8());
    qputenv("UNBOUND_ARRANGEMENTS_FILE",(files.path()+"/Arrangements.json").toUtf8());
    const bool floating=app.arguments().contains("--floating");
    auto main=new QWidget;main->setObjectName("UiMainWindow");main->setAttribute(Qt::WA_DontShowOnScreen);
    main->resize(1200,750);auto outer=new QVBoxLayout(main);
    auto page=new QWidget(main);page->setObjectName("WStackPage_Conform");outer->addWidget(page);
    auto native=new QHBoxLayout(page);
    auto panel=[&](const char *name) {
        auto widget=new QWidget(page);widget->setObjectName(name);auto box=new QVBoxLayout(widget);
        auto header=new QWidget(widget);auto row=new QHBoxLayout(header);auto options=new QToolButton(header);
        options->setObjectName("tbtnPlayerOptions");row->addWidget(options);box->addWidget(header);
        box->addWidget(new QPushButton(name,widget));native->addWidget(widget);return widget;
    };
    auto timeline=panel("frameConformTimelines");
    auto view=new QGraphicsView(timeline);view->setObjectName("m_TiTimelineView");
    view->setScene(new QGraphicsScene(view));view->scene()->addRect(0,0,200,40);
    timeline->layout()->addWidget(view);
    auto source=panel("widgetEditSourcePlayer");auto program=panel("widgetEditTimelinePlayer");
    main->show();settle();
    QuitMutations mutations;app.installEventFilter(&mutations);
    // Installed before PanelControls' connection, covering its whole quit slot.
    QObject::connect(&app,&QCoreApplication::aboutToQuit,&mutations,[&]{mutations.begin();});
    prepareNativeFixture();auto controls=new PanelControls;settle(500);
    auto click=[&](const char *name) {
        auto button=controls->findChild<QPushButton *>(name);check(button,"fixture command exists");
        if(button)button->click();settle(150);
    };
    click("undock_widgetEditSourcePlayer");
    if(floating)click("undock_widgetEditTimelinePlayer");
    auto host=dynamic_cast<Unbound::WorkspaceFloatingHost *>(source->window());
    check(host && bool(host->attachRequested),"source exposes the production group/drop route");
    if(host && host->attachRequested)host->attachRequested(floating?program->window():timeline,Qt::RightEdge);
    settle(250);
    check(source->window()==(floating?program->window():main),"fixture has a retained native or floating group");
    auto expansion=component<Unbound::PanelExpansionController>(controls);
    auto binding=component<Unbound::NativeWorkspaceBinding>(controls);
    const auto nativeBeforeExpansion=binding?binding->saved():QJsonObject();
    click("expand_widgetEditSourcePlayer");settle(250);
    check(expansion && expansion->index()>=0 && source->window()!=main,"group member is actually expanded");
    auto header=component<Unbound::PinnedWorkspaceHeader>(controls);check(header,"header owner exists");
    check(savedWorkspace().value("header").toObject().value("visible").toBool(true),"header change is not already saved");
    // Do not settle: this change must be committed by the quit path, not a timer.
    if(header)header->setShown(false);
    QPointer<QWidget> sourceParent=source->parentWidget(),timelineParent=timeline->parentWidget(),viewParent=view->parentWidget();
    bool observed=false;
    QObject::connect(&app,&QCoreApplication::aboutToQuit,&mutations,[&] {
        observed=true;
        check(source->parentWidget()==sourceParent && timeline->parentWidget()==timelineParent && view->parentWidget()==viewParent,
            "quit keeps native content in its current Qt owners");
        const auto saved=savedWorkspace();
        check(!saved.isEmpty() && !saved.value("header").toObject().value("visible").toBool(true),
            "quit commits the last unsaved header choice");
        if(!floating)check(saved.value("native_arrangement").toObject()==nativeBeforeExpansion,
            "quit saves the retained pre-expansion native arrangement");
        check(expansion && expansion->index()<0,"quit cancels the expansion owner without restoring widgets");
        mutations.noNewWidgets();
    });
    QTimer::singleShot(0,&app,&QCoreApplication::quit);
    check(app.exec()==0 && observed,"actual application event-loop quit reaches the disarm path");
    check(mutations.mutations==0,"aboutToQuit itself performs no window/layout transfers");
    // Exercise already queued callbacks while owners are still alive. They must
    // remain quiescent even if a host drains events during teardown.
    mutations.phase="queued callbacks";settle(300);
    check(source->parentWidget()==sourceParent && timeline->parentWidget()==timelineParent && view->parentWidget()==viewParent,
        "queued recovery and refresh callbacks leave native parents intact");
    mutations.noNewWidgets();
    mutations.phase="controller destruction";
    delete controls; // Repeated shutdown/destructor must not retry restoration.
    check(source->parentWidget()==sourceParent && timeline->parentWidget()==timelineParent && view->parentWidget()==viewParent,
        "controller destruction after quit does not transfer native content");
    check(mutations.mutations==0,"quit and lease teardown create/show/reparent no windows or content");
    mutations.armed=false;
    delete main; // Ordinary Qt-owned surface destruction is a separate phase.
    std::printf("workspace quit %s failures=%d\n",floating?"floating":"native",failures);
    return failures?1:0;
}
