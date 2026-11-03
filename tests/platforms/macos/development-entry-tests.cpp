// SPDX-License-Identifier: MIT
#include <QCoreApplication>
#include <QEventLoop>
#include <cstdio>
#include "developmententry.h"
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);int failures=0;
    auto check=[&](bool ok,const char *why){if(!ok){++failures;std::fprintf(stderr,"FAIL %s\n",why);}};
    auto create=[](QObject *owner){auto service=new QObject(owner);service->setObjectName("unboundControlService");return service;};
    {
        QObject application;auto installed=create(&application);int calls=0;
        check(!OpenGateDevelopment::reserveEntry(&application,0,[&]{++calls;create(&application);}),"development entry refuses an already running installed service");
        app.processEvents();check(calls==0 && application.findChildren<QObject *>("unboundControlService").size()==1,"installed owner stays unique");
        delete installed;
    }
    {
        QObject application;int candidate=0,installed=0;
        check(OpenGateDevelopment::reserveEntry(&application,0,[&]{++candidate;create(&application);}),"development entry reserves startup before the installed delay");
        if(!application.findChild<QObject *>("unboundControlService")){++installed;create(&application);}
        check(!OpenGateDevelopment::reserveEntry(&application,0,[&]{++candidate;create(&application);}),"repeated probe entry does not schedule another controller");
        app.processEvents();check(candidate==1 && installed==0 && application.findChildren<QObject *>("unboundControlService").size()==1,"candidate startup constructs one service and installed startup yields");
    }
    {
        int calls=0;auto application=new QObject;
        OpenGateDevelopment::reserveEntry(application,0,[&]{++calls;});delete application;app.processEvents();
        check(calls==0,"shutdown cancels a pending development bootstrap");
    }
    std::fprintf(stderr,"development entry failures=%d\n",failures);return failures?1:0;
}
