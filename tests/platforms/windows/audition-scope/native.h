// SPDX-License-Identifier: MIT
#pragma once
#include "mapreader.h"
#include <QCryptographicHash>
#include <QPointer>
#include <qt_windows.h>
#include <cstring>

inline bool scopeCodeSupported()
{
    const auto base=quintptr(GetModuleHandleW(nullptr));
    struct Region {quintptr rva; int size; const char *sha;};
    const Region regions[]={
        {0x4f8f020,0x2a8,"b46dab230426dd3b9633341b2adf46daf1abc5267e0b2ec5233fafd2b38124a4"},
        {0x4f900c0,0x2e0,"5e14fb28a175a67b8055204ca9468f4b9047d97cedea244d4f0556d12534ab77"}
    };
    for (auto region:regions) {
        QByteArray bytes(region.size,0);SIZE_T copied=0;
        if (!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void *>(base+region.rva),bytes.data(),
                SIZE_T(bytes.size()),&copied) || copied!=SIZE_T(bytes.size()) ||
            QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex()!=region.sha) return false;
    }
    return true;
}
// Caller has already validated the image, manager identity and map via snapshot.
// Read only a fixed manager prefix per event; no widget inventory or tree scan.
inline QJsonObject scopeNativeState(QObject *manager)
{
    char bytes[128];SIZE_T copied=0;
    if (!manager || !ReadProcessMemory(GetCurrentProcess(),manager,bytes,sizeof(bytes),&copied) || copied!=sizeof(bytes))
        return {{"valid",false}};
    qint32 scope=0,context=0;quintptr widget=0;
    std::memcpy(&scope,bytes+0x74,4);std::memcpy(&context,bytes+0x78,4);std::memcpy(&widget,bytes+0x40,8);
    return {{"valid",true},{"scope",scope},{"context",context},
        {"currentWidget",QStringLiteral("0x%1").arg(widget,0,16)}};
}
