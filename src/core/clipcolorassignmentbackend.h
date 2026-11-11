// SPDX-License-Identifier: MIT
#pragma once
#include <QByteArray>
#include <QGraphicsObject>
#include <QObject>
#include <QString>
#include <QVector>
#include <functional>

namespace Unbound {
// Native document boundary, not another assignment store or Undo stack. A read
// copies the current native record; commit must submit one host history event.
class ClipColorAssignmentBackend : public QObject {
  public:
    using QObject::QObject;
    struct Read {
        bool available = false, present = false;
        QByteArray bytes;
        QString nativeColor;
    };
    struct Edit {
        QGraphicsObject *clip = nullptr;
        Read before;
        bool present = false;
        QByteArray bytes;
        QString nativeColor;
    };
    virtual bool isActive() const { return true; }
    virtual Read read(QGraphicsObject *clip) = 0;
    virtual bool commit(const QVector<Edit> &edits, const QString &label) = 0;
};
} // namespace Unbound
