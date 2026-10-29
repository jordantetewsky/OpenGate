// SPDX-License-Identifier: MIT
#include "mapreader.h"
#include <QApplication>
#include <QMap>
#include <QPointer>
#include <QWidget>
#include <qt_windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static void check(bool condition,const char *message) {
    if (!condition) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
static bool readMemory(quintptr source,void *target,size_t count) {
    SIZE_T copied=0;
    return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void *>(source),target,count,&copied) && copied==count;
}
template<class T> void put(QByteArray &bytes,int offset,T value) {std::memcpy(bytes.data()+offset,&value,sizeof(value));}
int main(int argc,char **argv)
{
    QApplication app(argc,argv);
    struct Value {QPointer<QWidget> widget,associated;};
    QWidget first,second;
    QMap<int,Value> native;
    native.insert(100,{&first,&second});native.insert(-3,{&second,{}});native.insert(400,{{},&first});
    quintptr data=0;std::memcpy(&data,&native,sizeof(data));
    const auto result=AuditionMap::readMap(data,readMemory);
    check(result.valid && result.entries.size()==3,"Decode actual Qt 5.15.2 QMap layout");
    check(result.entries[0].context==-3 && result.entries[1].context==100 && result.entries[2].context==400,"Sorted keys");
    check(result.entries[1].widget==quintptr(&first) && result.entries[1].associated==quintptr(&second),"QPointer target offsets");
    auto dead=new QWidget;
    native[100].widget=dead;delete dead;
    std::memcpy(&data,&native,sizeof(data));
    check(AuditionMap::readMap(data,readMemory).valid,"Expired reference does not invalidate tree");
    native.clear();std::memcpy(&data,&native,sizeof(data));
    check(AuditionMap::readMap(data,readMemory).valid,"Empty shared map accepted");
    QMultiMap<int,Value> multiple;
    for (int i=0;i<20;++i) multiple.insert(100,{&first,&second});
    std::memcpy(&data,&multiple,sizeof(data));
    const auto duplicates=AuditionMap::readMap(data,readMemory);
    check(duplicates.valid && duplicates.entries.size()==20,"Multiple registrations for the same context preserved");

    // Deliberately corrupt structures live only in fixture byte arrays.
    QByteArray header(40,0),node(64,0);
    put<qint32>(header,4,1);put<quintptr>(header,16,0x2000);
    put<quintptr>(node,0,0x1008);put<qint32>(node,24,10);
    AuditionMap::Reader synthetic=[&](quintptr source,void *target,size_t count) {
        const auto *bytes=source==0x1000 ? &header : source==0x2000 ? &node : nullptr;
        if (!bytes || count>size_t(bytes->size())) return false;
        std::memcpy(target,bytes->constData(),count);return true;
    };
    check(AuditionMap::readMap(0x1000,synthetic).valid,"Synthetic tree accepted");
    put<quintptr>(node,8,0x2000);
    check(!AuditionMap::readMap(0x1000,synthetic).valid,"Cycle rejected");
    put<quintptr>(node,8,0);put<quintptr>(node,0,0x9998);
    check(!AuditionMap::readMap(0x1000,synthetic).valid,"Bad parent rejected");
    put<quintptr>(node,0,0x1008);put<qint32>(header,4,3000);
    check(!AuditionMap::readMap(0x1000,synthetic).valid,"Oversize rejected");
    put<qint32>(header,4,2);
    check(!AuditionMap::readMap(0x1000,synthetic).valid,"Count mismatch rejected");
    put<qint32>(header,4,1);put<quintptr>(header,16,0x3000);
    check(!AuditionMap::readMap(0x1000,synthetic).valid,"Unreadable node rejected");
    check(!AuditionMap::readMap(1,readMemory).valid,"Invalid address rejected without dereference");
    QString reason;
    check(!AuditionMap::supportedImage(readMemory,quintptr(GetModuleHandleW(nullptr)),&reason),"Non-Resolve runtime image rejected");
    check(!AuditionMap::supportedImage(readMemory,1,&reason),"Invalid image rejected safely");
    std::puts("PASS: actual Qt map/QPointer layout, ordering, empty/expired entries, bounded corrupt-tree rejection and runtime image gate");
}
