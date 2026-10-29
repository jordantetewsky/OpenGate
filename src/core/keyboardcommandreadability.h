// SPDX-License-Identifier: MIT
#pragma once
#include <QHeaderView>
#include <QHelpEvent>
#include <QPointer>
#include <QTimer>
#include <QToolTip>
#include <QTreeView>
#include <QStyledItemDelegate>
#include <QTextLayout>
#include <QtMath>
#include "diagnostics.h"
namespace Unbound {
class WrappedKeyboardCommandDelegate : public QStyledItemDelegate {
public:
    WrappedKeyboardCommandDelegate(QTreeView *tree,QAbstractItemDelegate *native,QObject *owner)
        :QStyledItemDelegate(owner),tree(tree),native(native) {
        connect(tree->viewport(),&QObject::destroyed,this,[this]{this->tree=nullptr;});
        // The default delegate remains connected to the view for other columns.
        // A replaced column-only delegate needs its editor signals forwarded.
        if(native!=tree->itemDelegate()) {
            connect(native,&QAbstractItemDelegate::commitData,this,[this](QWidget *editor){emit commitData(editor);});
            connect(native,&QAbstractItemDelegate::closeEditor,this,[this](QWidget *editor,EndEditHint hint){emit closeEditor(editor,hint);});
            connect(native,&QAbstractItemDelegate::sizeHintChanged,this,[this](const QModelIndex &index){emit sizeHintChanged(index);});
        }
    }
    void paint(QPainter *painter,const QStyleOptionViewItem &option,const QModelIndex &index) const override {
        auto wrapped=option;wrapped.features|=QStyleOptionViewItem::WrapText;
        if(native)native->paint(painter,wrapped,index);
    }
    QSize sizeHint(const QStyleOptionViewItem &option,const QModelIndex &index) const override {
        auto hint=native?native->sizeHint(option,index):QStyledItemDelegate::sizeHint(option,index);
        if(!tree)return hint;
        int depth=tree->rootIsDecorated()?1:0;
        for(auto parent=index.parent();parent.isValid()&&parent!=tree->rootIndex();parent=parent.parent())++depth;
        const int icon=index.data(Qt::DecorationRole).isValid()?tree->iconSize().width()+8:0;
        const int width=qMax(24,tree->columnWidth(index.column())-depth*tree->indentation()-icon-12);
        auto font=option.font;const auto fontRole=index.data(Qt::FontRole);
        if(fontRole.canConvert<QFont>())font=fontRole.value<QFont>();
        QTextLayout layout(index.data(Qt::DisplayRole).toString(),font);
        QTextOption textOption;textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);layout.setTextOption(textOption);
        qreal height=0;layout.beginLayout();
        while(true){auto line=layout.createLine();if(!line.isValid())break;line.setLineWidth(width);height+=line.height();}
        layout.endLayout();hint.setHeight(qMax(hint.height(),qCeil(height)+8));return hint;
    }
    QWidget *createEditor(QWidget *parent,const QStyleOptionViewItem &option,const QModelIndex &index) const override {return native?native->createEditor(parent,option,index):nullptr;}
    void destroyEditor(QWidget *editor,const QModelIndex &index) const override {if(native)native->destroyEditor(editor,index);}
    void setEditorData(QWidget *editor,const QModelIndex &index) const override {if(native)native->setEditorData(editor,index);}
    void setModelData(QWidget *editor,QAbstractItemModel *model,const QModelIndex &index) const override {if(native)native->setModelData(editor,model,index);}
    void updateEditorGeometry(QWidget *editor,const QStyleOptionViewItem &option,const QModelIndex &index) const override {if(native)native->updateEditorGeometry(editor,option,index);}
    bool editorEvent(QEvent *event,QAbstractItemModel *model,const QStyleOptionViewItem &option,const QModelIndex &index) override {return native&&native->editorEvent(event,model,option,index);}
    bool helpEvent(QHelpEvent *event,QAbstractItemView *view,const QStyleOptionViewItem &option,const QModelIndex &index) override {return native&&native->helpEvent(event,view,option,index);}
private:QPointer<QTreeView> tree;QPointer<QAbstractItemDelegate> native;
};
// Presentation only: preserve the native model, command labels, delegate,
// selection, shortcut editor and column widths. Wrapped command rows grow vertically.
class KeyboardCommandReadability : public QObject {
public:
    explicit KeyboardCommandReadability(QTreeView *tree):QObject(tree),tree(tree),
        originalElide(tree->textElideMode()),originalWrap(tree->wordWrap()),
        originalUniform(tree->uniformRowHeights()) {
        setObjectName("openGateKeyboardCommandReadability");
        // QWidget deletes its viewport/header before QObject clears the view's
        // QPointer. Do not restore presentation against half-destroyed internals.
        connect(tree->viewport(),&QObject::destroyed,this,[this]{this->tree=nullptr;timer.stop();});
        connect(tree->header(),&QObject::destroyed,this,[this]{this->tree=nullptr;timer.stop();});
        originalColumnDelegate=tree->itemDelegateForColumn(0);
        auto native=originalColumnDelegate?originalColumnDelegate.data():tree->itemDelegate();
        wrappingDelegate=new WrappedKeyboardCommandDelegate(tree,native,this);tree->setItemDelegateForColumn(0,wrappingDelegate);
        tree->setTextElideMode(Qt::ElideNone);tree->setWordWrap(true);tree->setUniformRowHeights(false);
        timer.setSingleShot(true);timer.setInterval(0);
        connect(&timer,&QTimer::timeout,this,[this] {fit();});
        connect(tree->header(),&QHeaderView::sectionResized,this,[this] {queue();});
        tree->installEventFilter(this);tree->viewport()->installEventFilter(this);queue();
    }
    ~KeyboardCommandReadability() override {
        if(!tree)return;
        tree->removeEventFilter(this);tree->viewport()->removeEventFilter(this);
        if(tree->itemDelegateForColumn(0)==wrappingDelegate)tree->setItemDelegateForColumn(0,originalColumnDelegate);
        tree->setTextElideMode(originalElide);tree->setWordWrap(originalWrap);tree->setUniformRowHeights(originalUniform);
        tree->doItemsLayout();
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if(tree && object==tree->viewport() && event->type()==QEvent::ToolTip) {
            auto help=static_cast<QHelpEvent *>(event);auto index=tree->indexAt(help->pos());
            if(index.isValid() && index.column()==0) {
                const auto text=index.data(Qt::DisplayRole).toString();
                if(!text.isEmpty()){QToolTip::showText(help->globalPos(),text,tree->viewport());return true;}
            }
        }
        if(event->type()==QEvent::Show || event->type()==QEvent::Resize || event->type()==QEvent::FontChange ||
           event->type()==QEvent::StyleChange || event->type()==QEvent::LayoutRequest)queue();
        return false;
    }
private:
    QPointer<QTreeView> tree;QPointer<QAbstractItemModel> observed;QTimer timer;
    QPointer<QAbstractItemDelegate> originalColumnDelegate;WrappedKeyboardCommandDelegate *wrappingDelegate=nullptr;
    Qt::TextElideMode originalElide;bool originalWrap,originalUniform;
    int revision=0,lastRevision=-1,lastWidth=-1;QString lastFont;
    void queue(){if(!timer.isActive())timer.start();}
    void changed(){++revision;queue();}
    void fit(){
        if(!tree || !tree->model() || tree->model()->columnCount()<2)return;
        auto model=tree->model();
        if(observed!=model) {
            if(observed)disconnect(observed,nullptr,this,nullptr);observed=model;++revision;
            connect(model,&QAbstractItemModel::modelReset,this,[this]{changed();});
            connect(model,&QAbstractItemModel::rowsInserted,this,[this]{changed();});
            connect(model,&QAbstractItemModel::rowsRemoved,this,[this]{changed();});
            connect(model,&QAbstractItemModel::layoutChanged,this,[this]{changed();});
            connect(model,&QAbstractItemModel::dataChanged,this,[this](const QModelIndex &,const QModelIndex &,const QVector<int> &roles){
                if(roles.isEmpty() || roles.contains(Qt::DisplayRole) || roles.contains(Qt::FontRole))changed();
            });
        }
        const int width=tree->columnWidth(0);const auto font=tree->font().key();
        if(lastRevision==revision && lastWidth==width && lastFont==font)return;
        lastRevision=revision;lastWidth=width;lastFont=font;
        // The existing native delegate computes wrapping and row height; do not
        // replace its shortcut/conflict controls or write size hints to the model.
        tree->doItemsLayout();
    }

};
inline bool wireKeyboardCommandReadability(QWidget *commands){
    if(!commands)return false;QList<QTreeView *> matches;
    for(auto view:commands->findChildren<QTreeView *>())
        if(view->model() && view->model()->columnCount()>=2)matches.append(view);
    if(matches.size()!=1)return false;
    auto view=matches.first();
    if(!view->findChild<QObject *>("openGateKeyboardCommandReadability",Qt::FindDirectChildrenOnly))
        new KeyboardCommandReadability(view);
    return true;
}
}
