// SPDX-License-Identifier: MIT
#ifndef UNBOUND_PANEFLOATSESSION_H
#define UNBOUND_PANEFLOATSESSION_H

#include <QBoxLayout>
#include <QCloseEvent>
#include <QLabel>
#include <QPointer>
#include <QTimer>
#include <QWidget>
#include <functional>

namespace Unbound {

// No cached target addresses. Owns only its window and placeholder, never the
// original parent. All operations must execute on the target GUI thread.
class PaneFloatSession : public QObject
{
public:
    explicit PaneFloatSession(QObject *parent = nullptr) : QObject(parent) {}
    ~PaneFloatSession() override
    {
        report = {};
        restore();
        // If the original container was destroyed, its former content has no
        // valid restoration destination. Dispose of our remaining owned window.
        if (m_host) delete m_host.data();
    }

    std::function<void(const QString &)> report;
    bool active() const { return !m_host.isNull(); }
    QWidget *floatingWindow() const { return m_host.data(); }

    QString start(QWidget *content, QWidget *viewer = nullptr, QWidget *divider = nullptr)
    {
        if (active()) return QStringLiteral("A pane is already floating. Restore it first.");
        if (!content || !content->isVisible() || content->isWindow())
            return QStringLiteral("The pane must be a visible child widget.");
        auto parent = content->parentWidget();
        auto layout = parent ? qobject_cast<QBoxLayout *>(parent->layout()) : nullptr;
        if (!layout || layout->count() != 1 || layout->itemAt(0)->widget() != content)
            return QStringLiteral("Expected a single-widget QBoxLayout; no changes made.");

        if (viewer) {
            auto root = parent->parentWidget();
            const auto paneRect = parent->geometry();
            const auto viewerRect = viewer->geometry();
            if (!root || root->layout() || viewer->parentWidget() != root || !viewer->isVisible()
                || !divider || divider->parentWidget() != root || !divider->isVisible()
                || qAbs(viewerRect.top() - paneRect.top()) > 2
                || qAbs(viewerRect.height() - paneRect.height()) > 2
                || qAbs(viewerRect.left() - paneRect.right() - 1) > 16
                || viewerRect.left() <= paneRect.left()
                || viewer->maximumWidth() < viewerRect.right() - paneRect.left() + 1)
                return QStringLiteral("Expected adjacent Media Pool and viewer containers with a visible divider. No changes made.");
        }

        ++m_generation;
        m_parent = parent;
        m_layout = layout;
        m_content = content;
        m_flags = content->windowFlags();
        m_geometry = content->geometry();
        m_wasHidden = content->isHidden();
        auto placeholder = new QLabel(QStringLiteral("Media Pool is floating.\nClose its window to restore it."), parent);
        placeholder->setObjectName(QStringLiteral("unboundMediaPoolPlaceholder"));
        placeholder->setAlignment(Qt::AlignCenter);
        placeholder->setSizePolicy(content->sizePolicy());
        placeholder->setMinimumSize(content->minimumSize());
        placeholder->setMaximumSize(content->maximumSize());
        auto oldItem = layout->replaceWidget(content, placeholder, Qt::FindDirectChildrenOnly);
        if (!oldItem) {
            delete placeholder;
            clear();
            return QStringLiteral("Could not insert placeholder; no pane was detached.");
        }
        delete oldItem; // QLayoutItem does not own the original widget.
        m_placeholder = placeholder;
        auto host = new QWidget(parent->window(), Qt::Window);
        host->setAttribute(Qt::WA_QuitOnClose, false);
        host->setObjectName(QStringLiteral("unboundFloatingEditMediaPool"));
        host->setWindowTitle(QStringLiteral("Media Pool"));
        host->resize(content->size().expandedTo(QSize(360, 300)));
        m_host = host;
        auto hostLayout = new QVBoxLayout(host);
        hostLayout->setContentsMargins(0, 0, 0, 0);
        content->setParent(host);
        hostLayout->addWidget(content);
        placeholder->show();
        content->show();
        parent->installEventFilter(this);
        host->installEventFilter(this);
        host->show();
        if (viewer) {
            m_viewer = viewer;
            m_divider = divider;
            m_root = parent->parentWidget();
            m_viewerGeometry = viewer->geometry();
            m_expandedGeometry = m_viewerGeometry;
            m_expandedGeometry.setLeft(parent->geometry().left());
            // Set this before hiding the slot so its Hide event doesn't invoke
            // the detach-only session's automatic restoration.
            m_reclaimed = true;
            parent->hide();
            divider->hide();
            viewer->setGeometry(m_expandedGeometry);
            m_root->installEventFilter(this);
            viewer->installEventFilter(this);
        }
        return QString();
    }

    QString restore()
    {
        if (!active()) return QString();
        if (m_restoring) return QString();
        if (!m_content || !m_parent || !m_layout || !m_placeholder)
            return QStringLiteral("The original pane or its container no longer exists; restart Resolve to reset the experiment.");
        if (m_content->parentWidget() != m_host || m_parent->layout() != m_layout
            || m_layout->indexOf(m_placeholder) < 0)
            return QStringLiteral("Resolve changed the pane hierarchy. Automatic restoration was stopped; restart Resolve to reset it.");
        m_restoring = true;
        m_parent->removeEventFilter(this);
        // replaceWidget reparents the content back and removes it from the
        // floating layout, preserving the original box-layout slot/stretch.
        auto oldItem = m_layout->replaceWidget(m_placeholder, m_content, Qt::FindDirectChildrenOnly);
        if (!oldItem) {
            m_parent->installEventFilter(this);
            m_restoring = false;
            return QStringLiteral("Restoration failed; the floating window remains open.");
        }
        delete oldItem;
        if (m_reclaimed) {
            if (m_root) m_root->removeEventFilter(this);
            if (m_viewer) {
                m_viewer->removeEventFilter(this);
                m_viewer->setGeometry(m_viewerGeometry);
            }
            if (m_divider) m_divider->show();
            m_parent->show();
        }
        m_content->setWindowFlags(m_flags);
        m_content->setGeometry(m_geometry);
        m_content->setVisible(!m_wasHidden);
        m_layout->activate();
        delete m_placeholder.data();
        auto host = m_host.data();
        host->removeEventFilter(this);
        clear();
        host->hide();
        host->deleteLater();
        m_restoring = false;
        return QString();
    }

protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        // Give control back before Resolve handles a resize of the shared root.
        // This prototype does not continually override Resolve's layout code.
        if (m_reclaimed && object == m_root && event->type() == QEvent::Resize && !m_restoring) {
            const auto error = restore();
            if (report) report(error);
            return false;
        }
        if (object == m_host && event->type() == QEvent::Close) {
            const auto error = restore();
            static_cast<QCloseEvent *>(event)->setAccepted(error.isEmpty());
            if (report) report(error);
            return true;
        }
        const bool restoreOnHide = (!m_reclaimed && object == m_parent && event->type() == QEvent::Hide)
            || (m_reclaimed && object == m_root && event->type() == QEvent::Hide);
        const bool layoutChanged = m_reclaimed && (
            (object == m_viewer && (event->type() == QEvent::Hide
                || ((event->type() == QEvent::Move || event->type() == QEvent::Resize)
                    && m_viewer->geometry() != m_expandedGeometry)))
            || (object == m_parent && event->type() == QEvent::Show));
        if ((restoreOnHide || layoutChanged) && !m_restoring) {
            // Finish Resolve's current hide operation before reparenting.
            const auto generation = m_generation;
            QTimer::singleShot(0, this, [this, generation]() {
                if (!active() || generation != m_generation) return;
                const auto error = restore();
                if (report) report(error);
            });
        }
        return QObject::eventFilter(object, event);
    }

private:
    void clear()
    {
        m_host = nullptr;
        m_content = nullptr;
        m_parent = nullptr;
        m_layout = nullptr;
        m_placeholder = nullptr;
        m_viewer = nullptr;
        m_root = nullptr;
        m_divider = nullptr;
        m_reclaimed = false;
    }
    QPointer<QWidget> m_host, m_content, m_parent, m_placeholder;
    QPointer<QBoxLayout> m_layout;
    QPointer<QWidget> m_viewer, m_root, m_divider;
    QRect m_viewerGeometry, m_expandedGeometry;
    bool m_reclaimed = false;
    Qt::WindowFlags m_flags;
    QRect m_geometry;
    bool m_wasHidden = false;
    bool m_restoring = false;
    unsigned m_generation = 0;
};
}
#endif
