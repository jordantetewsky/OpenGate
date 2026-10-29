// SPDX-License-Identifier: MIT
#pragma once
#include <QFile>
#include <QFileSystemWatcher>
#include <QHash>
#include <QKeySequence>
#include <QTimer>
#include <QSet>
#include <functional>
#include <QXmlStreamReader>
#include "diagnostics.h"
namespace Unbound {
struct NativePresetSnapshot {
    QString preset,error;
    bool present=false;
    QHash<QString,QList<QKeySequence>> commands;
    QHash<QString,QStringList> owners;
};
// Bounded decoder for the observed version-1 Qt serialization. Unsupported data
// fails closed. Resolve owns the files; this class never writes preferences.
class NativePresetBindings : public QObject {
public:
    struct Reader {
        QByteArray bytes;int offset=0;bool ok=true;
        quint32 integer() {
            if (!ok || offset>bytes.size()-4) { ok=false;return 0; }
            quint32 n=0;for(int i=0;i<4;++i)n=(n<<8)|quint8(bytes[offset++]);return n;
        }
        QByteArray blob() {
            const auto n=integer();if (!ok || n>quint32(bytes.size()-offset)) {ok=false;return {};}
            const auto result=bytes.mid(offset,int(n));offset+=int(n);return result;
        }
        QString string() {
            const auto data=blob();if (data.size()%2) {ok=false;return {};}
            QString result;result.reserve(data.size()/2);
            for(int i=0;i<data.size();i+=2)result.append(QChar((quint8(data[i])<<8)|quint8(data[i+1])));
            return result;
        }
        bool finished()const{return ok && offset==bytes.size();}
    };
    static QString field(const QByteArray &bytes,const QString &name,bool &ok) {
        QXmlStreamReader xml(bytes);QString value;int found=0;
        while(!xml.atEnd()) {
            xml.readNext();
            if(xml.tokenType()==QXmlStreamReader::DTD || xml.tokenType()==QXmlStreamReader::EntityReference) {ok=false;return {};}
            if(xml.isStartElement() && xml.name()==name) {value=xml.readElementText();++found;}
        }
        ok=!xml.hasError() && found==1;return value;
    }
    static NativePresetSnapshot decode(const QByteArray &config,const QByteArray &presets) {
        NativePresetSnapshot out;bool valid=false;out.preset=field(config,"CustomKeyboardPreset",valid);
        if(!valid || out.preset.isEmpty()){out.error="active-preset-unavailable";return out;}
        const auto hex=field(presets,"PresetListBA",valid).toLatin1();QByteArray compact;
        for(auto c:hex) {
            if(c==' ' || c=='\n' || c=='\r' || c=='\t')continue;
            if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F'))){valid=false;break;}
            compact+=c;
        }
        if(!valid || compact.size()%2 || compact.size()>16*1024*1024){out.error="invalid-preset-hex";return out;}
        Reader outer{QByteArray::fromHex(compact)};
        if(outer.integer()!=1){out.error="unsupported-preset-version";return out;}
        const auto count=outer.integer();if(count>256){out.error="excessive-presets";return out;}
        QSet<QString> names;QByteArray selected;
        for(quint32 i=0;i<count && outer.ok;++i){
            const auto name=outer.string();const auto data=outer.blob();
            if(names.contains(name)){outer.ok=false;break;}names.insert(name);
            if(name==out.preset){out.present=true;selected=data;}
        }
        if(!outer.finished()){out.error="invalid-preset-container";return out;}
        if(!out.present)return out; // Built-in/unassigned preset: no retained bindings.
        Reader inner{selected};if(inner.integer()!=1){out.error="unsupported-binding-version";return out;}
        const auto records=inner.integer();if(records>100000){out.error="excessive-commands";return out;}
        QSet<QString> labels;
        for(quint32 i=0;i<records && inner.ok;++i){
            const auto label=inner.string();if(labels.contains(label)){inner.ok=false;break;}labels.insert(label);
            const auto alternates=inner.integer();if(alternates>64){inner.ok=false;break;}
            QList<QKeySequence> keys;
            for(quint32 j=0;j<alternates && inner.ok;++j){
                const auto n=inner.integer();if(n!=1 && n!=4){inner.ok=false;break;}
                int parts[4]={0,0,0,0};bool ended=false;
                for(quint32 k=0;k<n;++k){parts[k]=int(inner.integer());if(!parts[k])ended=true;else if(ended)inner.ok=false;}
                QKeySequence sequence(parts[0],parts[1],parts[2],parts[3]);
                if(!sequence.isEmpty()){
                    keys.append(sequence);out.owners[sequence.toString(QKeySequence::PortableText)].append(label);
                }
            }
            if(label.startsWith("OpenGate - "))out.commands[label]=keys;
        }
        if(inner.ok && inner.offset<inner.bytes.size())inner.string(); // Optional base-preset name.
        if(!inner.finished()){out.error="invalid-binding-records";out.commands.clear();out.owners.clear();}
        return out;
    }
    explicit NativePresetBindings(QObject *parent,const QString &directory):QObject(parent),directory(directory) {
        timer.setSingleShot(true);timer.setInterval(100);
        connect(&timer,&QTimer::timeout,this,[this]{reload();});
        connect(&watcher,&QFileSystemWatcher::fileChanged,this,[this]{invalidate();});
        connect(&watcher,&QFileSystemWatcher::directoryChanged,this,[this]{invalidate();});
        reload();
    }
    const NativePresetSnapshot &snapshot() const {return state;}
    std::function<void()> changed;
    void reload() {
        auto read=[this](QString name){QFile f(directory+'/'+name);return f.open(QIODevice::ReadOnly)&&f.size()<=16*1024*1024 ? f.readAll():QByteArray();};
        state=decode(read("config.user.xml"),read("keyboard.preset.xml"));
        QStringList paths{directory,directory+"/config.user.xml",directory+"/keyboard.preset.xml"};
        for(const auto &p:paths)if(!watcher.files().contains(p)&&!watcher.directories().contains(p))watcher.addPath(p);
        logEvent("command.preset.loaded",{{"preset",state.preset},{"present",state.present},{"error",state.error},{"commands",state.commands.size()}});
        if(changed)changed();
    }
private:
    void invalidate(){state.commands.clear();state.owners.clear();state.error="reload-pending";timer.start();}
    QString directory;NativePresetSnapshot state;QFileSystemWatcher watcher;QTimer timer;
};
}
