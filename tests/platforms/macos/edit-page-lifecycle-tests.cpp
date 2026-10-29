// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QTemporaryDir>
#include <cstdio>
#include "editpagelifecycle.h"
int main(int argc,char **argv) {
    QApplication app(argc,argv); QTemporaryDir logs; qputenv("UNBOUND_DIAGNOSTICS_DIR",logs.path().toUtf8());
    int failures=0; auto check=[&](bool ok,const char *why) { if (!ok) { ++failures; std::fprintf(stderr,"FAIL: %s\n",why); } };
    QWidget main; main.setAttribute(Qt::WA_DontShowOnScreen); auto pages=new QStackedWidget(&main); (new QVBoxLayout(&main))->addWidget(pages);
    auto edit=new QWidget(pages); auto color=new QWidget(pages); pages->addWidget(edit); pages->addWidget(color); main.show();
    Unbound::EditPageLifecycle lifecycle; int snapshots=0,suspends=0,resumes=0; bool running=true; QString modified="latest unsaved view";
    lifecycle.running=[&] { return running; }; lifecycle.checkpoint=[&] { ++snapshots; check(modified=="latest unsaved view","leave snapshots latest modified state rather than named preset"); };
    lifecycle.suspend=[&] { ++suspends; }; lifecycle.resume=[&] { ++resumes; };
    lifecycle.observe(edit); QApplication::processEvents(); check(resumes==1,"initial visible Edit binds once");
    lifecycle.observe(edit); QApplication::processEvents(); check(resumes==1,"repeated discovery does not restore twice");
    pages->setCurrentWidget(color); check(snapshots==0 && suspends==0,"native Hide traversal never synchronously changes workspace ownership"); QApplication::processEvents(); check(snapshots==1 && suspends==1,"leaving Edit snapshots and suspends once");
    pages->setCurrentWidget(edit); QApplication::processEvents(); check(resumes==2 && modified=="latest unsaved view","return resumes retained view without loading preset");
    pages->setCurrentWidget(color); pages->setCurrentWidget(edit); pages->setCurrentWidget(color); QApplication::processEvents();
    check(resumes==2,"superseded rapid page return cannot restore Edit onto another page");
    pages->setCurrentWidget(edit); running=false; QApplication::processEvents(); check(resumes==2,"shutdown cancels queued presentation restoration");
    std::printf("Edit page lifecycle failures=%d\n",failures); return failures ? 1 : 0;
}
