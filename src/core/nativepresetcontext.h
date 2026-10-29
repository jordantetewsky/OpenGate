// SPDX-License-Identifier: MIT
#pragma once
#include <QString>
#include <QWidget>

namespace Unbound {
// Classifies native preset owners that cannot handle input in the current context.
// Unknown owners remain conflicts; this does not dispatch or consume input.
inline bool nativePresetOwnerInactiveOnEdit(const QString &owner, QWidget *focus) {
    if(owner.startsWith("FairlightTimeline."))return true;
    // Observed conflict: Resolve permits this Fusion playback assignment beside
    // an Edit command. Do not generalize the exception to all Fusion owners or
    // all Edit-page focus (Fusion controls can be embedded in other surfaces).
    if(owner!="FusionWidget.fuHotkey_FuFrame_Playback_Mode{ play = true, reverse = true}")return false;
    for(auto widget=focus;widget;widget=widget->parentWidget()) {
        if(widget->objectName()=="m_TiTimelineView" && widget->inherits("QGraphicsView"))return true;
        // A text editor inside a timeline must retain its native key ownership.
        if(widget->inherits("QLineEdit") || widget->inherits("QTextEdit") ||
           widget->inherits("QPlainTextEdit"))return false;
    }
    return false;
}
}
