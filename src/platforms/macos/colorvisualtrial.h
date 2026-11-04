// SPDX-License-Identifier: MIT
#pragma once
#include <QDir>
#include <QFileInfo>
#include <QString>

namespace Unbound {
// One-session process configuration for explicitly compiled local visual trials.
// No global launch environment, user profile, project or palette is written.
class ColorVisualTrial {
public:
    static bool activate(const QString &directory) {
        const auto profile = directory + "/profile";
        if (!QDir::isAbsolutePath(directory) || !QFileInfo(profile + "/Colors.json").isFile())
            return false;
        qputenv("OPENGATE_EXPERIMENTAL_RED_PREVIEW", "1");
        qputenv("OPENGATE_EXPERIMENTAL_COLOR_MANAGER", "1");
        qputenv("OPENGATE_EXPERIMENTAL_COLOR_PAINT", "1");
        qputenv("OPENGATE_EXPERIMENTAL_MEDIA_POOL_COLORS", "0");
        const QPair<const char *,const char *> paths[]{
            {"OPENGATE_COLOR_PALETTE_FILE", "Colors.json"},
            {"OPENGATE_COLOR_DISPLAY_FILE", "ColorDisplay.json"},
            {"UNBOUND_WORKSPACE_FILE", "Workspace.json"},
            {"UNBOUND_ARRANGEMENTS_FILE", "Arrangements.json"},
            {"UNBOUND_SETTINGS_FILE", "Unbound.json"},
            {"UNBOUND_SHORTCUTS_FILE", "Shortcuts.json"},
            {"UNBOUND_PROFILE_FILE", "Profile.json"}};
        for (const auto &entry : paths)
            qputenv(entry.first, (profile + "/" + entry.second).toUtf8());
        qputenv("UNBOUND_DIAGNOSTICS_DIR", (directory + "/diagnostics").toUtf8());
        qputenv("UNBOUND_LAUNCH_DIRECTORY", (directory + "/diagnostics").toUtf8());
        return true;
    }
};
}
