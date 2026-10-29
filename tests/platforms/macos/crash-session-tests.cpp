#include <QApplication>
#include <QTemporaryDir>
#include "crashsession.h"
#include <cstdio>
int main(int argc,char **argv) {
 QApplication app(argc,argv);QTemporaryDir dir;qputenv("UNBOUND_DIAGNOSTICS_DIR",dir.path().toUtf8());
 auto now=QDateTime::currentDateTime();auto previous=QJsonObject{{"pid",1},{"started",now.addSecs(-30).toUTC().toString(Qt::ISODateWithMs)}};
 const auto report="----------------------------------------------------------------------\n#TIME "+QLocale::c().toString(now.addSecs(-10),"ddd MMM d HH:mm:ss yyyy")+" - Uptime 00:00:20\n3 libFairlightPage.dylib 0x123 event + 1\nSignal Number = 11\n";
 auto evidence=Unbound::CrashSession::assess(previous,report,now);bool ok=evidence["native_report_time_match"].toBool() && !evidence["helper_frame_present"].toBool() && evidence["attribution"]=="unknown";
 auto withHelper=Unbound::CrashSession::assess(previous,report+"4 UnboundWidgetProbe.dylib 0x456 operation + 2\n",now);
 ok &= withHelper["helper_frame_present"].toBool() && withHelper["attribution"]=="unknown";
 previous["started"]=now.addSecs(10).toString(Qt::ISODateWithMs);ok &= !Unbound::CrashSession::assess(previous,report,now)["native_report_time_match"].toBool();
 Unbound::CrashSession session(nullptr);Unbound::logEvent("workspace.edit.suspend.begin");
 QFile f(dir.path()+"/active-session.json");f.open(QIODevice::ReadOnly);ok &= QJsonDocument::fromJson(f.readAll()).object()["ownership_breadcrumbs"].toArray().size()==1;
 f.close();Unbound::logEvent("accessory.attach.request",{{"panel","nativeFooter"}});
 f.open(QIODevice::ReadOnly);auto marker=QJsonDocument::fromJson(f.readAll()).object();
 ok &= !marker["session"].toString().isEmpty() && marker["ownership_breadcrumbs"].toArray().size()==2;
 ok &= marker["ownership_breadcrumbs"].toArray().last().toObject()["event"]=="accessory.attach.request";
 std::printf("crash correlation %s\n",ok ? "PASS" : "FAIL");return ok ? 0 : 1;
}
