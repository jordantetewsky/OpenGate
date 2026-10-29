// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QGraphicsRectItem>
#include <QVBoxLayout>
#include <QTemporaryDir>
#include <QEventLoop>
#include <QSaveFile>
#include <cstdio>
#include "nativeeditcommands.h"

// Models a native item's context-menu contract, including its exec() return
// path. Merely emitting QAction::triggered would not change this item's channel.
class AudioItem : public QGraphicsRectItem {
public:
    AudioItem():QGraphicsRectItem(0,0,160,40) {setFlag(ItemIsSelectable);}
    int channel=3,count=5,commits=0,menus=0;
    bool unsupported=false;
    std::function<void()> beforeChoice;
protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override {
        ++menus;
        QMenu root;root.setAttribute(Qt::WA_DontShowOnScreen);
        root.addAction("Clip Attributes...");
        QList<QAction *> channels;
        if(!unsupported) {
            auto menu=root.addMenu("Audio Channel");menu->setAttribute(Qt::WA_DontShowOnScreen);
            for(int n=1;n<=count;++n) {
                auto action=menu->addAction(QString::number(n)+" - Source");
                action->setCheckable(true);action->setChecked(n==channel);channels.append(action);
            }
        }
        if(beforeChoice)QTimer::singleShot(0,&root,beforeChoice);
        const auto selected=root.exec(event->screenPos());
        if(selected && channels.contains(selected)) {channel=channels.indexOf(selected)+1;++commits;}
        event->accept();
    }
};
int main(int argc,char **argv) {
    QApplication app(argc,argv);QTemporaryDir logs;qputenv("UNBOUND_DIAGNOSTICS_DIR",logs.path().toUtf8());
    qputenv("UNBOUND_SHORTCUTS_FILE",(logs.path()+"/Shortcuts.json").toUtf8());
    QWidget page;page.setAttribute(Qt::WA_DontShowOnScreen);page.setObjectName("UiMainWindow");
    auto layout=new QVBoxLayout(&page);QGraphicsScene scene;
    auto view=new QGraphicsView(&scene,&page);view->setObjectName("m_TiTimelineView");layout->addWidget(view);
    AudioItem item,other;other.setPos(0,80);scene.addItem(&item);scene.addItem(&other);item.setSelected(true);
    page.resize(400,300);page.show();QApplication::setActiveWindow(&page);view->setFocus();app.processEvents();
    qputenv("UNBOUND_LAUNCH_DIRECTORY",logs.path().toUtf8());
    qputenv("OPENGATE_WORKFLOW_SCRIPT_DIRECTORY",logs.path().toUtf8());
    Unbound::NativeEditCommands commands(nullptr,[&]{return &page;});int failures=0;
    Unbound::PanelCommands registry(&page);registry.context=[] {return true;};commands.addTo(&registry);
    auto check=[&](bool ok,const char *why){if(!ok){++failures;std::fprintf(stderr,"FAIL %s\n",why);}};
    for(const auto &suffix:{QString(" — All Source Channels"),QString(" — Skip Excluded Channels")})
      for(int direction:{-1,1}) {
        const auto label=QString(direction<0?"OpenGate - Previous Audio Channel":"OpenGate - Next Audio Channel")+suffix;
        QFile script(logs.path()+"/"+label+".py");script.open(QIODevice::WriteOnly);script.write("# opengate-command-route: native-audio-plan-v2");script.close();
        auto action=new QAction(label,&page);action->setObjectName("workspaceWIPlugins_"+QString::number(page.actions().size()));page.addAction(action);
        QObject::connect(action,&QAction::triggered,&page,[&,direction,skip=suffix.contains("Skip")] {
          QFile job(logs.path()+"/audio-channel-plan-job.json");job.open(QIODevice::ReadOnly);
          const auto request=QJsonDocument::fromJson(job.readAll()).object();
          const auto selected=Unbound::NativeTimelineSelection::inspect(view);
          auto clip=dynamic_cast<AudioItem *>(selected.item);if(!clip)return;
          int target=(clip->channel-1+direction+clip->count)%clip->count+1;
          if(skip)while(target<3)target=(target-1+direction+clip->count)%clip->count+1;
          QJsonObject plan{{"success",true},{"phase","plan-only"},{"audio_selected",1},{"video_selected",0},
            {"token",request["token"]},{"issued_ms",double(QDateTime::currentMSecsSinceEpoch())},
            {"items",QJsonArray{QJsonObject{{"status","ready"},{"before",clip->channel},{"after",target},{"source_channels",clip->count}}}}};
          QSaveFile result(logs.path()+"/audio-channel-plans/"+request["token"].toString()+".json");result.open(QIODevice::WriteOnly);result.write(QJsonDocument(plan).toJson());result.commit();
        });
      }
    auto pump=[&]{QEventLoop loop;QTimer::singleShot(30,&loop,&QEventLoop::quit);loop.exec();};
    auto run=[&](int direction,bool skip=false){const auto accepted=commands.audioChannel(direction,skip);pump();return accepted;};
    const auto selectedBefore=scene.selectedItems();
    for(auto expected:{4,5,1,2}) {
        QApplication::setActiveWindow(&page);view->setFocus();app.processEvents();
        check(registry.invoke("edit.audio.next-channel-all-source","fixture"),"registered native all-source request accepted");pump();
        check(item.channel==expected,"native exec-return path commits next channel and wrap");
    }
    for(auto expected:{1,5,4,3}) {
        QApplication::setActiveWindow(&page);view->setFocus();app.processEvents();
        check(registry.invoke("edit.audio.previous-channel-all-source","fixture"),"registered native previous request accepted");pump();
        check(item.channel==expected,"native exec-return path commits previous channel and wrap");
    }
    check(item.commits==8 && other.commits==0,"exactly one native commit per request, no other item changes");
    check(scene.selectedItems()==selectedBefore,"selection preserved");
    other.setSelected(true);check(!run(1,false),"multiple native selections fail closed");other.setSelected(false);
    item.setSelected(false);check(!run(1,false),"no selection fails closed");item.setSelected(true);
    item.unsupported=true;
    QApplication::setActiveWindow(&page);view->setFocus();app.processEvents();
    run(1,false);check(item.commits==8,"unsupported native menu makes no change");
    check(!QApplication::activePopupWidget(),"failed transaction leaves no popup behind");
    item.unsupported=false;item.count=1;item.channel=1;
    QApplication::setActiveWindow(&page);view->setFocus();app.processEvents();
    run(1,false);check(item.commits==8,"single-channel media makes no native commit");
    item.count=5;item.channel=3;
    item.beforeChoice=[&]{item.setSelected(false);other.setSelected(true);};
    QApplication::setActiveWindow(&page);view->setFocus();app.processEvents();
    run(1,false);
    check(item.commits==8 && other.commits==0,"selection change cancels pending transaction");
    check(!QApplication::activePopupWidget(),"cancellation closes only the transaction popup");
    item.beforeChoice={};other.setSelected(false);item.setSelected(false);
    // A Timeline can contain a proxy-embedded tracks view with its own scene.
    // The focused outer scene has no selectedItems, as observed in Resolve.
    QGraphicsScene tracks;
    auto inner=new QGraphicsView(&tracks);inner->resize(200,100);
    auto proxy=scene.addWidget(inner);proxy->setPos(0,140);inner->show();
    AudioItem nested;tracks.addItem(&nested);nested.setSelected(true);
    QApplication::setActiveWindow(&page);view->setFocus();app.processEvents();
    auto snapshot=Unbound::NativeTimelineSelection::inspect(view);
    check(snapshot.reason.isEmpty() && snapshot.scene==&tracks && snapshot.item==&nested,
        "proxy tracks selection resolves without outer scene selection");
    run(1,false);
    check(nested.channel==4 && nested.commits==1,"production transaction commits nested selected clip");
    item.setSelected(true);
    check(!run(1,false),"selection across outer and nested scenes is ambiguous");
    item.setSelected(false);nested.beforeChoice=[&]{other.setSelected(true);};
    QApplication::setActiveWindow(&page);view->setFocus();app.processEvents();
    run(1,false);
    check(nested.commits==1,"selection added in another contained scene cancels pending operation");
    tracks.removeItem(&nested);scene.removeItem(proxy);delete proxy;
    other.setSelected(false);item.setSelected(true);item.channel=3;item.count=5;
    for(const auto &step:QList<QPair<QString,int>>{
        {"edit.audio.previous-channel-all-source",2},{"edit.audio.next-channel",3},
        {"edit.audio.previous-channel",5},{"edit.audio.next-channel-all-source",1},
        {"edit.audio.next-channel",3}}) {
        QApplication::setActiveWindow(&page);view->setFocus();app.processEvents();
        check(registry.invoke(step.first,"fixture"),"all four registered commands accepted");pump();
        check(item.channel==step.second,"mode switches use fresh current channel and planned native target");
    }
    // Stack-owned native items must be removed before scene destruction.
    scene.removeItem(&item);scene.removeItem(&other);
    std::printf("native audio transaction failures=%d\n",failures);return failures?1:0;
}
