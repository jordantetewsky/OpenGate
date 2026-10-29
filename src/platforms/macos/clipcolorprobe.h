// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QMenu>
#include <QMetaProperty>
#include <QMetaMethod>
#include "diagnostics.h"
namespace Unbound {
// Read-only inventory of the known native clip palette. No project mutations.
class ClipColorProbe : public QObject {
public:
    explicit ClipColorProbe(QObject *owner) : QObject(owner) { qApp->installEventFilter(this); }
    ~ClipColorProbe() override { if (qApp) qApp->removeEventFilter(this); }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if (event->type() != QEvent::Show) return false;
        if (auto widget = qobject_cast<QWidget *>(object)) {
            if (QString::fromLatin1(widget->metaObject()->className()) == "UiColorClipsWidget") {
                QJsonArray signatures;
                for (int i=widget->metaObject()->methodOffset(); i<widget->metaObject()->methodCount(); ++i)
                    signatures.append(QString::fromLatin1(widget->metaObject()->method(i).methodSignature()));
                logEvent("capability.clip-color.interface",{{"class","UiColorClipsWidget"},
                    {"methods",signatures},{"read_only",true}});
            }
        }
        auto menu = qobject_cast<QMenu *>(object); if (!menu) return false;
        if (menu->objectName() != "markSetClipColor") {
            bool apricot=false,chocolate=false;
            for (auto action : menu->actions()) {
                auto label=action->text(); label.remove('&'); label=label.trimmed();
                apricot |= label == "Apricot"; chocolate |= label == "Chocolate";
            }
            if (!apricot || !chocolate) return false;
        }
        QJsonArray actions;
        const QStringList colors{"Orange","Apricot","Yellow","Lime","Olive","Green","Teal","Navy","Blue","Purple","Violet","Pink","Tan","Beige","Brown","Chocolate","Red","Clear Color"};
        for (auto action : menu->actions()) {
            auto label = action->text(); label.remove('&'); label = label.trimmed();
            QJsonArray properties;
            for (const auto &name : action->dynamicPropertyNames()) {
                const auto value = action->property(name.constData());
                properties.append(QJsonObject{{"name",QString::fromLatin1(name)},
                    {"type",QString::fromLatin1(value.typeName() ? value.typeName() : "invalid")}});
            }
            const auto data = action->data();
            QJsonObject item{{"object",action->objectName()},{"class",action->metaObject()->className()},
                {"label",colors.contains(label) ? label : QStringLiteral("<unrecognized>")},
                {"enabled",action->isEnabled()},{"checked",action->isChecked()},
                {"data_type",QString::fromLatin1(data.typeName() ? data.typeName() : "invalid")},
                {"dynamic_properties",properties}};
            if (data.type() == QVariant::Int || data.type() == QVariant::UInt) item["numeric_data"] = data.toInt();
            actions.append(item);
        }
        logEvent("capability.clip-color.palette",{{"menu",menu->objectName()},{"actions",actions},
            {"read_only",true},{"custom_storage_verified",false}});
        return false;
    }
};
}
