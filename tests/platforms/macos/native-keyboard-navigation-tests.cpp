// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QLineEdit>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QElapsedTimer>
#include <QThread>
#include <cstdio>
#include "nativekeyboardnavigation.h"
int main(int argc,char **argv) {
    QApplication app(argc,argv); QTemporaryDir logs; qputenv("UNBOUND_DIAGNOSTICS_DIR",logs.path().toUtf8());
    int failures=0; auto check=[&](bool ok,const char *why) { if (!ok) { ++failures; std::fprintf(stderr,"FAIL: %s\n",why); } };
    QWidget pane; auto layout=new QVBoxLayout(&pane); auto search=new QLineEdit(&pane); layout->addWidget(search);
    check(Unbound::filterNativeKeyboardCommands(&pane,"OpenGate") && search->text()=="OpenGate","navigation sets only the native command search");
    auto other=new QLineEdit(&pane); layout->addWidget(other); search->setText("unchanged"); other->setText("also unchanged");
    check(!Unbound::filterNativeKeyboardCommands(&pane,"OpenGate") && search->text()=="unchanged" && other->text()=="also unchanged","ambiguous native controls are not guessed");
    Unbound::NativeKeyboardNavigation navigation; int opened=0; QMenu native;
    auto action=native.addAction("Keyboard Customization..."); QObject::connect(action,&QAction::triggered,[&] { ++opened; });
    check(navigation.open("OpenGate") && opened==1,"OpenGate route invokes the same native editor command");
    action->setEnabled(false); check(!navigation.open("OpenGate") && opened==1,"unavailable native editor never falls back to a conflicting binding editor");
    QWidget commands;commands.setAttribute(Qt::WA_DontShowOnScreen);commands.resize(360,260);
    auto commandLayout=new QVBoxLayout(&commands);auto tree=new QTreeView(&commands);commandLayout->addWidget(tree);
    QStandardItemModel model;model.setHorizontalHeaderLabels({"Command","Keystroke"});
    const QString fullName="OpenGate - Maximize-Restore Workspace Toolbar and the selected grouped panel";
    model.appendRow({new QStandardItem(fullName),new QStandardItem("Alt+Shift+S, Alt+Shift+H")});
    model.appendRow({new QStandardItem("OpenGate - Edit - Stabilize"),new QStandardItem("Alt+Shift+S")});
    tree->setModel(&model);tree->header()->setStretchLastSection(false);tree->setColumnWidth(0,190);tree->setColumnWidth(1,130);
    tree->setWordWrap(false);tree->setUniformRowHeights(true);tree->setTextElideMode(Qt::ElideRight);
    commands.show();auto settle=[] {QElapsedTimer timer;timer.start();while(timer.elapsed()<80){QApplication::processEvents();QThread::msleep(1);}};settle();
    const auto first=model.index(0,0),second=model.index(1,0);const int baseline=tree->visualRect(first).height();
    const auto delegate=tree->itemDelegate();const int keyWidth=tree->columnWidth(1);tree->setCurrentIndex(first);
    check(Unbound::wireKeyboardCommandReadability(&commands),"unique native-style command tree gets readability adapter");settle();
    check(tree->wordWrap()&&!tree->uniformRowHeights()&&tree->textElideMode()==Qt::ElideNone,"wrapping replaces elision and permits taller rows");
    check(tree->visualRect(first).height()>baseline,"long command name increases its native row height");
    check(tree->visualRect(second).top()>tree->visualRect(first).bottom(),"wrapped command does not overlap the next row");
    check(tree->columnWidth(1)==keyWidth&&tree->itemDelegate()==delegate,"native keystroke column and delegate are retained");
    check(first.data().toString()==fullName&&model.index(0,1).data().toString()=="Alt+Shift+S, Alt+Shift+H"&&tree->currentIndex()==first,"command identity, bindings and selection unchanged");
    const int wrappedHeight=tree->visualRect(first).height();tree->setColumnWidth(0,145);settle();
    check(tree->visualRect(first).height()>=wrappedHeight&&tree->visualRect(second).top()>tree->visualRect(first).bottom(),"narrowing command column reflows without overlapping the next row");
    const auto capture=qEnvironmentVariable("OPENGATE_READABILITY_CAPTURE");if(!capture.isEmpty())tree->grab().save(capture);
    check(Unbound::wireKeyboardCommandReadability(&commands)&&tree->findChildren<QObject *>("openGateKeyboardCommandReadability").size()==1,"reopening does not duplicate adapter");
    delete tree->findChild<QObject *>("openGateKeyboardCommandReadability");settle();
    check(!tree->wordWrap()&&tree->uniformRowHeights()&&tree->textElideMode()==Qt::ElideRight,"teardown restores native presentation policy");
    auto another=new QTreeView(&commands);another->setModel(&model);new Unbound::KeyboardCommandReadability(another);delete another;settle();
    std::printf("native keyboard navigation failures=%d\n",failures); return failures ? 1 : 0;
}
