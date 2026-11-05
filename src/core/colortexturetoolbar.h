// SPDX-License-Identifier: MIT
#pragma once
#include "colordisplaysettings.h"
#include <QAbstractButton>
#include <QBoxLayout>
#include <QIconEngine>
#include <QPainter>
#include <QSignalBlocker>
namespace Unbound {
// Native-sized vector glyph: checked is bright, unchecked is muted, with no
// dependency on a font containing the texture character.
class TextureToolbarGlyph : public QIconEngine {
  public:
    QIconEngine *clone() const override {
        return new TextureToolbarGlyph;
    }
    void paint(QPainter *p, const QRect &rect, QIcon::Mode mode, QIcon::State state) override {
        p->save();
        p->setRenderHint(QPainter::Antialiasing);
        p->translate(rect.topLeft());
        p->scale(rect.width() / 20., rect.height() / 20.);
        const auto ink = mode == QIcon::Disabled ? QColor(74, 74, 78)
                         : state == QIcon::On    ? QColor(238, 238, 240)
                                                 : QColor(126, 126, 132);
        p->setPen(QPen(ink, 1.5));
        p->setBrush(Qt::NoBrush);
        p->drawRoundedRect(QRectF(2, 2, 16, 16), 1.5, 1.5);
        p->setPen(Qt::NoPen);
        p->setBrush(ink);
        for (int y = 0; y < 3; ++y)
            for (int x = 0; x < 3; ++x)
                if ((x + y) % 2 == 0)
                    p->drawRect(QRectF(4 + x * 4, 4 + y * 4, 3, 3));
        p->restore();
    }
    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override {
        QPixmap result(size);
        result.fill(Qt::transparent);
        QPainter painter(&result);
        paint(&painter, QRect(QPoint(), size), mode, state);
        return result;
    }
};
class TextureToolbarButton : public QAbstractButton {
  public:
    explicit TextureToolbarButton(QWidget *owner) : QAbstractButton(owner) {
        setAttribute(Qt::WA_LayoutUsesWidgetRect);
        setCheckable(true);
        setFocusPolicy(Qt::NoFocus);
    }
    QSize sizeHint() const override {
        return QSize(30, 30);
    }
    QSize minimumSizeHint() const override {
        return sizeHint();
    }

  protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        if (underMouse() || isDown()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(255, 255, 255, isDown() ? 32 : 18));
            painter.drawRoundedRect(rect(), 3, 3);
        }
        const auto bounds =
            QRect(QPoint((width() - iconSize().width()) / 2, (height() - iconSize().height()) / 2),
                  iconSize());
        icon().paint(&painter, bounds, Qt::AlignCenter,
                     isEnabled() ? QIcon::Normal : QIcon::Disabled,
                     isChecked() ? QIcon::On : QIcon::Off);
    }
    void enterEvent(QEvent *event) override {
        QAbstractButton::enterEvent(event);
        update();
    }
    void leaveEvent(QEvent *event) override {
        QAbstractButton::leaveEvent(event);
        update();
    }
};
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
        for (const auto &control : controls)
            if (control.button)
                delete control.button;
    }
    void wire(QWidget *toolbar) {
        if (!toolbar)
            return;
        for (const auto &control : controls)
            if (control.button && control.toolbar == toolbar)
                return;
        auto anchor = toolbar->findChild<QAbstractButton *>("btnEditToolbarPositionLock");
        auto row = anchor ? anchor->parentWidget()
                          : toolbar->findChild<QWidget *>("frameToolbarMiddleButtons");
        if (!row)
            row = toolbar;
        auto layout = qobject_cast<QBoxLayout *>(row->layout());
        if (!layout || (layout->direction() != QBoxLayout::LeftToRight &&
                        layout->direction() != QBoxLayout::RightToLeft))
            return;
        auto button = new TextureToolbarButton(row);
        button->setObjectName("openGateTimelineTextures");
        button->setText("Textures");
        button->setIcon(QIcon(new TextureToolbarGlyph));
        button->setIconSize(QSize(20, 20));
        button->setAccessibleName("Clip textures");
        const int side = anchor ? qMax(24, anchor->height()) : 30;
        button->setFixedSize(side, side);
        const int position = anchor ? layout->indexOf(anchor) : -1;
        layout->insertWidget(position >= 0 ? position + 1 : layout->count(), button, 0,
                             Qt::AlignVCenter);
        button->show();
        controls.append({toolbar, button});
        connect(button, &QAbstractButton::clicked, this, [this] {
            if (toggle)
                toggle();
            refresh();
        });
        refresh();
    }

  private:
    ColorDisplaySettings &settings;
    std::function<void()> toggle;
    struct Control {
        QPointer<QWidget> toolbar;
        QPointer<QAbstractButton> button;
    };
    QVector<Control> controls;
    void refresh() {
        for (const auto &control : controls)
            if (auto button = control.button.data()) {
                const QSignalBlocker blocked(button);
                button->setChecked(settings.texturesEnabled());
                button->setToolTip(settings.texturesEnabled() ? "Clip textures: On"
                                                              : "Clip textures: Off");
                button->setAccessibleDescription(button->toolTip());
            }
    }
};
} // namespace Unbound
