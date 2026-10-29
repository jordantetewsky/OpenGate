// SPDX-License-Identifier: MIT
#ifndef UNBOUND_NODESCANVASBOUNDS_H
#define UNBOUND_NODESCANVASBOUNDS_H
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QPointer>
#include <QTimer>
#include <QResizeEvent>
#include <QApplication>
namespace Unbound {
// Resolve's fixed scene rectangle can become smaller than a floating viewport.
// Items outside it still paint, but Resolve rejects dragging them. Expand the
// actual scene coordinates; do not scale or reposition the user's nodes.
class NodesCanvasBounds : public QObject
{
public:
    explicit NodesCanvasBounds(QWidget *content,QObject *parent=nullptr) : QObject(parent),m_content(content)
    {
        m_timer.setInterval(150);
        connect(&m_timer,&QTimer::timeout,this,[this] { updateBounds(); });
        m_timer.start();
        QTimer::singleShot(0,this,[this] { updateBounds(); });
    }
    void updateBounds()
    {
        if (m_updating || !m_content || !m_content->isVisible()
            || QApplication::mouseButtons()!=Qt::NoButton) return;
        if (!m_view) m_view=m_content->findChild<QGraphicsView *>(QStringLiteral("m_NodeGraphWgt"));
        if (!m_view || !m_view->isVisible() || !m_view->scene() || m_view->viewport()->size().isEmpty()) return;
        auto scene=m_view->scene();
        const auto visible=m_view->mapToScene(m_view->viewport()->rect()).boundingRect();
        const auto previous=scene->sceneRect();
        if (!visible.isValid() || previous.adjusted(-1,-1,1,1).contains(visible)) return;
        m_updating=true;
        // Allow for a viewport growing when Qt hides its scrollbars, plus
        // integer rounding at fractional zoom scales.
        const int extraX=qMax(0,m_view->width()-m_view->viewport()->width());
        const int extraY=qMax(0,m_view->height()-m_view->viewport()->height());
        const auto padded=m_view->mapToScene(m_view->viewport()->rect().adjusted(
            -extraX/2-2,-extraY/2-2,(extraX+1)/2+2,(extraY+1)/2+2)).boundingRect();
        scene->setSceneRect(previous.united(padded));
        m_view->centerOn(visible.center());
        // Resolve positions its input/output markers in its resize handler.
        // Refresh that calculation after changing the scene's coordinate bounds.
        // QAbstractScrollArea dispatches its virtual resizeEvent through
        // viewportEvent. Sending this to the outer view bypasses Resolve's
        // graph resize handler and leaves the endpoint positions stale.
        QResizeEvent resize(m_view->viewport()->size(),m_view->viewport()->size());
        QCoreApplication::sendEvent(m_view->viewport(),&resize);
        m_updating=false;
    }
private:
    QPointer<QWidget> m_content;
    QPointer<QGraphicsView> m_view;
    QTimer m_timer;
    bool m_updating=false;
};
}
#endif
