// SPDX-License-Identifier: MIT
#pragma once
#include <QLayout>
#include <QStyle>
#include <QWidget>
namespace Unbound {
inline int nativeWorkspaceTitleGap(QWidget *row) {
    if(!row)return 0;
    const int spacing=row->layout()?row->layout()->spacing():-1;
    return qMax(0,spacing>=0?spacing:row->style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing,nullptr,row));
}
inline QString nativeWorkspaceSnapLabel() { return QStringLiteral("Snap window edges (Alt bypasses)"); }
// The existing Windows backend discovers this outer Inspector frame. Prefer
// the common inner widget when present; never rename native Resolve widgets.
inline QString nativeWorkspacePanelAlias(const QString &name) {
    return name==QLatin1String("widgetEditInspector")?QStringLiteral("frameEditPageInspector"):QString();
}
}
