// SPDX-License-Identifier: MIT
// Compile locally against the installed SDK to verify the minimal Windows ABI.
#define IOPlugin SDKIOPlugin
#include <cstddef>
#include <cstdint>
#include <IOPluginDefs.h>
#include <IOPluginProps.h>
#undef IOPlugin
#include "../../../src/platforms/windows/iopluginabi.h"
#include <cstring>
#define SAME(name) static_assert(int(IOPlugin::name)==int(SDKIOPlugin::name),#name)
SAME(version);SAME(errNone);SAME(errVersion);SAME(errInvalidParam);SAME(errUnsupported);
SAME(msgPropSet);SAME(msgPluginStart);SAME(msgPluginTerminate);SAME(msgPluginListCodecs);
SAME(msgPluginListContainers);SAME(msgPluginGetInfo);SAME(propTypeUInt8);SAME(propTypeString);
static_assert(sizeof(IOPlugin::APIContext)==sizeof(SDKIOPlugin::APIContext));
static_assert(alignof(IOPlugin::APIContext)==alignof(SDKIOPlugin::APIContext));
static_assert(offsetof(IOPlugin::APIContext,pHandleMessage)==offsetof(SDKIOPlugin::APIContext,pHandleMessage));
int main(){return std::strcmp(IOPlugin::pIOPropUUID,SDKIOPlugin::pIOPropUUID)||std::strcmp(IOPlugin::pIOPropName,SDKIOPlugin::pIOPropName);}
