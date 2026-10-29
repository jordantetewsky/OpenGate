// SPDX-License-Identifier: MIT
#include <QApplication>
#include <cstdio>
#include "nativesurfacedoors.h"
int main(int argc,char **argv) {
 QApplication app(argc,argv);int failures=0;
 auto check=[&](bool ok,const char *why){if(!ok){++failures;std::fprintf(stderr,"FAIL %s\n",why);}};
 QWidget main;main.setAttribute(Qt::WA_DontShowOnScreen);QWidget parent(&main),source(&parent),program(&parent);
 main.show();parent.show();source.show();program.show();
 Unbound::NativeSurfaceDoors doors;
 check(doors.set("source",&source,false) && source.isHidden(),"Hide changes native visibility");
 check(source.parentWidget()==&parent && !source.isWindow(),"Hide retains native ownership without host");
 check(doors.modified("source"),"native visibility change tracked");
 check(doors.set("source",&source,true) && !doors.modified("source"),"Show restores same native surface");
 parent.hide();source.hide();
 check(!doors.set("source",&source,true) && source.isHidden(),"Show under hidden owner fails without false success");
 parent.show();source.show();
 doors.set("source",&source,false);doors.set("program",&program,false);
 QWidget other(&main);program.setParent(&other);
 check(!doors.restore().isEmpty() && source.isHidden(),"owner mismatch rejects entire return before visibility changes");
 program.setParent(&parent);check(doors.restore().isEmpty() && !source.isHidden() && !program.isHidden(),"Native return restores original visibility and owners");
 auto temporary=new QWidget(&parent);temporary->show();doors.set("temporary",temporary,false);delete temporary;
 check(doors.restore().isEmpty(),"deleted native target is safe");
 std::printf("native surface doors failures=%d\n",failures);return failures?1:0;
}
