// SPDX-License-Identifier: MIT
#pragma once
#include <QFrame>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QScreen>
#include <QSignalBlocker>
#include <QSlider>
#include <QVBoxLayout>
#include <functional>

namespace Unbound {
// Pick one RGB color through a clickable pill and a transient live picker.
// Draft selection, palette storage, clip assignment and history belong to callers.
class ColorPickerButton : public QPushButton {
  public:
    std::function<void(QColor)> colorChanged;
    explicit ColorPickerButton(QWidget *owner = nullptr) : QPushButton(owner) {
        setObjectName("openGateColorPickerButton");
        setIconSize(QSize(44, 18));
        connect(this, &QPushButton::clicked, this, [this] { openPicker(); });
        setColor(Qt::red);
    }
    void setColor(QColor color, QString target = "Solid color") {
        current = color;
        caption = std::move(target);
        QPixmap pill(QSize(44, 18) * devicePixelRatioF());
        pill.setDevicePixelRatio(devicePixelRatioF());
        pill.fill(Qt::transparent);
        QPainter painter(&pill);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QColor(15, 15, 18));
        painter.setBrush(color);
        painter.drawRoundedRect(QRectF(1, 1, 42, 16), 8, 8);
        painter.end();
        setIcon(QIcon(pill));
        setText(color.name().toUpper());
        setAccessibleName(caption + " " + color.name());
        setToolTip("Edit " + caption.toLower() + " · preview updates while dragging");
    }
    void dismissPicker() {
        if (popup)
            popup->close();
    }

  private:
    class Plane : public QWidget {
      public:
        std::function<void(double, double)> chosen;
        double hue = 0, saturation = 1, value = 1;
        explicit Plane(QWidget *owner) : QWidget(owner) {
            setObjectName("openGateColorPickerPlane");
            setMinimumSize(220, 150);
            setAccessibleName("Color saturation and brightness");
            setCursor(Qt::CrossCursor);
        }

      protected:
        void paintEvent(QPaintEvent *) override {
            QPainter p(this);
            QLinearGradient horizontal(0, 0, width(), 0);
            horizontal.setColorAt(0, Qt::white);
            horizontal.setColorAt(1, QColor::fromHsvF(hue, 1, 1));
            p.fillRect(rect(), horizontal);
            QLinearGradient vertical(0, 0, 0, height());
            vertical.setColorAt(0, Qt::transparent);
            vertical.setColorAt(1, Qt::black);
            p.fillRect(rect(), vertical);
            const QPointF at(saturation * (width() - 1), (1 - value) * (height() - 1));
            p.setRenderHint(QPainter::Antialiasing);
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(Qt::black, 3));
            p.drawEllipse(at, 5, 5);
            p.setPen(QPen(Qt::white, 1.5));
            p.drawEllipse(at, 5, 5);
        }
        void mousePressEvent(QMouseEvent *event) override {
            if (event->button() == Qt::LeftButton)
                select(event->localPos());
        }
        void mouseMoveEvent(QMouseEvent *event) override {
            if (event->buttons().testFlag(Qt::LeftButton))
                select(event->localPos());
        }

      private:
        void select(QPointF point) {
            saturation = qBound(0., point.x() / qMax(1, width() - 1), 1.);
            value = 1 - qBound(0., point.y() / qMax(1, height() - 1), 1.);
            if (chosen)
                chosen(saturation, value);
            update();
        }
    };
    class Popup : public QFrame {
      public:
        std::function<void(QColor)> changed;
        Popup(QWidget *owner, QColor color, const QString &caption)
            : QFrame(owner, Qt::Popup | Qt::BypassGraphicsProxyWidget), initial(color) {
            setObjectName("openGateColorPickerPopup");
            setAttribute(Qt::WA_DeleteOnClose);
            setFrameShape(QFrame::StyledPanel);
            setFocusPolicy(Qt::StrongFocus);
            auto layout = new QVBoxLayout(this);
            layout->addWidget(new QLabel(caption));
            plane = new Plane(this);
            layout->addWidget(plane);
            hue = new QSlider(Qt::Horizontal);
            hue->setObjectName("openGatePickerHue");
            hue->setAccessibleName("Hue");
            hue->setRange(0, 359);
            hue->setStyleSheet("QSlider::groove:horizontal { height: 12px; background: "
                               "qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 red,stop:0.166 yellow,"
                               "stop:0.333 lime,stop:0.5 cyan,stop:0.666 blue,stop:0.833 magenta,"
                              "stop:1 red); } QSlider::handle:horizontal { width: 12px; "
                              "margin: -4px 0; border: 2px solid white; border-radius: 7px; "
                              "background: #303038; }");
            layout->addWidget(hue);
            hex = new QLineEdit;
            hex->setObjectName("openGatePickerHex");
            hex->setAccessibleName("RGB hex color");
            hex->setMaxLength(7);
            layout->addWidget(hex);
            auto actions = new QHBoxLayout;
            auto cancel = new QPushButton("Cancel"), done = new QPushButton("Done");
            cancel->setObjectName("openGatePickerCancel");
            done->setObjectName("openGatePickerDone");
            actions->addStretch();
            actions->addWidget(cancel);
            actions->addWidget(done);
            layout->addLayout(actions);
            setColor(color);
            plane->chosen = [this](double s, double v) {
                publish(QColor::fromHsvF(plane->hue, s, v));
            };
            connect(hue, &QSlider::valueChanged, this, [this](int value) {
                plane->hue = value / 360.;
                publish(QColor::fromHsvF(plane->hue, plane->saturation, plane->value));
            });
            connect(hex, &QLineEdit::textEdited, this, [this](const QString &text) {
                const QRegularExpression valid("^#[0-9A-Fa-f]{6}$");
                if (valid.match(text).hasMatch())
                    publish(QColor(text));
            });
            connect(cancel, &QPushButton::clicked, this, [this] { cancelPick(); });
            connect(done, &QPushButton::clicked, this, [this] { close(); });
        }

      protected:
        void keyPressEvent(QKeyEvent *event) override {
            if (event->key() == Qt::Key_Escape) {
                cancelPick();
                event->accept();
            } else
                QFrame::keyPressEvent(event);
        }
        void hideEvent(QHideEvent *event) override {
            QFrame::hideEvent(event);
            deleteLater();
        }

      private:
        QColor initial;
        Plane *plane;
        QSlider *hue;
        QLineEdit *hex;
        void setColor(QColor color) {
            if (color.hsvHueF() >= 0)
                plane->hue = color.hsvHueF();
            plane->saturation = color.hsvSaturationF();
            plane->value = color.valueF();
            const QSignalBlocker h(hue), x(hex);
            hue->setValue(qRound(plane->hue * 360));
            hex->setText(color.name().toUpper());
            plane->update();
        }
        void publish(QColor color) {
            setColor(color);
            if (changed)
                changed(color);
        }
        void cancelPick() {
            if (changed)
                changed(initial);
            close();
        }
    };
    QColor current;
    QString caption;
    QPointer<Popup> popup;
    void openPicker() {
        dismissPicker();
        popup = new Popup(this, current, caption);
        if (window()->testAttribute(Qt::WA_DontShowOnScreen))
            popup->setAttribute(Qt::WA_DontShowOnScreen);
        QPointer<ColorPickerButton> alive = this;
        popup->changed = [alive](QColor color) {
            if (alive && alive->colorChanged)
                alive->colorChanged(color);
        };
        popup->adjustSize();
        const auto anchor = QRect(mapToGlobal(QPoint()), size());
        auto screen = QGuiApplication::screenAt(anchor.center());
        if (!screen)
            screen = QGuiApplication::primaryScreen();
        QPoint position(anchor.left(), anchor.bottom() + 3);
        if (screen) {
            const auto bounds = screen->availableGeometry();
            if (position.y() + popup->height() > bounds.bottom())
                position.setY(anchor.top() - popup->height() - 3);
            position.setX(qMax(bounds.left(), qMin(position.x(), bounds.right() - popup->width())));
            position.setY(
                qMax(bounds.top(), qMin(position.y(), bounds.bottom() - popup->height())));
        }
        popup->move(position);
        popup->show();
        popup->setFocus();
    }
};
} // namespace Unbound
