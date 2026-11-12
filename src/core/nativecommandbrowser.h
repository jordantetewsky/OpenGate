// SPDX-License-Identifier: MIT
#pragma once
#include <QAbstractItemView>
#include <QAbstractProxyModel>
#include <QComboBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QTreeView>
#include <QVBoxLayout>
#include "diagnostics.h"

namespace Unbound {
// Read-only device projection of Resolve's command model. Native storage and
// keyboard assignment columns stay owned by Resolve; selections are independent.
class DeviceCommandProjection : public QSortFilterProxyModel {
public:
    explicit DeviceCommandProjection(bool categories,QObject *owner)
        : QSortFilterProxyModel(owner),categories(categories) {
        setFilterCaseSensitivity(Qt::CaseInsensitive);
        setFilterKeyColumn(0);
        setRecursiveFilteringEnabled(true);
    }
    QVariant data(const QModelIndex &index,int role=Qt::DisplayRole) const override {
        if(index.column()>0 && (role==Qt::DisplayRole || role==Qt::EditRole || role==Qt::ToolTipRole))return {};
        return QSortFilterProxyModel::data(index,role);
    }
    QVariant headerData(int section,Qt::Orientation orientation,int role=Qt::DisplayRole) const override {
        if(orientation==Qt::Horizontal && role==Qt::DisplayRole)
            return section==0 ? QVariant(categories?"All Commands":"Command") : QVariant("Assignment");
        return QSortFilterProxyModel::headerData(section,orientation,role);
    }
    Qt::ItemFlags flags(const QModelIndex &index) const override {
        return QSortFilterProxyModel::flags(index)&~(Qt::ItemIsEditable|Qt::ItemIsDragEnabled|Qt::ItemIsDropEnabled|Qt::ItemIsUserCheckable);
    }
protected:
    bool filterAcceptsRow(int row,const QModelIndex &parent) const override {
        if(categories)return sourceModel()->hasChildren(sourceModel()->index(row,0,parent));
        return QSortFilterProxyModel::filterAcceptsRow(row,parent);
    }
private:
    bool categories;
};

// Borrows the complete native command tree through separate read-only proxies.
// No invocation, source-model mutation, synthetic command rows or preset writes.
class NativeCommandBrowser : public QWidget {
public:
    explicit NativeCommandBrowser(QWidget *owner=nullptr):QWidget(owner) {
        setObjectName("openGateDeviceCommandBrowser");
        auto layout=new QVBoxLayout(this);
        auto heading=new QHBoxLayout;
        auto title=new QLabel("Commands",this);heading->addWidget(title);heading->addStretch();
        auto filter=new QComboBox(this);filter->addItems({"Show All","Assigned","Unassigned"});
        filter->setEnabled(false);filter->setToolTip("Assignment filters will be enabled with device mapping.");
        heading->addWidget(filter);
        search=new QLineEdit(this);search->setPlaceholderText("Search");
        search->setObjectName("openGateDeviceCommandSearch");heading->addWidget(search);
        layout->addLayout(heading);
        auto splitter=new QSplitter(this);layout->addWidget(splitter,1);
        categoryView=new QTreeView(splitter);commandView=new QTreeView(splitter);
        categoryView->setObjectName("openGateDeviceCommandCategories");
        commandView->setObjectName("openGateDeviceCommands");
        categories=new DeviceCommandProjection(true,this);commands=new DeviceCommandProjection(false,this);
        categoryView->setModel(categories);commandView->setModel(commands);
        for(auto view:{categoryView,commandView}) {
            view->setEditTriggers(QAbstractItemView::NoEditTriggers);
            view->setAlternatingRowColors(true);view->setUniformRowHeights(true);
            view->setSelectionMode(QAbstractItemView::SingleSelection);
        }
        splitter->setStretchFactor(0,1);splitter->setStretchFactor(1,3);
        connect(search,&QLineEdit::textChanged,this,[this](const QString &text) {
            commandView->setRootIndex({});commands->setFilterFixedString(text);
            if(!text.isEmpty())commandView->expandAll();
        });
        connect(categoryView->selectionModel(),&QItemSelectionModel::currentChanged,this,[this](const QModelIndex &index) {
            search->clear();commandView->setRootIndex(commands->mapFromSource(categories->mapToSource(index)));
            commandView->expandToDepth(0);
        });
        unavailable=new QLabel("Open the native keyboard editor to load its command catalog.",this);
        unavailable->setWordWrap(true);unavailable->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Maximum);
        layout->addWidget(unavailable);
    }
    bool bind(QWidget *nativeEditor) {
        QAbstractItemModel *source=nullptr;
        if(nativeEditor)for(auto view:nativeEditor->findChildren<QTreeView *>()) {
            auto model=view->model();
            if(!model || model->columnCount()<2)continue;
            bool commandsWidget=false;
            for(auto parent=view->parentWidget();parent && parent!=nativeEditor;parent=parent->parentWidget())
                if(QByteArray(parent->metaObject()->className())=="UiActionsEditorCommandsWidgetImp")commandsWidget=true;
            if(!commandsWidget)continue;
            while(auto proxy=qobject_cast<QAbstractProxyModel *>(model)) {
                if(!proxy->sourceModel())break;
                model=proxy->sourceModel();
            }
            if(source && source!=model){source=nullptr;break;}
            source=model;
        }
        categories->setSourceModel(source);commands->setSourceModel(source);
        unavailable->setVisible(!source);search->setEnabled(source);
        logEvent("keyboard.device.command-model",{{"bound",bool(source)},
            {"model_class",source?QString::fromLatin1(source->metaObject()->className()):QString()},
            {"root_rows",source?source->rowCount():0},{"source_writes",false},{"complete_catalog_verified",false}});
        if(!source)return false;
        categoryView->expandToDepth(0);commandView->expandToDepth(0);
        for(int c=1;c<categories->columnCount();++c)categoryView->hideColumn(c);
        for(int c=2;c<commands->columnCount();++c)commandView->hideColumn(c);
        categoryView->header()->setSectionResizeMode(0,QHeaderView::Stretch);
        commandView->header()->setSectionResizeMode(0,QHeaderView::Stretch);
        return true;
    }
private:
    QLineEdit *search;
    QTreeView *categoryView,*commandView;
    DeviceCommandProjection *categories,*commands;
    QLabel *unavailable;
};
}
