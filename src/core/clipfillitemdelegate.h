// SPDX-License-Identifier: MIT
#pragma once
#include <QAbstractItemDelegate>
#include <QAbstractItemView>
#include <QPersistentModelIndex>
#include <QPointer>
#include <QSet>
#include <QStyleOptionViewItem>
#include "clipfillpaintdevice.h"
#include "diagnostics.h"
namespace Unbound {
// Paint selected model indices through their existing native delegates. Forward
// sizing, editors, input, tooltips and native editor signals without replacing them.
class ClipFillItemDelegate : public QAbstractItemDelegate {
public:
    ClipFillItemDelegate(QAbstractItemView *view,QAbstractItemDelegate *rowDelegate,QObject *owner)
        :QAbstractItemDelegate(owner),view(view),rowDelegate(rowDelegate) {
        if(rowDelegate) {
            connect(rowDelegate,&QAbstractItemDelegate::commitData,this,[this,rowDelegate](QWidget *e){if(!registered(rowDelegate))emit commitData(e);});
            connect(rowDelegate,&QAbstractItemDelegate::closeEditor,this,[this,rowDelegate](QWidget *e,EndEditHint hint){if(!registered(rowDelegate))emit closeEditor(e,hint);});
            connect(rowDelegate,&QAbstractItemDelegate::sizeHintChanged,this,[this,rowDelegate](const QModelIndex &i){if(!registered(rowDelegate))emit sizeHintChanged(i);});
        }
    }
    void setColor(const QModelIndex &index,QColor color){colors[QPersistentModelIndex(index)]=color;reported.remove(index);}
    QAbstractItemDelegate *priorRowDelegate() const {return rowDelegate;}
    void paint(QPainter *p,const QStyleOptionViewItem &option,const QModelIndex &index) const override {
        auto delegate=native(index);if(!delegate)return;
        const auto color=colors.value(index);if(!color.isValid()){delegate->paint(p,option,index);return;}
        auto inspection=option;inspection.state &= ~(QStyle::State_Selected|QStyle::State_MouseOver|QStyle::State_HasFocus);
        const auto result=ClipFillPaintDevice::render(p,color,
            [&](QPainter *target){delegate->paint(target,option,index);},
            [&](QPainter *target){delegate->paint(target,inspection,index);});
        if(!reported.contains(index)){reported.insert(index);logEvent("clip-color.preview.media-pool.paint",{
            {"source_fill",result.source.color.isValid()?result.source.color.name():QString()},
            {"changed_fill_commands",result.changed},{"target",color.name()},{"temporary",true}});}
    }
    QSize sizeHint(const QStyleOptionViewItem &o,const QModelIndex &i) const override {
        auto d=native(i);return d?d->sizeHint(o,i):QSize();
    }
    QWidget *createEditor(QWidget *parent,const QStyleOptionViewItem &o,const QModelIndex &i) const override {
        auto d=native(i);auto editor=d?d->createEditor(parent,o,i):nullptr;
        if(editor)editor->installEventFilter(d);return editor;
    }
    void destroyEditor(QWidget *editor,const QModelIndex &i) const override {
        if(auto d=native(i))d->destroyEditor(editor,i);else QAbstractItemDelegate::destroyEditor(editor,i);
    }
    void setEditorData(QWidget *e,const QModelIndex &i) const override {if(auto d=native(i))d->setEditorData(e,i);}
    void setModelData(QWidget *e,QAbstractItemModel *m,const QModelIndex &i) const override {if(auto d=native(i))d->setModelData(e,m,i);}
    void updateEditorGeometry(QWidget *e,const QStyleOptionViewItem &o,const QModelIndex &i) const override {if(auto d=native(i))d->updateEditorGeometry(e,o,i);}
    bool editorEvent(QEvent *e,QAbstractItemModel *m,const QStyleOptionViewItem &o,const QModelIndex &i) override {
        auto d=native(i);return d && d->editorEvent(e,m,o,i);
    }
    bool helpEvent(QHelpEvent *e,QAbstractItemView *v,const QStyleOptionViewItem &o,const QModelIndex &i) override {
        auto d=native(i);return d && d->helpEvent(e,v,o,i);
    }
private:
    QPointer<QAbstractItemView> view;QPointer<QAbstractItemDelegate> rowDelegate;
    QHash<QPersistentModelIndex,QColor> colors;mutable QSet<QPersistentModelIndex> reported;
    bool registered(QAbstractItemDelegate *delegate) const {
        if(!view || !view->model())return false;
        if(view->itemDelegate()==delegate)return true;
        for(int c=0;c<view->model()->columnCount(view->rootIndex());++c)if(view->itemDelegateForColumn(c)==delegate)return true;
        for(int r=0;r<view->model()->rowCount(view->rootIndex());++r)if(view->itemDelegateForRow(r)==delegate)return true;
        return false;
    }
    QAbstractItemDelegate *native(const QModelIndex &index) const {
        if(!view)return nullptr;
        auto delegate=rowDelegate.data();
        if(!delegate)delegate=view->itemDelegateForColumn(index.column());
        if(!delegate)delegate=view->itemDelegate();
        if(!delegate || delegate==this)return nullptr;
        return delegate;
    }
};
}
