// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <cstdio>
#include "../../../src/core/timelinepresentationprobe.h"
int main(int argc,char **argv) {
 QApplication app(argc,argv);QTemporaryDir dir;qputenv("UNBOUND_DIAGNOSTICS_DIR",dir.path().toUtf8());
 QWidget timeline;timeline.setObjectName("frameConformTimelines");auto layout=new QVBoxLayout(&timeline);
 auto toolbar=new QWidget;toolbar->setObjectName("frameEditToolbarTop");layout->addWidget(toolbar);
 toolbar->setFixedHeight(50);const auto geometry=toolbar->geometry();const auto parent=toolbar->parentWidget();
 Unbound::TimelinePresentationProbe probe(nullptr);int failures=0;
 probe.observe(toolbar,QEvent::Hide);app.processEvents();
 if(toolbar->parentWidget()!=parent || toolbar->geometry()!=geometry || toolbar->maximumHeight()!=50 || timeline.isVisible())++failures;
 QWidget unrelated;probe.observe(&unrelated,QEvent::Hide);probe.observe(toolbar,QEvent::MouseMove);
 Unbound::flushDiagnostics();QFile f(dir.path()+"/events.jsonl");f.open(QIODevice::ReadOnly);auto bytes=f.readAll();
 if(bytes.count("timeline.presentation.transition")!=2 || !bytes.contains("next-turn"))++failures;
 auto destroyed=new QWidget;destroyed->setObjectName("frameEditToolbarTop");probe.observe(destroyed,QEvent::ParentChange);delete destroyed;app.processEvents();
 for(int i=0;i<300;++i)probe.observe(toolbar,QEvent::Resize);
 app.processEvents();Unbound::flushDiagnostics();f.seek(0);bytes=f.readAll();
 if(bytes.count("timeline.presentation.transition")!=516 || !bytes.contains("destroyed"))++failures;
 probe.observe(toolbar,QEvent::Hide);
 QWidget tabs;tabs.setObjectName("timelineTabsHolder");probe.observe(&tabs,QEvent::Hide);
 app.processEvents();Unbound::flushDiagnostics();f.seek(0);bytes=f.readAll();
 if(bytes.count("timeline.presentation.transition")!=520 || !bytes.contains("timelineTabsHolder"))++failures;
 QGraphicsScene scene;QGraphicsView view(&scene,&timeline);view.setObjectName("m_TiTimelineView");
 auto nativeControl=new QWidget;nativeControl->setObjectName("nativeTrackControls");
 auto proxy=scene.addWidget(nativeControl);proxy->setPos(15,20);
 view.setTransform(QTransform::fromScale(1.25,1.25));
 const auto transform=view.transform();const auto sceneRect=scene.sceneRect();const auto position=proxy->pos();
 const auto topology=Unbound::TimelineContentInspection::capture(&timeline);
 if(topology["views"].toArray().size()!=1 || !topology["read_only"].toBool() ||
    view.transform()!=transform || scene.sceneRect()!=sceneRect || proxy->pos()!=position || proxy->widget()!=nativeControl)++failures;
 std::printf("timeline presentation probe failures=%d\n",failures);return failures?1:0;
}
