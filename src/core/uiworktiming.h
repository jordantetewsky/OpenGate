// SPDX-License-Identifier: MIT
#pragma once
#include <QElapsedTimer>
#include <QPointer>
#include "diagnostics.h"
namespace Unbound {
// Operation summaries are emitted once after the work, never for each child layout.
class UiWorkTiming
{
public:
    UiWorkTiming(QString operation,QWidget *window) : operation(std::move(operation)),window(window),before(window ? window->geometry() : QRect())
    {
        // Timing spans are routine telemetry. Actual button/ownership requests
        // already flush at their entry points; flushing every nested span would
        // add synchronous filesystem work to the operation being measured.
        logEvent("ui.work.begin",{{"operation",this->operation},{"window",window ? window->objectName() : QString()}});
        timer.start();
    }
    ~UiWorkTiming()
    {
        const auto elapsed = timer.nsecsElapsed();
        logEvent("ui.work",{{"operation",operation},{"work_ms",elapsed/1000000.0},
            {"window",window ? window->objectName() : QString()},
            {"outer_frame_changed",window && window->geometry() != before}});
    }
    UiWorkTiming(const UiWorkTiming &) = delete;
    UiWorkTiming &operator=(const UiWorkTiming &) = delete;
private:
    QString operation;
    QPointer<QWidget> window;
    QRect before;
    QElapsedTimer timer;
};
}
