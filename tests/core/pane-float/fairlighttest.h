// SPDX-License-Identifier: MIT
#pragma once

void verifyFairlightMixerRecovery()
{
    // Cover callbacks racing docking/page changes, including a synchronous
    // native host hide before the page itself becomes hidden.
    for(int mode=0;mode<3;++mode) {
        QWidget page; page.resize(600,400);
        auto splitter=new QSplitter(&page); splitter->resize(600,400);
        new QWidget(splitter);
        auto slot=new QWidget(splitter); slot->setObjectName("frameTimelineMixerContainer");
        auto layout=new QVBoxLayout(slot);
        auto content=new QLabel("Mixer",slot); layout->addWidget(content);
        auto alternate=new QLabel("Hidden alternate",slot); layout->addWidget(alternate); alternate->hide();
        QPushButton toggle(&page); toggle.setCheckable(true); toggle.setChecked(true);
        auto settle=[] {for(int i=0;i<8;++i) QCoreApplication::processEvents();};
        page.show(); settle();
        Unbound::SidebarFloatSession session;
        check(session.start(slot,"Mixer",&page,true).isEmpty(),"Mixer recovery setup failed"); settle();
        QPointer<QWidget> host=session.floatingWindow();
        QObject::connect(&toggle,&QPushButton::clicked,&page,[content](bool checked) {
            content->parentWidget()->setVisible(checked);
        });
        session.reclaimNativeSpace(&toggle);
        if(mode==1) page.hide();
        if(mode==2) check(session.dock().isEmpty(),"Dock during Mixer recovery failed");
        settle();
        check(!toggle.isChecked() && alternate->isHidden(),"Recovery changed toggle or exposed alternate Mixer");
        if(mode==2) {
            check(!session.active() && (!host || !host->isVisible()) && slot->layout()==layout,
                "Queued Mixer recovery reopened a docked host");
        } else {
            check(host && host->isVisible()==(mode==0),"Mixer recovery ignored page visibility");
            if(mode==1) {page.show(); settle(); check(host->isVisible(),"Mixer lost page-return visibility after native hide");}
            check(slot->isHidden() && content->isVisible(),"Mixer recovery failed to retain reclaimed space/content");
            check(session.dock().isEmpty(),"Mixer recovery docking failed");
        }
    }
}

bool runFairlightTest(QApplication &app)
{
    if (!app.arguments().contains("--fairlight-test")) return false;
    verifyFairlightMixerRecovery();
    QTemporaryDir directory;
    check(directory.isValid(), "Cannot create Fairlight settings directory");
    const auto path=directory.filePath("Unbound.json");
    qputenv("UNBOUND_SETTINGS_FILE",path.toUtf8());
    const bool restore=app.arguments().contains("--restore");
    const QStringList names={"frameAudioPanelMediaPoolContainer","frameAudioPanelEffectsLibraryContainer",
        "frameAudioPanelIndexContainer","frameAudioPanelGroupsContainer","frameAudioPanelSoundLibraryContainer",
        "frameAudioPanelAdrContainer","frameTimelineMixerContainer","frameMetadataContainer","frameInspectorContainer"};
    const QStringList suffixes={"MediaPool","EffectsLibrary","Index","Groups","SoundLibrary","Adr","Mixer","Metadata","Inspector"};
    const QStringList headers={"frameMediaPoolTitleBarTop","frameEffectTopBar","frameHeaderTitle","frameHeaderTitle",
        "frameMenu","headerFrame","frameHeaderContainer","frameMetadataTop","frameEffectsWidgetsTitle"};
    const QStringList menus={"btnMediaPoolOptions","btnOptions","m_BtnMenu","m_BtnMenu","m_BtnDisplayOptions",
        "optionsBtn","btnAudioMixerOptions","btnPresetsSelector","btnTitleOptions"};
    QWidget main; main.setObjectName("UiMainWindow");
    auto page=new QWidget(&main); page->setObjectName("WStackPage_Audio");
    QVector<QWidget *> paneSlots,contents;
    QVector<QLayout *> original;
    QVector<QHBoxLayout *> rows;
    QVector<QPushButton *> toggles,actions;
    QVector<int> edits(names.size(),0);
    Unbound::PaneSettings seed(path);
    for(int i=0;i<names.size();++i) {
        QWidget *parent=page;
        if(i==6) {auto splitter=new QSplitter(page); new QWidget(splitter); parent=splitter;}
        auto slot=new QWidget(parent); slot->setObjectName(names[i]);
        auto layout=new QVBoxLayout(slot);
        auto content=new QWidget(slot); layout->addWidget(content);
        auto column=new QVBoxLayout(content);
        auto header=new QWidget(content); header->setObjectName(headers[i]); column->addWidget(header);
        auto row=new QHBoxLayout(header); row->addWidget(new QLabel("Title",header)); row->addStretch();
        auto action=new QPushButton(i==3 ? "Create Group" : "...",header); action->setObjectName(menus[i]); row->addWidget(action);
        QObject::connect(action,&QPushButton::clicked,page,[&,i]{++edits[i];});
        column->addWidget(new QLabel("Live content",content));
        if(i==2) layout->addWidget(new QLabel("Index tabs",slot));
        if(i==6) {auto alternate=new QWidget(slot); alternate->setObjectName("hiddenMixer"); layout->addWidget(alternate); alternate->hide();}
        auto toggle=new QPushButton("Native toggle",page); toggle->setObjectName("btnAudioPanel"+suffixes[i]+"InSSM");
        toggle->setCheckable(true); toggle->setChecked(!restore);
        QObject::connect(toggle,&QPushButton::clicked,page,[slot,content,i](bool checked) {
            QTimer::singleShot(0,slot,[slot,content,i,checked]{
                slot->setVisible(checked); content->setVisible(checked);
                // Fairlight may address the mixer's current container through
                // its retained content pointer. After transfer this is the host.
                if(i==6) content->parentWidget()->setVisible(checked);
            });
        });
        paneSlots.append(slot); contents.append(content); original.append(layout); rows.append(row); actions.append(action); toggles.append(toggle);
        if(restore) {seed.saveFloating("Audio/"+names[i],true); seed.saveToggle("Audio/"+names[i],false); slot->hide();}
    }
    // The same generic slot name on another page must never be decorated.
    auto other=new QWidget(&main); auto decoy=new QWidget(other); decoy->setObjectName("frameInspectorContainer");
    (new QVBoxLayout(decoy))->addWidget(new QLabel("Other page",decoy));
    auto meters=new QWidget(page); meters->setObjectName("frameAudioPageMonitoringContainer");
    (new QVBoxLayout(meters))->addWidget(new QLabel("Meters",meters));
    main.show(); page->show(); other->hide();
    auto settle=[] {for(int i=0;i<40;++i) QCoreApplication::processEvents();}; settle();
    QLibrary helper(app.arguments().last()); helper.setLoadHints(QLibrary::PreventUnloadHint);
    check(helper.load(),qPrintable(helper.errorString())); auto start=resolveBootstrap(helper);
    check(start && start(nullptr)==1,"Fairlight bootstrap failed"); settle();
    check(!decoy->findChild<QToolButton *>("resolvePaneUndockButton") && !meters->findChild<QToolButton *>("resolvePaneUndockButton"),
        "Decorated another page or excluded Meters");
    for(int i=0;i<names.size();++i) {
        auto button=actions[i]->parentWidget()->findChild<QToolButton *>("resolvePaneUndockButton");
        check(button && rows[i]->indexOf(button)+1==rows[i]->indexOf(actions[i]),"Fairlight button not directly left of native action");
        if(!restore) {button->click(); settle();}
        auto host=contents[i]->window();
        check(host!=&main && host->isVisible() && contents[i]->isVisible()
            && !toggles[i]->isChecked() && paneSlots[i]->isHidden(),
            "Fairlight undock/restore did not reclaim slot and retain visible content");
        if(i==6) check(host->findChild<QWidget *>("hiddenMixer")->isHidden(),"Exposed alternate mixer");
        if(i==0) {
            // QWidget::isAncestorOf stops at window boundaries; the diagnostic
            // specifically retains the QObject/parentWidget chain across one.
            bool reachesPage=false;
            for(auto ancestor=contents[i]->parentWidget();ancestor;ancestor=ancestor->parentWidget())
                if(ancestor==page) reachesPage=true;
            check(host->isWindow() && host->parentWidget()==paneSlots[i] && reachesPage,
                "Floating audition lost its Fairlight ancestor path");
        }
        actions[i]->click(); check(edits[i]==1,"Lost native action signal");
        page->hide(); settle(); check(!host->isVisible(),"Fairlight host remained on another page");
        page->show(); settle(); check(host->isVisible(),"Fairlight host not retained on page return");
        const auto key="Audio/"+names[i];
        check(Unbound::PaneSettings(path).floating(key),"Fairlight floating state not persisted");
        check(!Unbound::PaneSettings(path).toggle(key),"Fairlight saved toggle preference changed");
        if(i==0) {
            page->hide(); settle(); host->close(); settle();
            check(!toggles[i]->isChecked(),"Inactive docking toggled page early"); page->show(); settle();
        } else if(i%2) {button->click(); settle();} else {host->close(); settle();}
        check(paneSlots[i]->layout()==original[i] && contents[i]->parentWidget()==paneSlots[i] && contents[i]->isVisible()
            && toggles[i]->isChecked() && !Unbound::PaneSettings(path).floating(key),"Fairlight docking failed to restore layout/visibility/state");
    }
    QFile file(path); check(file.open(QIODevice::ReadOnly),"Cannot read Fairlight settings");
    const auto pages=QJsonDocument::fromJson(file.readAll()).object().value("pages").toObject();
    check(pages.value("WStackPage_Audio").toObject().value("panes").toObject().size()==9,"Fairlight settings stored under wrong page");
    QTimer::singleShot(0,&app,&QCoreApplication::quit); app.exec();
    std::puts("PASS: nine Fairlight panes, native header placement, toggle reclamation, docking, page isolation and saved restoration");
    return true;
}
