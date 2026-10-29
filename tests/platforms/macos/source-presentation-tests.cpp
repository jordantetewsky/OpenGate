// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QLabel>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <cstdio>
#include "sourceplayerpresentation.h"
int main(int argc,char **argv) {
    QApplication app(argc,argv); QTemporaryDir logs; qputenv("UNBOUND_DIAGNOSTICS_DIR",logs.path().toUtf8());
    int failures=0; auto check=[&](bool pass,const char *why) { if (!pass) { ++failures; std::fprintf(stderr,"FAIL: %s\n",why); } };
    QWidget host; host.setAttribute(Qt::WA_DontShowOnScreen); auto layout=new QVBoxLayout(&host);
    auto stack=new QStackedWidget(&host); stack->setObjectName("stackedEditSourceOfflinePlayers"); layout->addWidget(stack);
    auto source=new QLabel("Native source contents"); source->setObjectName("widgetEditSourcePlayer");
    auto offline=new QLabel("Offline source"); offline->setObjectName("widgetEditOfflinePlayer"); stack->addWidget(source); stack->addWidget(offline);
    stack->setCurrentWidget(source); host.show(); QApplication::processEvents(); source->hide();
    stack->hide(); stack->show(); QApplication::processEvents();
    check(stack->isVisible() && source->isHidden(),"outer stack can be visible while explicitly hidden selected contents remain blank");
    Unbound::reconcileSourcePlayerPresentation(stack,"fixture");
    check(source->isVisible() && stack->currentWidget()==source && offline->isHidden(),"repair reveals only existing selected source page without changing selection");
    stack->setCurrentWidget(offline); offline->hide(); Unbound::reconcileSourcePlayerPresentation(stack,"offline-fixture");
    check(offline->isVisible() && stack->currentWidget()==offline && source->isHidden(),"offline page stays selected and is revealed without loading a clip");
    host.hide(); offline->hide(); Unbound::reconcileSourcePlayerPresentation(stack,"hidden-door-fixture");
    check(offline->isHidden() && host.isHidden(),"hidden Source door is not reopened by reconciliation");
    QStackedWidget unrelated; unrelated.setObjectName("otherStack"); auto child=new QLabel("Other"); unrelated.addWidget(child); child->hide();
    Unbound::reconcileSourcePlayerPresentation(&unrelated,"unrelated-fixture"); check(child->isHidden(),"unrelated native stacks are untouched");
    std::printf("source presentation failures=%d\n",failures); return failures ? 1 : 0;
}
