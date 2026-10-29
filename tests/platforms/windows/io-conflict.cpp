// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QLibrary>
#include <QTemporaryDir>
#include <QThread>
#include <QWidget>
#include <cstdio>
#include "../../core/workspace-callers/fixtureevidence.h"
#include "../../../src/platforms/windows/iopluginabi.h"
static IOPlugin::StatusCode message(IOPlugin::MessageID,...){return IOPlugin::errNone;}
int main(int argc,char **argv) {
    QApplication app(argc,argv);if(argc!=3)return 1;
    QTemporaryDir files(OpenGateTest::evidenceTemplate("io-conflict"));OpenGateTest::retainEvidence(files);
    qputenv("UNBOUND_DIAGNOSTICS_DIR",files.path().toUtf8());
    const auto copy=files.path()+"/Unbound.dll";
    if(!QFile::copy(QString::fromLocal8Bit(argv[1]),copy))return 1;
    QLibrary prior(copy);prior.setLoadHints(QLibrary::PreventUnloadHint);
    QLibrary plugin(QString::fromLocal8Bit(argv[2]));plugin.setLoadHints(QLibrary::PreventUnloadHint);
    if(!prior.load()||!plugin.load())return 1;
    QWidget window;window.show();
    const auto init=reinterpret_cast<IOPlugin::pluginInitFunc>(plugin.resolve("pluginInit"));
    IOPlugin::APIContext host{IOPlugin::version,message},callbacks{};
    if(!init||init(&host,&callbacks)!=IOPlugin::errNone)return 1;
    bool rejected=false;QElapsedTimer clock;clock.start();
    while(clock.elapsed()<5000) {
        QApplication::processEvents();QThread::msleep(10);
        if(app.findChild<QObject *>("openGateManagedWorkspaceService"))return 1;
        QFile log(files.path()+"/launcher.jsonl");
        if(log.open(QIODevice::ReadOnly)&&log.readAll().contains("bootstrap.io.other-helper-loaded")){rejected=true;break;}
    }
    if(!rejected){std::fputs("FAIL: startup did not reject the different loaded helper\n",stderr);return 1;}
    std::puts("PASS: automatic startup rejects another installation's helper without starting its controller");return 0;
}
