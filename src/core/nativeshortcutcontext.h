// SPDX-License-Identifier: MIT
#pragma once
#include <QStringList>
namespace Unbound {
// Translate explicit Resolve preset namespaces, not command-name guesses.
// Unqualified/unknown commands retain their conservative conflict behavior.
class NativeShortcutContext {
public:
    struct Scope { QString page; QStringList panels; bool known=false; };
    static Scope describe(const QString &label) {
        const auto domain=label.section('.',0,0);
        if (!label.contains('.')) return {};
        if (domain=="EditTimeline") return {"edit",{"frameConformTimelines"},true};
        if (domain=="MediaPool") return {{},{"frameEditPageMediaPool"},true};
        if (domain=="Viewer") return {{},{"widgetEditSourcePlayer","widgetEditTimelinePlayer"},true};
        if (domain=="Metadata") return {{},{"frameEditPageMetadata"},true};
        if (domain=="SoundLibrary") return {{},{"frameEditPageSoundLibrary"},true};
        if (domain=="KeyframeEditorWidget") return {{},{"frameEditPageKeyframeEditor"},true};
        if (domain=="FairlightTimeline" || domain=="FairlightADR") return {"fairlight",{},true};
        if (domain=="FusionWidget") return {"fusion",{},true};
        if (domain=="FirstCutTimeline") return {"cut",{},true};
        if (domain=="ColorNodegraph" || domain=="ColorViewer" || domain=="ColorKeyframes" || domain=="ColorTimelinePanel") return {"color",{},true};
        return {};
    }
    static bool active(const QString &label,const QString &page,const QString &panel) {
        const auto scope=describe(label);
        return !scope.known || ((scope.page.isEmpty() || scope.page==page) &&
            (scope.panels.isEmpty() || scope.panels.contains(panel)));
    }
    static bool overlaps(const QString &label,const QString &panel) {
        const auto scope=describe(label);
        return !scope.known || panel.isEmpty() || panel=="*" || scope.panels.isEmpty() || scope.panels.contains(panel);
    }
};
}
