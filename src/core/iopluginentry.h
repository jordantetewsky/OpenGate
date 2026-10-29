// SPDX-License-Identifier: MIT
#pragma once
#include <cstdarg>
#include <cstdint>
#include <cstring>
// The OS wrapper supplies the SDK ABI and startup callback. This is the existing
// Mac handshake/metadata/lifecycle policy, with no codecs or containers advertised.
namespace Unbound {
using namespace IOPlugin;
template<void (*Start)()> struct IOPluginEntry {
inline static APIContext hostAPI = {};

static StatusCode handleMessage(MessageID message, ...)
{
    switch (message) {
    case msgPluginStart:
        // Reuse the validated main-queue bootstrap; advertise no media codecs.
        Start();
        return errNone;
    case msgPluginTerminate:
    case msgPluginListCodecs:
    case msgPluginListContainers:
        return errNone;
    case msgPluginGetInfo: {
        va_list args;
        va_start(args, message);
        ObjectRef properties = va_arg(args, ObjectRef);
        va_end(args);
        if (!properties) return errInvalidParam;
        static const uint8_t uuid[] = {0x34,0x9e,0xb2,0xa0,0x68,0x29,0x4f,0x45,0xaf,0x6d,0xba,0xf9,0x93,0x7e,0x11,0x2c};
        StatusCode status = hostAPI.pHandleMessage(msgPropSet, properties, pIOPropUUID, propTypeUInt8, uuid, 16);
        if (status != errNone) return status;
        const char *name = "OpenGate by CineWaltz — experimental panel helper";
        return hostAPI.pHandleMessage(msgPropSet, properties, pIOPropName, propTypeString, name, int(std::strlen(name)));
    }
    default:
        return errUnsupported;
    }
}

static StatusCode initialize(const APIContext *host, APIContext *plugin)
{
    if (!host || !plugin || !host->pHandleMessage) return errInvalidParam;
    if (host->version < version) return errVersion;
    hostAPI = *host;
    plugin->version = version;
    plugin->pHandleMessage = handleMessage;
    Start();
    return errNone;
}

};
}
