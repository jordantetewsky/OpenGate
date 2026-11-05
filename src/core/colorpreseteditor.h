// SPDX-License-Identifier: MIT
#pragma once
#include "colorgradientstopbar.h"
#include "colorpreset.h"
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSlider>
#include <QUuid>
#include <QVBoxLayout>
#include <functional>
namespace Unbound {
// Edit one transient preset draft, including the selected gradient stop. The
// palette owner alone commits Update; this component never writes storage.
class ColorPresetEditor : public QWidget {
  public:
    std::function<void(const ColorPreset &)> preview, commit;
    explicit ColorPresetEditor(QWidget *owner = nullptr) : QWidget(owner) {
        setObjectName("openGateColorPresetEditor");
        auto box = new QVBoxLayout(this);
        auto scroll = new QScrollArea;
        scroll->setObjectName("openGatePresetFormScroll");
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setMinimumSize(0, 0);
        auto fields = new QWidget;
        form = new QFormLayout(fields);
        form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        form->setRowWrapPolicy(QFormLayout::WrapLongRows);
        scroll->setWidget(fields);
        box->addWidget(scroll, 1);
        name = new QLineEdit;
        name->setObjectName("openGatePresetName");
        name->setMaxLength(80);
        form->addRow("Swatch name", name);
        gradient = new QCheckBox("Gradient · 2–5 stops");
        form->addRow(gradient);
        bar = new ColorGradientStopBar;
        form->addRow(bar);
        stop = new QComboBox;
        stop->setObjectName("openGateGradientStop");
        form->addRow("Editing stop", stop);
        auto actions = new QHBoxLayout;
        add = new QPushButton("Add Stop");
        remove = new QPushButton("Remove Stop");
        actions->addWidget(add);
        actions->addWidget(remove);
        form->addRow(actions);
        position = number(0, 100);
        form->addRow("Stop position %", position);
        hex = new QLineEdit;
        hex->setObjectName("openGatePresetHex");
        hex->setMaxLength(7);
        form->addRow("Hex", hex);
        pick = new QPushButton("Choose Color…");
        form->addRow(pick);
        hue = slider(0, 359);
        saturation = slider(0, 100);
        lightness = slider(0, 100);
        form->addRow("Hue", hue);
        form->addRow("Saturation", saturation);
        form->addRow("Lightness", lightness);
        stopAlpha = number(0, 100);
        form->addRow("Stop opacity %", stopAlpha);
        angle = new QComboBox;
        angle->addItem("Left → right", 90);
        angle->addItem("Diagonal", 135);
        angle->addItem("Top → bottom", 180);
        form->addRow("Gradient direction", angle);
        texture = new QComboBox;
        texture->setObjectName("openGatePresetTexture");
        const auto ids = ColorPreset::textures(), names = ColorPreset::textureNames();
        for (int i = 0; i < ids.size(); ++i)
            texture->addItem(names[i], ids[i]);
        form->addRow("Texture for this preset", texture);
        strength = number(0, 100);
        form->addRow("Texture strength %", strength);
        opacity = number(0, 100);
        form->addRow("Paint opacity %", opacity);
        state = new QLabel;
        state->setWordWrap(true);
        box->addWidget(state);
        auto buttons = new QHBoxLayout;
        revert = new QPushButton("Revert");
        updateButton = new QPushButton("Update Preset");
        updateButton->setObjectName("openGateUpdatePreset");
        buttons->addStretch();
        buttons->addWidget(revert);
        buttons->addWidget(updateButton);
        box->addLayout(buttons);
        connect(name, &QLineEdit::textEdited, this, [this](const QString &v) {
            draft.name = v;
            changed();
        });
        connect(hex, &QLineEdit::textEdited, this, [this](const QString &v) {
            const QRegularExpression valid("^#[0-9A-Fa-f]{6}$");
            if (valid.match(v).hasMatch()) {
                setColor(QColor(v));
                changed();
            }
        });
        connect(pick, &QPushButton::clicked, this, [this] {
            const auto c = QColorDialog::getColor(color(), this, "Preset color",
                                                  QColorDialog::DontUseNativeDialog);
            if (c.isValid()) {
                setColor(c);
                changed();
            }
        });
        for (auto s : {hue, saturation, lightness})
            connect(s, &QSlider::valueChanged, this, [this] {
                if (rendering)
                    return;
                setColor(QColor::fromHslF(hue->value() / 360., saturation->value() / 100.,
                                          lightness->value() / 100.));
                changed();
            });
        connect(gradient, &QCheckBox::toggled, this, [this](bool enabled) {
            if (rendering)
                return;
            if (enabled)
                draft.stops = {{ColorPaletteId(), draft.color, 0, 1},
                               {ColorPaletteId(), QColor("#00d4ff"), 1, 1}};
            else {
                draft.color = color();
                draft.stops.clear();
            }
            selectedStop = 0;
            changed();
        });
        connect(stop, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int i) {
            if (rendering || i < 0)
                return;
            selectedStop = i;
            refresh();
        });
        bar->chosen = [this](const QString &id) {
            for (int i = 0; i < draft.stops.size(); ++i)
                if (draft.stops[i].id == id) {
                    selectedStop = i;
                    refresh();
                    break;
                }
        };
        bar->moved = [this](const QString &id, double value) {
            for (int i = 0; i < draft.stops.size(); ++i)
                if (draft.stops[i].id == id) {
                    selectedStop = i;
                    position->setValue(value * 100);
                    break;
                }
        };
        bar->addRequested = [this](double value) { addStop(value); };
        bar->removeRequested = [this](const QString &id) {
            bar->chosen(id);
            remove->click();
        };
        connect(add, &QPushButton::clicked, this, [this] { addStop(); });
        connect(remove, &QPushButton::clicked, this, [this] {
            if (draft.stops.size() > 2) {
                draft.stops.removeAt(selectedStop);
                selectedStop = qMin(selectedStop, draft.stops.size() - 1);
                changed();
            }
        });
        connect(position, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
                [this](double value) {
                    if (rendering || draft.stops.isEmpty())
                        return;
                    const auto id = draft.stops[selectedStop].id;
                    draft.stops[selectedStop].position = value / 100;
                    std::stable_sort(
                        draft.stops.begin(), draft.stops.end(),
                        [](const auto &a, const auto &b) { return a.position < b.position; });
                    for (int i = 0; i < draft.stops.size(); ++i)
                        if (draft.stops[i].id == id)
                            selectedStop = i;
                    changed();
                });
        connect(stopAlpha, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
                [this](double value) {
                    if (!rendering && !draft.stops.isEmpty()) {
                        draft.stops[selectedStop].opacity = value / 100;
                        changed();
                    }
                });
        connect(angle, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
            if (!rendering) {
                draft.angle = angle->currentData().toDouble();
                changed();
            }
        });
        connect(texture, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
            if (!rendering) {
                draft.texture = texture->currentData().toString();
                changed();
            }
        });
        connect(strength, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
                [this](double value) {
                    if (!rendering) {
                        draft.strength = value / 100;
                        changed();
                    }
                });
        connect(opacity, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
                [this](double value) {
                    if (!rendering) {
                        draft.opacity = value / 100;
                        changed();
                    }
                });
        connect(revert, &QPushButton::clicked, this, [this] {
            draft = saved;
            selectedStop = 0;
            changed();
        });
        connect(updateButton, &QPushButton::clicked, this, [this] {
            if (commit)
                commit(draft);
        });
    }
    void setPreset(const ColorPreset &preset) {
        restoreDraft(preset, preset);
    }
    void restoreDraft(const ColorPreset &preset, const ColorPreset &working) {
        saved = preset;
        draft = working;
        selectedStop = 0;
        refresh();
        if (preview)
            preview(draft);
    }
    ColorPreset value() const {
        return draft;
    }
    bool modified() const {
        return !(saved == draft);
    }

  private:
    ColorPreset saved, draft;
    ColorGradientStopBar *bar;
    int selectedStop = 0;
    bool rendering = false;
    QLineEdit *name, *hex;
    QCheckBox *gradient;
    QComboBox *stop, *angle, *texture;
    QDoubleSpinBox *position, *stopAlpha, *strength, *opacity;
    QSlider *hue, *saturation, *lightness;
    QPushButton *pick, *add, *remove, *revert, *updateButton;
    QLabel *state;
    QFormLayout *form;
    static QString ColorPaletteId() {
        return QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    static QDoubleSpinBox *number(double min, double max) {
        auto s = new QDoubleSpinBox;
        s->setRange(min, max);
        s->setDecimals(0);
        s->setKeyboardTracking(false);
        return s;
    }
    static QSlider *slider(int min, int max) {
        auto s = new QSlider(Qt::Horizontal);
        s->setRange(min, max);
        return s;
    }
    QColor color() const {
        return draft.stops.isEmpty() ? draft.color : draft.stops[selectedStop].color;
    }
    void setColor(QColor c) {
        if (draft.stops.isEmpty())
            draft.color = c;
        else {
            draft.stops[selectedStop].color = c;
            draft.color = draft.stops.front().color;
        }
    }
    void addStop(double at = -1) {
        if (draft.stops.size() < 2 || draft.stops.size() >= 5)
            return;
        if (at < 0) {
            double gap = -1;
            for (int i = 1; i < draft.stops.size(); ++i) {
                const auto distance = draft.stops[i].position - draft.stops[i - 1].position;
                if (distance > gap) {
                    gap = distance;
                    at = (draft.stops[i].position + draft.stops[i - 1].position) / 2;
                }
            }
        }
        int index = 0;
        while (index < draft.stops.size() && draft.stops[index].position < at)
            ++index;
        const auto &a = draft.stops[qMax(0, index - 1)],
                   &b = draft.stops[qMin(index, draft.stops.size() - 1)];
        const auto t = a.position == b.position
                           ? 0
                           : qBound(0., (at - a.position) / (b.position - a.position), 1.);
        const auto color =
            QColor::fromRgbF(a.color.redF() + (b.color.redF() - a.color.redF()) * t,
                             a.color.greenF() + (b.color.greenF() - a.color.greenF()) * t,
                             a.color.blueF() + (b.color.blueF() - a.color.blueF()) * t);
        const ColorStop stop{ColorPaletteId(), color, at, a.opacity + (b.opacity - a.opacity) * t};
        draft.stops.insert(index, stop);
        selectedStop = index;
        changed();
    }
    void changed(bool fields = true) {
        if (fields)
            refresh();
        else
            refreshStatus();
        if (preview)
            preview(draft);
    }
    void refreshStatus() {
        const bool changed = modified();
        updateButton->setEnabled(changed && !draft.name.trimmed().isEmpty());
        revert->setEnabled(changed);
        state->setText(changed ? "Unsaved preset changes · Update Preset to save"
                       : !draft.opaque()
                           ? "Preset saved · transparency is preview-only in this trial"
                           : "Preset saved");
    }
    void refresh() {
        rendering = true;
        const QSignalBlocker n(name), h(hex);
        name->setText(draft.name);
        hex->setText(color().name());
        gradient->setChecked(!draft.stops.isEmpty());
        bar->setVisible(!draft.stops.isEmpty());
        if (!draft.stops.isEmpty())
            bar->setStops(draft.stops, draft.stops[selectedStop].id);
        stop->clear();
        for (int i = 0; i < draft.stops.size(); ++i)
            stop->addItem(QString("Stop %1 · %2").arg(i + 1).arg(draft.stops[i].color.name()));
        stop->setCurrentIndex(selectedStop);
        for (auto w : QList<QWidget *>{stop, position, stopAlpha, angle, add, remove}) {
            w->setVisible(!draft.stops.isEmpty());
            if (auto label = form->labelForField(w))
                label->setVisible(!draft.stops.isEmpty());
        }
        add->setEnabled(draft.stops.size() < 5);
        remove->setEnabled(draft.stops.size() > 2);
        if (!draft.stops.isEmpty()) {
            position->setValue(draft.stops[selectedStop].position * 100);
            stopAlpha->setValue(draft.stops[selectedStop].opacity * 100);
        }
        const auto c = color();
        hue->setValue(qRound(qMax(0., c.hslHueF()) * 359));
        saturation->setValue(qRound(c.hslSaturationF() * 100));
        lightness->setValue(qRound(c.lightnessF() * 100));
        angle->setCurrentIndex(angle->findData(int(draft.angle)));
        texture->setCurrentIndex(texture->findData(draft.texture));
        strength->setValue(draft.strength * 100);
        strength->setEnabled(draft.texture != "flat");
        opacity->setValue(draft.opacity * 100);
        rendering = false;
        refreshStatus();
    }
};
} // namespace Unbound
