// SPDX-License-Identifier: MIT
#pragma once
#include <QDataStream>
#include <QKeySequence>

inline QByteArray nativePresetFixture(const QList<QPair<QString,QList<QKeySequence>>> &rows,const QString &name="Active") {
    QByteArray inner;QDataStream i(&inner,QIODevice::WriteOnly);i.setVersion(QDataStream::Qt_5_15);
    i<<quint32(1)<<quint32(rows.size());for(const auto &row:rows){i<<row.first<<quint32(row.second.size());for(const auto &key:row.second)i<<key;}i<<QString("DaVinci Resolve");
    QByteArray outer;QDataStream o(&outer,QIODevice::WriteOnly);o<<quint32(1)<<quint32(1)<<name<<inner;
    return "<SmKeyboardPresetList><PresetListBA>"+outer.toHex()+"</PresetListBA></SmKeyboardPresetList>";
}
