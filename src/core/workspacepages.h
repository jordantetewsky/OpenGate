// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonObject>
#include <QString>
namespace Unbound {
// Arrangements currently belong to exactly one Resolve page. Do not interpret
// missing page metadata as permission to reuse panels across pages: legacy
// arrangements are Edit arrangements.
// Future extension: panels from different pages may share a workspace, but that
// requires explicit source-page identity, lazy initialization, retained ownership
// and page-switch/restore semantics. Keep that separate from page-aware presets;
// relaxing the page checks alone is not sufficient or safe.
inline QString workspacePage(const QJsonObject &entry)
{
    return entry.value("page").toString(entry.value("workspace").toObject().value("page").toString("edit"));
}
inline QString workspacePageLabel(const QString &page)
{
    if (page == "media") return "Media";
    if (page == "cut") return "Cut";
    if (page == "edit") return "Edit";
    if (page == "fusion") return "Fusion";
    if (page == "color") return "Color";
    if (page == "fairlight") return "Fairlight";
    if (page == "deliver") return "Deliver";
    if (page == "photo") return "Photo";
    return {};
}
}
