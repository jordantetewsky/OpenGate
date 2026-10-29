// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QMetaMethod>
#include <QSignalMapper>
#include <QSet>
#include <QStackedWidget>
#include <QTimer>
#include "diagnostics.h"
#include "../platform/diagnosticplatform.h"

namespace Unbound {
// Observe only runtime-confirmed signals. Never invoke a private player method.
class ViewerDiagnostics : public QObject
{
public:
    explicit ViewerDiagnostics(QObject *parent) : QObject(parent)
    {
        if(loadingTrace)qApp->installEventFilter(this);
        presentationTimer.setSingleShot(true); presentationTimer.setInterval(100);
        connect(&presentationTimer,&QTimer::timeout,this,[this] { flushPresentation(); });
        focusSnapshot.setSingleShot(true); focusSnapshot.setInterval(100);
        focusSettled.setSingleShot(true); focusSettled.setInterval(120);
        connect(&focusSnapshot,&QTimer::timeout,this,[this] { capture("native-focus-coalesced",lastFocusSignal); });
        connect(&focusSettled,&QTimer::timeout,this,[this] { capture("native-focus-settled",lastFocusSignal); });
    }
    ~ViewerDiagnostics() override { flushPresentation(); if(qApp)qApp->removeEventFilter(this); }
    bool nativeLoadingTraceEnabled() const { return loadingTrace; }
    void scan()
    {
        for (auto widget : QApplication::allWidgets()) observe(widget);
    }
    void observe(QWidget *widget)
    {
            if(!widget)return;
            const bool player = widget->objectName()=="widgetEditSourcePlayer" || widget->objectName()=="widgetEditTimelinePlayer";
            const bool clipView = loadingTrace && widget->inherits("UiMp::UiMediaPoolClipView");
            const bool conformer = loadingTrace && widget->inherits("UiConformerPanelImp");
            if ((!player && !clipView && !conformer) || observed.contains(widget)) return;
            observed.insert(widget);
            if (player) players.insert(widget);
            connect(widget, &QObject::destroyed, this, [this, widget] { players.remove(widget); observed.remove(widget); });
            QJsonArray inventory;
            const auto meta = widget->metaObject();
            if(loadingTrace) {
                const QList<QByteArray> candidates = conformer
                    ? QList<QByteArray>{"OnMpPlaceOnSourceViewer","OnClipDropped"}
                    : clipView ? QList<QByteArray>{"PlaceClipOnSourceViewer"}
                    : QList<QByteArray>{"DragEntered","DropTriggered","MatchFrameTriggered","CurrentItemsChanged","CurrentTitleChanged"};
                QJsonArray methods;
                for(const auto &name:candidates) {
                    bool found=false;
                    for(int index=0;index<meta->methodCount();++index) {
                        const auto method=meta->method(index);if(method.name()!=name)continue;
                        found=true;
                        const bool signal=method.methodType()==QMetaMethod::Signal;
                        methods.append(QJsonObject{{"name",QString::fromLatin1(name)},
                            {"signature",QString::fromLatin1(method.methodSignature())},
                            {"method_type",int(method.methodType())},{"observable_as_signal",signal},
                            {"status",signal?"signal-connection-reported-separately":"native-handler-execution-not-observable"}});
                    }
                    if(!found)methods.append(QJsonObject{{"name",QString::fromLatin1(name)},
                        {"status","not-present-in-runtime-metaobject"},{"observable_as_signal",false}});
                }
                logEvent("viewer.native-methods",{{"object",widgetState(widget,false)},{"methods",methods},
                    {"silence_means","no-conclusion-without-confirmed-signal-connection"},
                    {"native_receiver_execution_observed",false}});
            }
            const QSet<QByteArray> observedSignals{"ChangePlayerCinemaMode", "ReplaceCinemaViewer", "IsActiveChanged", "FocusReceived", "DragEntered", "DropTriggered", "PlaceClipOnSourceViewer", "MatchFrameTriggered", "CurrentItemsChanged", "CurrentTitleChanged"};
            for (int index = 0; index < meta->methodCount(); ++index) {
                const auto method = meta->method(index);
                if (!observedSignals.contains(method.name()) || method.methodType() != QMetaMethod::Signal || (method.attributes() & QMetaMethod::Cloned)) continue;
                if(!loadingTrace && method.name()!="ChangePlayerCinemaMode" && method.name()!="ReplaceCinemaViewer" &&
                    method.name()!="IsActiveChanged" && method.name()!="FocusReceived") continue;
                auto mapper = new QSignalMapper(widget);
                const QString identity = widget->objectName() + ':' + QString::fromLatin1(method.methodSignature());
                mapper->setMapping(widget, identity);
                const auto slot = mapper->metaObject()->method(mapper->metaObject()->indexOfSlot("map()"));
                const bool connected = bool(QObject::connect(widget, method, mapper, slot, Qt::AutoConnection));
                connect(mapper, &QSignalMapper::mappedString, this,
                    [this](const QString &signal) {
                        if (signal.contains(":CurrentItemsChanged(") || signal.contains(":CurrentTitleChanged(")) {
                            logEvent("viewer.content-notification",{{"signal",signal},{"clip_identity_verified",false}});
                            return;
                        }
                        if (signal.contains(":DragEntered(") || signal.contains(":DropTriggered(") ||
                            signal.contains(":PlaceClipOnSourceViewer(") || signal.contains(":MatchFrameTriggered(")) {
                            // Observe emissions only: never read MIME/clip payloads or
                            // invoke a native slot. An emission is not load acceptance.
                            captureLoading("native-loading-request",signal);
                            QTimer::singleShot(0,this,[this,signal] { captureLoading("native-loading-next-turn",signal); });
                            QTimer::singleShot(120,this,[this,signal] { captureLoading("native-loading-settled",signal); });
                            return;
                        }
                        if (!signal.contains("CinemaViewer") && !signal.contains("CinemaMode")) {
                            // Keep every signal identity, but do not serialize the entire
                            // window/player tree repeatedly while focus is changing.
                            logEvent("viewer.native-signal",{{"signal",signal}});
                            lastFocusSignal = signal;
                            if (!focusSnapshot.isActive()) focusSnapshot.start();
                            focusSettled.start(); return;
                        }
                        capture("native-player-signal", signal);
                        QTimer::singleShot(120, this, [this, signal] { capture("native-player-settled", signal); });
                    });
                inventory.append(QJsonObject{{"signal", QString::fromLatin1(method.methodSignature())}, {"connected", connected}});
            }
            logEvent(QStringLiteral("viewer.native-signals"), {{"player", widgetState(widget,false)}, {"signals", inventory}});
    }
    void captureLoading(const QString &phase,const QString &origin)
    {
        QJsonArray viewers;
        for(auto player:players) {
            QJsonArray ancestry;
            for(auto parent=player->parentWidget();parent&&ancestry.size()<10;parent=parent->parentWidget())
                ancestry.append(widgetState(parent,false));
            QJsonObject state{{"player",widgetState(player,false)},{"ancestry",ancestry},
                {"accepts_drops",player->acceptDrops()}};
            if(auto stack=qobject_cast<QStackedWidget *>(player->parentWidget())) {
                state["stack_index"]=stack->currentIndex();state["selected_page"]=widgetState(stack->currentWidget(),false);
            }
            viewers.append(state);
        }
        QJsonArray controllers;
        for (auto widget:observed) if (widget->inherits("UiConformerPanelImp") && controllers.size()<8) {
            QJsonArray ancestry;
            for (auto parent=widget->parentWidget();parent&&ancestry.size()<10;parent=parent->parentWidget())
                ancestry.append(widgetState(parent,false));
            controllers.append(QJsonObject{{"controller",widgetState(widget,false)},{"ancestry",ancestry}});
        }
        // Bounded observed-player snapshots, no allWidgets scan, layout hints,
        // payload reads or forced disk flush on the loading input path.
        logEvent("viewer.loading.context",{{"phase",phase},{"origin",origin},{"players",viewers},{"native_controllers",controllers},
            {"relationship","temporal-observation-not-proof-of-causality"}});
    }
    void capture(const QString &phase, const QString &command)
    {
        QJsonArray windows, viewers;
        for (auto window : QApplication::topLevelWidgets()) if (window->isVisible()) {
            auto state = widgetState(window); state["state"] = int(window->windowState());
            state["native_window"] = diagnosticNativeWindowNumber(window); windows.append(state);
        }
        for (auto widget : players) {
            QJsonArray ancestry;
            for (auto parent = widget->parentWidget(); parent && ancestry.size() < 10; parent = parent->parentWidget()) ancestry.append(widgetState(parent));
            QJsonObject entry{{"player", widgetState(widget)}, {"ancestry", ancestry},
                {"accepts_drops",widget->acceptDrops()}};
            if (auto stack=qobject_cast<QStackedWidget *>(widget->parentWidget())) {
                entry["stack_index"]=stack->currentIndex();
                entry["selected_page"]=widgetState(stack->currentWidget(),false);
            }
            viewers.append(entry);
        }
        logEvent(QStringLiteral("viewer.cinema.context"), {{"phase", phase}, {"command", command}, {"windows", windows},
            {"players", viewers}, {"focus", widgetState(QApplication::focusWidget())}, {"native_key", diagnosticNativeKeyWindowNumber()}});
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override
    {
        const auto type=event->type();
        if(type!=QEvent::Show && type!=QEvent::Hide && type!=QEvent::ParentChange) return false;
        auto widget=qobject_cast<QWidget *>(object);if(!widget)return false;
        bool relevant=false;
        for(auto player:players) if(widget==player || widget==player->parentWidget() || widget==player->window()) {relevant=true;break;}
        if(!relevant)return false;
        const QString key=QString::number(quintptr(widget),16)+':'+QString::number(int(type));
        auto &record=presentationChanges[key];
        record["count"]=record["count"].toInt()+1;
        record["event_type"]=int(type);record["widget"]=widgetState(widget,false);
        record["phase"]="before-native-handler";
        if(!presentationTimer.isActive())presentationTimer.start();
        return false;
    }
private:
    void flushPresentation()
    {
        const auto pending=presentationChanges;presentationChanges.clear();presentationTimer.stop();
        for(auto state:pending)logEvent("viewer.presentation.events",state);
    }
    const bool loadingTrace = qEnvironmentVariableIntValue("OPENGATE_TRACE_NATIVE_LOADING")==1;
    QHash<QString,QJsonObject> presentationChanges;
    QTimer presentationTimer;
    QSet<QWidget *> players, observed;
    QTimer focusSnapshot,focusSettled;
    QString lastFocusSignal;
};
}
