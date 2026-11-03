// SPDX-License-Identifier: MIT
#pragma once
#include "colorpresetpaint.h"
#include <QPainterPath>
#include <QWidget>
namespace Unbound {
// A generic, non-project sample clip/waveform for editing colors. A user-selected
// thumbnail belongs to this preview only, never to the palette or native project.
class ColorPresetPreview : public QWidget {
  public:
    explicit ColorPresetPreview(QWidget *owner = nullptr) : QWidget(owner) {
        setMinimumHeight(170);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        setObjectName("openGateColorPresetPreview");
    }
    void setPreset(ColorPreset value) {
        preset = std::move(value);
        update();
    }
    void setThumbnail(QImage image) {
        thumbnail = std::move(image);
        update();
    }
    void setTextures(bool value) {
        textures = value;
        update();
    }
    void setReference(QColor color, QString name) {
        reference = color;
        referenceName = std::move(name);
        update();
    }

  protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.fillRect(rect(), QColor(32, 32, 37));
        const int gap = 8;
        const QRectF stock(gap, gap, (width() - 3 * gap) * .25, height() * .52);
        const QRectF video(stock.right() + gap, gap, width() - stock.right() - 2 * gap,
                           height() * .52),
            audio(gap, video.bottom() + gap, width() - 2 * gap,
                  height() - video.bottom() - 2 * gap);
        for (const auto &bounds : {video, audio})
            paint.fill(p, bounds, preset, {}, textures);
        p.fillRect(stock, reference);
        for (const auto &clip : {stock, video}) {
            auto film = clip.adjusted(3, 3, -3, -25);
            if (!thumbnail.isNull()) {
                p.save();
                p.setClipRect(film);
                const int frameWidth = qMax(1, qRound(film.height() * 16 / 9));
                for (double x = film.left(); x < film.right(); x += frameWidth)
                    p.drawImage(QRectF(x, film.top(), frameWidth, film.height()), thumbnail);
                p.restore();
            } else {
                p.fillRect(film, QColor(55, 59, 67));
                p.setPen(QColor(157, 164, 177));
                p.drawText(film, Qt::AlignCenter, "Sample video");
            }
        }
        const auto stockText = colorLabelStyle({reference});
        p.setPen(stockText.text);
        p.drawText(stock.adjusted(7, stock.height() - 23, -4, 0), Qt::AlignVCenter | Qt::AlignLeft,
                   QString("Stock %1").arg(referenceName));
        QPainterPath waveform;
        for (int x = 0; x < int(audio.width()); ++x) {
            const auto h = (audio.height() - 30) *
                           (.12 + .32 * std::abs(std::sin(x * .071) * std::sin(x * .023)));
            waveform.moveTo(audio.left() + x, audio.center().y() - h);
            waveform.lineTo(audio.left() + x, audio.center().y() + h);
        }
        p.setPen(QPen(QColor(220, 247, 230), 1));
        p.drawPath(waveform);
        const auto label = paint.label(preset, QColor(32, 32, 37), textures);
        for (const auto &bounds : {video, audio}) {
            const auto band = QRectF(bounds.left(), bounds.bottom() - 23, bounds.width(), 23);
            const auto text =
                bounds == video ? QString("Sample video clip") : QString("Sample audio clip");
            if (label.scrim.isValid()) {
                const auto width =
                    qMin(band.width(), double(p.fontMetrics().horizontalAdvance(text) + 14));
                p.fillRect(QRectF(band.left(), band.top(), width, band.height()), label.scrim);
            }
            p.setPen(label.text);
            p.drawText(band.adjusted(7, 0, -7, 0), Qt::AlignVCenter | Qt::AlignLeft, text);
        }
    }

  private:
    ColorPreset preset;
    ColorPresetPaint paint;
    QImage thumbnail;
    bool textures = true;
    QColor reference = QColor("#4376A1");
    QString referenceName = "Blue";
};
} // namespace Unbound
