// SPDX-License-Identifier: MIT
#include "../../../src/platform/timelineclipidentityinspection.h"
#include "../../../src/platforms/macos/nativeclipidentitysnapshot.h"
#include "../../../src/platforms/macos/nativeclipmetadatainspection.h"
#include "../../../src/core/timelinecontentinspection.h"
#include <QApplication>
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <thread>
#include <vector>

using namespace Unbound::NativeClipIdentity;
class InspectionContainer : public QGraphicsObject {
public:
    QRectF boundingRect() const override { return {0, 0, 100, 100}; }
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) override {}
};
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    qunsetenv("OPENGATE_CLIP_IDENTITY_PROBE");
    QObject ordinary;
    assert(isSupportedUiClassName("UiTiItem") && isSupportedUiClassName("UiTiAudioClip") &&
           isSupportedUiClassName("UiTiVideoClip"));
    assert(!isSupportedUiClassName(nullptr) && !isSupportedUiClassName("QObject") &&
           !isSupportedUiClassName("UiTiMedia") && !isSupportedUiClassName("UiTiVideoClipExtra"));
    assert(Unbound::inspectTimelineClipIdentity(&ordinary)["status"] == "disabled");
    assert(Unbound::inspectTimelineClipIdentity(&ordinary, true)["enabled"].toBool());
    qputenv("OPENGATE_CLIP_IDENTITY_PROBE", "1");
    assert(!Unbound::inspectTimelineClipIdentity(&ordinary)["persistent_identity_verified"].toBool());
#if defined(Q_OS_MAC) && defined(__arm64__)
    ordinary.setObjectName("UiTiVideoClip");
    assert(!Unbound::MacTimelineClipIdentity::isSupportedUiObject(&ordinary));
    QJsonObject offThread;
    std::thread worker([&] { offThread = Unbound::inspectTimelineClipIdentity(&ordinary); });
    worker.join();
    assert(offThread["status"] == "requires-gui-thread");
#endif
    qputenv("OPENGATE_CLIP_IDENTITY_READ_METADATA", "1");
    qunsetenv("OPENGATE_CLIP_IDENTITY_DISPOSABLE_IDS");
    // Reject before touching a model when no disposable SDK receipt is supplied.
    assert(Unbound::inspectDisposableClipMetadata(reinterpret_cast<QObject *>(1), "id")["status"] ==
           "requires-disposable-sdk-id-allowlist");

    InspectionContainer container;
    for (int i=0; i<90; ++i)
        new QGraphicsRectItem(&container);
    const auto topology = Unbound::TimelineContentInspection::captureClipMembers(&container);
    assert(topology["members"].toArray().size() == 64 && topology["truncated"].toBool());
    assert(!topology["persistent_identity_verified"].toBool() &&
           !topology["project_writes"].toBool() && !topology["private_calls"].toBool());
    {
        QGraphicsScene outerScene,innerScene;
        QGraphicsView outer(&outerScene);
        auto inner=new QGraphicsView(&innerScene);
        auto proxy=outerScene.addWidget(inner);
        proxy->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
        inner->setCacheMode(QGraphicsView::CacheBackground);
        auto effect=new QGraphicsOpacityEffect;
        inner->viewport()->setGraphicsEffect(effect);
        const auto state=Unbound::TimelineContentInspection::captureRenderState(inner);
        const auto ancestors=state["proxy_ancestry"].toArray();
        assert(state["view_cache_mode"].toInt()==int(QGraphicsView::CacheBackground));
        assert(ancestors.size()==1 && ancestors[0].toObject()["proxy_cache_mode"].toInt()==
               int(QGraphicsItem::DeviceCoordinateCache));
        assert(state["viewport"].toObject()["effect"].toObject()["class"]=="QGraphicsOpacityEffect");
        assert(inner->cacheMode()==QGraphicsView::CacheBackground &&
               proxy->cacheMode()==QGraphicsItem::DeviceCoordinateCache &&
               inner->viewport()->graphicsEffect()==effect);
    }
    qputenv("OPENGATE_CLIP_IDENTITY_DISPOSABLE_IDS", "another-id");
    assert(Unbound::inspectDisposableClipMetadata(reinterpret_cast<QObject *>(1), "id")["status"] ==
           "requires-disposable-sdk-id-allowlist");

    std::map<std::uintptr_t, std::vector<unsigned char>> memory;
    auto block = [&](std::uintptr_t p, std::size_t n) { memory[p].resize(n); };
    auto put = [&](std::uintptr_t p, auto value) {
        for (auto &[a, bytes] : memory)
            if (p >= a && p-a <= bytes.size() && sizeof(value) <= bytes.size()-(p-a)) {
                std::memcpy(bytes.data()+p-a, &value, sizeof(value));
                return;
            }
        assert(false);
    };
    constexpr std::uintptr_t ui=0x200000000ULL, weak=0x200001000ULL, owner=0x200002000ULL;
    constexpr std::uintptr_t proxy=0x200003000ULL, data=0x200004000ULL, slide=0x4000;
    block(ui, 0x200); block(weak, 0x30); block(owner, 0x200); block(proxy, 0x40); block(data, 0x100);
    put(ui, slide+0x11d473c90ULL); put(ui+0xc0, weak); put(weak, owner);
    put(weak+0x20, std::uint8_t(0)); put(owner, slide+0x11d2d78f0ULL);
    put(owner+0x100, proxy); put(proxy, slide+0x11d4e6de0ULL); put(proxy+0x30, weak);
    put(owner+0x18, data);
    const std::string expected="e779ae8e-862c-45b0-86b2-72a67d6ea49c";
    put(data+4, std::int32_t(expected.size())); put(data+16, std::int64_t(24));
    for (std::size_t i=0; i<expected.size(); ++i) put(data+24+i*2, char16_t(expected[i]));
    int reads=0;
    Reader reader = [&](std::uintptr_t p, void *out, std::size_t n) {
        ++reads;
        for (auto &[a, bytes] : memory)
            if (p >= a && p-a <= bytes.size() && n <= bytes.size()-(p-a)) {
                std::memcpy(out, bytes.data()+p-a, n);
                return true;
            }
        return false;
    };
    auto valid = inspect(reader, ui, slide, [] { return true; });
    assert(valid.valid && valid.id == expected && reads <= 32);
    // The two actual derived native classes keep the same zero-offset QObject
    // and inherited+0xc0 owner weak handle. Other/secondary vtables still reject.
    for (const auto &layout : uiLayouts) {
        put(ui, slide+layout.vtable);
        reads=0;
        const auto derived=inspect(reader, ui, slide, [] { return true; });
        assert(derived.valid && derived.id==expected && derived.model==owner && reads<=32);
    }
    for (const auto vtable : {0x11d471028ULL, 0x11d478a70ULL, 0x11d474e90ULL,
                              0x11d478771ULL}) {
        put(ui, slide+vtable);
        assert(inspect(reader, ui, slide, [] { return true; }).status==
               std::string("unsupported-ui-vtable"));
    }
    assert(!isSupportedUiVtable(0, std::numeric_limits<std::uintptr_t>::max()));
    put(ui, slide+0x11d478770ULL);
    int changingReads=0;
    Reader changingReader=[&](std::uintptr_t p,void *out,std::size_t n) {
        const bool result=reader(p,out,n);
        if(p==ui && ++changingReads==1)put(ui,slide+0x11d470d28ULL);
        return result;
    };
    assert(inspect(changingReader, ui, slide, [] { return true; }).status==
           std::string("identity-changed-during-read"));
    put(ui, slide+0x11d473c90ULL);
    put(proxy+0x30, weak+8);
    assert(inspect(reader, ui, slide, [] { return true; }).status ==
           std::string("proxy-owner-mismatch"));
    put(proxy+0x30, weak);
    put(weak+0x20, std::uint8_t(1));
    assert(!inspect(reader, ui, slide, [] { return true; }).valid);
    put(weak+0x20, std::uint8_t(0));
    put(owner+0x100, std::uintptr_t(0));
    assert(inspect(reader, ui, slide, [] { return true; }).status ==
           std::string("await-existing-sdk-proxy"));
    put(owner+0x100, proxy);
    put(data+4, std::int32_t(10000));
    assert(!inspect(reader, ui, slide, [] { return true; }).valid);
    put(data+4, std::int32_t(expected.size()));
    int checks=0;
    assert(!inspect(reader, ui, slide, [&] { return ++checks < 2; }).valid);
    assert(!inspect(reader, ui, slide+1, [] { return true; }).valid);
    assert(!inspect(reader, 0, slide, [] { return true; }).valid);
    put(data+16, std::int64_t(-1));
    assert(!inspect(reader, ui, slide, [] { return true; }).valid);
    put(data+16, std::int64_t(24));
    // No copy is attempted when an address addition would wrap.
    int unused=0;
    assert(!read(reader, std::numeric_limits<std::uintptr_t>::max()-1, 4, unused));
#if defined(Q_OS_MAC) && defined(__arm64__)
    int value=42, copied=0;
    assert(Unbound::MacTimelineClipIdentity::copyMemory(std::uintptr_t(&value), &copied, sizeof(copied)));
    assert(copied == value);
    assert(!Unbound::MacTimelineClipIdentity::copyMemory(1, &copied, sizeof(copied)));
    assert(!Unbound::MacTimelineClipIdentity::copyMemory(std::uintptr_t(&value), &copied, 65537));
    assert(!Unbound::MacTimelineClipIdentity::verifyImage().valid); // This fixture is not Resolve.
#endif
    std::cout << "PASS: bounded identity, native image rejection, proxy/lifetime guards and disposable metadata gate\n";
}
