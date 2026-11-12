// SPDX-License-Identifier: MIT
#pragma once
#include <QDialog>
#include <QString>
namespace Unbound::NativeKeyboardEditor {
inline bool available() { return false; }
inline QDialog *create(QWidget *parent, QString &error) {
    Q_UNUSED(parent)
    error = QStringLiteral("Native keyboard editor hosting is unavailable on this platform.");
    return nullptr;
}
} // namespace Unbound::NativeKeyboardEditor
