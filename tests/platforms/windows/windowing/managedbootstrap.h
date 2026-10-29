// SPDX-License-Identifier: MIT
#pragma once
#include <QLibrary>
#include <QPointer>
#include <windows.h>
#include <QFileInfo>
#include "../../../../src/platforms/windows/iopluginabi.h"
// Reuse the assembled flow's assertions, but obtain its controller through the
// shipped DLL's real launcher entry. No fixture-only production export is used.
class ManagedBootstrapFixture {
public:
    explicit ManagedBootstrapFixture(const QString &path):library(qEnvironmentVariableIntValue("OPENGATE_TEST_IO_BOOTSTRAP")?QFileInfo(path).dir().filePath("OpenGate.dvcp"):path) {
        library.setLoadHints(QLibrary::PreventUnloadHint);
        if(!library.load()){error=library.errorString();return;}
        if(qEnvironmentVariableIntValue("OPENGATE_TEST_IO_BOOTSTRAP")) {
            const auto init=reinterpret_cast<IOPlugin::pluginInitFunc>(library.resolve("pluginInit"));
            IOPlugin::APIContext host{},plugin{};host.version=IOPlugin::version;host.pHandleMessage=hostMessage;
            if(!init||init(nullptr,&plugin)!=IOPlugin::errInvalidParam){error="IO bootstrap validation failed";return;}
            host.version=0;if(init(&host,&plugin)!=IOPlugin::errVersion){error="IO bootstrap version validation failed";return;}
            host.version=IOPlugin::version;
            if(init(&host,&plugin)!=IOPlugin::errNone||!plugin.pHandleMessage||init(&host,&plugin)!=IOPlugin::errNone||
               plugin.pHandleMessage(IOPlugin::msgPluginStart)!=IOPlugin::errNone||
               plugin.pHandleMessage(IOPlugin::msgPluginGetInfo,&host)!=IOPlugin::errNone||properties!=2||
               plugin.pHandleMessage(IOPlugin::msgPluginListCodecs,nullptr)!=IOPlugin::errNone||
               plugin.pHandleMessage(IOPlugin::msgPluginListContainers,nullptr)!=IOPlugin::errNone||
               plugin.pHandleMessage(0xffffffff)!=IOPlugin::errUnsupported){error="IO bootstrap handshake/lifecycle failed";return;}
        } else {
            using Start=DWORD(WINAPI *)(void *);
            const auto start=reinterpret_cast<Start>(library.resolve("UnboundStart"));
            if(!start||start(nullptr)!=1||start(nullptr)!=1){error="Windows bootstrap failed or was not idempotent";return;}
        }
        QElapsedTimer timer;timer.start();
        while(timer.elapsed()<4000){
            QApplication::processEvents();
            service=qApp->findChild<QObject *>("openGateManagedWorkspaceService",Qt::FindDirectChildrenOnly);
            if(service)for(auto widget:QApplication::topLevelWidgets())
                if(widget->objectName()=="unboundPanelTest")controls=dynamic_cast<PanelControls *>(widget);
            if(controls)break;
            QThread::msleep(1);
        }
        if(!controls)error="Shipped DLL did not create the shared controller on the GUI thread";
        if(qApp->findChildren<QObject *>("openGateManagedWorkspaceService",Qt::FindDirectChildrenOnly).size()!=1)
            error="Repeated bootstrap created duplicate services";
    }
    ~ManagedBootstrapFixture(){delete service.data();}
    PanelControls *controller()const{return error.isEmpty()?controls:nullptr;}
    QString error;
private:
    inline static int properties=0;
    static IOPlugin::StatusCode hostMessage(IOPlugin::MessageID message,...) {
        if(message!=IOPlugin::msgPropSet)return IOPlugin::errUnsupported;
        va_list args;va_start(args,message);
        const auto object=va_arg(args,void *);const auto key=va_arg(args,const char *);
        const auto type=va_arg(args,int);const auto value=va_arg(args,const void *);const auto count=va_arg(args,int);va_end(args);
        if(!object||!value)return IOPlugin::errInvalidParam;
        if((!std::strcmp(key,"uuid")&&type==IOPlugin::propTypeUInt8&&count==16)||
           (!std::strcmp(key,"name")&&type==IOPlugin::propTypeString&&count>0&&
            !std::strncmp(static_cast<const char *>(value),"OpenGate by CineWaltz",20)))++properties;
        else return IOPlugin::errInvalidParam;
        return IOPlugin::errNone;
    }
    QLibrary library;
    QPointer<QObject> service;
    QPointer<PanelControls> controls;
};
