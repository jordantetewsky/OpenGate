// SPDX-License-Identifier: MIT
#include <QActionEvent>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMetaMethod>
#include <memory>
#include <QMenu>
#include "../../../src/core/timelineclipcolorpreview.h"
#include "../../core/workspace-callers/clipnativefillprobe.h"

namespace {
void record(QString event,QJsonObject data={}) {
    const auto path=qEnvironmentVariable("OPENGATE_NATIVE_FILL_TRACE_LOG");
    if(path.isEmpty())return;
    data["event"]=event;data["pid"]=double(QCoreApplication::applicationPid());
    data["time"]=QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    data["project_writes"]=false;QFile file(path);
    if(file.open(QIODevice::WriteOnly|QIODevice::Append))file.write(QJsonDocument(data).toJson(QJsonDocument::Compact)+'\n');
}
QVector<QPointer<QGraphicsObject>> selection(QGraphicsView *root) {
    QVector<QPointer<QGraphicsObject>> result;QList<QGraphicsView *> pending{root};QSet<QGraphicsView *> visited;
    while(!pending.isEmpty() && visited.size()<32) {
        auto view=pending.takeFirst();if(!view || visited.contains(view) || !view->scene())continue;visited.insert(view);
        for(auto item:view->scene()->selectedItems())for(auto parent=item;parent;parent=parent->parentItem())
            if(auto object=parent->toGraphicsObject())if(object->inherits("UiTiClipContainer")) {
                if(!result.contains(object))result.append(object);break;
            }
        for(auto item:view->scene()->items())if(auto proxy=qgraphicsitem_cast<QGraphicsProxyWidget *>(item))
            if(auto nested=qobject_cast<QGraphicsView *>(proxy->widget()))pending.append(nested);
    }
    return result;
}
class NativeFillProbe : public QObject {
public:
    NativeFillProbe():QObject(qApp) {
        setObjectName("openGateNativeFillProbe");qApp->installEventFilter(this);
        connect(qApp,&QCoreApplication::aboutToQuit,this,[this]{clear();});
        record("probe.loaded",{{"installed_runtime_replaced",false}});
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        auto menu=qobject_cast<QMenu *>(object);if(!menu)return false;
        if(event->type()==QEvent::ActionAdded)observe(menu,static_cast<QActionEvent *>(event)->action());
        if(event->type()==QEvent::Show)for(auto action:menu->actions())observe(menu,action);
        return false;
    }
private:
    QSet<QAction *> watched;QPointer<Unbound::TimelineClipColorPreview> preview;
    QVector<QPointer<QGraphicsProxyWidget>> proxies;QVector<QPointer<OpenGateTest::ClipNativeFillProbe>> effects;
    void clear() {
        if(preview){delete preview;preview=nullptr;}
        for(int i=0;i<proxies.size();++i)if(proxies[i] && effects[i] && proxies[i]->graphicsEffect()==effects[i])
            proxies[i]->setGraphicsEffect(nullptr);
        proxies.clear();effects.clear();
    }
    void observe(QMenu *menu,QAction *action) {
        if(!action || watched.contains(action))return;
        if(action->objectName()!="openGateRedClipPreview" && menu->objectName()!="markSetClipColor")return;
        watched.insert(action);connect(action,&QObject::destroyed,this,[this,action]{watched.remove(action);});
        if(action->objectName()!="openGateRedClipPreview") {
            connect(action,&QAction::triggered,this,[this]{clear();record("probe.cleared");});return;
        }
        connect(action,&QAction::triggered,this,[this]{request();});record("probe.red-action.observed");
    }
    void request() {
        QPointer<QGraphicsView> root;
        for(auto widget:QApplication::allWidgets())if(widget->objectName()=="m_TiTimelineView")
            if(auto view=qobject_cast<QGraphicsView *>(widget))if(!selection(view).isEmpty()) {
                if(root){record("probe.rejected",{{"reason","ambiguous-selected-timeline"}});return;}root=view;
            }
        if(!root){record("probe.rejected",{{"reason","no-native-selection"}});return;}
        const auto selected=selection(root);record("probe.request",{{"selected",selected.size()}});
        // Let the installed unsuccessful per-clip preview finish its cleanup.
        QTimer::singleShot(500,this,[this,root,selected]{
            if(!root || selected!=selection(root)){record("probe.rejected",{{"reason","selection-changed"}});return;}
            clear();
            if(auto parent=root->parentWidget())parent->grab().save(qEnvironmentVariable("OPENGATE_NATIVE_FILL_TRACE_LOG")+".before.png");
            preview=new Unbound::TimelineClipColorPreview(root,this);
            const auto count=preview->applySelected(Qt::red);
            auto bindings=Unbound::TimelineClipColorBindings::find(root);const auto regions=bindings?bindings->regions():QVector<Unbound::ClipFillRegion>();
            QJsonArray bounds;for(const auto &region:regions)bounds.append(QJsonArray{region.rect.x(),region.rect.y(),region.rect.width(),region.rect.height()});
            record("probe.clip-bounds",{{"bounds",bounds}});
            for(const auto &clip:selected)if(clip) {
                QJsonArray methods;auto meta=clip->metaObject();
                for(int i=0;i<meta->methodCount();++i)if(meta->method(i).name().toLower().contains("color"))methods.append(QString::fromLatin1(meta->method(i).methodSignature()));
                record("probe.clip-color-methods",{{"methods",methods}});
            }
            for(auto item:root->scene()->items())if(auto proxy=qgraphicsitem_cast<QGraphicsProxyWidget *>(item)) {
                if(!qobject_cast<QGraphicsView *>(proxy->widget()) || proxy->graphicsEffect())continue;
                bool relevant=false;for(const auto &region:regions)relevant|=proxy->sceneBoundingRect().intersects(region.visible);
                if(!relevant)continue;
                auto commands=std::make_shared<QJsonArray>();
                auto effect=new OpenGateTest::ClipNativeFillProbe(root,proxy,[commands](const Unbound::ClipFillPaintDevice::Result &result){
                    record("probe.native-paint",{{"changed_fill_commands",result.changed},{"rectangles",result.trace.rects},{"pixmaps",result.trace.pixmaps},{"commands",*commands}});
                },[commands,regions](const char *kind,const QRectF &rect,QColor source,QColor replacement){
                    if(commands->size()>=256)return;
                    for(const auto &region:regions)if(rect.intersects(region.visible)) {
                        commands->append(QJsonObject{{"kind",kind},{"rect",QJsonArray{rect.x(),rect.y(),rect.width(),rect.height()}},
                            {"source",source.isValid()?source.name(QColor::HexArgb):QString()},{"replacement",replacement.isValid()?replacement.name():QString()}});break;
                    }
                });proxies.append(proxy);effects.append(effect);proxy->setGraphicsEffect(effect);
            }
            record("probe.attached",{{"selected",count},{"view_proxies",proxies.size()},{"regions",regions.size()}});
            QTimer::singleShot(250,this,[root]{if(root)if(auto parent=root->parentWidget())
                parent->grab().save(qEnvironmentVariable("OPENGATE_NATIVE_FILL_TRACE_LOG")+".after.png");});
        });
    }
};
}
extern "C" __attribute__((visibility("default"))) void unboundInspectWidgets() {
    if(!qApp || qApp->findChild<QObject *>("openGateNativeFillProbe"))return;
    if(qEnvironmentVariable("OPENGATE_NATIVE_FILL_TRACE_LOG").isEmpty())return;
    new NativeFillProbe;
}
