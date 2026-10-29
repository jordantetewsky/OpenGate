// SPDX-License-Identifier: MIT
#include "trace.h"
#include <QDateTime>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QKeyEvent>
#include <QMetaMethod>
#include <QThread>
#include <private/qobject_p.h>
#include <qt_windows.h>

static constexpr int recordLimit=2500;
static constexpr int inventoryLimit=100000;
static QString pointer(const void *p) { return QStringLiteral("0x%1").arg(quintptr(p),0,16); }
static QJsonObject identity(const QObject *o)
{
    if (!o) return {};
    return {{"id",pointer(o)},{"class",o->metaObject()->className()},{"name",o->objectName()}};
}
static QJsonObject widgetState(QWidget *w)
{
    auto result=identity(w);
    if (w) {
        result.insert("window",identity(w->window()));
        result.insert("parent",identity(w->parentWidget()));
        result.insert("visible",w->isVisible());
        result.insert("isWindow",w->isWindow());
        result.insert("hasFocus",w->hasFocus());
        // effectiveWinId reads existing native identity; never force winId().
        result.insert("existingEffectiveWinId",pointer(reinterpret_cast<void *>(w->effectiveWinId())));
    }
    return result;
}
static QString outputDirectory()
{
    auto override=qEnvironmentVariable("UNBOUND_AUDITION_TRACE_DIR");
    if (!override.isEmpty()) return override;
    HMODULE module=nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&outputDirectory),&module);
    wchar_t path[32768]; const auto length=GetModuleFileNameW(module,path,32768);
    return QDir(QString::fromWCharArray(path,int(length)).section('\\',0,-2)).filePath("logs");
}
AuditionTrace::AuditionTrace(QObject *parent):QObject(parent)
{
    m_deadline.setSingleShot(true);
    m_deadline.setInterval(180000);
    connect(&m_deadline,&QTimer::timeout,this,[this]{finish("time-limit");});
    connect(qApp,&QCoreApplication::aboutToQuit,this,[this]{finish("application-exit");});
}
bool AuditionTrace::guiThread()
{
    if (QThread::currentThread()==qApp->thread()) return true;
    ++m_offThread;
    return false;
}
bool AuditionTrace::begin(bool observe)
{
    if (QByteArray(qVersion())!="5.15.2") return false;
    if (m_active) { checkpoint(); return true; }
    m_records.clear(); m_connections.clear(); m_players.clear(); m_managers.clear();
    m_main=nullptr; m_page=nullptr; m_slot=nullptr;
    m_count=0; m_checkpoint=0; m_offThread=0; m_clock.start();
    const auto directory=outputDirectory();
    if (!QDir().mkpath(directory)) return false;
    m_file=QDir(directory).filePath(QStringLiteral("focus-%1-%2.jsonl").arg(GetCurrentProcessId())
        .arg(QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss-zzz")));
    QFile file(m_file);
    if (!file.open(QIODevice::WriteOnly|QIODevice::NewOnly)) return false;
    file.close(); m_active=true;
    record("start",{{"utc",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
        {"pid",int(GetCurrentProcessId())},{"qt",qVersion()},{"observing",observe},
        {"recordLimit",recordLimit},{"durationMs",m_deadline.interval()}});
    discover(observe);
    snapshot(observe ? "start" : "inventory");
    if (observe) {
        m_connections.append(connect(qApp,&QApplication::focusChanged,this,[this](QWidget *old,QWidget *now) {
            record("qt-focus",{{"old",identity(old)},{"new",identity(now)},{"state",state()}});
        }));
        qApp->installEventFilter(this); m_deadline.start(); flush();
    } else finish("inventory-complete");
    return true;
}
void AuditionTrace::discover(bool observe)
{
    // Explicit start only: widgets plus parent/event-filter references expose
    // parentless managers. Read-only Qt 5.15.2 private filter enumeration; no
    // Resolve offsets or private calls. No playback/paint-driven discovery.
    QList<QObject *> pending{qApp};
    for (auto w:QApplication::allWidgets()) pending.append(w);
    pending.append(qApp->children());
    QSet<QObject *> seen;
    while (!pending.isEmpty() && seen.size()<inventoryLimit) {
        auto object=pending.takeLast();
        if (seen.contains(object)) continue;
        seen.insert(object);
        if (object->thread()!=qApp->thread()) continue;
        if (object->parent()) pending.append(object->parent());
        const auto privateData=QObjectPrivate::get(object);
        if (privateData->extraData) for (const auto &filter:privateData->extraData->eventFilters)
            if (filter) pending.append(filter.data());
        if (object->inherits("UiFocusedWidgetManager")) m_managers.append(object);
        auto widget=qobject_cast<QWidget *>(object);
        if (!widget) continue;
        if (widget->objectName()=="UiMainWindow") m_main=widget;
        if (widget->objectName()=="WStackPage_Audio") m_page=widget;
        if (widget->objectName()=="frameAudioPanelMediaPoolContainer") m_slot=widget;
        if (widget->inherits("UiPlayerWidgetImp")) m_players.append(widget);
    }
    // Restrict player signals to the two named Fairlight players. The page may
    // already contain a parented top-level host; use QObject ancestry here.
    QList<QPointer<QWidget>> selected;
    for (auto w:m_players) {
        bool fairlight=false;
        for (auto parent=w.data();parent;parent=parent->parentWidget()) if (parent==m_page) fairlight=true;
        if (fairlight && (w->objectName()=="previewPlayer" || w->objectName()=="widgetAudioPagePlayer")) selected.append(w);
    }
    m_players=selected;
    record("discovery",{{"objects",seen.size()},{"truncated",!pending.isEmpty()},
        {"managers",m_managers.size()},{"players",m_players.size()}});
    auto metadata=[this](QObject *object) {
        QJsonArray signalNames;
        const auto meta=object->metaObject();
        for (int i=0;i<meta->methodCount();++i) {
            auto method=meta->method(i);
            if (method.methodType()==QMetaMethod::Signal &&
                (method.name()=="FocusChanged" || method.name()=="FocusReceived" ||
                 method.name()=="IsActiveChanged" || method.name()=="MuteAudioChanged" ||
                 method.name()=="TransportControlTriggered"))
                signalNames.append(QString::fromLatin1(method.methodSignature()));
        }
        record("object-signals",{{"object",identity(object)},{"signals",signalNames}});
    };
    for (auto manager:m_managers) {
        metadata(manager);
        if (observe) connectSignal(manager,"FocusChanged(int,int)","contextChanged(int,int)");
    }
    for (auto player:m_players) {
        metadata(player);
        if (!observe) continue;
        connectSignal(player,"IsActiveChanged(bool)","playerBool(bool)");
        connectSignal(player,"MuteAudioChanged(bool)","playerBool(bool)");
        // Prefix-compatible zero-argument slots observe custom-enum signals
        // without guessing enum ABI/values or adding private type registrations.
        connectSignal(player,"FocusReceived(FrameRequestEventSource,bool)","playerEvent()");
        connectSignal(player,"TransportControlTriggered(TransportControlMode,bool)","playerEvent()");
    }
}
void AuditionTrace::connectSignal(QObject *object,const char *signature,const char *slot)
{
    const int signalIndex=object->metaObject()->indexOfSignal(signature);
    const int slotIndex=metaObject()->indexOfSlot(slot);
    QMetaObject::Connection connection;
    if (signalIndex>=0 && slotIndex>=0 && object->thread()==qApp->thread())
        connection=QObject::connect(object,object->metaObject()->method(signalIndex),this,
            metaObject()->method(slotIndex),Qt::DirectConnection);
    m_connections.append(connection);
    record("connection",{{"object",identity(object)},{"signature",signature},{"connected",bool(connection)}});
}
QJsonObject AuditionTrace::state() const
{
    QJsonArray players;
    for (auto w:m_players) if (w) {
        auto item=widgetState(w);
        item.insert("pageIsAncestor",m_page && m_page->isAncestorOf(w));
        item.insert("slotIsAncestor",m_slot && m_slot->isAncestorOf(w));
        item.insert("mainIsWindow",m_main && w->window()==m_main);
        players.append(item);
    }
    return {{"focus",identity(QApplication::focusWidget())},
        {"activeWindow",identity(QApplication::activeWindow())},{"players",players}};
}
void AuditionTrace::record(const QString &kind,QJsonObject data)
{
    if (!m_active || m_finishing) return;
    if (m_count>=recordLimit) { finish("record-limit"); return; }
    data.insert("kind",kind); data.insert("n",m_count++); data.insert("ms",double(m_clock.elapsed()));
    m_records.append(QJsonDocument(data).toJson(QJsonDocument::Compact)+'\n');
}
void AuditionTrace::snapshot(const QString &label)
{
    record("checkpoint",{{"label",label},{"state",state()},
        {"main",identity(m_main)},{"page",identity(m_page)},{"slot",identity(m_slot)}});
}
void AuditionTrace::checkpoint()
{
    if (m_active) { snapshot(QStringLiteral("mark-%1").arg(++m_checkpoint)); flush(); }
}
void AuditionTrace::flush()
{
    if (m_records.isEmpty()) return;
    QFile file(m_file);
    if (!file.open(QIODevice::WriteOnly|QIODevice::Append)) return;
    for (const auto &line:m_records) if (file.write(line)!=line.size()) return;
    m_records.clear();
}
void AuditionTrace::finish(const QString &reason)
{
    if (!m_active || m_finishing) return;
    m_finishing=true; m_deadline.stop(); qApp->removeEventFilter(this);
    for (const auto &connection:m_connections) disconnect(connection);
    m_connections.clear();
    m_records.append(QJsonDocument(QJsonObject{{"kind","stop"},{"reason",reason},{"n",m_count},
        {"ms",double(m_clock.elapsed())},{"offThreadSignalsSkipped",m_offThread.load()}}).toJson(QJsonDocument::Compact)+'\n');
    flush(); m_active=false; m_finishing=false;
}
void AuditionTrace::contextChanged(int previous,int current)
{
    if (!guiThread()) return;
    record("resolve-context",{{"sender",identity(sender())},{"old",previous},{"new",current},{"state",state()}});
}
void AuditionTrace::playerBool(bool value)
{
    if (!guiThread()) return;
    auto source=sender(); if (!source) return;
    record("player-bool",{{"sender",identity(source)},
        {"signal",QString::fromLatin1(source->metaObject()->method(senderSignalIndex()).methodSignature())},
        {"value",value},{"state",state()}});
}
void AuditionTrace::playerEvent()
{
    if (!guiThread()) return;
    auto source=sender(); if (!source) return;
    record("player-event",{{"sender",identity(source)},
        {"signal",QString::fromLatin1(source->metaObject()->method(senderSignalIndex()).methodSignature())},
        {"state",state()}});
}
bool AuditionTrace::eventFilter(QObject *object,QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress: case QEvent::FocusIn: case QEvent::FocusOut:
    case QEvent::WindowActivate: case QEvent::WindowDeactivate:
    case QEvent::ParentAboutToChange: case QEvent::ParentChange:
    case QEvent::Show: case QEvent::Hide: case QEvent::KeyPress: break;
    default: return false;
    }
    if (!object->isWidgetType()) return false;
    auto widget=static_cast<QWidget *>(object);
    bool scoped=widget==m_main;
    for (auto p=widget;p;p=p->parentWidget()) if (p==m_page) {scoped=true;break;}
    if (!scoped) return false;
    if (event->type()==QEvent::KeyPress && static_cast<QKeyEvent *>(event)->key()!=Qt::Key_Space) return false;
    record("widget-event",{{"object",identity(widget)},{"event",int(event->type())},{"state",state()}});
    return false;
}

extern "C" __declspec(dllexport) DWORD WINAPI UnboundStart(void *argument)
{
    auto app=qobject_cast<QApplication *>(QCoreApplication::instance());
    const auto operation=quintptr(argument);
    if (!app || QCoreApplication::closingDown() || operation<1 || operation>4) return 0;
    return QMetaObject::invokeMethod(app,[operation] {
        static QPointer<AuditionTrace> trace;
        if (!trace) trace=new AuditionTrace(qApp);
        if (operation==1) trace->begin();
        else if (operation==2) trace->checkpoint();
        else if (operation==3) trace->finish("user-stop");
        else if (!trace->active()) trace->begin(false);
    },Qt::QueuedConnection) ? 1 : 0;
}
