// SPDX-License-Identifier: MIT
#pragma once
#include <QElapsedTimer>
#include "diagnostics.h"
#include "latencystats.h"
namespace Unbound {
class DragTiming
{
public:
    class Sample {
    public:
        explicit Sample(DragTiming &owner,QString stage = {}) : owner(owner),stage(std::move(stage)) { timer.start(); }
        ~Sample() {
            const auto ns = timer.nsecsElapsed();
            if (!stage.isEmpty()) { owner.stages[stage].add(ns); return; }
            ++owner.count; owner.total += ns; owner.maximum = qMax(owner.maximum, ns); owner.preview.add(ns);
        }
    private:
        DragTiming &owner;
        QElapsedTimer timer;
        QString stage;
    };
    void begin() { count = 0; total = maximum = 0; preview.clear(); stages.clear(); }
    void report(const QString &window) const
    {
        QJsonObject breakdown;
        for (auto stage = stages.begin();stage != stages.end();++stage) breakdown[stage.key()] = stage.value().summary();
        logEvent(QStringLiteral("drag.timing"), {{"window", window}, {"preview_updates", count}, {"preview",preview.summary()}, {"stages",breakdown},
            {"preview_mean_ms", count ? double(total)/count/1000000 : 0}, {"preview_max_ms", double(maximum)/1000000}});
    }
private:
    int count = 0;
    qint64 total = 0, maximum = 0;
    LatencyStats preview;
    QHash<QString,LatencyStats> stages;
};
}
