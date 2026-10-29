// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QWidget>
#include "workspacepages.h"
namespace Unbound {
inline QWidget *resolveMainWindow()
{
    for (auto window : QApplication::topLevelWidgets())
        if (window->objectName() == "UiMainWindow") return window;
    return nullptr;
}
inline QString currentWorkspacePage(QWidget *main)
{
    if (!main) return {};
    const QPair<const char *,const char *> pages[] = {
        {"WStackPage_Browse","media"}, {"WStackPage_FirstCut","cut"}, {"WStackPage_Conform","edit"},
        {"WStackPage_Fusion","fusion"}, {"WStackPage_Color","color"}, {"WStackPage_Audio","fairlight"},
        {"WStackPage_Deliver","deliver"}, {"WStackPage_Photo","photo"}};
    for (const auto &page : pages) {
        auto widget = main->findChild<QWidget *>(QString::fromLatin1(page.first));
        if (widget && widget->isVisible()) return QString::fromLatin1(page.second);
    }
    return {};
}
}
