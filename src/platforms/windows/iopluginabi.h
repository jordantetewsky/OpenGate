// SPDX-License-Identifier: MIT
#pragma once
// Minimal IO Encode Plugin ABI used by the no-codec startup adapter.
// Values/layout verified against Resolve Developer/CodecPlugin SDK 0.0.0.1.
// Keep this small: no codec implementation or third-party SDK is distributed.
namespace IOPlugin {
constexpr unsigned version=1;
using ObjectRef=void *;
using MessageID=unsigned;
enum StatusCode {errNone=0,errVersion=-2,errInvalidParam=-3,errUnsupported=-7};
constexpr MessageID msgPropSet=0x00020001, msgPluginStart=0x10000001,
    msgPluginTerminate=0x10000002,msgPluginListCodecs=0x10000003,
    msgPluginListContainers=0x10000004,msgPluginGetInfo=0x10000005;
constexpr int propTypeUInt8=2,propTypeString=10;
constexpr const char *pIOPropUUID="uuid",*pIOPropName="name";
#pragma pack(push,1)
struct APIContext {unsigned version;StatusCode (*pHandleMessage)(MessageID,...);};
#pragma pack(pop)
using pluginInitFunc=StatusCode (*)(const APIContext *,APIContext *);
}
