// SPDX-License-Identifier: MIT
#include "mapreader.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QHash>
#include <QJsonArray>
#include <QSet>
#include <QThread>
#include <QWidget>
#include <qt_windows.h>
#include <algorithm>
#include <cstring>
#include <limits>

namespace AuditionMap {
namespace {
template<class T> T field(const void *bytes,size_t offset) {
    T value; std::memcpy(&value,static_cast<const char *>(bytes)+offset,sizeof(value)); return value;
}
template<class T> bool readAt(const Reader &read,quintptr address,T &value) {
    return address && read(address,&value,sizeof(value));
}
QString address(quintptr value) {return QStringLiteral("0x%1").arg(value,0,16);}
QJsonObject identity(QWidget *widget) {
    if (!widget) return {};
    return {{"address",address(quintptr(widget))},{"class",widget->metaObject()->className()},
        {"name",widget->objectName()}};
}
bool processRead(quintptr source,void *target,size_t count) {
    if (!source || !count || count>4096 || source>std::numeric_limits<quintptr>::max()-count) return false;
    SIZE_T copied=0;
    return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void *>(source),target,count,&copied)
        && copied==count;
}
}

Result readMap(quintptr data,const Reader &read)
{
    Result result;
    auto fail=[&](const QString &error) {result.error=error;result.entries.clear();return result;};
    if (!data || data%8 || data>std::numeric_limits<quintptr>::max()-40) return fail("invalid-map-address");
    char header[40];
    if (!read(data,header,sizeof(header))) return fail("unreadable-map-header");
    result.advertisedSize=field<qint32>(header,4);
    if (result.advertisedSize<0 || result.advertisedSize>2048) return fail("map-size-out-of-range");
    const auto root=field<quintptr>(header,16);
    if ((root==0)!=(result.advertisedSize==0)) return fail("root-size-mismatch");
    struct Work {quintptr node,parent; qint64 lower,upper;};
    QVector<Work> pending;
    if (root) pending.append({root,data+8,std::numeric_limits<qint64>::min(),std::numeric_limits<qint64>::max()});
    QSet<quintptr> seen;
    while (!pending.isEmpty()) {
        const auto work=pending.takeLast();
        if (!work.node || work.node%8 || seen.contains(work.node) || seen.size()>=result.advertisedSize)
            return fail("invalid-repeated-or-excess-node");
        seen.insert(work.node);
        char node[64];
        if (!read(work.node,node,sizeof(node))) return fail("unreadable-node");
        if ((field<quintptr>(node,0)&~quintptr(3))!=work.parent) return fail("parent-link-mismatch");
        const auto key=field<qint32>(node,24);
        // Resolve can register multiple widgets for one context (QMultiMap).
        // Red/black rotations may place equal keys on either side of a node.
        if (key<work.lower || key>work.upper)
            return fail(QString("key-order-mismatch:%1-outside-%2..%3").arg(key).arg(work.lower).arg(work.upper));
        result.entries.append({key,work.node,field<quintptr>(node,32),field<quintptr>(node,40),
            field<quintptr>(node,48),field<quintptr>(node,56)});
        const auto left=field<quintptr>(node,8),right=field<quintptr>(node,16);
        if (right) pending.append({right,work.node,key,work.upper});
        if (left) pending.append({left,work.node,work.lower,key});
    }
    if (seen.size()!=result.advertisedSize) return fail("node-count-mismatch");
    std::sort(result.entries.begin(),result.entries.end(),[](const Entry &a,const Entry &b){return a.context<b.context;});
    result.valid=true;
    return result;
}

bool supportedImage(const Reader &read,quintptr base,QString *reason)
{
    auto fail=[&](const QString &error){*reason=error;return false;};
    if (!base || base>std::numeric_limits<quintptr>::max()-0x250fd000) return fail("invalid-image-base");
    char dos[64];
    if (!read(base,dos,sizeof(dos)) || field<quint16>(dos,0)!=0x5a4d) return fail("invalid-DOS-header");
    const auto peOffset=field<qint32>(dos,60);
    if (peOffset<64 || peOffset>4096) return fail("invalid-PE-offset");
    char pe[96];
    if (!read(base+peOffset,pe,sizeof(pe)) || field<quint32>(pe,0)!=0x4550 ||
        field<quint16>(pe,4)!=0x8664 || field<quint16>(pe,24)!=0x20b ||
        field<quint32>(pe,8)!=0x6abdeef4 || field<quint32>(pe,80)!=0x250fd000)
        return fail("unsupported-Resolve-image");
    struct Region {quintptr rva; int size; const char *sha;};
    const Region regions[]={
        {0x4f8ea00,0x3c1,"d1bca7e7a2bb5820d2818ca63fee81502bf85ee0637b93a97b55cc8c5d1b38af"},
        {0x4f903b0,0xdf,"225f54d8a9f5a5fd4bba424c4e04584bb9f1ba2ad77f56fbf096449bbf03cd1b"},
        {0x4f8fda0,0x300,"542160203f0c3136f54845f383eaca5237ab6d5fc616ec5df6a999a68023787c"}
    };
    for (const auto &region:regions) {
        QByteArray bytes(region.size,0);
        if (!read(base+region.rva,bytes.data(),size_t(bytes.size())) ||
            QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex()!=region.sha)
            return fail("context-code-fingerprint-mismatch");
    }
    *reason="verified-21.1.1.10-context-code";
    return true;
}

QJsonObject snapshot(QObject *manager,QWidget *preview,QWidget *page,QWidget *slot)
{
    QJsonObject result{{"readOnly",true}};
    const Reader read=processRead;
    const auto base=quintptr(GetModuleHandleW(nullptr));
    QString gate;
    if (QThread::currentThread()!=qApp->thread() || QByteArray(qVersion())!="5.15.2")
        return {{"valid",false},{"error","Qt-version-or-thread-mismatch"}};
    if (!supportedImage(read,base,&gate)) return {{"valid",false},{"error",gate}};
    result.insert("imageGate",gate);
    // Static metadata identity further guards manager offsets against a lookalike.
    if (!manager || quintptr(manager->metaObject())!=base+0x1b0c4510)
        return {{"valid",false},{"error","manager-metaobject-mismatch"}};
    char state[128];
    if (!read(quintptr(manager),state,sizeof(state))) return {{"valid",false},{"error","manager-unreadable"}};
    const auto data=field<quintptr>(state,0x28);
    auto decoded=readMap(data,read);
    result.insert("mapAddress",address(data));
    result.insert("valid",decoded.valid);
    result.insert("advertisedSize",decoded.advertisedSize);
    if (!decoded.valid) {result.insert("error",decoded.error);return result;}

    // Resolve raw pointers exclusively through Qt's live widget inventory on the
    // GUI thread. Never call a method on a decoded/unrecognized target pointer.
    QHash<quintptr,QWidget *> widgets;
    for (auto widget:QApplication::allWidgets())
        if (widget->thread()==qApp->thread()) widgets.insert(quintptr(widget),widget);
    auto describe=[&](quintptr control,quintptr raw) {
        QJsonObject description{{"address",address(raw)},{"controlAddress",address(control)}};
        qint32 strong=0;
        bool readable=control && control<=std::numeric_limits<quintptr>::max()-8 && readAt(read,control+4,strong);
        description.insert("controlReadable",readable);
        if (readable) description.insert("strongRef",strong);
        auto widget=readable && strong!=0 ? widgets.value(raw,nullptr) : nullptr;
        description.insert("resolvedLiveWidget",widget!=nullptr);
        if (!widget) return description;
        description.insert("identity",identity(widget));
        description.insert("window",identity(widget->window()));
        description.insert("visible",widget->isVisible());
        description.insert("hidden",widget->isHidden());
        description.insert("focusPolicy",int(widget->focusPolicy()));
        description.insert("isPreview",widget==preview);
        description.insert("qtAncestorOfPreview",preview && widget->isAncestorOf(preview));
        description.insert("qtAncestorOfSlot",slot && widget->isAncestorOf(slot));
        description.insert("pageIsAncestor",page && page->isAncestorOf(widget));
        bool parentChain=false;
        for (auto p=preview;p;p=p->parentWidget()) if (p==widget) parentChain=true;
        description.insert("parentChainContainsWidget",parentChain);
        QJsonArray path;
        auto ancestor=widget;
        for (int i=0;ancestor && i<48;++i,ancestor=ancestor->parentWidget()) path.append(identity(ancestor));
        description.insert("parentPath",path);
        description.insert("pathTruncated",ancestor!=nullptr);
        return description;
    };
    QJsonArray entries;
    for (const auto &entry:decoded.entries) {
        entries.append(QJsonObject{{"context",entry.context},{"contextHex",address(quint32(entry.context))},
            {"registeredWidget",describe(entry.widgetControl,entry.widget)},
            {"associatedWidget",describe(entry.associatedControl,entry.associated)}});
    }
    result.insert("entries",entries);
    result.insert("currentContext",field<qint32>(state,0x78));
    result.insert("lookupScope",field<qint32>(state,0x74));
    result.insert("field70",field<qint32>(state,0x70));
    result.insert("currentWidget",describe(field<quintptr>(state,0x38),field<quintptr>(state,0x40)));
    result.insert("preview",identity(preview));
    result.insert("slot",identity(slot));
    result.insert("page",identity(page));
    return result;
}
}
