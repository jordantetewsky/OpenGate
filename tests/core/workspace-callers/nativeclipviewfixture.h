// SPDX-License-Identifier: MIT
#pragma once
#include <QAbstractScrollArea>
#include <QPushButton>
#include <QMouseEvent>

// Resolve's observed clip viewport is not QAbstractItemView. Model its native
// Source Clip activation, including a nested event turn before activation.
// This tests routing only; it cannot establish native pictures or blank hit tests.
class NativeClipViewFixture : public QAbstractScrollArea {
public:
    explicit NativeClipViewFixture(QPushButton *sourceMode):sourceMode(sourceMode) {
        setObjectName("mediaPoolClipView");setMinimumSize(120,90);
    }
protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override {
        loadOnRelease=event->button()==Qt::LeftButton && QRect(5,5,100,40).contains(event->pos());
        QApplication::processEvents();
    }
    void mouseReleaseEvent(QMouseEvent *) override {
        if(!loadOnRelease)return;
        loadOnRelease=false;
        sourceMode->click();sourceMode->click(); // Native trace includes repeated activation.
    }
private:
    QPushButton *sourceMode;
    bool loadOnRelease=false;
};
