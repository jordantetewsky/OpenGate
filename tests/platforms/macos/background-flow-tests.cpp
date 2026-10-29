// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QTemporaryDir>
#include <cstdio>
#include "panelmanagerview.h"
#include "workspacesnapshot.h"
int main(int argc,char **argv) {
    QApplication app(argc,argv);QTemporaryDir files;
    qputenv("UNBOUND_DIAGNOSTICS_DIR",files.path().toUtf8());
    qputenv("UNBOUND_ARRANGEMENTS_FILE",(files.path()+"/Arrangements.json").toUtf8());
    int failures=0;auto check=[&](bool value,const char *message){if(!value){++failures;std::fprintf(stderr,"FAIL: %s\n",message);}};
    for(const auto &legacy:QList<QJsonObject>{{},{ {"hide_background",true},{"native_workspace_mode","floating"}}}) {
        QWidget owner;owner.setAttribute(Qt::WA_DontShowOnScreen);
        Unbound::PanelManagerView view(&owner,legacy);
        check(!view.background->isChecked(),"ordinary and legacy arrangements use automatic shell policy");
        check(view.background->parentWidget()->isHidden(),"manual background override is outside the normal manager controls");
    }
    Unbound::WorkspaceSnapshot snapshot;snapshot.hideBackground=true;
    const auto saved=snapshot.json();
    check(saved["background_policy"]=="manual-hidden","explicit manual override has a distinct persisted policy");
    QWidget owner;owner.setAttribute(Qt::WA_DontShowOnScreen);
    Unbound::PanelManagerView view(&owner,saved);
    check(view.background->isChecked(),"explicit override survives reopening without conflating it with an arrangement");
    snapshot.hideBackground=false;
    check(snapshot.json()["background_policy"]=="automatic","clearing override returns to automatic handling");
    std::printf("background flow failures=%d\n",failures);return failures?1:0;
}
