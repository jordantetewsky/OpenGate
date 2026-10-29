// SPDX-License-Identifier: MIT
#pragma once
#include "productidentity.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>
#include "panelcommands.h"
namespace Unbound {
inline QDialog *createPanelShortcutEditor(PanelCommands *registry,QWidget *owner) {
    auto dialog=new QDialog(owner); dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setObjectName("unboundPanelShortcuts"); dialog->setProperty("unboundShortcutEditor",true);
    dialog->setWindowTitle("OpenGate Panel Shortcuts"); dialog->resize(590,520);
    auto layout=new QVBoxLayout(dialog);
    auto note=new QLabel("OpenGate bindings are saved separately from Resolve’s Keyboard Customization. Click a shortcut to assign it; Backspace clears it. Existing native bindings are retained.",dialog);
    note->setWordWrap(true); layout->addWidget(note);
    auto table=new QTableWidget(registry->entries().size(),2,dialog); table->setHorizontalHeaderLabels({"Command","Shortcut"});
    table->verticalHeader()->hide(); table->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1,QHeaderView::ResizeToContents);
    QList<QKeySequenceEdit *> editors; int row=0;
    const auto stored=registry->assigned();
    for (const auto &command : registry->entries()) {
        auto label=new QTableWidgetItem(command.label); label->setFlags(label->flags() & ~Qt::ItemIsEditable); table->setItem(row,0,label);
        auto edit=new QKeySequenceEdit(QKeySequence::fromString(stored.value(command.id).toString(),QKeySequence::PortableText),table);
        edit->setObjectName("binding_"+command.id); table->setCellWidget(row++,1,edit); editors.append(edit);
    }
    layout->addWidget(table); auto error=new QLabel(dialog); error->setWordWrap(true); layout->addWidget(error);
    auto buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel,dialog); layout->addWidget(buttons);
    QObject::connect(buttons,&QDialogButtonBox::rejected,dialog,&QDialog::reject);
    QObject::connect(buttons,&QDialogButtonBox::accepted,dialog,[registry,dialog,editors,error] {
        QJsonObject proposed; int row=0;
        for (const auto &command : registry->entries()) proposed[command.id]=editors[row++]->keySequence().toString(QKeySequence::PortableText);
        const auto failure=registry->save(proposed); if (!failure.isEmpty()) { error->setText(failure); return; }
        dialog->accept();
    });
    return dialog;
}
}
