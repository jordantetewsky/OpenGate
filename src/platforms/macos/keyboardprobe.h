// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QMetaMethod>
#include <QTimer>
#include "diagnostics.h"
namespace Unbound {
// Inventory only command-related method signatures on the native keyboard dialog.
// Never reads editor text, model rows, shortcut bindings or project data.
class KeyboardProbe : public QObject {
public:
    explicit KeyboardProbe(QObject *owner) : QObject(owner) { qApp->installEventFilter(this); }
    ~KeyboardProbe() override { if (qApp) qApp->removeEventFilter(this); }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if (event->type() != QEvent::Show) return false;
        auto widget = qobject_cast<QWidget *>(object); if (!widget || !widget->isWindow()) return false;
        const auto name = widget->objectName();
        const auto type = QString::fromLatin1(widget->metaObject()->className());
        if (!(name.contains("keyboard",Qt::CaseInsensitive) || type.contains("keyboard",Qt::CaseInsensitive) ||
            name.contains("shortcut",Qt::CaseInsensitive) || type.contains("shortcut",Qt::CaseInsensitive) ||
            type.startsWith("UiActionsEditorDialog"))) return false;
        if (name.contains("FirstCut") || type.contains("FirstCut")) return false;
        QJsonArray methods, interfaces;
        for (auto child : widget->findChildren<QObject *>()) {
            const auto childType = QString::fromLatin1(child->metaObject()->className());
            if (!childType.startsWith("UiActionsEditor") && childType != "UiActionManager") continue;
            QJsonArray signatures;
            for (int i=child->metaObject()->methodOffset(); i<child->metaObject()->methodCount(); ++i)
                signatures.append(QString::fromLatin1(child->metaObject()->method(i).methodSignature()));
            interfaces.append(QJsonObject{{"class",childType},{"methods",signatures}});
        }
        for (int i=0; i<widget->metaObject()->methodCount(); ++i) {
            const auto method = widget->metaObject()->method(i); const auto signature = method.methodSignature();
            const auto lower = signature.toLower();
            if (lower.contains("command") || lower.contains("shortcut") || lower.contains("keyboard") || lower.contains("preset"))
                methods.append(QString::fromLatin1(signature));
        }
        logEvent("capability.keyboard.interface",{{"object",name},{"class",type},
            {"methods",methods},{"interfaces",interfaces},{"native_registration_verified",false},{"read_only",true}});
        return false;
    }
};
}
