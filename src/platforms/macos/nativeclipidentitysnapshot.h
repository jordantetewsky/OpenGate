// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <string>

namespace Unbound::NativeClipIdentity {
// One evidenced Resolve ARM64 layout. Reads are supplied by the platform adapter.
inline constexpr char imageHash[] =
    "801848888e276e40cefcc89011155702ddf67bb6ecfccff4de60ff505685e2a9";
inline constexpr std::uint64_t imageSize = 511507696;
using Reader = std::function<bool(std::uintptr_t, void *, std::size_t)>;
// QObject is the zero-offset base in each evidenced exact-image UI layout.
// The graphics subobject address points (offset-to-top -0x10) are excluded.
struct UiLayout { const char *className; std::uintptr_t vtable; };
inline constexpr UiLayout uiLayouts[] = {
    {"UiTiItem", 0x11d473c90ULL},
    {"UiTiAudioClip", 0x11d470d28ULL},
    {"UiTiVideoClip", 0x11d478770ULL},
};
inline bool isSupportedUiClassName(const char *name) {
    if (!name) return false;
    for (const auto &layout : uiLayouts)
        if (std::strcmp(name, layout.className) == 0) return true;
    return false;
}
inline bool isSupportedUiVtable(std::uintptr_t value, std::uintptr_t slide) {
    for (const auto &layout : uiLayouts)
        if (slide <= std::numeric_limits<std::uintptr_t>::max() - layout.vtable &&
            value == slide + layout.vtable) return true;
    return false;
}
struct Snapshot {
    std::string id;
    const char *status = "unavailable";
    std::uintptr_t model = 0; // Ephemeral; the JSON/public result never exports it.
    bool valid = false;
};
template <class T>
bool read(const Reader &reader, std::uintptr_t base, std::size_t offset, T &value) {
    if (!base || offset > std::numeric_limits<std::uintptr_t>::max() - base)
        return false;
    return reader(base + offset, &value, sizeof(value));
}
inline Snapshot inspect(const Reader &reader, std::uintptr_t object, std::uintptr_t slide,
                        const std::function<bool()> &alive) {
    auto fail = [](const char *status) { return Snapshot{{}, status, 0, false}; };
    if (!reader || !alive || !alive())
        return fail("expired-ui-object");
    std::uintptr_t uiV = 0, weak = 0, owner = 0, ownerV = 0, proxy = 0, proxyV = 0;
    std::uintptr_t proxyWeak = 0;
    std::uint8_t flags = 1;
    if (!read(reader, object, 0, uiV) || !isSupportedUiVtable(uiV, slide))
        return fail("unsupported-ui-vtable");
    if (!read(reader, object, 0xc0, weak) || !read(reader, weak, 0x20, flags) ||
        (flags & 1) || !read(reader, weak, 0, owner) || !owner)
        return fail("expired-ui-owner");
    if (!read(reader, owner, 0, ownerV) ||
        (ownerV != slide + 0x11d2d78f0ULL && ownerV != slide + 0x11d2d9908ULL &&
         ownerV != slide + 0x11d2d8938ULL))
        return fail("unsupported-model-vtable");
    if (!read(reader, owner, 0x100, proxy) || !proxy)
        return fail("await-existing-sdk-proxy");
    if (!read(reader, proxy, 0, proxyV) || proxyV != slide + 0x11d4e6de0ULL)
        return fail("unsupported-script-proxy");
    if (!read(reader, proxy, 0x30, proxyWeak) || proxyWeak != weak)
        return fail("proxy-owner-mismatch");
    std::uintptr_t data = 0;
    std::int32_t count = 0;
    std::int64_t offset = 0;
    if (!read(reader, owner, 0x18, data) || !read(reader, data, 4, count) ||
        !read(reader, data, 16, offset) || count < 1 || count > 128 || offset < 24 ||
        offset > 4096)
        return fail("invalid-id-layout");
    char16_t chars[128]{};
    if (std::uint64_t(offset) > std::numeric_limits<std::uintptr_t>::max() - data ||
        !reader(data + std::uintptr_t(offset), chars, std::size_t(count) * 2))
        return fail("unreadable-id");
    std::string id;
    for (std::int32_t i = 0; i < count; ++i) {
        if (chars[i] < 0x20 || chars[i] > 0x7e)
            return fail("non-ascii-id");
        id.push_back(char(chars[i]));
    }
    std::uintptr_t uiV2 = 0, weak2 = 0, owner2 = 0, proxy2 = 0, proxyV2 = 0;
    std::uintptr_t proxyWeak2 = 0, data2 = 0, ownerV2 = 0;
    std::uint8_t flags2 = 1;
    std::int32_t count2 = 0;
    std::int64_t offset2 = 0;
    if (!alive() || !read(reader, object, 0, uiV2) || uiV2 != uiV ||
        !read(reader, object, 0xc0, weak2) || weak2 != weak ||
        !read(reader, weak, 0x20, flags2) || (flags2 & 1) ||
        !read(reader, weak, 0, owner2) || owner2 != owner ||
        !read(reader, owner, 0, ownerV2) || ownerV2 != ownerV ||
        !read(reader, owner, 0x100, proxy2) || proxy2 != proxy ||
        !read(reader, proxy, 0, proxyV2) || proxyV2 != proxyV ||
        !read(reader, proxy, 0x30, proxyWeak2) || proxyWeak2 != weak ||
        !read(reader, owner, 0x18, data2) || data2 != data ||
        !read(reader, data, 4, count2) || count2 != count ||
        !read(reader, data, 16, offset2) || offset2 != offset)
        return fail("identity-changed-during-read");
    return {id, "identity-chain-read", owner, true};
}
} // namespace Unbound::NativeClipIdentity
