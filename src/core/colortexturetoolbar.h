// SPDX-License-Identifier: MIT
#pragma once
#include "colordisplaysettings.h"
#include <QBoxLayout>
#include <QSignalBlocker>
#include <QToolButton>
namespace Unbound {
// Present the shared texture display setting in native Timeline tools. Native
// toolbar ownership, scaling/overflow, commands and settings stay with callers.
class ColorTextureToolbar : public QObject {
  public:
    ColorTextureToolbar(QObject *owner, ColorDisplaySettings &settings,
                        std::function<void()> toggle)
        : QObject(owner), settings(settings), toggle(std::move(toggle)) {
        settings.subscribe(this, [this](bool) { refresh(); });
    }
    ~ColorTextureToolbar() override {
        for (auto button : buttons)
            if (button)
                delete button;
    }
    void wire(QWidget *toolbar) {
        if (!toolbar)
            return;
        for (auto button : buttons)
            if (button && button->parentWidget() == toolbar)
                return;
        auto layout = qobject_cast<QBoxLayout *>(toolbar->layout());
        if (!layout)
            return;
        auto button = new QToolButton(toolbar);
        button->setObjectName("openGateTimelineTextures");
        button->setText(QString::fromUtf8("░"));
        button->setAccessibleName("Clip textures");
        button->setCheckable(true);
        button->setAutoRaise(true);
        button->setFocusPolicy(Qt::NoFocus);
        button->setFixedSize(24, 24);
        layout->addWidget(button);
        buttons.append(button);
        connect(button, &QToolButton::clicked, this, [this] {
            if (toggle)
                toggle();
            refresh();
        });
        refresh();
    }

  private:
    ColorDisplaySettings &settings;
    std::function<void()> toggle;
    QVector<QPointer<QToolButton>> buttons;
    void refresh() {
        for (auto button : buttons)
            if (button) {
                const QSignalBlocker blocked(button);
                button->setChecked(settings.texturesEnabled());
                button->setToolTip(settings.texturesEnabled() ? "Clip textures: On"
                                                              : "Clip textures: Off");
            }
    }
};
} // namespace Unbound
