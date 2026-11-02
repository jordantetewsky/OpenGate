// SPDX-License-Identifier: MIT
#pragma once
#include <QMouseEvent>
#include <QScrollBar>
#include <QToolButton>
#include <QSignalBlocker>
#include <QtMath>
#include <functional>
#include "panelscalecontrols.h"

namespace Unbound {
// Only the divider gesture and the header overflow/pin controls. State changes
// go through the caller; this widget never writes workspace settings.
class TimelineHeaderDivider : public QWidget {
public:
    explicit TimelineHeaderDivider(QWidget *owner) : QWidget(owner) {
        setObjectName("openGateTimelineHeaderDivider");
        setCursor(Qt::SplitHCursor);
        setToolTip("Drag to resize track controls");
        setAccessibleName("Track controls width");
    }
    std::function<void(double)> moved;
protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() != Qt::LeftButton)
            return;
        dragging = true;
        origin = event->globalPos().x();
        event->accept();
    }
    void mouseMoveEvent(QMouseEvent *event) override {
        if (!dragging)
            return;
        const auto current = event->globalPos().x();
        if (moved)
            moved(current - origin);
        origin = current;
        event->accept();
    }
    void mouseReleaseEvent(QMouseEvent *event) override { dragging = false; event->accept(); }
private:
    bool dragging = false;
    int origin = 0;
};

class TimelineTrackHeaderControls {
public:
    explicit TimelineTrackHeaderControls(QWidget *owner) {
        divider = new TimelineHeaderDivider(owner);
        bar = new QScrollBar(Qt::Horizontal, owner);
        bar->setObjectName("openGateTimelineHeaderScroll");
        bar->setAccessibleName("Scroll track controls");
        bar->setFocusPolicy(Qt::NoFocus);
        // Resolve/macOS may use transient scrollbars. This independent header
        // control must remain visibly draggable even before the first scroll.
        bar->setStyleSheet(
            "QScrollBar:horizontal { background:#202124; border:1px solid #111214; height:16px; margin:0; }"
            "QScrollBar::handle:horizontal { background:#60666e; min-width:24px; border-radius:4px; margin:2px; }"
            "QScrollBar::handle:horizontal:hover { background:#858c96; }"
            "QScrollBar::add-line:horizontal,QScrollBar::sub-line:horizontal { width:0; }"
            "QScrollBar::add-page:horizontal,QScrollBar::sub-page:horizontal { background:none; }");
        bar->setToolTip("Scroll all track controls, including blue/red buttons");
        pin = new QToolButton(owner);
        pin->setObjectName("openGateTimelinePinTrackControls");
        pin->setText(QString::fromUtf8("●"));
        pin->setToolTip("Keep the whole track-control area at the left while panning time");
        pin->setAccessibleName("Pin track-control area");
        pin->setCheckable(true);
        pin->setChecked(true);
        pin->setAutoRaise(true);
        pin->setFocusPolicy(Qt::NoFocus);
        scaleButton=new QToolButton(owner);scaleButton->setObjectName("openGateTimelineHeaderScale");
        scaleButton->setAutoRaise(true);scaleButton->setFocusPolicy(Qt::NoFocus);scaleButton->setPopupMode(QToolButton::InstantPopup);
        scaleButton->setToolTip("Track controls and timecode size; row alignment is shared with tracks");
        auto menu=new QMenu(scaleButton);scaleButton->setMenu(menu);
        QObject::connect(menu,&QMenu::aboutToShow,scaleButton,[this,menu] {
            menu->clear();menu->addSection("Track controls and timecode size");
            if(value && set)populatePanelScaleMenu(menu,scaleButton,value,set,maximum?maximum():3.);
            menu->addSeparator();auto match=menu->addAction("Match Timeline size");
            QObject::connect(match,&QAction::triggered,scaleButton,[this]{if(set)set(0.);});
        });
    }
    static constexpr int footerHeight = 18;
    static constexpr int footerControlsWidth=66,minimumWidth=144;
    void retire() {
        value={};maximum={};set={};divider->moved={};
        scaleButton->menu()->clear();scaleButton->menu()->close();
    }
    std::function<double()> value,maximum;
    std::function<void(double)> set;
    void arrange(int width, int height, int bottom, double scale, double offset, double maximum, bool pinned) {
        const int thickness = qMax(1, height-bottom);
        const int top = qBound(0, bottom, qMax(0, height - thickness));
        divider->setGeometry(width - 3, 0, 6, top);
        const int pinWidth = 18;
        pin->setGeometry(0, top, pinWidth, thickness);
        auto font=pin->font();font.setPixelSize(qBound(6,thickness-2,12));pin->setFont(font);
        scaleButton->setGeometry(pinWidth,top,footerControlsWidth-pinWidth,thickness);
        if(value)scaleButton->setText(QString::number(qRound(value()*100))+"%");
        bar->setGeometry(footerControlsWidth, top, qMax(0, width-footerControlsWidth), thickness);
        const QSignalBlocker barBlock(bar), pinBlock(pin);
        bar->setRange(0, qCeil(maximum * scale));
        bar->setPageStep(qMax(1,width));
        bar->setValue(maximum>0. && offset>=maximum-0.000001?bar->maximum():qRound(offset * scale));
        pin->setChecked(pinned);
        divider->setVisible(width>0);pin->show();scaleButton->show();bar->setVisible(width>footerControlsWidth);
        divider->raise();pin->raise();scaleButton->raise();bar->raise();
    }
    TimelineHeaderDivider *divider;
    QScrollBar *bar;
    QToolButton *pin;
    QToolButton *scaleButton;
};
}
