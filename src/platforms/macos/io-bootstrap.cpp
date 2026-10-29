// SPDX-License-Identifier: MIT
#include <cstddef>
#include <cstdint>
#include <IOPluginDefs.h>
#include <IOPluginProps.h>
#include "../../core/iopluginentry.h"
extern "C" int OfxGetNumberOfPlugins(void);
static void scheduleStartup() { OfxGetNumberOfPlugins(); }
extern "C" __attribute__((visibility("default"))) IOPlugin::StatusCode pluginInit(const IOPlugin::APIContext *host, IOPlugin::APIContext *plugin)
{
    return Unbound::IOPluginEntry<scheduleStartup>::initialize(host,plugin);
}
