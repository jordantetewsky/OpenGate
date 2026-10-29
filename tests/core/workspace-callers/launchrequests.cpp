// SPDX-License-Identifier: MIT
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QSaveFile>
#include <QElapsedTimer>
#include <QThread>
#include <cstdio>
#include "../../../src/core/controllaunchrequests.h"
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);QTemporaryDir directory;
    qputenv("UNBOUND_DIAGNOSTICS_DIR",directory.path().toUtf8());
    int failures=0,shown=0;
    auto check=[&](bool ok,const char *why){if(!ok){++failures;std::fprintf(stderr,"FAIL: %s\n",why);}};
    const auto path=directory.filePath("show-controls.request");
    auto write=[&](const QByteArray &token){QSaveFile file(path);check(file.open(QIODevice::WriteOnly),"request opens");file.write(token);check(file.commit(),"request publishes atomically");};
    auto wait=[&]{QElapsedTimer timer;timer.start();while(QFile::exists(path)&&timer.elapsed()<2000){app.processEvents();QThread::msleep(1);}app.processEvents();};
    write("previous-session");
    {QFile file(path);check(file.open(QIODevice::ReadWrite),"stale request opens");check(file.setFileTime(QDateTime::currentDateTimeUtc().addSecs(-60),QFileDevice::FileModificationTime),"stale timestamp applies");}
    Unbound::ControlLaunchRequests requests(&app,directory.path(),[&]{++shown;});
    requests.consume(true);
    check(shown==0&&!QFile::exists(path),"stale startup request is consumed without showing controls");
    write("explicit-launch");wait();
    check(shown==1&&!QFile::exists(path),"filesystem wake consumes an explicit launch exactly once");
    write("explicit-launch");wait();
    check(shown==1&&!QFile::exists(path),"duplicate launch token cannot reopen controls");
    write("second-launch");wait();
    check(shown==2&&!QFile::exists(path),"a later launch reuses the same dispatch callback");
    write("");wait();check(shown==2&&!QFile::exists(path),"empty request is consumed without dispatch");
    write("recent-startup");
    {Unbound::ControlLaunchRequests startup(nullptr,directory.path(),[&]{++shown;});startup.consume(true);}
    check(shown==3&&!QFile::exists(path),"explicit launch immediately before bootstrap is honored");
    Unbound::flushDiagnostics();std::printf("Control launch request failures=%d\n",failures);return failures?1:0;
}
