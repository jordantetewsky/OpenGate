// SPDX-License-Identifier: MIT
#pragma once
#include <QLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QPointer>
#include <QStackedWidget>
#include <QTimer>
#include "diagnostics.h"
namespace Unbound {
// Limit only the disposable group wrapper, never Resolve's native constraints.
// A toolbar leaf should fit its controls instead of taking a splitter fraction.
class CompactToolbar : public QObject {
public:
    CompactToolbar(QWidget *wrapper,QWidget *surface):QObject(wrapper),wrapper(wrapper),surface(surface) {
        timer.setSingleShot(true);
        connect(&timer,&QTimer::timeout,this,[this] { fit(); });
        surface->installEventFilter(this);wrapper->installEventFilter(this);timer.start(0);
    }
protected:
    bool eventFilter(QObject *,QEvent *event) override {
        if (event->type()==QEvent::Resize || event->type()==QEvent::LayoutRequest || event->type()==QEvent::Show || event->type()==QEvent::FontChange)
            if (!timer.isActive()) timer.start(0);
        return false;
    }
private:
    void fit() {
        if (!wrapper || !surface) return;
        QWidget *row=surface;
        if (auto stack=qobject_cast<QStackedWidget *>(surface.data())) if (stack->currentWidget()) row=stack->currentWidget();
        const int hint=qMax(row->minimumHeight(),qMax(row->minimumSizeHint().height(),row->sizeHint().height()));
        if (hint<=0 || hint>120) return;
        int scroll=0;
        if (auto area=wrapper->findChild<QScrollArea *>())
            if (row->minimumSizeHint().width()>area->viewport()->width()) scroll=area->horizontalScrollBar()->sizeHint().height();
        const int height=qMax(20,hint)+scroll+wrapper->contentsMargins().top()+wrapper->contentsMargins().bottom();
        if (wrapper->maximumHeight()!=height) {
            wrapper->setMaximumHeight(height);wrapper->updateGeometry();
            logEvent("toolbar.compact.height",{{"height",height},{"surface",surface->objectName()}});
        }
    }
    QPointer<QWidget> wrapper,surface;QTimer timer;
};
}
