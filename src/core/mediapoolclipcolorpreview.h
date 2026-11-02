// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QAbstractItemView>
#include "clipfillitemdelegate.h"
#include "panelancestry.h"
namespace Unbound {
// Own reversible per-row delegate leases for Media Pool clip views. The model,
// native delegates and persistent index identity stay owned by Resolve.
class MediaPoolClipColorPreview : public QObject {
public:
    explicit MediaPoolClipColorPreview(QObject *owner):QObject(owner){}
    ~MediaPoolClipColorPreview() override {clear(nullptr);}
    bool hasSelection(QWidget *root) const {
        for(auto view:views(root))if(view->selectionModel() && !view->selectionModel()->selectedIndexes().isEmpty())return true;
        return false;
    }
    int applySelected(QWidget *root,QColor color) {
        if(!root || !color.isValid())return 0;
        int count=0;
        for(auto view:views(root))if(view->selectionModel())for(const auto &index:view->selectionModel()->selectedIndexes()) {
            if(!index.isValid())continue;
            ClipFillItemDelegate *delegate=nullptr;
            for(const auto &lease:leases)if(lease.view==view && lease.row==index.row() && lease.delegate &&
                view->itemDelegateForRow(index.row())==lease.delegate)delegate=lease.delegate;
            if(!delegate){delegate=new ClipFillItemDelegate(view,view->itemDelegateForRow(index.row()),this);
                view->setItemDelegateForRow(index.row(),delegate);leases.append({view,index.row(),delegate});}
            delegate->setColor(index,color);view->viewport()->update();++count;
        }
        logEvent("clip-color.preview.media-pool.requested",{{"indices",count},{"color",color.name()},{"project_writes",false}});
        return count;
    }
    void clear(QWidget *root) {
        for(int i=leases.size()-1;i>=0;--i){const auto lease=leases[i];
            if(root && lease.view && !panelContains(root,lease.view))continue;
            if(lease.view && lease.delegate && lease.view->itemDelegateForRow(lease.row)==lease.delegate) {
                lease.view->setItemDelegateForRow(lease.row,lease.delegate->priorRowDelegate());lease.view->viewport()->update();}
            if(lease.delegate)delete lease.delegate;leases.removeAt(i);
        }
    }
private:
    struct Lease {QPointer<QAbstractItemView> view;int row;QPointer<ClipFillItemDelegate> delegate;};
    QVector<Lease> leases;
    static QList<QAbstractItemView *> views(QWidget *root) {
        QList<QAbstractItemView *> result;if(!root)return result;
        for(auto widget:QApplication::allWidgets())if(auto view=qobject_cast<QAbstractItemView *>(widget)) {
            if(!view->isVisible() || !panelContains(root,view))continue;
            bool clips=false;
            for(auto p=widget;p && p!=root;p=panelParent(p))if(p->objectName()=="mediaPoolClipView")clips=true;
            if(clips)result.append(view);
        }
        return result;
    }
};
}
