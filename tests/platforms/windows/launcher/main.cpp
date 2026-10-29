// Exercise the real launcher module reader with controlled Windows API failures.
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <stdexcept>
static DWORD snapshotError = 0;
static int failures = 0, attempts = 0, delays = 0;
static HANDLE WINAPI snapshot(DWORD flags, DWORD pid)
{
    ++attempts;
    if (failures-- > 0) {
        SetLastError(snapshotError);
        return INVALID_HANDLE_VALUE;
    }
    return CreateToolhelp32Snapshot(flags, pid);
}
static void WINAPI delay(DWORD milliseconds)
{
    if (milliseconds != 1000) throw std::runtime_error("Wrong polling interval");
    ++delays;
}
#define CreateToolhelp32Snapshot snapshot
#define Sleep delay
#define UNBOUND_VERSION L"test"
#include "../../../../src/platforms/windows/launcher.cpp"
#undef Sleep
#undef CreateToolhelp32Snapshot
int main()
{
    try {
        snapshotError = ERROR_BAD_LENGTH; failures = 3;
        const auto loaded = modules(GetCurrentProcessId());
        if (loaded.empty() || attempts != 4 || delays != 3)
            throw std::runtime_error("Transient snapshot failure did not recover");
        for (DWORD error : {DWORD(ERROR_ACCESS_DENIED), DWORD(ERROR_PARTIAL_COPY)}) {
            attempts = delays = 0; failures = 1; snapshotError = error;
            bool caught = false;
            try { modules(GetCurrentProcessId()); }
            catch (const std::wstring &message) {
                caught = message.find(L"Windows error " + std::to_wstring(error)) != std::wstring::npos;
            }
            if (!caught || attempts != 1 || delays != 0)
                throw std::runtime_error("Unexpected error was hidden or misreported");
        }
        std::cout << "PASS: loader-change retries recover; other errors retain their Windows code without retrying\n";
        return 0;
    } catch (const std::exception &error) { std::cerr << error.what(); return 1; }
    catch (...) { std::cerr << "Unexpected module inspection error"; return 1; }
}
