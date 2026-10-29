#include <cstddef>
#include <cstdint>
#include <cstdarg>
#include <cstring>
#include <cstdio>
#include <dlfcn.h>
#include <IOPluginDefs.h>
#include <IOPluginProps.h>
using namespace IOPlugin;
static int properties = 0;
static StatusCode hostMessage(MessageID message, ...)
{
    if (message != msgPropSet) return errUnsupported;
    va_list args;
    va_start(args, message);
    ObjectRef object = va_arg(args, ObjectRef);
    const char *key = va_arg(args, const char *);
    int type = va_arg(args, int);
    const void *value = va_arg(args, const void *);
    int count = va_arg(args, int);
    va_end(args);
    if (!object || !value) return errInvalidParam;
    if ((!std::strcmp(key, pIOPropUUID) && type == propTypeUInt8 && count == 16) ||
        (!std::strcmp(key, pIOPropName) && type == propTypeString && count > 0 &&
         !std::strncmp(static_cast<const char *>(value), "OpenGate by CineWaltz", 21))) ++properties;
    else return errInvalidParam;
    return errNone;
}
int main(int argc, char **argv)
{
    if (argc != 2) return 1;
    void *library = dlopen(argv[1], RTLD_NOW);
    if (!library) { std::fprintf(stderr, "%s\n", dlerror()); return 1; }
    auto init = reinterpret_cast<pluginInitFunc>(dlsym(library, "pluginInit"));
    if (!init) return 1;
    APIContext host{}, plugin{};
    host.version = version;
    host.pHandleMessage = hostMessage;
    if (init(nullptr, &plugin) != errInvalidParam) return 1;
    host.version = 0;
    if (init(&host, &plugin) != errVersion) return 1;
    host.version = version;
    if (init(&host, &plugin) != errNone || !plugin.pHandleMessage) return 1;
    if (plugin.pHandleMessage(msgPluginGetInfo, &host) != errNone || properties != 2) return 1;
    if (plugin.pHandleMessage(msgPluginGetInfo, nullptr) != errInvalidParam) return 1;
    if (plugin.pHandleMessage(msgPluginStart) != errNone ||
        plugin.pHandleMessage(msgPluginListCodecs, nullptr) != errNone ||
        plugin.pHandleMessage(msgPluginListContainers, nullptr) != errNone ||
        plugin.pHandleMessage(0xFFFFFFFF) != errUnsupported ||
        plugin.pHandleMessage(msgPluginTerminate) != errNone) return 1;
    dlclose(library);
    std::puts("PASS: native bootstrap handshake, metadata, empty codec lists, lifecycle and error handling");
}
