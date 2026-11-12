// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QDialog>
#include <QScrollArea>
#include <QScrollBar>
#include <QTabWidget>

namespace OpenGateTest {
template<class Check>
void inputHostingChecks(QTabWidget *tabs,QDialog *native,QScrollArea *area,Check check) {
    auto bar=tabs->tabBar();
    const auto originalStyle=bar->styleSheet();
    // Reproduce Resolve-style fixed-width tab rules against the real owned
    // tab bar; full labels must survive at normal and enlarged metric scales.
    bar->setStyleSheet("QTabBar::tab { width: 70px; }");
    const auto originalFont=bar->font();
    for(const auto factor:{1.,1.7}) {
        auto font=originalFont;
        if(font.pixelSize()>0)font.setPixelSize(qRound(font.pixelSize()*factor));
        else font.setPointSizeF(font.pointSizeF()*factor);
        bar->setFont(font);QApplication::processEvents();
        for(int index=0;index<bar->count();++index)
            check(bar->tabRect(index).width()>=bar->fontMetrics().horizontalAdvance(bar->tabText(index))+16,
                  "customization tab labels fit despite native fixed-width tab styling");
    }
    bar->setStyleSheet(originalStyle);bar->setFont(originalFont);QApplication::processEvents();
    tabs->setCurrentIndex(0);QApplication::processEvents();
    auto owner=native->parentWidget();
    const auto expected=owner->layout()->contentsRect().topLeft();
    native->move(-700,120);QApplication::processEvents();
    check(native->pos()==expected,"embedded native header cannot move keyboard content out of its layout");
    if(area && area->horizontalScrollBar()->maximum()>0) {
        area->horizontalScrollBar()->setValue(area->horizontalScrollBar()->maximum());
        QApplication::processEvents();
        check(area->horizontalScrollBar()->value()>0 && native->pos()==expected,
              "bounded overflow scrolls the panel without moving its native editor inside the tab");
        area->horizontalScrollBar()->setValue(0);
    }
}
}
