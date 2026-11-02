// SPDX-License-Identifier: MIT
#pragma once
#include "diagnostics.h"

namespace Unbound {
// Event-driven command-to-frame evidence. No images, playback polling, command
// dispatch or additional state writer; existing rotating diagnostics own output.
class TimelineCommandPaintEvidence {
public:
    void requested(const QString &source,quint64 revision) {
        if(!pending){before=revision;++sequence;}
        pending=true;
        if(sources.size()<16 && !sources.contains(source))sources.append(source);
    }
    bool waiting() const {return pending;}
    void presented(quint64 revision,quint64 fingerprint,double content,double header,QWidget *view) {
        if(!pending)return;pending=false;
        logEvent("timeline.command.presented",{{"request",double(sequence)},{"sources",QJsonArray::fromStringList(sources)},
            {"native_view",QString::number(quintptr(view),16)},{"raster_revision_before",double(before)},
            {"raster_revision",double(revision)},{"fresh_native_render",revision>before},
            {"sampled_raster_fingerprint",QString::number(fingerprint,16)},
            {"content_scale",content},{"header_scale",header},
            {"meaning","native raster presentation; not model correctness or display scanout proof"}});
        sources.clear();
    }
private:
    QStringList sources;quint64 before=0,sequence=0;bool pending=false;
};
}
