// SPDX-License-Identifier: MIT
#include "../../../src/core/inputcustomizationpanel.h"
#include "../../../src/core/panelsession.h"
#include "../../core/input-customization/diagramchecks.h"
#include <QApplication>
#include <QStandardItemModel>
#include <QTemporaryDir>
#include <cstdio>

int main(int argc,char **argv) {
    QApplication app(argc,argv);
    QPalette palette;
    palette.setColor(QPalette::Window,QColor("#28292e"));
    palette.setColor(QPalette::WindowText,QColor("#dddddf"));
    palette.setColor(QPalette::Base,QColor("#222328"));
    palette.setColor(QPalette::AlternateBase,QColor("#27282d"));
    palette.setColor(QPalette::Text,QColor("#ccccce"));
    palette.setColor(QPalette::Button,QColor("#313238"));
    palette.setColor(QPalette::ButtonText,QColor("#dddddf"));
    app.setPalette(palette);
    QTemporaryDir directory;
    qputenv("UNBOUND_DIAGNOSTICS_DIR",directory.path().toUtf8());
    qputenv("UNBOUND_TEST_HIDDEN_HOSTS","1");
    int failures=0;
    const auto check=[&](bool good,const char *message){if(!good){++failures;std::fprintf(stderr,"FAIL: %s\n",message);}};
    OpenGateTest::speedEditorDiagramChecks(check);
    {
        QStandardItemModel native;
        native.setHorizontalHeaderLabels({"Command","Keystroke"});
        auto clip=new QStandardItem("Clip");
        clip->appendRow({new QStandardItem("Close Up"),new QStandardItem("Shift+C")});
        native.appendRow({clip,new QStandardItem});
        Unbound::DeviceCommandProjection device(false,nullptr),categories(true,nullptr);
        device.setSourceModel(&native);categories.setSourceModel(&native);
        const auto command=device.index(0,0,device.index(0,0));
        check(command.data().toString()=="Close Up" && device.index(0,1,device.index(0,0)).data().isNull(),
              "device projection exposes commands without presenting keyboard bindings as device assignments");
        check(!(device.flags(command)&Qt::ItemIsEditable),"device catalog cannot mutate native assignments");
        device.setFilterFixedString("Close");
        check(device.rowCount()==1 && native.item(0)->child(0,1)->text()=="Shift+C",
              "device search preserves native source and keyboard assignment");
        check(categories.rowCount()==1 && categories.rowCount(categories.index(0,0))==0,
              "category projection keeps command leaves out of navigation tree");
    }
    QWidget main;
    main.setAttribute(Qt::WA_DontShowOnScreen);main.resize(1400,1000);
    QWidget page(&main);page.setObjectName("WStackPage_Conform");page.resize(main.size());
    main.show();page.show();
    QPointer<QDialog> native;
    int creations=0,saves=0;
    {
        Unbound::InputCustomizationPanel panel([&](QWidget *parent,QString &) {
            ++creations;auto dialog=new QDialog(parent);native=dialog;
            dialog->setObjectName("UiActionsEditorDialog");dialog->resize(960,720);
            auto layout=new QVBoxLayout(dialog);
            auto label=new QLabel("Native fixture editor",dialog);label->setObjectName("fixtureNativeLabel");
            layout->addWidget(label);auto save=new QPushButton("Save",dialog);save->setObjectName("fixtureNativeSave");
            layout->addWidget(save);QObject::connect(save,&QPushButton::clicked,dialog,[&]{++saves;});
            return dialog;
        });
        QString error;auto content=panel.ensure(&page,error);
        check(content && error.isEmpty() && content==panel.ensure(&page,error) && creations==1,
              "panel entry points retain one owned native editor");
        auto tabs=qobject_cast<QTabWidget *>(content);
        check(tabs && tabs->count()==2 && tabs->widget(0)==native && !native->isWindow(),
              "actual native editor remains first tab and device editor is its sibling");
        const auto nativeMinimum=native->minimumSize();
        const auto font=native->findChild<QLabel *>("fixtureNativeLabel")->font();
        Unbound::ManagedPanelSession session;
        session.restoreInterfaceState({{"interface_scale",.8}});
        check(session.undock(content,&page,Unbound::InputCustomizationPanel::label(),true).isEmpty(),
              "customization uses the existing managed session");
        check(session.interfaceScale()==1.0,"restored scale cannot bypass the native content minimum");
        panel.closeRequested=[&]{session.setHiddenByUser(true);};
        check(session.supportsContentScale() && session.setInterfaceScale(1.5),"existing panel scale applies to native editor");
        check(native->minimumWidth()>nativeMinimum.width(),"scaled native minimum preserves reachable content");
        check(!session.setInterfaceScale(.9) && session.setInterfaceScale(1.0),"native baseline is the minimum content scale");
        check(native->minimumSize()==nativeMinimum && native->findChild<QLabel *>("fixtureNativeLabel")->font()==font,
              "reset restores native control metrics without drift");
        session.window()->resize(560,420);QApplication::processEvents();
        auto area=session.window()->findChild<QScrollArea *>();
        check(area && area->horizontalScrollBar()->maximum()>0 && area->verticalScrollBar()->maximum()>0,
              "small host reaches full native minimum through existing overflow");
        native->findChild<QPushButton *>("fixtureNativeSave")->click();
        check(saves==1,"native Save connection survives hosting");
        tabs->setCurrentIndex(1);QApplication::processEvents();
        check(!native->isVisible() && tabs->currentWidget()->objectName()=="openGateSpeedEditorCustomization",
              "device tab is selectable without replacing native keyboard ownership");
        if(const auto path=qEnvironmentVariable("OPENGATE_INPUT_PREVIEW_SNAPSHOT");!path.isEmpty()) {
            auto device=tabs->currentWidget();device->resize(device->sizeHint().expandedTo(QSize(1200,1000)));
            check(device->grab().save(path),"device page render is captured");
        }
        tabs->setCurrentIndex(0);native->reject();
        check(session.active() && session.hiddenByUser(),"native Close hides retained panel rather than destroying it");
        panel.recalled();session.setHiddenByUser(false);QApplication::processEvents();
        check(native && native->isVisible() && creations==1,"recall reuses native editor instance");
        QWidget group(&main,Qt::Window);QWidget container(&group);
        check(session.attachToGroup(&container).isEmpty() && session.grouped(),"tabbed customization joins existing panel groups");
        session.detachFromGroup();
        check(!session.grouped() && session.active(),"panel group detach restores original managed host");
        check(session.dock().isEmpty() && !content->isVisible(),"Native return parks retained customization surface");
    }
    check(native.isNull(),"native editor has one deletion owner after session release");
    QPointer<QTabWidget> abandonedTabs;
    int callbacksAfterQuit=0;
    {
        Unbound::InputCustomizationPanel panel([](QWidget *parent,QString &) {return new QDialog(parent);});
        QString error;auto tabs=qobject_cast<QTabWidget *>(panel.ensure(&page,error));
        abandonedTabs=tabs;panel.closeRequested=[&]{++callbacksAfterQuit;};
        check(tabs,"quit fixture constructs retained tabs");
        panel.abandonForQuit();
    }
    if(abandonedTabs) {
        abandonedTabs->setCurrentIndex(1);
        qobject_cast<QDialog *>(abandonedTabs->widget(0))->reject();
        for(auto button:abandonedTabs->widget(1)->findChildren<QPushButton *>())if(button->text()=="Close")button->click();
    }
    check(callbacksAfterQuit==0,"retained tabs cannot call the destroyed panel owner after quit abandonment");
    std::printf("input customization failures=%d\n",failures);
    return failures?1:0;
}
