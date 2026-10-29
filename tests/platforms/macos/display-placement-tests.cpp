// SPDX-License-Identifier: MIT
#include <QCoreApplication>
#include <cstdio>
#include "../../../src/core/displayplacement.h"
int main(int argc,char **argv){
 QCoreApplication app(argc,argv);int errors=0;
 auto check=[&](bool value,const char *why){if(!value){++errors;std::fprintf(stderr,"FAIL: %s\n",why);}};
 using namespace Unbound;
 QList<WorkspaceDisplay> screens{{"left","a","maker","model",{-1920,0,1920,1080},false},{"center","b","maker","model",{0,30,2560,1410},true},{"right","c","maker","model",{2560,-200,1920,1080},false},{"above","d","maker","model",{0,-1200,1920,1200},false}};
 const QRect frame(2700,-100,900,600);const auto saved=DisplayPlacement::capture(frame,screens);
 check(DisplayPlacement::resolve(saved,screens)==frame,"four-display placement retains negative coordinates and target screen");
 auto reordered=screens;std::swap(reordered[0],reordered[2]);check(DisplayPlacement::resolve(saved,reordered)==frame,"enumeration order does not change placement");
 auto moved=screens;moved[2].available.moveTopLeft({-4000,100});
 check(DisplayPlacement::resolve(saved,moved)==QRect(-3860,200,900,600),"display relocation preserves logical position relative to that display");
 auto smaller=screens;smaller[2].available={2560,0,640,480};auto fitted=DisplayPlacement::resolve(saved,smaller);
 check(smaller[2].available.contains(fitted)&&fitted.size()==QSize(640,480),"changed display resolution keeps complete window reachable");
 QList<WorkspaceDisplay> missing{screens[1]};const auto fallback=DisplayPlacement::resolve(saved,missing);
 check(missing[0].available.contains(fallback),"missing display falls back to available work area");
 const auto preserved=DisplayPlacement::capture(fallback,missing,saved);
 check(preserved==saved&&DisplayPlacement::resolve(preserved,screens)==frame,"automatic fallback saves preserve original placement for reconnect");
 auto adjusted=saved;adjusted["display_fallback"]=DisplayPlacement::rect(fallback.translated(0,22));
 check(DisplayPlacement::capture(fallback.translated(0,22),missing,adjusted)==adjusted,"native title-frame adjustment is remembered as fallback, not mistaken for a user move");
 const auto partial=DisplayPlacement::capture(QRect(10,20,400,300),screens);
 check(DisplayPlacement::resolve(partial,screens)==QRect(10,20,400,300),"unchanged display topology preserves intentional partial offscreen placement");
 const auto deliberate=DisplayPlacement::capture(fallback.translated(-10,-10),missing,saved);
 check(deliberate["display"].toObject()["serial"]=="b","intentional move on fallback display records the new placement");
 const auto old=DisplayPlacement::rect(QRect(-1800,100,500,500));
 check(DisplayPlacement::resolve(old,screens)==QRect(-1800,100,500,500),"legacy absolute records on nonprimary screen stay there");
 check(DisplayPlacement::resolve(saved,{})==frame,"no screen inventory preserves legacy coordinates without fabricating a display");
 auto duplicates=screens;for(auto &s:duplicates){s.serial.clear();s.name="generic";}
 const auto dup=DisplayPlacement::capture(frame,duplicates);check(DisplayPlacement::resolve(dup,duplicates)==frame,"indistinguishable display identities use saved geometry as deterministic tie breaker");
 if(!errors)std::puts("PASS: n-display identity, moved/negative coordinates, resolution changes, fallback preservation and reconnect");return errors?1:0;
}
