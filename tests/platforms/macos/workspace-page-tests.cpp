// SPDX-License-Identifier: MIT
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cstdio>
#include "arrangementstore.h"
int main(int argc,char **argv)
{
    QCoreApplication app(argc,argv); QTemporaryDir files; int failures = 0;
    const auto check = [&](bool pass,const char *message) { if (!pass) { ++failures; std::fprintf(stderr,"FAIL: %s\n",message); } };
    const auto path = files.path()+"/Arrangements.json";
    qputenv("UNBOUND_ARRANGEMENTS_FILE",path.toUtf8());
    const QByteArray legacy = R"({"version":1,"arrangements":[{"id":"legacy-edit","name":"the support team","workspace":{"version":1}}]})";
    QFile file(path); check(file.open(QIODevice::WriteOnly),"open legacy fixture"); file.write(legacy); file.close();
    Unbound::ArrangementStore store;
    check(store.error().isEmpty() && store.list("edit").size() == 1 && store.find("legacy-edit")["name"] == "the support team",
        "existing untagged presets retain their IDs and become Edit presets without rewriting the file");
    file.open(QIODevice::ReadOnly); check(file.readAll() == legacy,"page migration is non-destructive on load"); file.close();
    QString color,other,duplicate;
    check(store.create("the support team",{{"version",1},{"page","color"}},color).isEmpty(),"the same workspace name is allowed on another page");
    check(!store.create("THE SUPPORT TEAM",{{"version",1}},duplicate).isEmpty(),"names remain unique within their page");
    check(store.create("Second Edit",{{"version",1},{"page","edit"}},other).isEmpty(),"create another Edit layout");
    const auto colorBefore = store.find(color);
    check(!store.update(color,{{"version",2},{"page","edit"}}).isEmpty() && store.find(color) == colorBefore,
        "updating from Edit cannot overwrite a Color preset");
    check(store.update(color,{{"version",2},{"page","color"}}).isEmpty(),"same-page updates remain valid");
    check(store.reorderPage("edit",{other,"legacy-edit"}).isEmpty() && store.list()[1].toObject()["id"] == color,
        "reordering Edit presets preserves another page's stored position");
    check(!store.reorderPage("edit",{other,color}).isEmpty(),"page ordering rejects IDs from another page");
    Unbound::ArrangementStore reloaded;
    check(reloaded.error().isEmpty() && reloaded.list("edit")[0].toObject()["id"] == other && reloaded.list("color").size() == 1,
        "page tags, stable IDs and per-page ordering survive reload");
    check(Unbound::workspacePageLabel("edit") == "Edit" && Unbound::workspacePageLabel("fairlight") == "Fairlight" &&
        Unbound::workspacePageLabel("unknown").isEmpty(),"page labels do not guess unknown page identities");
    if (!failures) std::puts("PASS: legacy Edit migration, page isolation, stable IDs and per-page preset ordering (no GUI windows)");
    return failures ? 1 : 0;
}
