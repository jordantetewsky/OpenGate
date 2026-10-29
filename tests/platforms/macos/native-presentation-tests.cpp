// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTemporaryDir>
#include <cstdio>
#include "nativevacancycollapse.h"
#include "nativeworkspacepresentation.h"
int main(int argc,char **argv) {
    QApplication app(argc,argv); QTemporaryDir logs; qputenv("UNBOUND_DIAGNOSTICS_DIR",logs.path().toUtf8());
    int failures=0; auto check=[&](bool ok,const char *why) { if (!ok) { ++failures; std::fprintf(stderr,"FAIL: %s\n",why); } };
    QWidget main; main.setAttribute(Qt::WA_DontShowOnScreen); auto outer=new QVBoxLayout(&main);
    auto page=new QWidget(&main); outer->addWidget(page); auto layout=new QVBoxLayout(page);
    auto branch=new QWidget(page); auto row=new QHBoxLayout(branch); layout->addWidget(branch);
    auto vacancy=new QWidget(branch); row->addWidget(vacancy);
    auto survivor=new QPushButton("Retained real control",branch); row->addWidget(survivor);
    main.show(); QApplication::processEvents(); vacancy->hide();
    Unbound::NativeVacancyCollapse collapse; collapse.update(page,{vacancy});
    check(!branch->isHidden(),"native branch with a surviving control stays visible");
    survivor->hide(); collapse.update(page,{vacancy});
    check(branch->isHidden(),"empty native ancestor collapses without hiding the page");
    survivor->show(); collapse.update(page,{vacancy});
    check(!branch->isHidden() && survivor->isVisible(),"native control reopening restores its collapsed ancestor");
    survivor->hide(); collapse.update(page,{vacancy}); collapse.restore();
    check(!branch->isHidden() && survivor->isHidden(),"restoration preserves user-hidden native children");
    branch->hide(); collapse.update(page,{vacancy}); collapse.restore();
    check(branch->isHidden(),"preexisting hidden native branches are never shown by recovery");
    QWidget floating; Unbound::NativeWorkspacePresentation presentation; int writes=0; bool hidden=false;
    presentation.visibility=[&](QWidget *,const QList<QWidget *> &,bool hide) { ++writes; hidden=hide; return true; };
    presentation.update(&main,true,true,false,true,1,{&floating},false);
    check(writes==0,"partially native workspace remains the real native window");
    presentation.update(&main,true,true,false,false,3,{&floating},false);
    check(hidden && writes==1,"last editing surface leaving automatically retires empty native shell");
    presentation.update(&main,true,true,false,false,3,{&floating},false);
    check(writes==1,"unchanged shell state does not blink or reorder native windows");
    presentation.update(&main,true,true,false,true,3,{&floating},false);
    check(!hidden && writes==2,"returning native content restores the original shell");
    presentation.update(&main,true,true,false,false,3,{&floating},false);
    presentation.update(&main,true,false,false,false,3,{&floating},false);
    check(!hidden,"leaving Edit restores native shell for other pages");
    presentation.update(&main,true,true,true,false,3,{&floating},false);
    check(!hidden,"native arrangement surface is never mistaken for empty background");
    std::printf("native presentation failures=%d\n",failures); return failures ? 1 : 0;
}
