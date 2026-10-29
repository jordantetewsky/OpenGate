// SPDX-License-Identifier: MIT
#include <windows.h>
#include <atomic>
#include <string>
#include "iopluginabi.h"
#include "resolvereadiness.h"
#define OPENGATE_DIAGNOSTIC_MODULE "OpenGate.dvcp"
#include "launcherdiagnostics.h"
#include "../../core/iopluginentry.h"
namespace {
std::atomic<bool> scheduled{false};
// Inno already owns this machine-wide installation record. Reuse it so custom
// installation directories work without a second configuration/autorun system.
std::wstring helperPath() {
    wchar_t directory[32768]{};DWORD bytes=sizeof(directory);
    const auto error=RegGetValueW(HKEY_LOCAL_MACHINE,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\OpenGate-CineWaltz-Prototype_is1",
        L"InstallLocation",RRF_RT_REG_SZ|RRF_SUBKEY_WOW6464KEY,nullptr,directory,&bytes);
    if(error!=ERROR_SUCCESS||!directory[0])return {};
    std::wstring path=directory;
    if(path.back()!=L'\\'&&path.back()!=L'/')path+=L'\\';
    return path+L"Unbound.dll";
}
DWORD WINAPI startWhenReady(void *pinnedModule) {
    Unbound::launcherDiagnostic("bootstrap.io.waiting-for-qt",{},GetCurrentProcessId());
    bool complete=false;
    for(unsigned attempt=0;attempt<120;++attempt) {
        WindowSearch search{GetCurrentProcessId()};EnumWindows(findQtWindow,reinterpret_cast<LPARAM>(&search));
        const auto qt=GetModuleHandleW(L"Qt5Core.dll");
        if(search.ready&&qt&&GetModuleHandleW(L"Qt5Gui.dll")&&GetModuleHandleW(L"Qt5Widgets.dll")) {
            wchar_t path[32768]{};
            const auto length=GetModuleFileNameW(qt,path,32768);
            if(!length||length>=32768||!qtCompatible(path)) {
                Unbound::launcherDiagnostic("bootstrap.io.incompatible-qt",{},GetCurrentProcessId());break;
            }
            const auto file=helperPath();
            if(file.empty()) {Unbound::launcherDiagnostic("bootstrap.io.installation-record-missing",{},GetCurrentProcessId());break;}
            // Reuse only this installation's helper. An old Unbound build has
            // the same DLL/export names but a different panel controller.
            auto helper=GetModuleHandleW(L"Unbound.dll");
            if(helper) {
                wchar_t loaded[32768]{},expected[32768]{};
                const auto loadedLength=GetModuleFileNameW(helper,loaded,32768);
                const auto expectedLength=GetFullPathNameW(file.c_str(),32768,expected,nullptr);
                if(!loadedLength||loadedLength>=32768||!expectedLength||expectedLength>=32768||_wcsicmp(loaded,expected)!=0) {
                    Unbound::launcherDiagnostic("bootstrap.io.other-helper-loaded",L"A different OpenGate or Unbound helper is already loaded. Restart Resolve before using this installation.",GetCurrentProcessId());break;
                }
            }
            if(!helper) {
                helper=LoadLibraryExW(file.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
                if(!helper) {Unbound::launcherDiagnostic("bootstrap.io.helper-load-failed",L"Windows error "+std::to_wstring(GetLastError()),GetCurrentProcessId());break;}
            }
            using Start=DWORD(WINAPI *)(void *);
            const auto start=reinterpret_cast<Start>(GetProcAddress(helper,"UnboundStart"));
            if(!start) {Unbound::launcherDiagnostic("bootstrap.io.entry-missing",{},GetCurrentProcessId());break;}
            if(start(nullptr)==1) {
                complete=true;Unbound::launcherDiagnostic("bootstrap.io.queued",{},GetCurrentProcessId());break;
            }
        }
        Sleep(500);
    }
    if(!complete) {
        scheduled=false;
        Unbound::launcherDiagnostic("bootstrap.io.not-started",{},GetCurrentProcessId());
    }
    // Keep callbacks loaded while waiting; never unload the resident Qt helper.
    FreeLibraryAndExitThread(static_cast<HMODULE>(pinnedModule),complete?0:1);
}
void scheduleStartup() {
    if(scheduled.exchange(true))return;
    HMODULE pin=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        reinterpret_cast<LPCWSTR>(&scheduleStartup),&pin)) {scheduled=false;return;}
    const auto thread=CreateThread(nullptr,0,startWhenReady,pin,0,nullptr);
    if(thread)CloseHandle(thread);
    else {FreeLibrary(pin);scheduled=false;Unbound::launcherDiagnostic("bootstrap.io.schedule-failed");}
}
}
// Resolve calls this entry outside DllMain. It advertises no media support.
extern "C" __declspec(dllexport) IOPlugin::StatusCode pluginInit(const IOPlugin::APIContext *host,IOPlugin::APIContext *plugin)
{
    return Unbound::IOPluginEntry<scheduleStartup>::initialize(host,plugin);
}
