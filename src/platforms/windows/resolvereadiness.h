// SPDX-License-Identifier: MIT
#pragma once
#include <windows.h>
#include <vector>
#include <cwchar>
static bool qtCompatible(const wchar_t *path)
{
    DWORD unused;
    const DWORD size = GetFileVersionInfoSizeW(path, &unused);
    if (!size) return false;
    std::vector<BYTE> data(size);
    if (!GetFileVersionInfoW(path, 0, size, data.data())) return false;
    VS_FIXEDFILEINFO *info = nullptr; UINT length = 0;
    if (!VerQueryValueW(data.data(), L"\\", reinterpret_cast<void **>(&info), &length) || length < sizeof(*info)) return false;
    return HIWORD(info->dwFileVersionMS) == 5 && LOWORD(info->dwFileVersionMS) == 15 && HIWORD(info->dwFileVersionLS) == 2;
}
struct WindowSearch { DWORD pid; bool ready = false; };
static BOOL CALLBACK findQtWindow(HWND window, LPARAM parameter)
{
    auto &search = *reinterpret_cast<WindowSearch *>(parameter);
    DWORD pid = 0;
    GetWindowThreadProcessId(window, &pid);
    if (pid != search.pid || !IsWindowVisible(window)) return TRUE;
    wchar_t name[256]{};
    GetClassNameW(window, name, 256);
    // A visible Qt window (including Project Manager) proves QApplication has
    // been constructed. Merely finding Qt DLLs during process startup does not.
    if (wcsncmp(name, L"Qt5", 3) == 0) { search.ready = true; return FALSE; }
    return TRUE;
}
