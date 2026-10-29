#include <QApplication>
#include <QHBoxLayout>
#include <QTemporaryDir>
#include <cstdio>
#include "vacatedcontents.h"
int main(int argc,char **argv) {
 QTemporaryDir logs;qputenv("UNBOUND_LOG_DIR",logs.path().toUtf8());QApplication app(argc,argv);
 QWidget main;main.setAttribute(Qt::WA_DontShowOnScreen);auto row=new QHBoxLayout(&main);
 auto slot=new QWidget(&main);slot->setMinimumSize(600,200);row->addWidget(slot);auto survivor=new QWidget(&main);row->addWidget(survivor);
 main.resize(1000,500);main.show();app.processEvents();const auto minimum=slot->minimumSize();
 Unbound::VacatedContents lease;lease.hide(slot);app.processEvents();
 for(int i=0;i<4;++i){slot->show();app.processEvents();if(!slot->isHidden()){std::fprintf(stderr,"FAIL: native Show reopened leased slot\n");return 1;}}
 main.hide();slot->show();app.processEvents();if(!slot->isHidden())return 2;main.show();app.processEvents();if(!slot->isHidden())return 3;
 lease.restore();app.processEvents();if(slot->isHidden() || slot->minimumSize()!=minimum){std::fprintf(stderr,"FAIL: exact restore\n");return 1;}
 slot->hide();slot->show();if(slot->isHidden())return 1;
 std::puts("PASS: repeated native Show cannot reserve empty leased slot; restore releases visibility guard");return 0;
}
