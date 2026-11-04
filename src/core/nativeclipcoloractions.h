// SPDX-License-Identifier: MIT
#pragma once
#include "nativecommandactivity.h"
#include <QMenu>
namespace Unbound {
// Observe genuine native clip-color actions independent of menu/key activation.
// Never execute a command, mutate an action or own clip assignment/history.
class NativeClipColorActions : public QObject {
  public:
    using Triggered = std::function<void(QAction *)>;
    NativeClipColorActions(QObject *owner, Triggered triggered)
        : QObject(owner), triggered(std::move(triggered)) {
        token = NativeCommandActivity::shared()->subscribeActions(this, [this](QAction *action) {
            if (!action || action->property("openGateCommandId").isValid())
                return;
            if ((watched.contains(action) || namedColor(action)) && this->triggered)
                this->triggered(action);
        });
    }
    ~NativeClipColorActions() override {
        if (auto activity = NativeCommandActivity::existing())
            activity->unsubscribe(token);
    }
    void watch(QMenu *menu) {
        if (!menu)
            return;
        for (auto action : menu->actions())
            if (!action->isSeparator() && !action->menu() &&
                !action->property("openGateCommandId").isValid() && !watched.contains(action)) {
                watched.insert(action);
                connect(action, &QObject::destroyed, this,
                        [this, action] { watched.remove(action); });
                NativeCommandActivity::shared()->observe(action);
            }
    }

  private:
    Triggered triggered;
    quint64 token = 0;
    QSet<QAction *> watched;
    static bool namedColor(const QAction *action) {
        static const QSet<QString> suffixes{
            "Default", "Orange", "Apricot", "Yellow", "Lime", "Olive", "Green", "Teal",     "Navy",
            "Blue",    "Purple", "Violet",  "Pink",   "Tan",  "Beige", "Brown", "Chocolate"};
        const QString prefix = "markSetClipColor";
        const auto name = action->objectName();
        return name.startsWith(prefix) && suffixes.contains(name.mid(prefix.size()));
    }
};
} // namespace Unbound
