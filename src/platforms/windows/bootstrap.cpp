// SPDX-License-Identifier: MIT
#include <windows.h>
#include "controller.h"

// Invoked by the launcher on a short-lived remote thread; no work in DllMain.
extern "C" __declspec(dllexport) DWORD WINAPI UnboundStart(void *argument)
{
    if (argument) return 0;
    return Unbound::startController() ? 1 : 0;
}
