// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QPushButton>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QThread>
#include <cstdio>
#include "pagepanelinventory.h"
class HintTrap : public QWidget {
public:
 mutable int hints=0;
 QSize minimumSizeHint() const override {++hints;return QSize(300,100);}
};
int main(int argc,char **argv) {
 QApplication app(argc,argv);QWidget page;page.setObjectName("WStackPage_Fusion");
 auto first=new HintTrap;first->setParent(&page);auto second=new QWidget(&page);
 auto a=new QPushButton("PRIVATE PROJECT TEXT",first);a->setObjectName("duplicate");auto b=new QPushButton("PRIVATE CLIP TEXT",second);b->setObjectName("duplicate");
 QWidget unrelated;unrelated.setObjectName("other-page");QWidget floating(&page,Qt::Window);floating.setObjectName("cross-page-window");
 const auto before=a->parentWidget();const auto inventory=Unbound::PagePanelInventory::capture(&page);
 const auto bytes=QJsonDocument(inventory).toJson();const auto nodes=inventory["nodes"].toArray();int duplicates=0;QSet<QString> parents;
 for(const auto &value:nodes){const auto row=value.toObject();if(row["name"]=="duplicate"){++duplicates;parents.insert(row["parent_id"].toString());}}
 bool ok=duplicates==2 && parents.size()==2 && !inventory["truncated"].toBool() && first->hints==0 && a->parentWidget()==before;
 ok &= !bytes.contains("PRIVATE") && !bytes.contains("other-page") && !bytes.contains("cross-page-window");
 const auto bounded=Unbound::PagePanelInventory::capture(&page,2);ok &= bounded["truncated"].toBool() && bounded["nodes"].toArray().size()==2;
 QTemporaryDir logs;qputenv("UNBOUND_DIAGNOSTICS_DIR",logs.path().toUtf8());
 QWidget main;main.setObjectName("UiMainWindow");main.setAttribute(Qt::WA_DontShowOnScreen);
 auto title=new QPushButton("PRIVATE TITLE",&main);title->setObjectName("UiProjectsMainBarImp");main.show();
 Unbound::PagePanelInventory windows(nullptr,Unbound::PagePanelInventory::Scope::MainWindow);
 QElapsedTimer clock;clock.start();while(clock.elapsed()<1100){QApplication::processEvents();QThread::msleep(1);}
 const QDir output(logs.path()+"/WindowInventory");const auto records=output.entryList({"*-UiMainWindow-*.json"},QDir::Files);
 ok &= records.size()==1;
 if(records.size()==1){QFile file(output.filePath(records.first()));ok &= file.open(QIODevice::ReadOnly);const auto captured=file.readAll();ok &= captured.contains("UiProjectsMainBarImp")&&!captured.contains("PRIVATE TITLE");}
 ok &= title->parentWidget()==&main&&title->text()=="PRIVATE TITLE";
 std::printf("page inventory %s\n",ok?"PASS":"FAIL");return ok?0:1;
}
