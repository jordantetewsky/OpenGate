// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QTemporaryDir>
#include <QSaveFile>
#include "settingsprofile.h"
#include "starterworkspacegeometry.h"
#include <cstdio>
int main(int argc,char **argv) {
    QApplication app(argc,argv);QTemporaryDir temporary;
    auto marker=temporary.path()+"/Profile.json";qputenv("UNBOUND_PROFILE_FILE",marker.toUtf8());
    auto put=[&](const QString &name) { QSaveFile f(marker);f.open(QIODevice::WriteOnly);f.write(QJsonDocument(QJsonObject{{"active",name}}).toJson());f.commit(); };
    const auto normal=Unbound::settingsProfilePath("Workspace.json");put("first-install-preview");
    bool ok=Unbound::settingsProfilePath("Workspace.json").contains("/Profiles/first-install-preview/");
    put("../../outside");ok &= Unbound::settingsProfilePath("Workspace.json")==normal;
    put("");ok &= Unbound::settingsProfilePath("Arrangements.json").endsWith("/Unbound/Arrangements.json");
    auto value=Unbound::fitStarterWorkspace({{"fit_first_display",true},{"groups",QJsonArray{QJsonObject{{"x",99999},{"width",99999}}}}});
    auto group=value["groups"].toArray().first().toObject();auto frame=QGuiApplication::primaryScreen()->availableGeometry();
    ok &= group["x"].toInt()==frame.x() && group["width"].toInt()==frame.width();
    std::printf("settings profile %s\n",ok ? "PASS" : "FAIL");return ok ? 0 : 1;
}
