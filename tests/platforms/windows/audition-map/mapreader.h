// SPDX-License-Identifier: MIT
#pragma once
#include <QByteArray>
#include <QJsonObject>
#include <QVector>
#include <QString>
#include <functional>
class QObject;
class QWidget;

namespace AuditionMap {
using Reader=std::function<bool(quintptr,void *,size_t)>;
struct Entry {
    qint32 context=0;
    quintptr node=0,widgetControl=0,widget=0,associatedControl=0,associated=0;
};
struct Result {
    bool valid=false;
    QString error;
    qint32 advertisedSize=0;
    QVector<Entry> entries;
};
// Pure, bounded parser. Never dereferences an address supplied by the target.
Result readMap(quintptr data,const Reader &read);
bool supportedImage(const Reader &read,quintptr base,QString *reason);
QJsonObject snapshot(QObject *manager,QWidget *preview,QWidget *page,QWidget *slot);
}
