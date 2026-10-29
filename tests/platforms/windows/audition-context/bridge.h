// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QFocusEvent>
#include <QJsonObject>
#include <QPointer>
#include <QScopedValueRollback>
#include <QWidget>
#include <functional>

// Diagnostic only: deliver a focus notification on the original slot's branch.
// QApplication::notify remains responsible for Resolve's native context update.
// Never write private state, emit native signals, or move/setFocus on content.
class AuditionContextBridge : public QObject
{
public:
    using Reporter=std::function<void(const QString &,QJsonObject)>;
    AuditionContextBridge(QWidget *page,QWidget *slot,QWidget *player,Reporter reporter,QObject *parent=nullptr)
        :QObject(parent),m_page(page),m_slot(slot),m_player(player),m_report(std::move(reporter))
    {
        if (!page || !slot || !player || !page->isAncestorOf(slot)) return;
        auto proxy=new QWidget(slot);
        proxy->setObjectName("unboundAuditionContextProxy");
        proxy->setFocusPolicy(Qt::StrongFocus);
        proxy->hide(); // Never participate in layout or actual keyboard focus.
        m_proxy=proxy;
        m_enabled=true;
        qApp->installEventFilter(this);
    }
    ~AuditionContextBridge() override {stop(); delete m_proxy.data();}
    bool enabled() const {return m_enabled;}
    void stop() {m_enabled=false; qApp->removeEventFilter(this);}
protected:
    bool eventFilter(QObject *object,QEvent *event) override
    {
        // The application's notify override sees the proxy event first; prevent
        // further widget delivery. The proxy itself has no native handlers.
        if (object==m_proxy && event->type()==QEvent::FocusIn) return true;
        if (!m_enabled || m_relaying || !m_page || !m_slot || !m_player || !m_proxy) return false;
        if (event->type()!=QEvent::MouseButtonPress && event->type()!=QEvent::FocusIn) return false;
        if (!object->isWidgetType() || !m_page->isVisible() || !m_player->isVisible()) return false;
        auto widget=static_cast<QWidget *>(object);
        auto host=m_player->window();
        if (host==m_page->window() || !host->isWindow() || widget->window()!=host) return false;
        // Restrict intervention to the known Media Pool host, never a different
        // floating pane or the native Dock control itself.
        if (host->objectName()!="unboundFloating_frameAudioPanelMediaPoolContainer") return false;
        if (widget->objectName()=="resolvePaneUndockButton") return false;
        if (!m_page->isAncestorOf(m_slot) || m_proxy->parentWidget()!=m_slot) return false;
        QScopedValueRollback<bool> guard(m_relaying,true);
        QPointer<QWidget> focusBefore=QApplication::focusWidget();
        QPointer<QWidget> windowBefore=QApplication::activeWindow();
        m_report("bridge-before",{{"trigger",int(event->type())},{"target",widget->objectName()},
            {"proxy",m_proxy->objectName()},{"proxyVisible",m_proxy->isVisible()}});
        if (!m_enabled || !m_proxy) return false;
        QFocusEvent notification(QEvent::FocusIn,Qt::OtherFocusReason);
        QApplication::sendEvent(m_proxy,&notification);
        m_report("bridge-after",{{"qtFocusUnchanged",focusBefore==QApplication::focusWidget()},
            {"activeWindowUnchanged",windowBefore==QApplication::activeWindow()}});
        // Original user event continues unchanged to its actual recipient.
        return false;
    }
private:
    QPointer<QWidget> m_page,m_slot,m_player,m_proxy;
    Reporter m_report;
    bool m_enabled=false,m_relaying=false;
};
