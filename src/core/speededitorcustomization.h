// SPDX-License-Identifier: MIT
#pragma once
#include "speededitordiagram.h"
#include "nativecommandbrowser.h"
#include <QComboBox>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>

namespace Unbound {
// Device-tab presentation and selection. This preview deliberately has no input
// dispatcher or profile writer; it cannot change hardware or keyboard mappings.
class SpeedEditorCustomization : public QWidget {
public:
    explicit SpeedEditorCustomization(QWidget *owner=nullptr):QWidget(owner) {
        setObjectName("openGateSpeedEditorCustomization");
        auto layout=new QVBoxLayout(this);layout->setContentsMargins(20,12,20,16);
        auto header=new QHBoxLayout;
        auto title=new QLabel("Speed Editor Customization",this);header->addWidget(title);header->addStretch();
        profiles=new QComboBox(this);profiles->setObjectName("openGateSpeedEditorProfiles");
        profiles->addItem("Native Speed Editor","native");
        profiles->setMinimumContentsLength(28);header->addWidget(profiles);
        auto options=new QToolButton(this);options->setText(QString::fromUtf8("•••"));
        options->setPopupMode(QToolButton::InstantPopup);auto menu=new QMenu(options);
        for(const auto label:{"Save as New Preset…","Import Preset…","Export Preset","Delete Preset"}) {
            auto action=menu->addAction(QString::fromUtf8(label));action->setEnabled(false);
        }
        options->setMenu(menu);options->setToolTip("Device profile editing will be enabled with the mapping layer.");
        header->addWidget(options);layout->addLayout(header);
        diagram=new SpeedEditorDiagram(this);layout->addWidget(diagram,0,Qt::AlignHCenter);
        auto lower=new QHBoxLayout;
        auto selectedBox=new QWidget(this);auto details=new QVBoxLayout(selectedBox);
        details->addWidget(new QLabel("Active Key",selectedBox));
        active=new QLabel(selectedBox);active->setObjectName("openGateSpeedEditorActiveKey");details->addWidget(active);
        details->addWidget(new QLabel("Button behavior (preview)",selectedBox));
        behavior=new QComboBox(selectedBox);behavior->addItems({"Button actions","Jogwheel mode"});
        behavior->setObjectName("openGateSpeedEditorBehaviorPreview");
        behavior->setEnabled(false);details->addWidget(behavior);
        modeHelp=new QLabel("Jog, Shuttle, and Scroll change how the wheel navigates the timeline. "
                            "A custom mode can control zoom, move between edits or markers, or adjust another directional value. "
                            "Press to activate it; the mode stays active after release until another mode is selected.",selectedBox);
        modeHelp->setObjectName("openGateSpeedEditorModeHelp");
        modeHelp->setWordWrap(true);details->addWidget(modeHelp);
        gestureLabel=new QLabel("Gesture",selectedBox);details->addWidget(gestureLabel);
        gesture=new QComboBox(selectedBox);gesture->setObjectName("openGateSpeedEditorGesturePreview");
        gesture->setEnabled(false);details->addWidget(gesture);
        assignmentLabel=new QLabel("Assignment",selectedBox);details->addWidget(assignmentLabel);
        assignment=new QComboBox(selectedBox);assignment->setObjectName("openGateSpeedEditorAssignmentPreview");
        assignment->addItem("Native — preserve Resolve's behavior","native");
        assignment->setToolTip("Native applies independently to this gesture. Custom assignment and saving are not enabled in this preview.");
        details->addWidget(assignment);
        auto reset=new QPushButton("Reset Key to Native",selectedBox);reset->setEnabled(false);details->addWidget(reset);
        auto notice=new QLabel("Preview only — explore behavior and gesture options. No assignments are saved; device mappings remain native.",selectedBox);notice->setWordWrap(true);
        details->addStretch();details->addWidget(notice);lower->addWidget(selectedBox,1);
        browser=new NativeCommandBrowser(this);lower->addWidget(browser,3);layout->addLayout(lower,1);
        auto footer=new QHBoxLayout;footer->addStretch();
        auto close=new QPushButton("Close",this);auto save=new QPushButton("Save",this);save->setEnabled(false);
        footer->addWidget(close);footer->addWidget(save);layout->addLayout(footer);
        connect(close,&QPushButton::clicked,this,[this]{if(closeRequested)closeRequested();});
        connect(behavior,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{refreshPreview();});
        diagram->selected=[this](const QString &key){select(key);};
        diagram->setSelectedKey("roll");select("roll");
    }
    bool bindNativeCommands(QWidget *editor) {return browser->bind(editor);}
    std::function<void()> closeRequested;
protected:
    void mousePressEvent(QMouseEvent *event) override {
        if(event->button()==Qt::LeftButton){diagram->clearSelection();event->accept();return;}
        QWidget::mousePressEvent(event);
    }
private:
    SpeedEditorDiagram *diagram;
    NativeCommandBrowser *browser;
    QComboBox *profiles,*behavior,*gesture,*assignment;
    QLabel *active,*modeHelp,*gestureLabel,*assignmentLabel;
    void select(const QString &key) {
        auto button=diagram->button(key);active->setText(button?button->text():QString("Select a button or the dial."));
        const bool mode=key=="shtl"||key=="jog"||key=="scrl";
        behavior->setToolTip(mode?"Native dial mode selector":"Native button behavior is preserved");
        behavior->setCurrentIndex(mode?1:0);
        refreshPreview();
    }
    void refreshPreview() {
        const auto key=diagram->selectedKey();
        const bool selected=!key.isEmpty(),dial=key=="dial";
        const bool mode=selected && !dial && behavior->currentIndex()==1;
        const bool action=selected && !dial && behavior->currentIndex()==0;
        behavior->setEnabled(selected && !dial);
        modeHelp->setVisible(mode);
        gesture->clear();
        if(dial||mode)gesture->addItems({"Dial left","Dial right"});
        else if(action)gesture->addItems({"Single press","Double press","Hold","Hold + dial left","Hold + dial right"});
        gestureLabel->setText(dial||mode?"Dial direction":"Gesture");
        gestureLabel->setVisible(dial||mode||action);
        gesture->setVisible(dial||mode||action);
        gesture->setEnabled(dial||mode||action);
        assignmentLabel->setVisible(action);assignment->setVisible(action);
    }
};
}
