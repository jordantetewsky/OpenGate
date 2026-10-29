// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QChildEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPointer>
#include <QTimer>
#include "windowintegration.h"

namespace Unbound {
// Observe only a recognized native header's subtree. Application-wide delivery
// is needed only while a background/title gesture owns the mouse grab.
class PaneHeaderDrag : public QObject
{
public:
    PaneHeaderDrag(QWidget *header, QObject *owner, int stripHeight = 0, int stripTop = 0)
        : QObject(owner), m_header(header), m_stripHeight(stripHeight), m_stripTop(stripTop)
    {
        observe(header);
    }
    ~PaneHeaderDrag() override { finish(true); }
protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (event->type() == QEvent::ChildAdded && m_header
            && (object == m_header || m_header->isAncestorOf(qobject_cast<QWidget *>(object)))) {
            const QPointer<QObject> child = static_cast<QChildEvent *>(event)->child();
            QTimer::singleShot(0, this, [this, child] {
                if (auto widget = qobject_cast<QWidget *>(child.data()))
                    if (m_header && m_header->isAncestorOf(widget)) observe(widget);
            });
        }
        if (m_source && (((event->type() == QEvent::Hide || event->type() == QEvent::Destroy)
                && (object == m_source || object == m_header || object == m_host))
            || (event->type() == QEvent::UngrabMouse && object == m_source)
            || event->type() == QEvent::ApplicationDeactivate
            || (event->type() == QEvent::ParentChange && object == m_header))) {
            finish(true);
        }
        if (m_source && event->type() == QEvent::KeyPress
            && static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape) {
            finish(true);
            return true;
        }
        // QWidgetWindow must process native mouse delivery first, including
        // clearing Qt's implicit grab on release. Consume only widget events.
        if (!object->isWidgetType() && (event->type() == QEvent::MouseMove
            || event->type() == QEvent::MouseButtonRelease)) return false;
        if (event->type() == QEvent::MouseButtonPress && !m_source) {
            auto mouse = static_cast<QMouseEvent *>(event);
            auto source = qobject_cast<QWidget *>(object);
            auto host = m_header ? dynamic_cast<PaneFloatingWindow *>(m_header->window()) : nullptr;
            if (mouse->button() != Qt::LeftButton || !host || !host->isVisible()
                || !safeSurface(source, mouse->globalPos())) return false;
            m_host = host;
            m_source = source;
            m_press = mouse->globalPos();
            qApp->installEventFilter(this);
            source->grabMouse();
            return true;
        }
        if (m_source && event->type() == QEvent::MouseMove) {
            auto mouse = static_cast<QMouseEvent *>(event);
            if (!(mouse->buttons() & Qt::LeftButton)) { finish(true); return false; }
            if (!m_moving && (mouse->globalPos() - m_press).manhattanLength() >= QApplication::startDragDistance()) {
                if (!m_host || !m_host->beginButtonMove()) { finish(true); return true; }
                m_moving = true;
            } else if (m_moving && m_host) m_host->updateButtonMove();
            return true;
        }
        if (m_source && event->type() == QEvent::MouseButtonRelease
            && static_cast<QMouseEvent *>(event)->button() == Qt::LeftButton) {
            finish(false);
            return true;
        }
        return false;
    }
private:
    void observe(QWidget *widget)
    {
        widget->installEventFilter(this);
        for (auto child : widget->findChildren<QWidget *>()) child->installEventFilter(this);
    }
    bool safeSurface(QWidget *source, const QPoint &global) const
    {
        if (!m_header || !source || (source != m_header && !m_header->isAncestorOf(source))) return false;
        const auto point = m_header->mapFromGlobal(global);
        if (!m_header->rect().contains(point)
            || (m_stripHeight && (point.y() < m_stripTop || point.y() >= m_stripTop + m_stripHeight))) return false;
        // Check the actual hit child too: ignored control events can propagate
        // up to a plain QFrame. They must not become background drags there.
        auto hit = m_header->childAt(point);
        for (auto widget = hit ? hit : source; widget; widget = widget->parentWidget()) {
            if (widget == m_header) return true;
            const QByteArray type = widget->metaObject()->className();
            if (type == "QLabel" || type == "BmdDavUI::UiElidedLabel") {
                auto label = qobject_cast<QLabel *>(widget);
                if (!label) return false;
                const auto flags = label->textInteractionFlags();
                if (flags & (Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard)) return false;
                if ((flags & (Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard))
                    && label->textFormat() != Qt::PlainText && label->text().contains("<a", Qt::CaseInsensitive)) return false;
            } else if (type != "QWidget" && type != "QFrame") return false;
        }
        return false;
    }
    void finish(bool cancel)
    {
        const auto source = m_source;
        const auto host = m_host;
        const bool moving = m_moving;
        m_source = nullptr; m_host = nullptr; m_moving = false;
        if (qApp) qApp->removeEventFilter(this);
        if (moving && host) host->finishButtonMove(cancel);
        if (source && QWidget::mouseGrabber() == source) source->releaseMouse();
    }
    QPointer<QWidget> m_header, m_source;
    QPointer<PaneFloatingWindow> m_host;
    QPoint m_press;
    int m_stripHeight = 0, m_stripTop = 0;
    bool m_moving = false;
};
}
