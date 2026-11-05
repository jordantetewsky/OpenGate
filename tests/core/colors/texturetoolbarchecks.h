// SPDX-License-Identifier: MIT
#pragma once
#include "../../../src/core/colortexturetoolbar.h"
#include "../../../src/core/paneloverflow.h"
#include <QCheckBox>

namespace OpenGateTest {
template <class Check>
void textureToolbarChecks(Check check, Unbound::ColorDisplaySettings &settings) {
    const auto initialState = settings.texturesEnabled();
    const auto settle = [] {
        for (int i = 0; i < 8; ++i)
            QApplication::processEvents();
    };
    QWidget host;
    host.setAttribute(Qt::WA_DontShowOnScreen);
    host.resize(900, 80);
    auto hostLayout = new QVBoxLayout(&host);
    hostLayout->setContentsMargins(0, 0, 0, 0);
    auto toolbar = new QWidget;
    toolbar->setObjectName("frameEditToolbarTop");
    auto outer = new QVBoxLayout(toolbar);
    outer->setContentsMargins(0, 10, 0, 10);
    auto row = new QWidget;
    row->setObjectName("frameToolbarMiddleButtons");
    row->setFixedHeight(30);
    auto horizontal = new QHBoxLayout(row);
    horizontal->setContentsMargins(0, 0, 0, 0);
    horizontal->setSpacing(4);
    auto snapping = new QPushButton("Snapping"), link = new QPushButton("Link");
    snapping->setFixedHeight(30);
    link->setFixedHeight(30);
    horizontal->addWidget(snapping);
    horizontal->addWidget(link);
    auto lock = new QCheckBox;
    lock->setObjectName("btnEditToolbarPositionLock");
    lock->setFixedSize(30, 30);
    // Resolve's custom toolbar has a captured 30px control row. Stock macOS
    // test buttons otherwise demand Aqua layout margins (42px minimum), which
    // do not model that native row even when their widget height is fixed.
    for (auto native : {static_cast<QAbstractButton *>(snapping),
                        static_cast<QAbstractButton *>(link), static_cast<QAbstractButton *>(lock)})
        native->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    horizontal->addWidget(lock);
    auto rest = new QWidget;
    rest->setFixedSize(700, 30);
    horizontal->addWidget(rest);
    outer->addWidget(row);
    Unbound::PanelOverflow overflow;
    auto area = qobject_cast<QScrollArea *>(overflow.wrap(toolbar, &host, true));
    hostLayout->addWidget(area);
    int invocations = 0;
    Unbound::ColorTextureToolbar controls(&host, settings, [&] {
        ++invocations;
        settings.setTexturesEnabled(!settings.texturesEnabled());
    });
    controls.wire(toolbar);
    controls.wire(toolbar);
    QWidget unfinished;
    new QVBoxLayout(&unfinished);
    controls.wire(&unfinished);
    check(!unfinished.findChild<QAbstractButton *>("openGateTimelineTextures"),
          "an unfinished vertical toolbar never gets a fallback second row");
    auto button = toolbar->findChild<QAbstractButton *>("openGateTimelineTextures");
    check(button && button->parentWidget() == row &&
              horizontal->indexOf(button) == horizontal->indexOf(lock) + 1 && outer->count() == 1,
          "texture control joins the native horizontal lock row without adding an outer row");
    if (!button)
        return;
    host.show();
    toolbar->show();
    settle();
    const int height = toolbar->sizeHint().height();
    for (int width : {1200, 400, 180, 900}) {
        host.resize(width, 80);
        settle();
        check(button->geometry().center().y() == lock->geometry().center().y() &&
                  toolbar->sizeHint().height() == height && button->height() == lock->height(),
              "narrow and wide toolbars keep textures on the native row at the native button size");
    }
    host.resize(180, 80);
    settle();
    const auto at = button->mapTo(toolbar, button->rect().center());
    area->ensureVisible(at.x(), at.y(), 20, 0);
    settle();
    check(area->horizontalScrollBar()->maximum() > 0 && area->verticalScrollBar()->maximum() == 0 &&
              area->viewport()->rect().contains(
                  button->mapTo(area->viewport(), button->rect().center())),
          "existing horizontal overflow keeps the single-row texture control reachable");
    settings.setTexturesEnabled(false);
    const auto off = button->icon().pixmap(QSize(40, 40), QIcon::Normal, QIcon::Off).toImage();
    button->click();
    const auto on = button->icon().pixmap(QSize(40, 40), QIcon::Normal, QIcon::On).toImage();
    check(invocations == 1 && button->isChecked() && settings.texturesEnabled() && off != on &&
              on.pixelColor(10, 10).lightness() > off.pixelColor(10, 10).lightness() &&
              button->toolTip().endsWith("On") && button->focusPolicy() == Qt::NoFocus,
          "texture toggle has distinct bright-on/muted-off vector states and invokes once");
    if (const auto image = qEnvironmentVariable("OPENGATE_TEXTURE_TOOLBAR_SNAPSHOT");
        !image.isEmpty()) {
        host.resize(1000, 80);
        settle();
        check(host.grab().save(image), "single-row toolbar preview is retained");
        settings.setTexturesEnabled(false);
        settle();
        check(host.grab().save(image + ".off.png"), "toolbar off-state preview is retained");
    }
    overflow.releaseInto(&host);
    settings.setTexturesEnabled(initialState);
}
} // namespace OpenGateTest
