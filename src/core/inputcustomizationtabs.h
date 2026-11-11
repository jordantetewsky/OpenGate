// SPDX-License-Identifier: MIT
#pragma once
#include <QStylePainter>
#include <QStyleOptionTab>
#include <QTabBar>
#include <QTabWidget>

namespace Unbound {
// Native tab chrome with text-sized labels. Resolve's fixed-width tab label
// styling must not crop these longer names, including after metric scaling.
class InputCustomizationTabBar : public QTabBar {
public:
    explicit InputCustomizationTabBar(QWidget *parent=nullptr):QTabBar(parent) {
        setExpanding(false);setElideMode(Qt::ElideNone);setUsesScrollButtons(true);
    }
protected:
    QSize tabSizeHint(int index) const override {
        const auto metrics=fontMetrics();
        const auto padding=qMax(16,metrics.height());
        return QTabBar::tabSizeHint(index).expandedTo(
            QSize(metrics.horizontalAdvance(tabText(index))+2*padding,metrics.height()+padding));
    }
    QSize minimumTabSizeHint(int index) const override {return tabSizeHint(index);}
    void paintEvent(QPaintEvent *) override {
        QStylePainter painter(this);
        // Let Resolve draw the same native tab shape/underline, but give the
        // label our full measured rectangle instead of its fixed text subrect.
        const auto paint=[&](int index) {
            QStyleOptionTab option;initStyleOption(&option,index);
            const auto text=option.text;option.text.clear();
            painter.drawControl(QStyle::CE_TabBarTab,option);
            painter.setFont(font());
            const auto inset=qMax(8,fontMetrics().height()/2);
            style()->drawItemText(&painter,tabRect(index).adjusted(inset,0,-inset,0),
                Qt::AlignCenter|Qt::TextShowMnemonic,option.palette,isTabEnabled(index),text,QPalette::WindowText);
        };
        for(int index=0;index<count();++index)if(index!=currentIndex())paint(index);
        if(currentIndex()>=0)paint(currentIndex());
    }
};
class InputCustomizationTabs : public QTabWidget {
public:
    explicit InputCustomizationTabs(QWidget *parent=nullptr):QTabWidget(parent) {
        setTabBar(new InputCustomizationTabBar(this));
    }
};
}
