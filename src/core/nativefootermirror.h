// SPDX-License-Identifier: MIT
#pragma once
#include <QAbstractButton>
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QScopedValueRollback>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <functional>
namespace Unbound {
// Render the complete original footer at native logical size, retaining its
// layout, background, logo and native controls. No reconstructed button row.
class NativeFooterCanvas : public QWidget {
public:
    explicit NativeFooterCanvas(QWidget *parent=nullptr):QWidget(parent) {setMouseTracking(true);}
    QPointer<QWidget> source;
    std::function<void(QAbstractButton *)> activate;
    QAbstractButton *buttonAt(QPoint point) const {
        if(!source)return nullptr;
        auto child=source->childAt(point);
        for(auto object=child;object && object!=source;object=object->parentWidget())
            if(auto button=qobject_cast<QAbstractButton *>(object))return button;
        return nullptr;
    }
protected:
    void paintEvent(QPaintEvent *) override {
        if(!source || rendering)return;
        QScopedValueRollback<bool> guard(rendering,true);QPainter painter(this);
        source->render(&painter,QPoint(),QRegion(source->rect()),QWidget::DrawWindowBackground|QWidget::DrawChildren);
    }
    void mousePressEvent(QMouseEvent *event) override {
        if(event->button()==Qt::LeftButton){pressed=buttonAt(event->pos());event->accept();return;}
        QWidget::mousePressEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent *event) override {
        if(event->button()==Qt::LeftButton){auto button=pressed;pressed=nullptr;
            if(button && button==buttonAt(event->pos()) && button->isEnabled() && activate)activate(button);
            event->accept();return;}
        QWidget::mouseReleaseEvent(event);
    }
    void mouseMoveEvent(QMouseEvent *event) override {
        auto button=buttonAt(event->pos());setToolTip(button?(button->toolTip().isEmpty()?button->text():button->toolTip()):QString());
        QWidget::mouseMoveEvent(event);
    }
private:
    QPointer<QAbstractButton> pressed;bool rendering=false;
};
class NativeFooterMirror : public QScrollArea {
public:
    explicit NativeFooterMirror(QWidget *host):QScrollArea(host) {
        setObjectName("openGateNativeFooterMirror");setFrameShape(QFrame::NoFrame);setMinimumWidth(0);
        setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);setWidgetResizable(false);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        canvas=new NativeFooterCanvas;setWidget(canvas);
        connect(horizontalScrollBar(),&QScrollBar::rangeChanged,this,[this]{fit();});
        timer.setSingleShot(true);connect(&timer,&QTimer::timeout,this,[this]{fit();});
    }
    ~NativeFooterMirror() override {if(source)source->removeEventFilter(this);}
    std::function<void(QAbstractButton *)> activate;
    void setSource(QWidget *value) {
        if(source!=value){if(source)source->removeEventFilter(this);source=value;if(source)source->installEventFilter(this);}
        canvas->source=source;canvas->activate=[this](QAbstractButton *button){if(activate)activate(button);};fit();
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        // Never watch Paint: render() itself paints the source and would cause
        // a self-sustaining repaint loop. No polling or native relayout here.
        if(object==source && (event->type()==QEvent::Resize || event->type()==QEvent::LayoutRequest ||
            event->type()==QEvent::Show || event->type()==QEvent::StyleChange))if(!timer.isActive())timer.start(0);
        return false;
    }
private:
    void fit() {
        if(!source)return;const auto size=source->size();
        if(canvas->size()!=size)canvas->resize(size);
        const int scroll=size.width()>viewport()->width()?horizontalScrollBar()->sizeHint().height():0;
        const int height=size.height()+scroll;
        if(minimumHeight()!=height || maximumHeight()!=height)setFixedHeight(height);
        canvas->update();
    }
    QPointer<QWidget> source;NativeFooterCanvas *canvas;QTimer timer;
};
}
