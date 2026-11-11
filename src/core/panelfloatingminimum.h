// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QScreen>
#include <QWidget>
#include <QWindow>
namespace Unbound {
// Optional floating-host resize floor. Native content keeps its own minimum and
// overflow; this floor never exceeds the destination screen's usable client area.
inline void applyPanelFloatingMinimum(QWidget *content,QWidget *host,const QRect &destination={}) {
    if(!content || !host)return;
    const auto desired=content->property("openGateFloatingMinimumSize").toSize();
    if(!desired.isValid() || desired.isEmpty())return;
    auto screen=destination.isValid()?QGuiApplication::screenAt(destination.center()):host->screen();
    if(!screen)screen=QApplication::primaryScreen();
    if(!screen)return;
    const auto margins=host->windowHandle()?host->windowHandle()->frameMargins():QMargins();
    const auto available=screen->availableGeometry().marginsRemoved(margins).size();
    const auto minimum=desired.boundedTo(available).expandedTo(QSize(1,1));
    if(host->minimumSize()!=minimum)host->setMinimumSize(minimum);
}
}
