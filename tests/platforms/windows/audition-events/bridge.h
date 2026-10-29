// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QEvent>
#include <QJsonObject>
#include <QPointer>
#include <QScopedValueRollback>
#include <QWidget>
#include <functional>

// Public Qt APIs only. No Resolve manager access, memory offsets or discovery.
// Bind these three already-known widgets when attaching the pane session.
class AuditionActivationBridge : public QObject
{
public:
    using Reporter=std::function<void(const QString &,QJsonObject)>;
    AuditionActivationBridge(QWidget *main,QWidget *page,QWidget *preview,Reporter report={},QObject *parent=nullptr)
        :QObject(parent),m_main(main),m_page(page),m_preview(preview),m_report(std::move(report))
    {
        if (!main || !page || !preview || !main->isWindow() || page->window()!=main) return;
        m_enabled=true;qApp->installEventFilter(this);
    }
    ~AuditionActivationBridge() override {stop();}
    bool enabled() const {return m_enabled;}
    void stop() {m_enabled=false;if (!m_synthetic) qApp->removeEventFilter(this);}
protected:
    bool eventFilter(QObject *object,QEvent *event) override
    {
        // The application's native notify handler has already processed this
        // exact event. Prevent ordinary QWidget activation/child propagation.
        if (event==m_synthetic && object==m_main) return true;
        if (event->type()!=QEvent::WindowActivate || !m_enabled || m_synthetic) return false;
        if (!m_main || !m_page || !m_preview || !m_page->isVisible() || !m_preview->isVisible()) return false;
        auto host=m_preview->window();
        if (object!=host || host==m_main || !host->isWindow() || host->isMinimized() ||
            host->objectName()!="unboundFloating_frameAudioPanelMediaPoolContainer" ||
            QApplication::activeWindow()!=host || QApplication::activeModalWidget() || QApplication::activePopupWidget()) return false;
        // One notification per actual host activation, before its normal focus
        // delivery. Propagated child activation and later clicks do no work.
        QPointer<QWidget> oldFocus=QApplication::focusWidget(),oldActive=QApplication::activeWindow();
        const auto activation=++m_activation;
        report("activation-before",{{"activation",activation},{"host",host->objectName()}});
        if (!m_enabled || !m_main || !m_page || !m_page->isVisible()) return false;
        {
            QEvent notification(QEvent::WindowActivate);
            QScopedValueRollback<QEvent *> guard(m_synthetic,&notification);
            QApplication::sendEvent(m_main,&notification);
        }
        if (!m_enabled) qApp->removeEventFilter(this);
        else report("activation-after",{{"activation",activation},
            {"qtFocusUnchanged",oldFocus==QApplication::focusWidget()},
            {"activeWindowUnchanged",oldActive==QApplication::activeWindow()}});
        return false; // Let the host activation and genuine focus event continue.
    }
private:
    void report(const QString &kind,QJsonObject data) {if (m_report) m_report(kind,std::move(data));}
    QPointer<QWidget> m_main,m_page,m_preview;
    Reporter m_report;
    QEvent *m_synthetic=nullptr;
    bool m_enabled=false;
    int m_activation=0;
};
