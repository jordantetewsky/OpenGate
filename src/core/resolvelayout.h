// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonDocument>
#include <QJsonObject>
#include "workspacepresetpolicy.h"
namespace Unbound {
// Existing bundled emulator's panel tree, embedded using the existing native
// root lease. Header/footer remain outside the panel tree; no second window.
inline QJsonObject resolveLayoutEntry(const QString &page) {
    if(page!="edit")return {}; // Other pages require their own verified adapter.
    const auto workspace=QJsonDocument::fromJson(R"layout({"version":1,"page":"edit","layout_mode":"managed","header":{"visible":true},"footer":{"hidden":false},"native_arrangement":{"version":1,"kind":"managed","page":"edit","tree":{"orientation":"horizontal","children":[{"orientation":"tabs","selected":0,"children":[{"panel":"frameEditPageMediaPool"},{"panel":"frameEditPageEditIndex"}]},{"orientation":"vertical","children":[{"orientation":"horizontal","children":[{"panel":"widgetEditSourcePlayer"},{"panel":"widgetEditTimelinePlayer"}],"sizes":[1,1]},{"orientation":"horizontal","children":[{"panel":"frameConformTimelines"},{"panel":"frameEditAudioMeter"}],"sizes":[4,1]}],"sizes":[3,2]},{"panel":"widgetEditInspector"}],"sizes":[2,6,2]},"panels":{"frameEditPageMediaPool":{"hidden":false},"frameEditPageEditIndex":{"hidden":false},"widgetEditSourcePlayer":{"hidden":false},"widgetEditTimelinePlayer":{"hidden":false},"frameConformTimelines":{"hidden":false},"widgetEditInspector":{"hidden":false},"frameEditAudioMeter":{"hidden":false}}},"groups":[],"panels":{"frameEditPageMediaPool":{"floating":true,"hidden":false,"pinned":false,"interface_scale":1},"frameEditPageEditIndex":{"floating":true,"hidden":false,"pinned":false,"interface_scale":1},"widgetEditSourcePlayer":{"floating":true,"hidden":false,"pinned":false,"interface_scale":1},"widgetEditTimelinePlayer":{"floating":true,"hidden":false,"pinned":false,"interface_scale":1},"frameConformTimelines":{"floating":true,"hidden":false,"pinned":false,"interface_scale":1},"widgetEditInspector":{"floating":true,"hidden":false,"pinned":false,"interface_scale":1},"frameEditAudioMeter":{"floating":true,"hidden":false,"pinned":true,"interface_scale":1}}})layout").object();
    return {{"id",WorkspacePresetPolicy::resolveLayoutId(page)},{"name","Resolve Layout"},{"page",page},{"read_only",true},{"workspace",workspace}};
}
}
