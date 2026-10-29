// SPDX-License-Identifier: MIT
#pragma once
#include "../platform/workspaceuiconventions.h"
#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QToolButton>
#include "arrangementcontrols.h"
#include "controlscroll.h"
#include "productidentity.h"

namespace Unbound {
// Widget construction and labels only. No panel sessions, native transfers or
// persistence writes. QWidget parentage owns the controls; the coordinator wires
// their actions to the appropriate transition owners.
class PanelManagerView : public QObject {
public:
    struct PanelRow { QPushButton *undock,*dock,*show,*hide,*expand,*center; };
    PanelManagerView(QWidget *owner,const QJsonObject &saved) : QObject(owner),owner(owner)
    {
        auto layout=new QVBoxLayout(owner);
        status=new QLabel("Open a project on the Edit page. Source Monitor can float with no clip loaded.",owner);
        status->setWordWrap(true); layout->addWidget(status);
        presets=new ArrangementControls(owner,layout,saved["selected_arrangement"].toString());
        scroll=new ControlScroll(owner); scroll->setObjectName("panelOptionsScroll"); scroll->setWidgetResizable(true);
        body=new QWidget(scroll); bodyLayout=new QVBoxLayout(body); auto options=new QVBoxLayout;
        pin=new QCheckBox("Keep controls on top",owner); pin->setObjectName("pinControls"); pin->setChecked(saved["pin_controls"].toBool());
        constrain=new QCheckBox("Keep a window grip reachable",owner); constrain->setChecked(saved["keep_on_screen"].toBool(true));
        options->addWidget(pin); options->addWidget(constrain); bodyLayout->addLayout(options);
        snap=new QCheckBox(nativeWorkspaceSnapLabel(),owner); snap->setObjectName("snapWindows");
        snap->setChecked(saved["snap_windows"].toBool(true)); options->addWidget(snap);
        // Ordinary arrangements use automatic shell visibility. Historical
        // hide_background/native_workspace_mode values describe the old flow;
        // only an explicit current policy restores a manual override.
        auto advanced=new QToolButton(owner);advanced->setText("Advanced");advanced->setCheckable(true);
        advanced->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);advanced->setArrowType(Qt::RightArrow);
        options->addWidget(advanced);
        auto advancedBody=new QWidget(owner);auto advancedLayout=new QVBoxLayout(advancedBody);
        advancedLayout->setContentsMargins(0,0,0,0);
        background=new QCheckBox("Hide native shell (manual override)",advancedBody);background->setObjectName("hideResolveBackground");
        background->setToolTip("Normally OpenGate handles the native background automatically. Use this override only for a fully floating workspace.");
        background->setChecked(saved["background_policy"]=="manual-hidden");advancedLayout->addWidget(background);
        options->addWidget(advancedBody);advancedBody->hide();
        // Retain recovery state and wiring, but do not expose the old shell
        // switch as part of the Native Edit flow.
        background->hide();advanced->hide();
        connect(advanced,&QToolButton::toggled,advancedBody,[advanced,advancedBody](bool expanded) {
            advanced->setArrowType(expanded?Qt::DownArrow:Qt::RightArrow);advancedBody->setVisible(expanded);
        });
        hierarchy=new QCheckBox("Use panel hierarchy",owner); hierarchy->setObjectName("panelHierarchy");
        hierarchy->setChecked(saved["panel_hierarchy"].toBool(true)); bodyLayout->addWidget(hierarchy);
        auto styleRow=new QHBoxLayout; styleRow->addWidget(new QLabel("Panel title bars",owner));
        style=new QComboBox(owner); style->setObjectName("windowChrome");
        style->addItems({"Standard","Compact traffic lights","Hidden — drag grip and resize edges"});
        style->setCurrentIndex(qBound(0,saved["window_chrome"].toInt(),2)); styleRow->addWidget(style); bodyLayout->addLayout(styleRow);
        auto bulk=new QGridLayout;
        const QStringList actions{"Undock All","Dock All","Show All","Hide All","Recover Windows","Quick Export…"};
        for (int i=0;i<actions.size();++i) {
            auto button=new QPushButton(actions[i],owner); button->setObjectName(QStringLiteral("bulk_%1").arg(i));
            bulk->addWidget(button,i/2,i%2); bulkButtons.append(button);
        }
        layout->addLayout(bulk); layout->addWidget(scroll,1); scroll->setWidget(body);
        owner->setMinimumSize(280,200);
    }
    void addGroups(const QStringList &labels)
    {
        auto box=new QGroupBox("Join panels or their groups",body); auto layout=new QGridLayout(box);
        first=new QComboBox(box); second=new QComboBox(box); first->setObjectName("groupFirst"); second->setObjectName("groupSecond");
        first->addItems(labels); second->addItems(labels); first->setCurrentIndex(2); second->setCurrentIndex(3);
        layout->addWidget(first,0,0); layout->addWidget(second,0,1);
        const QStringList names{"Side by Side","Stack","Release Groups","Tabs"};
        for (int i=0;i<names.size();++i) {
            auto button=new QPushButton(names[i],box); button->setObjectName(QStringLiteral("group_%1").arg(i));
            layout->addWidget(button,i>=2 ? 2 : 1,i>=2 ? i-2 : i); groupButtons.append(button);
        }
        bodyLayout->addWidget(box); panelGrid=new QGridLayout; bodyLayout->addLayout(panelGrid);
    }
    PanelRow addPanel(int index,const QString &name,const QString &label)
    {
        PanelRow row{new QPushButton("Undock",owner),new QPushButton("Dock",owner),new QPushButton("Show",owner),
            new QPushButton("Hide",owner),new QPushButton("Expand",owner),new QPushButton("Center",owner)};
        const QList<QPushButton *> buttons{row.undock,row.dock,row.show,row.hide,row.expand,row.center};
        const QStringList ids{"undock_","dock_","show_","hide_","expand_","center_"};
        panelGrid->addWidget(new QLabel(label,owner),index,0);
        for (int i=0;i<buttons.size();++i) { buttons[i]->setObjectName(ids[i]+name); panelGrid->addWidget(buttons[i],index,i+1); }
        return row;
    }
    void addScale(int row,QWidget *control) { panelGrid->addWidget(control,row,7); }
    void addFooter()
    {
        const int row=panelGrid->rowCount(); panelGrid->addWidget(new QLabel("Page Navigation",body),row,0);
        showFooter=new QPushButton("Show",body);showFooter->setObjectName("showPageNavigation");panelGrid->addWidget(showFooter,row,1);
        hideFooter=new QPushButton("Hide",body);hideFooter->setObjectName("hidePageNavigation");panelGrid->addWidget(hideFooter,row,2);
        pinFooter=new QPushButton("Pin to Main Surface",body);pinFooter->setObjectName("pinPageNavigation");pinFooter->setCheckable(true);pinFooter->setToolTip("Reserve or remove the Page Navigation band on the main surface");pinFooter->hide(); // Compatibility object only; Footer uses Show/Hide, never optional pinning.
        auto credit=new QLabel(productCredit(),body); credit->setWordWrap(true); credit->setObjectName("openGateFoundationCredit");
        bodyLayout->addWidget(credit); bodyLayout->addStretch();
    }
    QLabel *status=nullptr;
    ArrangementControls *presets=nullptr;
    ControlScroll *scroll=nullptr;
    QCheckBox *pin=nullptr,*constrain=nullptr,*snap=nullptr,*background=nullptr,*hierarchy=nullptr;
    QComboBox *style=nullptr,*first=nullptr,*second=nullptr;
    QList<QPushButton *> bulkButtons,groupButtons;
    QPushButton *pinFooter=nullptr,*showFooter=nullptr,*hideFooter=nullptr;
private:
    QWidget *owner,*body=nullptr;
    QVBoxLayout *bodyLayout=nullptr;
    QGridLayout *panelGrid=nullptr;
};
}
