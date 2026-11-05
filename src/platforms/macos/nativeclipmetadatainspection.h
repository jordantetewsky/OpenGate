// SPDX-License-Identifier: MIT
#pragma once
#include <QCryptographicHash>
#include <QApplication>
#include <QJsonObject>
#include <QMetaProperty>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QThread>
#include <QVariant>

namespace Unbound {
// Public Qt metadata reads, restricted to an explicit disposable SDK-ID allowlist.
inline QJsonObject inspectDisposableClipMetadata(QObject *model, const QString &id) {
    QJsonObject result{{"read_only", true}, {"getter_refreshes_native_cache", true}};
    if (qEnvironmentVariableIntValue("OPENGATE_CLIP_IDENTITY_READ_METADATA") != 1) {
        result["status"] = "disabled";
        return result;
    }
    const auto ids = qEnvironmentVariable("OPENGATE_CLIP_IDENTITY_DISPOSABLE_IDS")
                         .split(',', Qt::SkipEmptyParts);
    if (ids.isEmpty() || ids.size() > 8 || !ids.contains(id)) {
        result["status"] = "requires-disposable-sdk-id-allowlist";
        return result;
    }
    if (!qApp || QThread::currentThread() != qApp->thread()) {
        result["status"] = "requires-gui-thread";
        return result;
    }
    QPointer<QObject> alive = model;
    if (!alive || !model->inherits("Sm2TiItem")) {
        result["status"] = "unsupported-model-qobject";
        return result;
    }
    const auto meta = model->metaObject();
    const int index = meta->indexOfProperty("ImportExportMetadataBA");
    if (index < 0) {
        result["status"] = "metadata-property-absent";
        return result;
    }
    const auto property = meta->property(index);
    if (!property.isReadable() || property.userType() != QMetaType::QByteArray) {
        result["status"] = "unsupported-metadata-property-type";
        return result;
    }
    const QVariant value = property.read(model);
    if (!alive || value.userType() != QMetaType::QByteArray) {
        result["status"] = "metadata-read-failed";
        return result;
    }
    const auto bytes = value.toByteArray();
    result["status"] = "disposable-metadata-read";
    result["bytes"] = bytes.size();
    result["sha256"] = QString::fromLatin1(
        QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
    // No native map contents, source paths or model pointers enter the receipt.
    return result;
}
} // namespace Unbound
