#include <QApplication>
#include <QTemporaryDir>
#include <cstdio>
#include "resolvelayout.h"
#include "nativearrangementplan.h"
#include "arrangementstore.h"
#include "../../../src/core/bundledpresetseed.h"
int main(int argc,char **argv){
 QApplication app(argc,argv);QTemporaryDir dir;qputenv("UNBOUND_ARRANGEMENTS_FILE",(dir.path()+"/arrangements.json").toUtf8());
 const auto bundled=dir.path()+"/bundle.json",destination=dir.path()+"/arrangements.json";
 QFile file(bundled);if(!file.open(QIODevice::WriteOnly))return 1;
 file.write(R"({"arrangements":[{"id":"default","read_only":true},{"id":"example","name":"Example Preset","workspace":{"page":"edit"}}]})");file.close();
 if(!Unbound::seedBundledPresets(destination,bundled).isEmpty())return 1;
 QFile seeded(destination);if(!seeded.open(QIODevice::ReadOnly))return 1;
 const auto records=QJsonDocument::fromJson(seeded.readAll()).object()["arrangements"].toArray();seeded.close();
 if(records.size()!=1||records[0].toObject()["id"]!="example")return 1;
 if(!seeded.open(QIODevice::WriteOnly|QIODevice::Truncate))return 1;
 seeded.write("existing user bytes");seeded.close();
 if(!Unbound::seedBundledPresets(destination,bundled).isEmpty()||!seeded.open(QIODevice::ReadOnly)||seeded.readAll()!="existing user bytes")return 1;
 seeded.close();QFile::remove(destination);
 const auto entry=Unbound::resolveLayoutEntry("edit");const auto value=entry["workspace"].toObject();const auto plan=value["native_arrangement"].toObject();
 QSet<QString> available;for(const auto &id:plan["panels"].toObject().keys())available.insert(id);
 Unbound::NativeArrangementPlan parsed;const auto error=Unbound::NativeArrangementPlan::parse(plan,"edit",available,parsed);
 if(!error.isEmpty()){std::fprintf(stderr,"FAIL: %s\n",qPrintable(error));return 1;}
 QStringList names;Unbound::groupTreeNames(parsed.tree,names);
 if(!parsed.managed||names.contains("unboundWorkspaceToolbar")||names.contains(Unbound::nativeRemainderId())||
    names.size()!=7||!names.contains("frameEditAudioMeter")) {
  std::fprintf(stderr,"FAIL: Resolve Layout must contain seven independent panels including Mixer, without bands or a native remainder\n");return 1;
 }
 Unbound::ArrangementStore store;const auto id=entry["id"].toString();
 if(store.update(id,value).isEmpty()||store.remove(id).isEmpty()||store.rename(id,"Changed").isEmpty())return 3;
 QString saved;if(!store.create("My workspace",value,saved).isEmpty()||!store.update(saved,value).isEmpty())return 4;
 if(!Unbound::resolveLayoutEntry("fusion").isEmpty())return 5;
 std::puts("PASS: existing emulator tree becomes band-free embedded recipe; Resolve Layout cannot be updated, renamed or deleted; user copy remains writable");return 0;
}
