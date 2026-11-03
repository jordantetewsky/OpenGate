// SPDX-License-Identifier: MIT
#pragma once
#include "colorcategoryeditor.h"
#include "colormanagerfiles.h"
#include "colorpalettedrafts.h"
#include "colorpreseteditor.h"
#include "colorpresetpreview.h"
#include "colorstockcomparison.h"
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QSplitter>
namespace Unbound {
// Compose the palette, category editor, one-preset draft editor and neutral
// preview. Hosting/docking, native clip assignment and history live elsewhere.
class ColorManagerWidget : public QWidget {
  public:
    explicit ColorManagerWidget(QWidget *owner = nullptr, QString path = {})
        : QWidget(owner), palette(std::move(path)), drafts(this, palette.location() + ".drafts") {
        setObjectName("openGateColorManagerPanel");
        setWindowTitle("Clips & Tracks Color Manager");
        resize(1000, 780);
        auto box = new QVBoxLayout(this);
        box->setContentsMargins(8, 8, 8, 8);
        box->setSpacing(8);
        tools = new QGridLayout;
        auto import = new QPushButton("Import Presets…"),
             exportButton = new QPushButton("Export Presets…"),
             thumbnail = new QPushButton("Preview Thumbnail…");
        textureToggle = new QCheckBox("Textures");
        textureToggle->setChecked(true);
        tools->addWidget(import, 0, 0);
        tools->addWidget(exportButton, 0, 1);
        tools->addWidget(thumbnail, 1, 0);
        tools->addWidget(textureToggle, 1, 1);
        headerActions = {import, exportButton, thumbnail, textureToggle};
        box->addLayout(tools);
        splitter = new QSplitter;
        box->addWidget(splitter);
        categories = new ColorCategoryEditor;
        splitter->addWidget(categories);
        auto right = new QWidget;
        auto layout = new QVBoxLayout(right);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(8);
        sample = new ColorPresetPreview;
        layout->addWidget(sample);
        auto comparison = new ColorStockComparison;
        comparison->selected = [this](QColor color, QString name) {
            sample->setReference(color, std::move(name));
        };
        layout->addWidget(comparison);
        auto scroll = new QScrollArea;
        scroll->setWidgetResizable(true);
        editor = new ColorPresetEditor;
        scroll->setWidget(editor);
        layout->addWidget(scroll);
        splitter->addWidget(right);
        splitter->setStretchFactor(1, 1);
        splitter->setSizes({250, 750});
        auto actions = new QHBoxLayout;
        auto add = new QPushButton("Add Color"), duplicate = new QPushButton("Duplicate"),
             remove = new QPushButton("Remove Color");
        actions->addWidget(add);
        actions->addWidget(duplicate);
        actions->addWidget(remove);
        actions->addStretch();
        layout->addLayout(actions);
        status = new QLabel;
        status->setWordWrap(true);
        box->addWidget(status);
        editor->preview = [this, comparison](const ColorPreset &p) {
            sample->setPreset(p);
            comparison->setPreset(p);
            if (const auto saved = palette.find(p.id))
                drafts.record(p, *saved);
        };
        editor->commit = [this](const ColorPreset &p) {
            const auto error = palette.update(p);
            if (!report(error))
                return;
            drafts.discard(p.id);
            categories->setData(palette.state(), p.id);
            editor->setPreset(p);
        };
        categories->selected = [this](const QString &id) { select(id); };
        categories->changed = [this](const ColorPalette::Data &next) {
            report(palette.replace(next));
            categories->setData(palette.state(), selected);
        };
        connect(add, &QPushButton::clicked, this, [this] {
            ColorPreset p;
            p.id = ColorPalette::newId();
            p.name = "New Color";
            p.category = palette.state().categories.front().id;
            create(p);
        });
        connect(duplicate, &QPushButton::clicked, this, [this] {
            auto p = editor->value();
            p.id = ColorPalette::newId();
            p.name = (p.name + " copy").left(80);
            for (auto &s : p.stops)
                s.id = ColorPalette::newId();
            create(p);
        });
        connect(remove, &QPushButton::clicked, this, [this] {
            auto next = palette.state();
            for (int i = 0; i < next.presets.size(); ++i)
                if (next.presets[i].id == selected) {
                    next.presets.removeAt(i);
                    break;
                }
            if (report(palette.replace(next))) {
                drafts.discard(selected);
                selected.clear();
                reload();
            }
        });
        connect(textureToggle, &QCheckBox::toggled, sample, &ColorPresetPreview::setTextures);
        connect(thumbnail, &QPushButton::clicked, this, [this] {
            const auto file = QFileDialog::getOpenFileName(this, "Preview thumbnail", {},
                                                           "Images (*.png *.jpg *.jpeg *.webp)");
            if (file.isEmpty())
                return;
            QImage image;
            if (report(ColorManagerFiles::readThumbnail(file, image)))
                sample->setThumbnail(image);
        });
        connect(import, &QPushButton::clicked, this, [this] {
            const auto file = QFileDialog::getOpenFileName(this, "Import palette builder presets",
                                                           {}, "Palette JSON (*.json)");
            if (file.isEmpty())
                return;
            QByteArray bytes;
            if (!report(ColorManagerFiles::readPalette(file, bytes)))
                return;
            if (report(palette.import(bytes))) {
                selected.clear();
                reload();
            }
        });
        connect(exportButton, &QPushButton::clicked, this, [this] {
            const auto file = QFileDialog::getSaveFileName(this, "Export saved presets",
                                                           "OpenGate Palette Presets.json",
                                                           "Palette JSON (*.json)");
            if (file.isEmpty())
                return;
            report(ColorManagerFiles::writePalette(file, ColorPalette::encode(palette.state())));
        });
        reload();
        if (!palette.error().isEmpty())
            report(palette.error());
        else if (!drafts.error().isEmpty())
            report(drafts.error());
    }
    ColorPalette &store() {
        return palette;
    }
    ~ColorManagerWidget() override {
        editor->preview = {};
        editor->commit = {};
        categories->selected = {};
        categories->changed = {};
    }

  protected:
    void resizeEvent(QResizeEvent *event) override {
        QWidget::resizeEvent(event);
        const bool stack = width() < 480;
        if (tools && stack != headerStacked) {
            headerStacked = stack;
            for (auto item : headerActions)
                tools->removeWidget(item);
            for (int i = 0; i < headerActions.size(); ++i)
                tools->addWidget(headerActions[i], stack ? i : i / 2, stack ? 0 : i % 2, 1,
                                 stack ? 2 : 1);
        }
        const auto orientation = width() < 720 ? Qt::Vertical : Qt::Horizontal;
        if (splitter && splitter->orientation() != orientation)
            splitter->setOrientation(orientation);
    }

  private:
    ColorPalette palette;
    ColorCategoryEditor *categories;
    ColorPresetEditor *editor;
    ColorPresetPreview *sample;
    QLabel *status;
    QCheckBox *textureToggle;
    QSplitter *splitter = nullptr;
    QGridLayout *tools = nullptr;
    QList<QWidget *> headerActions;
    bool headerStacked = false;
    QString selected;
    ColorPaletteDrafts drafts;
    bool report(const QString &error) {
        status->setText(error);
        return error.isEmpty();
    }
    void select(QString id) {
        const auto saved = palette.find(id);
        if (!saved)
            return;
        auto draft = drafts.get(*saved);
        selected = id;
        editor->setEnabled(true);
        editor->restoreDraft(*saved, draft);
    }
    void reload() {
        categories->setData(palette.state(), selected);
        if (!palette.find(selected))
            selected =
                palette.state().presets.isEmpty() ? QString() : palette.state().presets.front().id;
        if (selected.isEmpty()) {
            editor->setEnabled(false);
            return;
        }
        categories->setData(palette.state(), selected);
        select(selected);
    }
    void create(ColorPreset p) {
        if (!report(palette.update(p)))
            return;
        selected = p.id;
        reload();
    }
};
} // namespace Unbound
