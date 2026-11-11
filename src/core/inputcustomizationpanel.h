// SPDX-License-Identifier: MIT
#pragma once
#include "../platform/nativekeyboardeditor.h"
#include "speededitorcustomization.h"
#include "inputcustomizationtabs.h"
#include "embeddeddialoganchor.h"
#include <QPointer>
#include <QTabWidget>
#include <QTabBar>
#include <QTimer>

namespace Unbound {
// Owns one native editor and its sibling device tab in a parked panel surface.
// ManagedPanelSession remains the sole owner of placement, scale and persistence.
class InputCustomizationPanel {
public:
    using Factory=std::function<QDialog *(QWidget *,QString &)>;
    explicit InputCustomizationPanel(Factory factory=NativeKeyboardEditor::create):factory(std::move(factory)){}
    InputCustomizationPanel(const InputCustomizationPanel &)=delete;
    InputCustomizationPanel &operator=(const InputCustomizationPanel &)=delete;
    ~InputCustomizationPanel() {
        disconnectCallbacks();
        if(abandoned)return;
        delete content;
        delete parking;
    }
    static QString id() {return "openGateInputCustomizationPanel";}
    static QString label() {return "Keyboard & Speed Editor Customization";}
    std::function<void()> closeRequested;
    QWidget *ensure(QWidget *page,QString &error) {
        if(content)return content;
        if(!page){error="The Edit workspace is not available.";return nullptr;}
        auto parked=new QWidget(page);parked->setObjectName("openGateInputCustomizationParking");parked->hide();
        auto editor=factory(parked,error);
        if(!editor){delete parked;return nullptr;}
        parking=parked;
        editor->ensurePolished();
        const auto nativeSize=editor->size().expandedTo(editor->minimumSizeHint());
        auto tabs=new InputCustomizationTabs(parked);content=tabs;keyboard=editor;
        tabs->setObjectName(id());tabs->setProperty("openGateReachableOverflow",true);
        tabs->setProperty("openGateMinimumContentScale",1.0);
        tabs->setProperty("openGatePersistSelectedTab",true);
        tabs->setDocumentMode(true);
        tabs->setPalette(editor->palette());tabs->setFont(editor->font());
        editor->setParent(tabs,Qt::Widget);
        editor->setWindowModality(Qt::NonModal);
        editor->setMinimumSize(nativeSize);
        tabs->addTab(editor,"Keyboard Customization");
        new EmbeddedDialogAnchor(editor);
        auto device=new SpeedEditorCustomization(tabs);speedEditor=device;
        tabs->addTab(device,"Speed Editor Customization");
        device->bindNativeCommands(editor);
        device->closeRequested=[this]{if(closeRequested)closeRequested();};
        finishedConnection=QObject::connect(editor,&QDialog::finished,tabs,[this]{
            if(closeRequested)closeRequested();
        });
        tabConnection=QObject::connect(tabs,&QTabWidget::currentChanged,tabs,[this,device](int index){
            if(index==0 && keyboard)keyboard->show();
            if(index==1 && keyboard)device->bindNativeCommands(keyboard);
        });
        tabs->resize(nativeSize.width(),nativeSize.height()+tabs->tabBar()->sizeHint().height());
        tabs->setProperty("openGateFloatingMinimumSize",tabs->size());
        tabs->hide();
        return tabs;
    }
    void recalled() {if(keyboard && content && content->currentIndex()==0)keyboard->show();}
    void abandonForQuit() {abandoned=true;closeRequested={};disconnectCallbacks();}
private:
    Factory factory;
    QPointer<QWidget> parking;
    QPointer<QTabWidget> content;
    QPointer<QDialog> keyboard;
    QPointer<SpeedEditorCustomization> speedEditor;
    QMetaObject::Connection finishedConnection,tabConnection;
    bool abandoned=false;
    void disconnectCallbacks() {
        QObject::disconnect(finishedConnection);QObject::disconnect(tabConnection);
        if(speedEditor)speedEditor->closeRequested={};
    }
};
}
