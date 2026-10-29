// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QPointer>
#include "diagnostics.h"
#include "nativechrome.h"

namespace Unbound {
// Keep Resolve's page command scope when one of our windows becomes active.
// This follows the existing Windows audition bridge without changing Qt focus.
class ResolveActivationBridge : public QObject
{
public:
    explicit ResolveActivationBridge(QObject *owner) : QObject(owner) { qApp->installEventFilter(this); }
    ~ResolveActivationBridge() override { qApp->removeEventFilter(this); }
protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        // Resolve's application notify sees this, but Qt widgets must not.
        if (event == synthetic && object == mainWindow) return true;
        if (synthetic || event->type() != QEvent::WindowActivate) return false;
        auto window = qobject_cast<QWidget *>(object);
        if (!window || !window->isWindow() || !window->objectName().startsWith("unboundFloating_") ||
            QApplication::activeWindow() != window || window->isMinimized() ||
            QApplication::activeModalWidget() || QApplication::activePopupWidget()) return false;
        mainWindow = nullptr;
        for (auto candidate : QApplication::topLevelWidgets())
            if (candidate->objectName() == QLatin1String("UiMainWindow")) mainWindow = candidate;
        if (!mainWindow || window->parentWidget() != mainWindow) return false;
        auto page = mainWindow->findChild<QWidget *>(QStringLiteral("WStackPage_Conform"));
        if (!page || !page->isVisible()) return false;
        const QPointer<QWidget> focus = QApplication::focusWidget();
        const auto key = nativeKeyWindowNumber();
        QEvent notification(QEvent::WindowActivate);
        synthetic = &notification;
        QPointer<ResolveActivationBridge> alive(this);
        QApplication::sendEvent(mainWindow, &notification);
        if (!alive) return false;
        synthetic = nullptr;
        logEvent(QStringLiteral("input.page-scope.restore"), {{"host", window->objectName()},
            {"qt_focus_preserved", focus == QApplication::focusWidget()}, {"key_window_preserved", key == nativeKeyWindowNumber()}});
        return false;
    }
private:
    QEvent *synthetic = nullptr;
    QPointer<QWidget> mainWindow;
};
}
