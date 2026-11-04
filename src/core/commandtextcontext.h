// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QDateTime>
#include <QKeyEvent>
#include <QPointer>
#include <QTimer>
#include <QVector>
#include "textediting.h"

namespace Unbound {
// Remembers only editing intervals, never typed text or keys. The existing
// command filter supplies input observations; this adds no filter or polling.
class CommandTextContext : public QObject {
public:
    static CommandTextContext *shared()
    {
        static QPointer<CommandTextContext> instance;
        if (!instance && qApp) instance = new CommandTextContext;
        return instance;
    }
    void observeInput(QEvent::Type type)
    {
        switch (type) {
        case QEvent::ShortcutOverride:
        case QEvent::Shortcut:
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
            observe();
            // Graphics scenes can toggle item editability during delivery
            // without changing QWidget focus.
            if (!pending) {
                pending = true;
                QTimer::singleShot(0, this, [this] { pending = false; observe(); });
            }
            break;
        default:
            break;
        }
    }
    bool blocks(qint64 issued)
    {
        observe();
        if (editing || issued <= discardedThrough) return true;
        for (const auto &interval : intervals)
            if (issued >= interval.start && issued <= interval.end) return true;
        return false;
    }
    static bool plainTextKey(const QKeyEvent *key)
    {
        return !key->text().isEmpty() &&
               !(key->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier));
    }
private:
    struct Interval { qint64 start, end; };
    QVector<Interval> intervals;
    qint64 began = 0, discardedThrough = -1;
    bool editing = false, pending = false;
    CommandTextContext() : QObject(qApp)
    {
        connect(qApp, &QApplication::focusChanged, this, [this] { observe(); });
        observe();
    }
    void observe()
    {
        const auto now = QDateTime::currentMSecsSinceEpoch();
        const bool current = textEditor(QApplication::focusWidget());
        if (current && !editing) began = now;
        if (!current && editing) intervals.append({began, now});
        editing = current;
        while (!intervals.isEmpty() && intervals.first().end < now - 15000)
            intervals.removeFirst();
        // Requests already expire at 15 seconds. Bound unusually rapid focus
        // churn too, rejecting any request whose evidence was discarded.
        while (intervals.size() > 256) {
            discardedThrough = intervals.first().end;
            intervals.removeFirst();
        }
    }
};
}
