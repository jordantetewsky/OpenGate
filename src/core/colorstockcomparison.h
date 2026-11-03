// SPDX-License-Identifier: MIT
#pragma once
#include "colorpreset.h"
#include "perceptualcolormatch.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QWidget>
#include <functional>
namespace Unbound {
// Read-only native-swatch reference for the sample preview. These sampled
// reference values are not authority for project fallback materialization.
class ColorStockComparison : public QWidget {
  public:
    std::function<void(QColor, QString)> selected;
    explicit ColorStockComparison(QWidget *owner = nullptr) : QWidget(owner) {
        auto layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(8);
        layout->addWidget(new QLabel("Stock color"));
        colors = new QComboBox;
        colors->setObjectName("openGateStockColorComparison");
        layout->addWidget(colors, 1);
        closest = new QPushButton("Closest");
        closest->setToolTip(
            "Compare a solid preset against sampled stock swatches; no project color is changed.");
        layout->addWidget(closest);
        connect(closest, &QPushButton::clicked, this, [this] {
            int best = -1;
            double distance = std::numeric_limits<double>::infinity();
            for (int i = 0; i < colors->count(); ++i) {
                const auto d =
                    colorDifference2000(preset.color, colors->itemData(i).value<QColor>());
                if (d < distance) {
                    distance = d;
                    best = i;
                }
            }
            if (best >= 0)
                colors->setCurrentIndex(best);
        });
        const QVector<QPair<QString, QString>> values{
            {"Orange", "#EB6E01"},   {"Apricot", "#FFA833"}, {"Yellow", "#D4AD1F"},
            {"Lime", "#9FC615"},     {"Olive", "#5F9921"},   {"Green", "#448F65"},
            {"Teal", "#019899"},     {"Navy", "#005278"},    {"Blue", "#4376A1"},
            {"Purple", "#9972A0"},   {"Violet", "#D0568D"},  {"Pink", "#E98CB5"},
            {"Tan", "#B9AF97"},      {"Beige", "#C4A07C"},   {"Brown", "#996601"},
            {"Chocolate", "#8C5A3F"}};
        for (const auto &entry : values) {
            QPixmap icon(18, 12);
            icon.fill(QColor(entry.second));
            colors->addItem(QIcon(icon), entry.first, QColor(entry.second));
        }
        colors->setCurrentIndex(8);
        connect(colors, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
            if (selected)
                selected(colors->currentData().value<QColor>(), colors->currentText());
        });
    }
    void setPreset(ColorPreset value) {
        preset = std::move(value);
        closest->setEnabled(preset.stops.isEmpty() && preset.opacity == 1);
    }

  private:
    QComboBox *colors;
    QPushButton *closest;
    ColorPreset preset;
};
} // namespace Unbound
