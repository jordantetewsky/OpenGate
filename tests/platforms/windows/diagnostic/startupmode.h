// SPDX-License-Identifier: MIT
#ifndef UNBOUND_STARTUPMODE_H
#define UNBOUND_STARTUPMODE_H
#include <string>
namespace Unbound {
// Passed as an integer-valued remote-thread argument, never dereferenced.
enum class StartupMode : unsigned { Normal, LoadOnly, BootstrapOnly, NoPlacement, NoSnap, NoDecorations, NoPolling };
inline bool parseStartupMode(const std::wstring &command, StartupMode &mode)
{
    const wchar_t *names[] = {L"", L"--diagnostic=load-only", L"--diagnostic=bootstrap-only",
        L"--diagnostic=no-placement", L"--diagnostic=no-snap",
        L"--diagnostic=no-decorations", L"--diagnostic=no-polling"};
    for (unsigned i=0; i<sizeof(names)/sizeof(*names); ++i)
        if (command == names[i]) { mode=static_cast<StartupMode>(i); return true; }
    return false;
}
}
#endif
