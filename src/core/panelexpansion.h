// SPDX-License-Identifier: MIT
#pragma once
#include <QPointer>
#include <QApplication>
#include <QWidget>
#include "../platform/nativewindowcontract.h"
namespace Unbound {
class PanelExpansion : public QObject
{
public:
    PanelExpansion() { qApp->installEventFilter(this); }
    ~PanelExpansion() override { restore(); if (qApp) qApp->removeEventFilter(this); }
    void foreground(QWidget *host)
    {
        restore(); current = host;
        if (!current) return;
        current->setProperty("unboundExpanded", true);
        setNativeExpansionPriority(current, true);
        current->raise(); current->activateWindow();
    }
    void restore()
    {
        if (current) {
            setNativeExpansionPriority(current, false);
            current->setProperty("unboundExpanded", false);
        }
        current = nullptr;
    }
protected:
    bool eventFilter(QObject *, QEvent *event) override
    {
        if (current && event->type() == QEvent::ApplicationStateChange)
            setNativeExpansionPriority(current, QGuiApplication::applicationState() == Qt::ApplicationActive);
        return false;
    }
private:
    QPointer<QWidget> current;
};
}
