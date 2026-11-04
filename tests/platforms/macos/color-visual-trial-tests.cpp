// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QTemporaryDir>
#include <QFile>
#include <cstdio>
#include "colorvisualtrial.h"
#include "../../../src/core/colormanagerpanel.h"
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QTemporaryDir directory;
    int failures=0;
    auto check=[&](bool ok,const char *message) { if(!ok){++failures;std::fprintf(stderr,"FAIL: %s\n",message);} };
    qunsetenv("OPENGATE_EXPERIMENTAL_COLOR_MANAGER");
    qunsetenv("OPENGATE_EXPERIMENTAL_COLOR_PAINT");
    check(!Unbound::ColorVisualTrial::activate(directory.path()) &&
          qEnvironmentVariableIsEmpty("OPENGATE_EXPERIMENTAL_COLOR_MANAGER"),
          "missing trial profile cannot partially activate features");
    QDir().mkpath(directory.path()+"/profile");
    {
        Unbound::ColorPalette palette(directory.path()+"/profile/Colors.json");
        auto preset=*palette.find("red");preset.name="Trial Red";
        check(palette.update(preset).isEmpty(),"fixture creates its isolated trial palette");
    }
    check(Unbound::ColorVisualTrial::activate(directory.path()),"trial activates without launch environment flags");
    check(qEnvironmentVariableIntValue("OPENGATE_EXPERIMENTAL_COLOR_MANAGER")==1 &&
          qEnvironmentVariableIntValue("OPENGATE_EXPERIMENTAL_COLOR_PAINT")==1 &&
          qEnvironmentVariableIntValue("OPENGATE_EXPERIMENTAL_MEDIA_POOL_COLORS")==0,
          "ordinary trial startup enables the intended manager and timeline paint only");
    Unbound::ColorManagerPanel manager;
    check(manager.palette().find("red") && manager.palette().find("red")->name=="Trial Red",
          "manager opens the isolated trial palette rather than the default palette");
    check(qEnvironmentVariable("UNBOUND_WORKSPACE_FILE")==directory.path()+"/profile/Workspace.json",
          "trial startup retains its independent workspace settings");
    std::printf("color visual trial failures=%d\n",failures);
    return failures?1:0;
}
