#include <QCoreApplication>
#include <cstdio>
#include "panelplacement.h"
static int failures=0;
static void check(bool result,const char *message) { if (!result) { ++failures; std::fprintf(stderr,"FAIL: %s\n",message); } }
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv); Unbound::PanelPresentation panel;
    check(panel.move(Unbound::PanelPlacement::FloatingSurface,"group-1"),"panel enters a named floating surface");
    const auto before=panel.snapshot(); panel.visible=false; panel.expanded=true; panel.expanded=false; panel.visible=true;
    check(panel.snapshot()==before,"hide/reveal and temporary expansion preserve durable surface placement");
    check(panel.move(Unbound::PanelPlacement::NativeSurface,"edit-custom-root"),"managed native layout is distinct from floating and original native slot");
    const auto native=panel.snapshot();
    check(!panel.move(Unbound::PanelPlacement::FloatingSurface) && panel.snapshot()==native,"invalid destination cannot erase remembered placement");
    check(panel.move(Unbound::PanelPlacement::NativeSlot) && panel.surface.isEmpty(),"restoring native slot clears custom surface identity");
    if (!failures) std::puts("PASS: panel ownership/placement separation, hide/reveal and expansion state invariants");
    return failures ? 1 : 0;
}
