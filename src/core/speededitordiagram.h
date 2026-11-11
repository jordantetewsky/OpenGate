// SPDX-License-Identifier: MIT
#pragma once
#include <QAbstractButton>
#include <QButtonGroup>
#include <QFontMetricsF>
#include <QMap>
#include <QPainter>
#include <QResizeEvent>
#include <QtMath>
#include <QWidget>
#include <functional>

namespace Unbound {
// A selectable physical control, with native-editor keycap styling.
// Its checked state indicates editor selection, not a live hardware LED or mode.
class SpeedEditorKey : public QAbstractButton {
public:
    enum class Tone { SpaceGray, DarkGray, White, Red, Dial };
    SpeedEditorKey(QString id, QString legend, QString alternate, Tone tone,
                   QWidget *parent) : QAbstractButton(parent), alternate(alternate), tone(tone) {
        setObjectName("openGateSpeedEditorKey_" + id);
        setText(legend);
        setAccessibleName(legend.simplified());
        setToolTip(legend.simplified());
        setCheckable(true);
        setFocusPolicy(Qt::StrongFocus);
        setCursor(Qt::PointingHandCursor);
        setProperty("speedEditorKey", id);
    }
    Tone keyTone() const { return tone; }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const qreal em = QFontMetricsF(font()).height();
        const qreal line = qMax(1., em * .065);
        const QRectF outer = QRectF(rect()).adjusted(line, line, -line, -line);
        if (tone == Tone::Dial) {
            paintDial(painter, outer, em, line);
            return;
        }
        const QColor face = tone == Tone::White ? QColor("#e5e3df") :
                            tone == Tone::Red ? QColor("#a63740") :
                            tone == Tone::DarkGray ? QColor("#2b2e31") : QColor("#51575b");
        const QColor ink = tone == Tone::White ? QColor("#343536") : QColor("#e3e4e5");
        const qreal radius = em * .22;
        painter.setPen(QPen(QColor("#101112"), line));
        painter.setBrush(face.darker(145));
        painter.drawRoundedRect(outer, radius, radius);
        QRectF top = outer.adjusted(em * .12, em * .09, -em * .12, -em * .24);
        if (isDown()) top.translate(0, em * .07);
        painter.setPen(QPen(face.lighter(118), line * .65));
        painter.setBrush(underMouse() ? face.lighter(110) : face);
        painter.drawRoundedRect(top, radius * .7, radius * .7);
        painter.setPen(ink);
        auto legendFont = font();
        if (legendFont.pixelSize() > 0)
            legendFont.setPixelSize(qMax(1, qRound(legendFont.pixelSize() * .72)));
        else legendFont.setPointSizeF(legendFont.pointSizeF() * .72);
        legendFont.setWeight(QFont::DemiBold);
        painter.setFont(legendFont);
        const qreal bottom = alternate.isEmpty() ? 0 : em * .6;
        painter.drawText(top.adjusted(em * .08, 0, -em * .08, -bottom),
                         Qt::AlignCenter, text());
        if (!alternate.isEmpty()) {
            auto small = font();
            if (small.pixelSize() > 0) small.setPixelSize(qMax(1, qRound(small.pixelSize() * .45)));
            else small.setPointSizeF(small.pointSizeF() * .45);
            painter.setFont(small);
            painter.setPen(ink.darker(tone == Tone::White ? 100 : 115));
            painter.drawText(top.adjusted(0, top.height() - em * .75, 0, 0),
                             Qt::AlignHCenter | Qt::AlignVCenter, alternate);
        }
        paintSelection(painter, outer, radius, line);
    }
    bool hitButton(const QPoint &point) const override {
        if (tone != Tone::Dial) return rect().contains(point);
        const QPointF distance = QPointF(point) - QRectF(rect()).center();
        const qreal radius = qMin(width(), height()) * .5;
        return distance.x() * distance.x() + distance.y() * distance.y() <= radius * radius;
    }

private:
    QString alternate;
    Tone tone;
    void paintSelection(QPainter &painter, const QRectF &rect, qreal radius, qreal line) {
        if (!isChecked() && !hasFocus()) return;
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor("#f0f0f0"), line * 1.7,
                            isChecked() ? Qt::SolidLine : Qt::DashLine));
        painter.drawRoundedRect(rect, radius, radius);
    }
    void paintDial(QPainter &painter, QRectF outer, qreal em, qreal line) {
        const qreal diameter = qMin(outer.width(), outer.height());
        outer = QRectF(outer.center() - QPointF(diameter / 2, diameter / 2),
                       QSizeF(diameter, diameter));
        painter.setPen(QPen(QColor("#08090a"), line * 1.6));
        painter.setBrush(QColor("#17191b"));
        painter.drawEllipse(outer);
        const auto face = outer.adjusted(em * .24, em * .24, -em * .24, -em * .24);
        QLinearGradient finish(face.topLeft(), face.bottomRight());
        finish.setColorAt(0, QColor("#35383b"));
        finish.setColorAt(1, QColor("#141618"));
        painter.setPen(QPen(QColor("#44474a"), line));
        painter.setBrush(finish);
        painter.drawEllipse(face);
        const qreal recess = diameter * .11;
        const QPointF center = face.center() + QPointF(-diameter * .20, diameter * .20);
        QRadialGradient grip(center, recess);
        grip.setColorAt(0, QColor("#101214"));
        grip.setColorAt(.75, QColor("#25282a"));
        grip.setColorAt(1, QColor("#303336"));
        painter.setPen(Qt::NoPen);
        painter.setBrush(grip);
        painter.drawEllipse(center, recess, recess);
        paintSelection(painter, outer, diameter / 2, line);
    }
};

// Owns the device diagram and selection only; profile and dispatch owners live outside it.
class SpeedEditorDiagram : public QWidget {
public:
    explicit SpeedEditorDiagram(QWidget *parent = nullptr) : QWidget(parent) {
        setObjectName("openGateSpeedEditorDiagram");
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        auto group = new QButtonGroup(this);
        group->setExclusive(true);
        using T = SpeedEditorKey::Tone;
        const auto add = [&](QString id, QString text, QString alternate,
                             T tone, qreal x, qreal y, qreal width = 1., qreal height = 1.) {
            auto key = new SpeedEditorKey(id, text, alternate, tone, this);
            group->addButton(key);
            keys.insert(id, key);
            positions.append({key, QRectF(x, y, width, height)});
            connect(key, &QAbstractButton::clicked, this, [this, id] {
                current = id;
                if (selected) selected(current);
            });
        };
        // Physical rows and group spacing follow Blackmagic's Speed Editor.
        add("smart_insrt", "SMART\nINSRT", "CLIP", T::SpaceGray, 0, 0);
        add("appnd", "APPND", "CLIP", T::SpaceGray, 1, 0);
        add("ripl_owr", "RIPL\nO/WR", "", T::SpaceGray, 2, 0);
        add("close_up", "CLOSE\nUP", "YPOS", T::SpaceGray, 0, 1);
        add("place_on_top", "PLACE\nON TOP", "", T::SpaceGray, 1, 1);
        add("src_owr", "SRC\nO/WR", "", T::SpaceGray, 2, 1);
        add("in", "IN", "CLR", T::White, 0, 2.4, 1.5);
        add("out", "OUT", "CLR", T::White, 1.5, 2.4, 1.5);
        add("trim_in", "TRIM\nIN", "", T::DarkGray, 0, 3.4);
        add("trim_out", "TRIM\nOUT", "", T::DarkGray, 1, 3.4);
        add("roll", "ROLL", "SLIDE", T::DarkGray, 2, 3.4);
        add("slip_src", "SLIP\nSRC", "", T::DarkGray, 0, 4.4);
        add("slip_dest", "SLIP\nDEST", "", T::DarkGray, 1, 4.4);
        add("trans_dur", "TRANS\nDUR", "SET", T::DarkGray, 2, 4.4);
        add("cut", "CUT", "", T::SpaceGray, 0, 5.4);
        add("dis", "DIS", "", T::SpaceGray, 1, 5.4);
        add("smth_cut", "SMTH\nCUT", "", T::SpaceGray, 2, 5.4);

        add("esc", "ESC", "UNDO", T::SpaceGray, 3.7, 0);
        add("sync_bin", "SYNC\nBIN", "", T::SpaceGray, 4.7, 0);
        add("audio_level", "AUDIO\nLEVEL", "MARK", T::DarkGray, 5.7, 0);
        add("full_view", "FULL\nVIEW", "RVW", T::Red, 6.7, 0);
        add("trans", "TRANS", "TITLE", T::SpaceGray, 3.7, 1);
        add("split", "SPLIT", "MOVE", T::SpaceGray, 4.7, 1);
        add("snap", "SNAP", "", T::SpaceGray, 5.7, 1);
        add("ripl_del", "RIPL\nDEL", "", T::SpaceGray, 6.7, 1);
        for (int row = 0; row < 3; ++row) for (int column = 0; column < 3; ++column) {
            const int number = (2 - row) * 3 + column + 1;
            const auto id = QString("cam_%1").arg(number);
            const auto legend = QString("CAM\n%1").arg(number);
            add(id, legend, {}, T::DarkGray, 3.7 + column, 2.4 + row);
        }
        add("live_owr", "LIVE\nO/WR", "RND", T::DarkGray, 6.7, 2.4);
        add("video_only", "VIDEO\nONLY", "", T::SpaceGray, 6.7, 3.4);
        add("audio_only", "AUDIO\nONLY", "", T::SpaceGray, 6.7, 4.4);
        add("stop_play", "STOP/PLAY", "", T::SpaceGray, 3.7, 5.4, 4);

        add("source", "SOURCE", "", T::SpaceGray, 8.4, 0, 1.5);
        add("timeline", "TIMELINE", "", T::SpaceGray, 9.9, 0, 1.5);
        add("shtl", "SHTL", "", T::White, 8.4, 1.4);
        add("jog", "JOG", "", T::White, 9.4, 1.4);
        add("scrl", "SCRL", "", T::White, 10.4, 1.4);
        add("dial", "Search Dial", "", T::Dial, 8.35, 3.25, 3.15, 3.15);
        setSelectedKey("roll");
    }
    QString selectedKey() const { return current; }
    bool setSelectedKey(const QString &id) {
        auto key = keys.value(id);
        if (!key) return false;
        current = id;
        key->setChecked(true);
        return true;
    }
    SpeedEditorKey *button(const QString &id) const { return keys.value(id); }
    QStringList keyIds() const { return keys.keys(); }
    std::function<void(QString)> selected;
    QSize minimumSizeHint() const override {
        const qreal unit = QFontMetricsF(font()).height() * 3.3;
        return QSize(qCeil(12.1 * unit), qCeil(7.1 * unit));
    }
    QSize sizeHint() const override { return minimumSizeHint(); }

protected:
    void resizeEvent(QResizeEvent *) override { arrange(); }
    void changeEvent(QEvent *event) override {
        QWidget::changeEvent(event);
        if (event->type() == QEvent::FontChange) {
            updateGeometry();
            arrange();
        }
    }
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const auto bounds = deviceRect();
        const qreal em = QFontMetricsF(font()).height();
        painter.setPen(QPen(QColor("#191a1c"), qMax(1., em * .065)));
        painter.setBrush(QColor("#3a3c3f"));
        painter.drawRoundedRect(bounds, em * .65, em * .65);
    }

private:
    struct Position { SpeedEditorKey *key; QRectF units; };
    QList<Position> positions;
    QMap<QString, SpeedEditorKey *> keys;
    QString current;
    QRectF deviceRect() const {
        const qreal unit = qMin(width() / 12.1, height() / 7.1);
        const QSizeF size(unit * 12.1, unit * 7.1);
        return QRectF((width() - size.width()) / 2, (height() - size.height()) / 2,
                      size.width(), size.height());
    }
    void arrange() {
        const auto bounds = deviceRect();
        const qreal unit = bounds.width() / 12.1;
        const QPointF origin = bounds.topLeft() + QPointF(unit * .35, unit * .35);
        const qreal gap = unit * .045;
        for (const auto &position : positions) {
            const auto r = position.units;
            position.key->setGeometry(QRectF(origin + QPointF(r.x() * unit, r.y() * unit),
                                              QSizeF(r.width() * unit - gap, r.height() * unit - gap)).toAlignedRect());
        }
    }
};
} // namespace Unbound
