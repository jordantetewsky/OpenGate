// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QTemporaryDir>
#include <cstdio>
#include "../../../src/core/nativeaudiochannelprobe.h"
#include "../../../src/core/nativeclipattributesprobe.h"
#include <QTableWidget>
#include <QDialog>
int main(int argc,char **argv) {
 QApplication app(argc,argv);QTemporaryDir dir;qputenv("UNBOUND_DIAGNOSTICS_DIR",dir.path().toUtf8());
 int calls=0,failures=0;
 QMenu root;auto channels=root.addMenu("Audio Channel");
 auto first=channels->addAction("1 - MixL");first->setData(1);first->setCheckable(true);first->setChecked(true);
 QObject::connect(first,&QAction::triggered,[&]{++calls;});
 auto second=channels->addAction("2 - MixR");second->setData(2);second->setCheckable(true);
 auto third=channels->addAction("3 - Channel Three");third->setCheckable(true);
 using Choice=Unbound::NativeAudioChannelChoice;
 auto previous=Choice::inspect(channels,-1),next=Choice::inspect(channels,1);
 if(previous.target!=3 || previous.action!=third || next.target!=2 || next.action!=second)++failures;
 first->setChecked(false);third->setChecked(true);
 if(Choice::inspect(channels,1).action!=first || Choice::inspect(channels,-1).action!=second)++failures;
 second->setChecked(true);if(Choice::inspect(channels,1).reason!="ambiguous-current-channel")++failures;
 third->setChecked(false);second->setChecked(false);
 if(Choice::inspect(channels,1).reason!="unknown-current-channel")++failures;
 first->setChecked(true);second->setEnabled(false);
 if(Choice::inspect(channels,1).action)++failures;
 second->setEnabled(true);second->setText("4 - gap");if(Choice::inspect(channels,1).action)++failures;
 second->setText("2 - MixR");if(Choice::inspect(channels,0).action)++failures;
 QMenu single("Audio Channel");auto only=single.addAction("1 - Mono");only->setCheckable(true);only->setChecked(true);
 if(Choice::inspect(&single,1).reason!="single-channel-source")++failures;
 auto transient=new QMenu("Audio Channel");auto a=transient->addAction("1 - A"),b=transient->addAction("2 - B");
 a->setCheckable(true);a->setChecked(true);b->setCheckable(true);
 auto retained=Choice::inspect(transient,1);delete transient;if(retained.action)++failures;
 const auto geometry=root.geometry();const auto owners=first->associatedWidgets();
 Unbound::NativeAudioChannelProbe probe(nullptr);
 QMenu unrelated;unrelated.addAction("Audio Channel");probe.inspectShownMenu(&unrelated);
 root.setProperty("unboundOwnMenu",true);probe.inspectShownMenu(&root);root.setProperty("unboundOwnMenu",false);
 probe.inspectShownMenu(&root);probe.inspectShownMenu(channels);
 if(calls || !first->isChecked() || root.isVisible() || root.geometry()!=geometry || owners!=first->associatedWidgets())++failures;
 first->trigger();if(calls!=1)++failures;
 Unbound::flushDiagnostics();QFile f(dir.path()+"/events.jsonl");f.open(QIODevice::ReadOnly);auto bytes=f.readAll();
 if(bytes.count("audio.native-menu.inventory")!=2 || bytes.count("audio.native-menu.triggered")!=1)++failures;
 for(int i=0;i<100;++i)probe.inspectShownMenu(&root);
 Unbound::flushDiagnostics();f.seek(0);bytes=f.readAll();if(bytes.count("audio.native-menu.inventory")!=32)++failures;
 // Dialog inspection uses existing Show forwarding and must not alter the
 // native model or combo selection, including after deferred destruction.
 Unbound::NativeClipAttributesProbe attributes(nullptr);
 QDialog dialog;dialog.setAttribute(Qt::WA_DontShowOnScreen);dialog.setWindowTitle("Clip Attributes");
 QTableWidget table(1,2,&dialog);table.setHorizontalHeaderLabels({"Format","Source Channel"});
 table.setItem(0,0,new QTableWidgetItem("Mono"));table.setItem(0,1,new QTableWidgetItem("1 - MixL"));
 QComboBox combo(&dialog);combo.addItems({"1 - MixL","2 - MixR"});combo.setCurrentIndex(1);
 int edits=0;QObject::connect(&table,&QTableWidget::itemChanged,[&]{++edits;});
 QObject::connect(&combo,QOverload<int>::of(&QComboBox::currentIndexChanged),[&]{++edits;});
 dialog.show();attributes.inspectShownWidget(&dialog);attributes.inspectShownWidget(&combo);
 QApplication::processEvents();Unbound::flushDiagnostics();f.seek(0);bytes=f.readAll();
 if(bytes.count("audio.native-attributes.inventory")!=1 || !bytes.contains("Source Channel") ||
    edits || combo.currentIndex()!=1 || table.item(0,1)->text()!="1 - MixL")++failures;
 auto deleted=new QDialog;deleted->setWindowTitle("Clip Attributes");
 attributes.inspectShownWidget(deleted);delete deleted;QApplication::processEvents();
 dialog.hide();attributes.inspectShownWidget(&dialog);QApplication::processEvents();
 Unbound::flushDiagnostics();f.seek(0);bytes=f.readAll();
 if(bytes.count("audio.native-attributes.inventory")!=1)++failures;
 std::printf("native audio probe failures=%d\n",failures);return failures?1:0;
}
