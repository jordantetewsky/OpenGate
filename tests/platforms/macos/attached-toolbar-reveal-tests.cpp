// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QVBoxLayout>
#include <QTemporaryDir>
#include "attachedtoolbarreveal.h"
#include <cstdio>
int main(int argc,char **argv) {
    QApplication app(argc,argv); QTemporaryDir logs; qputenv("UNBOUND_DIAGNOSTICS_DIR",logs.path().toUtf8());
    QWidget page,host; page.setAttribute(Qt::WA_DontShowOnScreen); host.setAttribute(Qt::WA_DontShowOnScreen);
    host.setObjectName("unboundFloating_group_timeline"); auto layout=new QVBoxLayout(&host);
    auto timeline=new QWidget(&host); timeline->setObjectName("frameConformTimelines"); layout->addWidget(timeline);
    auto row=new QVBoxLayout(timeline); auto toolbar=new QWidget(timeline); toolbar->setObjectName("frameEditToolbarTop"); row->addWidget(toolbar);
    page.show(); host.show(); QApplication::processEvents(); toolbar->hide(); auto parent=toolbar->parentWidget();
    bool ok=Unbound::revealAttachedTimelineToolbar(&page,toolbar,timeline) && toolbar->isVisible() && toolbar->parentWidget()==parent;
    toolbar->hide(); page.hide(); ok &= !Unbound::revealAttachedTimelineToolbar(&page,toolbar,timeline) && toolbar->isHidden();
    page.show(); host.setObjectName("unmanaged"); ok &= !Unbound::revealAttachedTimelineToolbar(&page,toolbar,timeline) && toolbar->isHidden();
    std::printf("attached toolbar reveal %s\n",ok ? "PASS" : "FAIL"); return ok ? 0 : 1;
}
