// SPDX-License-Identifier: MIT
#pragma once
#include "colorpresetmenu.h"
#include "menutrackingcommand.h"
#include "panelcommands.h"
#include "timelineclipcolorpreview.h"
#include <QScopedValueRollback>
namespace Unbound {
// Route committed palette IDs through the existing scoped command registry to
// the supplied Timeline selection adapter. No paint implementation or storage.
class ClipColorPresetCommands : public QObject {
  public:
    using Resolve = std::function<TimelineClipColorPreview *(QWidget *)>;
    ClipColorPresetCommands(QObject *owner, PanelCommands *commands, ColorPalette &palette,
                            Resolve resolve)
        : QObject(owner), commands(commands), palette(palette), resolve(std::move(resolve)),
          menu(this) {
        palette.subscribe(this, [this] { sync(); });
        sync();
    }
    ~ClipColorPresetCommands() override {
        if (commands)
            for (const auto &id : registered)
                commands->remove(commandId(id));
    }
    void prepare(QMenu *native, QWidget *origin) {
        auto target = resolve ? resolve(origin) : nullptr;
        if (!target) {
            menu.hide(native);
            return;
        }
        const bool selection = !textEditor(origin) && target->hasSelection();
        menu.prepare(
            native, palette, target->selectedPresetId(),
            [selection](const ColorPreset &p) { return selection && p.opaque(); },
            [this, origin = QPointer<QWidget>(origin)](QString id) {
                MenuTrackingCommand::queue(this, [this, origin, id] {
                    if (!origin || !commands)
                        return;
                    QScopedValueRollback<QPointer<QWidget>> guard(explicitFocus, origin);
                    commands->invoke(commandId(id), "opengate-command-menu");
                });
            });
    }

  private:
    QPointer<PanelCommands> commands;
    ColorPalette &palette;
    Resolve resolve;
    ColorPresetMenu menu;
    QPointer<QWidget> explicitFocus;
    QSet<QString> registered;
    static QString commandId(const QString &id) {
        return "clip-color.timeline.preset." + id;
    }
    TimelineClipColorPreview *target() const {
        auto focus = explicitFocus ? explicitFocus.data() : QApplication::focusWidget();
        return focus && !textEditor(focus) && resolve ? resolve(focus) : nullptr;
    }
    void sync() {
        if (!commands)
            return;
        QSet<QString> next;
        QPointer<ClipColorPresetCommands> alive = this;
        for (const auto &preset : palette.state().presets) {
            const auto id = preset.id;
            next.insert(id);
            commands->replace({commandId(id), "Timeline Clip Color / " + preset.name,
                               [alive, id] {
                                   if (!alive)
                                       return false;
                                   const auto preset = alive->palette.find(id);
                                   const auto target = alive->target();
                                   return preset && preset->opaque() && target &&
                                          target->hasSelection();
                               },
                               [alive, id] {
                                   if (!alive)
                                       return;
                                   const auto preset = alive->palette.find(id);
                                   const auto target = alive->target();
                                   if (preset && target)
                                       target->applySelected(*preset);
                               },
                               "frameConformTimelines"});
        }
        for (const auto &id : registered)
            if (!next.contains(id))
                commands->remove(commandId(id));
        registered = std::move(next);
    }
};
} // namespace Unbound
