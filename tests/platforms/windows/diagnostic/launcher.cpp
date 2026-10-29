// SPDX-License-Identifier: MIT
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <cwchar>
#include "startupmode.h"

struct Handle {
    HANDLE h = nullptr;
    explicit Handle(HANDLE value) : h(value) {}
    ~Handle() { if (h && h != INVALID_HANDLE_VALUE) CloseHandle(h); }
    Handle(const Handle &) = delete;
};
static void require(bool condition, const wchar_t *message)
{
    if (!condition) throw std::wstring(message);
}
static std::vector<MODULEENTRY32W> modules(DWORD pid)
{
    HANDLE value;
    for (;;) {
        value = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (value != INVALID_HANDLE_VALUE) break;
        const DWORD code = GetLastError();
        // Documented Toolhelp contract: concurrent DLL loading can invalidate
        // the snapshot. A visible Qt splash window does not imply loading ended.
        if (code == ERROR_BAD_LENGTH) { Sleep(1000); continue; }
        wchar_t description[1024]{};
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, code, 0, description, 1024, nullptr);
        throw std::wstring(L"Cannot inspect Resolve's loaded libraries (process ") +
            std::to_wstring(pid) + L", Windows error " + std::to_wstring(code) +
            L").\n" + description;
    }
    Handle snapshot(value);
    MODULEENTRY32W entry{}; entry.dwSize = sizeof(entry);
    std::vector<MODULEENTRY32W> result;
    if (Module32FirstW(snapshot.h, &entry)) do { result.push_back(entry); } while (Module32NextW(snapshot.h, &entry));
    return result;
}
static MODULEENTRY32W findModule(const std::vector<MODULEENTRY32W> &list, const wchar_t *name)
{
    for (const auto &entry : list) if (_wcsicmp(entry.szModule, name) == 0) return entry;
    return MODULEENTRY32W{};
}
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
static DWORD invoke(HANDLE process, LPTHREAD_START_ROUTINE address, void *argument)
{
    Handle thread(CreateRemoteThread(process, nullptr, 0, address, argument, 0, nullptr));
    require(thread.h != nullptr, L"Unable to start the helper in Resolve. Access may be blocked by process permissions or security software.");
    require(WaitForSingleObject(thread.h, 30000) == WAIT_OBJECT_0, L"Resolve did not respond within 30 seconds. No thread was forcibly terminated. Restart Resolve before retrying.");
    DWORD result = 0;
    require(GetExitCodeThread(thread.h, &result) != 0, L"Cannot read helper startup result.");
    return result;
}
static DWORD resolveProcess()
{
    Handle processes(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    require(processes.h != INVALID_HANDLE_VALUE, L"Cannot enumerate running processes.");
    PROCESSENTRY32W pe{}; pe.dwSize = sizeof(pe); DWORD pid = 0;
    if (Process32FirstW(processes.h, &pe)) do {
        if (_wcsicmp(pe.szExeFile, L"Resolve.exe") != 0) continue;
        require(pid == 0, L"More than one Resolve process exists. Close any stale Resolve process before attaching.");
        pid = pe.th32ProcessID;
    } while (Process32NextW(processes.h, &pe));
    return pid;
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
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR command, int)
{
    try {
        if (std::wstring(command) == L"--version") {
            MessageBoxW(nullptr, UNBOUND_VERSION, L"Unbound", MB_OK);
            return 0;
        }
        Unbound::StartupMode mode;
        require(Unbound::parseStartupMode(command, mode), L"Unknown argument. Use --diagnostic=load-only, bootstrap-only, no-placement, no-snap, no-decorations, or no-polling (one mode per fresh Resolve session).");
        // Keep repeated launches from accumulating invisible waiting helpers.
        Handle waiter(CreateMutexW(nullptr, FALSE, L"Local\\Unbound.Launcher.Wait"));
        const DWORD mutexError = GetLastError();
        require(waiter.h != nullptr, L"Cannot initialize Unbound's launcher guard.");
        if (mutexError == ERROR_ALREADY_EXISTS) return 0;
        DWORD pid = 0;
        std::vector<MODULEENTRY32W> loaded;
        for (;;) {
            pid = resolveProcess();
            WindowSearch search{pid};
            if (pid) EnumWindows(findQtWindow, reinterpret_cast<LPARAM>(&search));
            if (search.ready) {
                loaded = modules(pid);
                if (findModule(loaded, L"Qt5Core.dll").modBaseAddr &&
                    findModule(loaded, L"Qt5Gui.dll").modBaseAddr &&
                    findModule(loaded, L"Qt5Widgets.dll").modBaseAddr) break;
            }
            Sleep(1000);
        }
        require(!findModule(loaded, L"gammaray_probe.dll").modBaseAddr, L"GammaRay is already attached. Restart Resolve before using standalone Unbound.");
        require(!findModule(loaded, L"Unbound.dll").modBaseAddr, L"Unbound is already loaded. Restart Resolve before loading another build.");
        auto qt = findModule(loaded, L"Qt5Core.dll");
        require(qt.modBaseAddr && qtCompatible(qt.szExePath) && findModule(loaded, L"Qt5Widgets.dll").modBaseAddr,
            L"This build requires Resolve with Qt 5.15.2 x64. Its required Qt libraries are not loaded or are incompatible.");
        wchar_t executable[32768];
        const DWORD count = GetModuleFileNameW(nullptr, executable, 32768);
        require(count > 0 && count < 32768, L"Cannot locate Unbound.exe.");
        std::wstring dll(executable, count);
        dll = dll.substr(0, dll.find_last_of(L"\\/") + 1) + L"Unbound.dll";
        require(GetFileAttributesW(dll.c_str()) != INVALID_FILE_ATTRIBUTES, L"Keep Unbound.dll beside Unbound.exe.");
        HMODULE local = LoadLibraryExW(dll.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES);
        require(local != nullptr, L"Unbound.dll cannot be read as a Windows x64 DLL.");
        auto start = GetProcAddress(local, "UnboundStart");
        const auto offset = reinterpret_cast<ULONG_PTR>(start) - reinterpret_cast<ULONG_PTR>(local);
        FreeLibrary(local);
        require(start != nullptr, L"Unbound.dll is missing its startup export.");
        Handle process(OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid));
        require(process.h != nullptr, L"Cannot attach. Run Resolve and Unbound at the same privilege level; no elevation is performed automatically.");
        BOOL wow64 = FALSE;
        require(IsWow64Process(process.h, &wow64) && !wow64, L"Only 64-bit Resolve is supported.");
        const auto load = GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
        HMODULE owner = nullptr;
        require(load && GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(load), &owner), L"Cannot locate the system DLL loader.");
        wchar_t ownerPath[MAX_PATH]; GetModuleFileNameW(owner, ownerPath, MAX_PATH);
        const wchar_t *ownerName = wcsrchr(ownerPath, L'\\'); ownerName = ownerName ? ownerName + 1 : ownerPath;
        auto remoteOwner = findModule(loaded, ownerName);
        require(remoteOwner.modBaseAddr != nullptr, L"Resolve's system loader module was not found.");
        auto remoteLoad = reinterpret_cast<LPTHREAD_START_ROUTINE>(remoteOwner.modBaseAddr +
            (reinterpret_cast<ULONG_PTR>(load) - reinterpret_cast<ULONG_PTR>(owner)));
        const SIZE_T bytes = (dll.size() + 1) * sizeof(wchar_t);
        void *argument = VirtualAllocEx(process.h, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        require(argument != nullptr, L"Cannot allocate the DLL path in Resolve.");
        SIZE_T written = 0;
        if (!WriteProcessMemory(process.h, argument, dll.c_str(), bytes, &written) || written != bytes) {
            VirtualFreeEx(process.h, argument, 0, MEM_RELEASE);
            throw std::wstring(L"Cannot transfer the DLL path to Resolve.");
        }
        // On a timeout leave the tiny argument allocation valid for the pending loader.
        invoke(process.h, remoteLoad, argument);
        VirtualFreeEx(process.h, argument, 0, MEM_RELEASE);
        auto injected = findModule(modules(pid), L"Unbound.dll");
        require(injected.modBaseAddr != nullptr, L"Resolve could not load Unbound.dll. Its existing Qt/MSVC runtime must satisfy the helper dependencies.");
        if (mode == Unbound::StartupMode::LoadOnly) return 0;
        require(invoke(process.h, reinterpret_cast<LPTHREAD_START_ROUTINE>(injected.modBaseAddr + offset),
            reinterpret_cast<void *>(static_cast<ULONG_PTR>(mode))) == 1,
            L"Unbound loaded, but could not queue startup on Resolve's GUI thread. Restart Resolve before retrying.");
        return 0;
    } catch (const std::wstring &error) {
        MessageBoxW(nullptr, error.c_str(), L"Unbound", MB_OK | MB_ICONERROR);
        return 1;
    }
}
