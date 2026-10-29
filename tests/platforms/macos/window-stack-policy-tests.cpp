// SPDX-License-Identifier: MIT
#include <QApplication>
#include <cstdio>
#include "windowstackpolicy.h"
int main(int argc,char **argv) {
    qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM","1");
    QApplication app(argc,argv); int failed=0;
    auto check=[&](bool ok,const char *why) { if (!ok) { ++failed; std::fprintf(stderr,"FAIL: %s\n",why); } };
    QWidget group; group.setObjectName("unboundFloating_group_media_source");
    QWidget timeline(&group); timeline.setObjectName("frameConformTimelines");
    QWidget inspector; inspector.setObjectName("unboundFloating_widgetEditInspector");
    check(Unbound::workspaceWindowTier(&group)<Unbound::workspaceWindowTier(&inspector),"Timeline group cannot cover separate Inspector on activation");
    timeline.setParent(nullptr); check(Unbound::workspaceWindowTier(&group)==1,"tier follows current members after tear-off");
    group.setProperty("unboundPinned",true); check(Unbound::workspaceWindowTier(&group)>Unbound::workspaceWindowTier(&inspector),"explicit group pin remains above utility windows");
    group.setProperty("unboundPinned",false); check(Unbound::workspaceWindowTier(&group)==1,"unpin restores the ordinary tier");
    if (!failed) std::puts("PASS: dynamic group membership, utility stacking and explicit pin tier"); return failed ? 1 : 0;
}
