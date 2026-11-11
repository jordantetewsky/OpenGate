// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonObject>
#include <QPointer>
#include <QTabWidget>
#include <functional>
namespace Unbound {
// Binds an opted-in panel's tab identity to the existing session snapshot.
// There is no separate settings file or persistence writer.
class PanelTabSelection {
public:
    ~PanelTabSelection(){release();}
    std::function<void()> changed;
    void attach(QWidget *surface) {
        auto candidate=qobject_cast<QTabWidget *>(surface);
        if(!candidate || !candidate->property("openGatePersistSelectedTab").toBool())candidate=nullptr;
        if(candidate==tabs)return;
        release();tabs=candidate;
        if(!tabs)return;
        apply();
        connection=QObject::connect(tabs,&QTabWidget::currentChanged,tabs,[this] {
            selected=current();if(changed)changed();
        });
    }
    QJsonObject snapshot() const {
        const auto id=tabs?current():selected;
        return id.isEmpty()?QJsonObject{}:QJsonObject{{"selected_tab",id}};
    }
    void restore(const QJsonObject &value) {selected=value["selected_tab"].toString();apply();}
    void release() {
        if(tabs)selected=current();
        QObject::disconnect(connection);connection={};tabs=nullptr;
    }
private:
    QString current() const {return tabs && tabs->currentWidget()?tabs->currentWidget()->objectName():QString();}
    void apply() {
        if(!tabs)return;
        for(int i=0;i<tabs->count();++i)if(tabs->widget(i)->objectName()==selected){tabs->setCurrentIndex(i);return;}
        if(selected.isEmpty() && tabs->count())tabs->setCurrentIndex(0);
    }
    QPointer<QTabWidget> tabs;
    QMetaObject::Connection connection;
    QString selected;
};
}
