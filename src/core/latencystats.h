// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonObject>
#include <QVector>
#include <algorithm>
namespace Unbound {
// Bounded samples; sorting happens when a summary is requested, never per move.
class LatencyStats
{
public:
    void add(qint64 nanoseconds)
    {
        const double milliseconds = qMax(qint64(0), nanoseconds) / 1000000.0;
        ++count; maximum = qMax(maximum,milliseconds);
        if (milliseconds > 16.667) ++overFrame;
        if (samples.size() < 4096) samples.append(milliseconds);
        else samples[int((count-1)%4096)] = milliseconds;
    }
    QJsonObject summary() const
    {
        auto ordered = samples; std::sort(ordered.begin(),ordered.end());
        const auto percentile = [&](double fraction) {
            return ordered.isEmpty() ? 0.0 : ordered[qBound(0,int((ordered.size()-1)*fraction+0.5),ordered.size()-1)];
        };
        return {{"count",double(count)}, {"p50_ms",percentile(.50)}, {"p95_ms",percentile(.95)},
            {"p99_ms",percentile(.99)}, {"max_ms",maximum}, {"over_16_67_ms",double(overFrame)}};
    }
    bool empty() const { return count == 0; }
    void clear() { samples.clear(); count = overFrame = 0; maximum = 0; }
private:
    QVector<double> samples;
    quint64 count = 0, overFrame = 0;
    double maximum = 0;
};
}
