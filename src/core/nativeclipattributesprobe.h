// SPDX-License-Identifier: MIT
#pragma once
#include <QAbstractItemView>
#include <QComboBox>
#include <QWidget>
#include <QJsonArray>
#include <QPointer>
#include <QSet>
#include <QTimer>
#include "diagnostics.h"

namespace Unbound {
// Bounded read-only native dialog schema capture. Never edits a model, opens a
// dialog, invokes an action, or treats captured state as proof of native Undo.
class NativeClipAttributesProbe : public QObject {
public:
    explicit NativeClipAttributesProbe(QObject *owner):QObject(owner) {}
    void inspectShownWidget(QWidget *widget) {
        if(!widget || captures>=16)return;
        auto dialog=widget->window();
        if(!dialog || dialog->windowTitle()!="Clip Attributes" ||
           dialog->property("unboundOwnMenu").toBool() || pending.contains(dialog))return;
        pending.insert(dialog);
        QPointer<QWidget> guard=dialog;
        QTimer::singleShot(0,this,[this,guard,dialog] {
            pending.remove(dialog);
            if(!guard || !guard->isVisible() || captures>=16)return;
            ++captures;
            QJsonArray controls;
            const auto children=guard->findChildren<QWidget *>();
            for(auto child:children.mid(0,192)) {
                QJsonObject control{{"class",child->metaObject()->className()},
                    {"name",child->objectName()},{"visible",child->isVisible()},
                    {"enabled",child->isEnabled()},
                    {"parent_class",child->parent()?child->parent()->metaObject()->className():""}};
                if(auto combo=qobject_cast<QComboBox *>(child)) {
                    QJsonArray choices;
                    for(int i=0;i<qMin(combo->count(),64);++i)choices.append(combo->itemText(i).left(128));
                    control["choices"]=choices;control["current_index"]=combo->currentIndex();
                    control["choice_count"]=combo->count();
                }
                if(auto view=qobject_cast<QAbstractItemView *>(child)) {
                    auto model=view->model();
                    if(model) {
                        const auto root=view->rootIndex();
                        QJsonArray rows,headers;
                        for(int c=0;c<qMin(model->columnCount(root),8);++c)
                            headers.append(model->headerData(c,Qt::Horizontal).toString().left(128));
                        for(int r=0;r<qMin(model->rowCount(root),16);++r) {
                            QJsonArray cells;
                            for(int c=0;c<qMin(model->columnCount(root),8);++c)
                                cells.append(model->data(model->index(r,c,root),Qt::DisplayRole).toString().left(128));
                            rows.append(cells);
                        }
                        control["headers"]=headers;control["rows"]=rows;
                        control["row_count"]=model->rowCount(root);
                        control["column_count"]=model->columnCount(root);
                    }
                }
                controls.append(control);
            }
            logEvent("audio.native-attributes.inventory",{{"dialog_class",guard->metaObject()->className()},
                {"dialog_name",guard->objectName()},{"controls",controls},
                {"controls_truncated",children.size()>192},{"read_only",true}});
        });
    }
private:
    int captures=0;
    QSet<QWidget *> pending;
};
}
