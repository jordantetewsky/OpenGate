// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QTemporaryDir>
#include <cstdio>
#include "arrangementcontrols.h"
int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QTemporaryDir temporary;
    qputenv("UNBOUND_ARRANGEMENTS_FILE", (temporary.path()+"/Arrangements.json").toUtf8());
    qputenv("UNBOUND_DIAGNOSTICS_DIR", temporary.path().toUtf8());
    Unbound::ArrangementStore store;
    QString edit, fusion;
    int failures = 0;
    const auto check = [&](bool pass, const char *message) {
        if (!pass) { ++failures; std::fprintf(stderr,"FAIL: %s\n",message); }
    };
    check(store.create("My workspace",{{"page","edit"}},edit).isEmpty(),"create Edit preset");
    check(store.create("My workspace",{{"page","fusion"}},fusion).isEmpty(),"same name allowed on Fusion");
    QWidget owner; owner.setAttribute(Qt::WA_DontShowOnScreen);
    auto layout = new QVBoxLayout(&owner);
    Unbound::ArrangementControls controls(&owner,layout,edit);
    int restores = 0,writes=0;
    controls.changed=[&]{++writes;};
    controls.restoreRequested = [&] { ++restores; };
    controls.report = [](QString) {};
    check(controls.selection()->count()==3 && controls.selection()->currentData()==edit,"Edit lists and selects only Edit preset");
    controls.updateState({{"page","edit"},{"layout_mode","native"}});
    check(owner.findChild<QLabel *>("arrangementState")->text()=="Matches saved arrangement","legacy preset comparison normalizes inferred layout mode");
    controls.setPage("fusion");
    check(controls.selection()->count()==2 && controls.selection()->findData(edit)<0 && controls.selection()->findData(fusion)>=0,"Fusion list excludes Edit");
    controls.invoke("Restore",edit);
    check(restores==0,"cross-page restore rejected");
    controls.invoke("Restore",fusion);
    check(restores==1,"matching-page restore dispatched");
    check(writes==0,"requesting a preset does not persist selection before workspace restoration completes");
    controls.setPage("edit");
    check(controls.selection()->currentData()==edit,"Edit selection retained independently");
    controls.setPage("fusion");
    check(controls.selection()->currentData()==fusion,"Fusion selection retained independently");
    controls.workspace = [] { return QJsonObject{{"page","edit"},{"wrong",true}}; };
    controls.invoke("Update",fusion);
    Unbound::ArrangementStore reread;
    check(!reread.find(fusion)["workspace"].toObject().contains("wrong"),"mismatched capture cannot update Fusion preset");
    return failures ? 1 : 0;
}
