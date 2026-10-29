// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QFocusEvent>
#include <QJsonObject>
#include <QPointer>
#include <QScopedValueRollback>
#include <QTimer>
#include <QWidget>
#include <functional>

class AuditionScopeBridge : public QObject
{
public:
    using Reader=std::function<QJsonObject()>;
    using Reporter=std::function<void(const QString &,QJsonObject)>;
    AuditionScopeBridge(QWidget *main,QWidget *page,QWidget *preview,Reader reader,Reporter reporter,QObject *parent=nullptr)
        :QObject(parent),m_main(main),m_page(page),m_preview(preview),m_read(std::move(reader)),m_report(std::move(reporter))
    {
        if (!main || !page || !preview || !main->isWindow() || page->window()!=main) return;
        m_enabled=true;qApp->installEventFilter(this);
    }
    ~AuditionScopeBridge() override {stop();}
    bool enabled() const {return m_enabled;}
    void stop() {m_enabled=false;if (!m_synthetic) qApp->removeEventFilter(this);}
protected:
    bool eventFilter(QObject *object,QEvent *event) override
    {
        // Resolve's application notify handler runs first. Consume only our
        // exact synthetic event instances before ordinary QWidget delivery.
        if (m_synthetic==event && object==m_syntheticTarget) return true;
        if (!m_enabled || m_relaying || !m_main || !m_page || !m_preview || !object->isWidgetType()) return false;
        if (event->type()!=QEvent::MouseButtonPress && event->type()!=QEvent::FocusIn) return false;
        if (!m_page->isVisible() || !m_preview->isVisible()) return false;
        auto widget=static_cast<QWidget *>(object);
        auto host=m_preview->window();
        const bool floating=host!=m_main && host->isWindow() &&
            host->objectName()=="unboundFloating_frameAudioPanelMediaPoolContainer";
        const bool inHost=floating && widget->window()==host;
        bool insidePreview=false;
        for (auto p=widget;p;p=p->parentWidget()) if (p==m_preview) insidePreview=true;
        const bool play=event->type()==QEvent::MouseButtonPress && widget->objectName()=="btnTransportPlay";
        const bool dock=widget->objectName()=="resolvePaneUndockButton";
        if (!inHost && !(play && widget->window()==m_main)) return false;
        if (dock) return false;
        const auto before=m_read();
        if (!before.value("valid").toBool()) {m_report("scope-read-failed",{});stop();return false;}
        const int action=++m_action;
        const QString phase=insidePreview ? (floating ? "floating-audition" : "docked-audition") :
            inHost ? "floating-media-pool" : "main-timeline";
        m_report("scope-action-before",{{"action",action},{"phase",phase},{"target",widget->objectName()},
            {"event",int(event->type())},{"playClick",play},{"native",before}});
        if (!m_enabled) return false;
        QScopedValueRollback<bool> guard(m_relaying,true);
        QPointer<QWidget> target=widget;
        QPointer<QWidget> focusBefore=QApplication::focusWidget(),activeBefore=QApplication::activeWindow();
        bool attempted=false;
        // Apply only the measured mismatch, with Resolve actually foreground on
        // this pane. Main timeline and docked audition stay entirely native.
        if (inHost && before.value("scope").toInt()==0x7000000 && activeBefore==host && widget->isVisible()) {
            attempted=true;
            QEvent activation(QEvent::WindowActivate);
            sendNotification(m_main,&activation);
            const auto afterActivation=m_read();
            m_report("scope-main-notification",{{"action",action},{"native",afterActivation}});
            if (m_enabled && target && target->isVisible() && m_page && m_page->isVisible() &&
                afterActivation.value("valid").toBool() && afterActivation.value("scope").toInt()==0x5000000) {
                QFocusEvent focus(QEvent::FocusIn,Qt::OtherFocusReason);
                sendNotification(target,&focus);
            }
        }
        if (m_enabled) {
            m_report("scope-action-after",{{"action",action},{"phase",phase},{"attempted",attempted},
                {"native",m_read()},{"qtFocusUnchanged",focusBefore==QApplication::focusWidget()},
                {"activeWindowUnchanged",activeBefore==QApplication::activeWindow()}});
            // One deferred record after native button handling, never polling.
            QTimer::singleShot(0,this,[this,action,phase] {
                if (m_enabled) m_report("scope-action-settled",{{"action",action},{"phase",phase},{"native",m_read()}});
            });
        }
        return false; // Never consume or replace the user's original event.
    }
private:
    void sendNotification(QObject *target,QEvent *event) {
        {
            QScopedValueRollback<QEvent *> eventGuard(m_synthetic,event);
            QScopedValueRollback<QObject *> targetGuard(m_syntheticTarget,target);
            QApplication::sendEvent(target,event);
        }
        if (!m_enabled) qApp->removeEventFilter(this);
    }
    QPointer<QWidget> m_main,m_page,m_preview;
    Reader m_read;
    Reporter m_report;
    bool m_enabled=false,m_relaying=false;
    int m_action=0;
    QEvent *m_synthetic=nullptr;
    QObject *m_syntheticTarget=nullptr;
};
