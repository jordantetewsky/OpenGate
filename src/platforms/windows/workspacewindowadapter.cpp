// SPDX-License-Identifier: MIT
#include <QAbstractNativeEventFilter>
#include <QApplication>
#include <QCursor>
#include <QPointer>
#include <QTimer>
#include "workspacewindowadapter.h"
#include "workspacetitleaccess.h"
#include "../../core/diagnostics.h"

namespace Unbound {
namespace {
HWND handle(QWidget *widget) { return widget?reinterpret_cast<HWND>(widget->winId()):nullptr; }
QJsonObject rectangle(const RECT &r) {return {{"x",int(r.left)},{"y",int(r.top)},{"width",int(r.right-r.left)},{"height",int(r.bottom-r.top)}};}
constexpr UINT pinCommand=0x1f10;
class CommandRequestWatch : public QObject,public QAbstractNativeEventFilter {
public:
    CommandRequestWatch(QObject *owner,std::function<void()> ready):QObject(owner),ready(std::move(ready)),messageId(RegisterWindowMessageW(L"OpenGate.WorkflowCommandReady.v1")){
        setProperty("openGateNativeWakeReady",messageId!=0);qApp->installNativeEventFilter(this);
    }
    ~CommandRequestWatch() override {if(qApp)qApp->removeNativeEventFilter(this);}
    bool nativeEventFilter(const QByteArray &,void *message,long *) override {
        const auto msg=static_cast<MSG *>(message);
        if(messageId&&msg->message==messageId&&!queued) {
            queued=true;QTimer::singleShot(0,this,[this]{queued=false;if(ready)ready();});
        }
        return false;
    }
private:std::function<void()> ready;UINT messageId=0;bool queued=false;
};
class PinWatch : public QObject,public QAbstractNativeEventFilter {
public:
    explicit PinWatch(QWidget *widget):QObject(widget),window(widget){setObjectName("openGateNativePinWatch");qApp->installNativeEventFilter(this);}
    ~PinWatch() override {if(qApp)qApp->removeNativeEventFilter(this);}
    std::function<void(bool)> changed;
    bool nativeEventFilter(const QByteArray &,void *message,long *result) override {
        auto msg=static_cast<MSG *>(message);
        if(!window||msg->hwnd!=handle(window)||msg->message!=WM_SYSCOMMAND||(msg->wParam&0xfff0)!=pinCommand)return false;
        const bool next=!window->property("unboundPinned").toBool();
        QTimer::singleShot(0,this,[this,next]{if(window&&changed)changed(next);});*result=0;return true;
    }
private:QPointer<QWidget> window;
};
class PointerReleaseWatch : public QObject,public QAbstractNativeEventFilter {
public:
    PointerReleaseWatch(QObject *owner,std::function<void(QPoint,bool)> callback):QObject(owner),callback(std::move(callback)){qApp->installNativeEventFilter(this);}
    ~PointerReleaseWatch() override {if(qApp)qApp->removeNativeEventFilter(this);}
    bool nativeEventFilter(const QByteArray &,void *message,long *) override {
        const auto msg=static_cast<MSG *>(message);
        if(delivered||(msg->message!=WM_LBUTTONUP&&msg->message!=WM_NCLBUTTONUP))return false;
        delivered=true;const auto position=QCursor::pos();const bool bypass=(GetAsyncKeyState(VK_MENU)&0x8000)!=0;
        QTimer::singleShot(0,this,[this,position,bypass]{if(callback)callback(position,bypass);});return false;
    }
private:std::function<void(QPoint,bool)> callback;bool delivered=false;
};
class WindowMoveWatch : public QObject,public QAbstractNativeEventFilter {
public:
    WindowMoveWatch(QWidget *window,std::function<void()> begin,std::function<void(QPoint)> moved,std::function<void(QPoint,bool)> ended)
        :QObject(window),window(window),begin(std::move(begin)),moved(std::move(moved)),ended(std::move(ended)){qApp->installNativeEventFilter(this);}
    ~WindowMoveWatch() override {if(qApp)qApp->removeNativeEventFilter(this);}
    bool nativeEventFilter(const QByteArray &,void *message,long *) override {
        const auto msg=static_cast<MSG *>(message);if(!window||msg->hwnd!=handle(window))return false;
        if(msg->message==WM_NCLBUTTONDOWN&&msg->wParam==HTCAPTION) {
            moving=true;if(begin)begin();
        } else if(moving&&msg->message==WM_WINDOWPOSCHANGED&&!moveQueued) {
            moveQueued=true;QTimer::singleShot(0,this,[this]{moveQueued=false;if(moving&&moved)moved(QCursor::pos());});
        } else if(moving&&(msg->message==WM_LBUTTONUP||msg->message==WM_NCLBUTTONUP||msg->message==WM_CAPTURECHANGED||msg->message==WM_CANCELMODE||msg->message==WM_EXITSIZEMOVE)) {
            moving=false;const bool bypass=msg->message==WM_CANCELMODE||(GetAsyncKeyState(VK_MENU)&0x8000)!=0;
            const auto position=QCursor::pos();QTimer::singleShot(0,this,[this,position,bypass]{if(ended)ended(position,bypass);});
        }
        return false;
    }
private:QPointer<QWidget> window;std::function<void()> begin;std::function<void(QPoint)> moved;std::function<void(QPoint,bool)> ended;bool moving=false,moveQueued=false;
};
}
void applyNativeChrome(QWidget *widget,bool) {
    if(auto host=dynamic_cast<WorkspaceHostWindow *>(widget))host->setNativeTitleBarEnabled(!widget->windowFlags().testFlag(Qt::FramelessWindowHint));
    if(widget)applyPaneDarkCaption(widget,widget->parentWidget());
}
bool setNativeHeaderMenu(QWidget *widget,bool enabled,const std::function<void()> &opened) {
    if(!widget)return false;
    auto access=dynamic_cast<WorkspaceTitleAccess *>(widget->findChild<QObject *>("openGateWorkspaceTitleAccess",Qt::FindDirectChildrenOnly));
    if(!access&&!enabled)return true;
    if(!access)access=new WorkspaceTitleAccess(widget);
    return access->update(enabled,opened);
}
int nativeChromeFlags(QWidget *widget) {
    if(!widget)return 0;const auto style=GetWindowLongPtr(handle(widget),GWL_STYLE);
    return ((style&WS_CAPTION)==WS_CAPTION?1:0)|((style&WS_THICKFRAME)?2:0);
}
QObject *watchNativeCommandRequests(QObject *owner,std::function<void()> ready){return new CommandRequestWatch(owner,std::move(ready));}
bool setNativePin(QWidget *widget,bool pinned,const std::function<void(bool)> &changed) {
    if(!widget||QGuiApplication::platformName()!=QLatin1String("windows"))return false;
    const auto native=handle(widget);auto menu=GetSystemMenu(native,FALSE);if(!menu)return false;
    auto watcher=dynamic_cast<PinWatch *>(widget->findChild<QObject *>("openGateNativePinWatch",Qt::FindDirectChildrenOnly));
    if(!watcher)watcher=new PinWatch(widget);watcher->changed=changed;
    if(GetMenuState(menu,pinCommand,MF_BYCOMMAND)==UINT(-1))AppendMenuW(menu,MF_STRING,pinCommand,L"Pin above Resolve");
    CheckMenuItem(menu,pinCommand,MF_BYCOMMAND|(pinned?MF_CHECKED:MF_UNCHECKED));
    const bool expanded=widget->property("openGateNativeExpansion").toBool();
    SetWindowPos(native,pinned||expanded?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
    return true; // Windows system-menu control replaces the Qt fallback pin.
}
void orderNativeWindows(const QList<QWidget *> &windows) {
    for(auto widget:windows)if(widget&&widget->isVisible()&&!widget->isMinimized())SetWindowPos(handle(widget),HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
}
qint64 nativeKeyWindowNumber(){return qint64(reinterpret_cast<quintptr>(GetForegroundWindow()));}
qint64 nativeWindowNumber(QWidget *widget){return qint64(reinterpret_cast<quintptr>(handle(widget)));}
QList<qint64> nativeOrderedWindowNumbers(){
    QList<qint64> result;
    EnumWindows([](HWND window,LPARAM data)->BOOL{DWORD pid=0;GetWindowThreadProcessId(window,&pid);if(pid==GetCurrentProcessId())reinterpret_cast<QList<qint64> *>(data)->append(qint64(reinterpret_cast<quintptr>(window)));return TRUE;},reinterpret_cast<LPARAM>(&result));
    return result;
}
QJsonObject nativeWindowGeometryState(QWidget *widget){
    if(!widget)return {};const auto native=handle(widget);RECT outer{},client{};POINT origin{};
    GetWindowRect(native,&outer);GetClientRect(native,&client);ClientToScreen(native,&origin);
    return {{"native_outer",rectangle(outer)},{"native_client",rectangle(client)},{"native_client_x",int(origin.x)},{"native_client_y",int(origin.y)},
        {"native_units","physical-pixels"},{"qt_units","logical-client"},{"dpi",int(GetDpiForWindow(native))},{"native_id",nativeWindowNumber(widget)}};
}
double nativeInputAgeMilliseconds(quint64){return -1;}
double nativeCurrentDragAgeMilliseconds(){return -1;}
QObject *watchNativePointerRelease(QObject *owner,std::function<void(QPoint,bool)> released){return new PointerReleaseWatch(owner,std::move(released));}
QObject *watchNativeWindowMove(QWidget *window,std::function<void()> begin,std::function<void(QPoint)> moved,std::function<void(QPoint,bool)> ended){return new WindowMoveWatch(window,std::move(begin),std::move(moved),std::move(ended));}
bool setNativeGroupSurfaceClipping(QWidget *widget){
    if(!widget)return false;const auto native=handle(widget);const auto style=GetWindowLongPtr(native,GWL_STYLE);
    SetWindowLongPtr(native,GWL_STYLE,style|WS_CLIPCHILDREN|WS_CLIPSIBLINGS);
    return (GetWindowLongPtr(native,GWL_STYLE)&(WS_CLIPCHILDREN|WS_CLIPSIBLINGS))==(WS_CLIPCHILDREN|WS_CLIPSIBLINGS);
}
void normalizeNativePanel(QWidget *widget){
    if(!widget||widget->windowType()!=Qt::Tool)return;
    const auto native=handle(widget);const auto style=GetWindowLongPtr(native,GWL_EXSTYLE);
    SetWindowLongPtr(native,GWL_EXSTYLE,(style&~LONG_PTR(WS_EX_TOOLWINDOW))|WS_EX_APPWINDOW);
}
void setNativeExpansionPriority(QWidget *widget,bool expanded){
    if(!widget)return;const auto native=handle(widget);
    if(expanded&&!widget->property("openGateNativeExpansion").toBool())widget->setProperty("openGateNativePriorTopmost",bool(GetWindowLongPtr(native,GWL_EXSTYLE)&WS_EX_TOPMOST));
    widget->setProperty("openGateNativeExpansion",expanded);
    const bool topmost=expanded||widget->property("unboundPinned").toBool()||widget->property("openGateNativePriorTopmost").toBool();
    SetWindowPos(native,topmost?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
    if(!expanded)widget->setProperty("openGateNativePriorTopmost",QVariant());
}
int nativeVisibilityFlags(QWidget *widget){const auto native=handle(widget);return (IsWindowVisible(native)?1:0)|(IsIconic(native)?4:0);}
bool setNativeBackgroundHidden(QWidget *main,const QList<QWidget *> &panels,bool hidden) {
    if(!main||QGuiApplication::platformName()!=QLatin1String("windows"))return false;
    const auto background=handle(main);
    if(hidden) {
        ShowWindow(background,SW_HIDE);
        // Windows hides owned HWNDs with their owner. Reveal only the retained
        // panels that were logically visible; hidden/minimized panels stay so.
        for(auto panel:panels)if(panel&&panel->isVisible()&&!panel->isMinimized())ShowWindow(handle(panel),SW_SHOWNOACTIVATE);
        for(auto panel:panels)if(panel&&panel->isVisible()&&!panel->isMinimized()&&!IsWindowVisible(handle(panel))) {
            ShowWindow(background,SW_SHOWNOACTIVATE);return false;
        }
    } else ShowWindow(background,SW_SHOWNOACTIVATE);
    main->setProperty("unboundBackgroundHidden",hidden);return true;
}
}
