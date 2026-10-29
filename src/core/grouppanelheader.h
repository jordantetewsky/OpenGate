// SPDX-License-Identifier: MIT
#pragma once
#include <QLabel>
#include <QTabWidget>
#include <QStackedWidget>
#include "nativepaneldecoration.h"
#include "panelscalecontrols.h"
#include "nativepanelheaderlease.h"
namespace Unbound {
// One name row when untabbed; move only controls into the selected tab strip.
class GroupPanelHeader : public QObject {
public:
    GroupPanelHeader(QWidget *wrapper,const QString &title,std::function<double()> scale,std::function<void(double)> setScale,
        std::function<void()> detach,std::function<void()> hide,std::function<void(QMenu *)> extras):QObject(wrapper),detachAction(std::move(detach)),hideAction(std::move(hide)),extrasAction(std::move(extras)) {
        bar=new QWidget(wrapper);bar->setObjectName("openGateGroupHeaderBar");auto row=new QHBoxLayout(bar);row->setContentsMargins(6,1,4,1);
        label=new QLabel(title,bar);label->setObjectName("unboundGroupHeader");label->setMinimumWidth(0);label->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);
        label->setToolTip(title+"; drag to move or detach");row->addWidget(label,1);
        controls=new QWidget(bar);controls->setProperty("openGateChrome",true);auto tools=new QHBoxLayout(controls);tools->setContentsMargins(0,0,0,0);tools->setSpacing(2);
        tools->addWidget(new PanelScaleControls(controls,std::move(scale),std::move(setScale)));
        button=new NativeUndockButton(controls);button->setObjectName("unboundDetachPanel");button->setAutoRaise(true);button->setFocusPolicy(Qt::NoFocus);
        setPlacementControl(button,false,"Float "+title+"; drag to place");tools->addWidget(button);row->addWidget(controls);
        connect(button,&QToolButton::clicked,this,[this] { QTimer::singleShot(0,this,[this] { if (detachAction) detachAction(); }); });
        menu=new QMenu(button);menu->setProperty("unboundOwnMenu",true);button->setMenu(menu);button->setPopupMode(QToolButton::MenuButtonPopup);
        connect(menu,&QMenu::aboutToShow,this,[this] {
            menu->clear();auto action=menu->addAction("Hide Panel");action->setObjectName("openGateHidePanel");
            connect(action,&QAction::triggered,this,[this] { QTimer::singleShot(0,this,[this] { if (hideAction) hideAction(); }); });
            if (extrasAction) extrasAction(menu);
        });
        for (auto target:{bar.data(),static_cast<QWidget *>(label.data())}) {
            target->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(target,&QWidget::customContextMenuRequested,this,[this,target](QPoint point) { menu->popup(target->mapToGlobal(point)); });
        }
    }
    ~GroupPanelHeader() override { nativeHeader.restore();park(); }
    void borrowNativeHeader(QWidget *surface,const QString &id){nativeHeader.acquire(surface,id);}
    void releaseNativeHeader(){nativeHeader.restore();}
    void presentFor(QWidget *cell,bool toolbar=false) {
        // Only a direct tab page trades its header for the tab's title. A
        // split section inside a tab still contains independently named panels.
        auto stack=cell?qobject_cast<QStackedWidget *>(cell->parentWidget()):nullptr;
        auto owner=stack?qobject_cast<QTabWidget *>(stack->parentWidget()):nullptr;
        present(owner,owner&&owner->currentWidget()==cell,toolbar);
    }
    void park() {
        if (!controls || !bar) return;
        if (tabs && tabs->cornerWidget(Qt::TopRightCorner)==controls) tabs->setCornerWidget(nullptr,Qt::TopRightCorner);
        tabs=nullptr;if (controls->parentWidget()!=bar) { controls->setParent(bar);bar->layout()->addWidget(controls); }controls->show();
    }
    void present(QTabWidget *owner,bool selected,bool toolbar=false) {
        if (!bar || !controls) return;
        bar->setVisible(!toolbar && !owner);
        if (owner && selected && !toolbar) { tabs=owner;owner->setCornerWidget(controls,Qt::TopRightCorner);controls->show(); }
    }
    std::function<void()> detachAction,hideAction;std::function<void(QMenu *)> extrasAction;
    QPointer<QWidget> bar,controls;QPointer<QLabel> label;QPointer<NativeUndockButton> button;QPointer<QMenu> menu;
private:QPointer<QTabWidget> tabs;NativePanelHeaderLease nativeHeader;
};
}
