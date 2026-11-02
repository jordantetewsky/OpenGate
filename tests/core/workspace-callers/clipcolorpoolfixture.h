// SPDX-License-Identifier: MIT
#pragma once
#include <QTreeView>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QSplitter>
#include <QVBoxLayout>
namespace OpenGateTest {
class NativeClipDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    mutable int commits=0;
    QSize sizeHint(const QStyleOptionViewItem &,const QModelIndex &) const override {return {240,70};}
    void paint(QPainter *p,const QStyleOptionViewItem &o,const QModelIndex &i) const override {
        const QColor color=i.data(Qt::UserRole).value<QColor>();
        p->fillRect(o.rect,o.state.testFlag(QStyle::State_Selected)?QColor(80,80,150):QColor(30,30,30));
        p->fillRect(o.rect.adjusted(4,4,-4,-4),color);
        QImage thumbnail(20,20,QImage::Format_RGB32);thumbnail.fill(color);p->drawImage(o.rect.topLeft()+QPoint(10,8),thumbnail);
        p->fillRect(QRect(o.rect.topLeft()+QPoint(8,38),QSize(160,22)),Qt::black);
        p->setPen(Qt::white);p->drawText(o.rect.topLeft()+QPoint(12,54),i.data().toString());
    }
    void setModelData(QWidget *e,QAbstractItemModel *m,const QModelIndex &i) const override {++commits;QStyledItemDelegate::setModelData(e,m,i);}
};
struct ClipColorPoolFixture {
    QWidget root;QVBoxLayout layout;QSplitter splitter;QTreeView view;QStandardItemModel model;
    NativeClipDelegate delegate;
    explicit ClipColorPoolFixture(QWidget *owner=nullptr):root(owner),layout(&root),splitter(&root),view(&splitter),delegate(&view) {
        root.setObjectName("frameEditPageMediaPool");root.setAttribute(Qt::WA_DontShowOnScreen);
        layout.setContentsMargins(0,0,0,0);layout.addWidget(&splitter);splitter.setObjectName("mediaPoolViewSplitter");
        view.setObjectName("mediaPoolClipView");view.setRootIsDecorated(false);view.setHeaderHidden(true);
        model.appendRow(new QStandardItem("Clip one"));model.appendRow(new QStandardItem("Clip two"));
        for(int row=0;row<2;++row)model.setData(model.index(row,0),QColor(30,130,180),Qt::UserRole);
        view.setModel(&model);view.setItemDelegate(&delegate);root.resize(350,220);root.show();
        view.setSelectionMode(QAbstractItemView::ExtendedSelection);view.setSelectionBehavior(QAbstractItemView::SelectRows);
        view.setCurrentIndex(model.index(0,0));
    }
};
}
