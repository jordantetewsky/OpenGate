// SPDX-License-Identifier: MIT
#pragma once
#include <windows.h>
#include <fstream>
#include <cstdio>
#include <filesystem>
#include <sstream>
#include <string>
#ifndef UNBOUND_BUILD_ID
#define UNBOUND_BUILD_ID "development"
#endif
#ifndef OPENGATE_DIAGNOSTIC_MODULE
#define OPENGATE_DIAGNOSTIC_MODULE "Unbound.exe"
#endif
namespace Unbound {
// The launcher must work before Qt is loaded. Use the diagnostic envelope,
// without linking Resolve's Qt or changing bootstrap/injection mechanics.
inline std::string diagnosticJsonString(const std::wstring &value) {
    const int length=WideCharToMultiByte(CP_UTF8,0,value.data(),int(value.size()),nullptr,0,nullptr,nullptr);
    std::string bytes(length,'\0');
    if(length)WideCharToMultiByte(CP_UTF8,0,value.data(),int(value.size()),bytes.data(),length,nullptr,nullptr);
    std::ostringstream result;result << '"';
    for(unsigned char c:bytes) {
        if(c=='"'||c=='\\')result << '\\' << char(c);
        else if(c<32){const char *hex="0123456789abcdef";result << "\\u00" << hex[c>>4] << hex[c&15];}
        else result << char(c);
    }
    result << '"';return result.str();
}
inline void launcherDiagnostic(const char *event,const std::wstring &message={},DWORD resolvePid=0) noexcept {
    try {
        wchar_t local[32768]{};DWORD size=GetEnvironmentVariableW(L"UNBOUND_DIAGNOSTICS_DIR",local,32768);
        std::filesystem::path directory;
        if(size&&size<32768)directory=local;
        else {size=GetEnvironmentVariableW(L"LOCALAPPDATA",local,32768);if(!size||size>=32768)return;directory=std::filesystem::path(local)/L"OpenGate"/L"Logs";}
        std::filesystem::create_directories(directory);
        const auto file=directory/L"launcher.jsonl";
        if(std::filesystem::exists(file)&&std::filesystem::file_size(file)>2*1024*1024) {
            const auto prior=directory/L"launcher.jsonl.1";std::filesystem::remove(prior);std::filesystem::rename(file,prior);
        }
        static const auto session=std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64());
        static unsigned sequence=0;SYSTEMTIME time{};GetSystemTime(&time);char timestamp[40]{};
        std::snprintf(timestamp,sizeof(timestamp),"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",unsigned(time.wYear),unsigned(time.wMonth),unsigned(time.wDay),unsigned(time.wHour),unsigned(time.wMinute),unsigned(time.wSecond),unsigned(time.wMilliseconds));
        std::ofstream output(file,std::ios::app|std::ios::binary);
        output << "{\"component\":\"OpenGate\",\"vendor\":\"CineWaltz\",\"module\":\"" OPENGATE_DIAGNOSTIC_MODULE "\",\"build\":\"" UNBOUND_BUILD_ID "\",\"session\":\"" << session
            << "\",\"event_sequence\":" << ++sequence << ",\"pid\":" << GetCurrentProcessId() << ",\"resolve_pid\":" << resolvePid
            << ",\"time\":\"" << timestamp << "\",\"event\":\"" << event << "\",\"message\":" << diagnosticJsonString(message) << "}\n";
    } catch (...) { /* Diagnostic storage failure must not prevent launching. */ }
}
}
