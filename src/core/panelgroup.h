// SPDX-License-Identifier: MIT
#pragma once
#include "panelscalecontrols.h"
#include "grouppanelheader.h"
#include "paneltitlegesture.h"
#include "compacttoolbar.h"
#include <QHash>
#include <QPushButton>
#include <QTabWidget>
#include "grouplayout.h"
#include "panellayoutsurface.h"
#include "panellayoutproportions.h"
#include "panellayoutsnapshot.h"
#include "panelsession.h"
#include "workspacesettings.h"
#include "tabtearoff.h"
#include "groupvisibility.h"
#include "panellayoutvisibility.h"
#include "docksurfaces.h"
#include "uiworktiming.h"
#include "grouphostreserve.h"
#include "hiddenpanelcallers.h"

namespace Unbound {
class PanelGroup : public QObject
{
public:
    explicit PanelGroup(QObject *owner, GroupHostReserve *reserve = nullptr) : QObject(owner), reserve(reserve)
    {
        hiddenCallers = new HiddenPanelCallers(this);
        timer.setSingleShot(true); timer.setInterval(180);
        connect(&timer, &QTimer::timeout, this, [this] { if (host) report("group.arrangement"); });
    }
    ~PanelGroup() override { changed = {}; dissolve(); }
    bool active() const { return host; }
    QWidget *window() const { return host; }
    bool windowHidden() const { return host && host->property("unboundGroupDoorHidden").toBool(); }
    void setWindowHidden(bool hidden,bool activate=true) {
        if (!host) return;host->setProperty("unboundGroupDoorHidden",hidden);refresh();
        if (!hidden && activate) { host->raise();host->activateWindow(); }
        report("group.window.visibility");
    }
    bool contains(ManagedPanelSession *session) const { return host && members.contains(session); }
    void setChrome(WindowChrome value) { if (host) host->setChrome(value); }
    void setSnapping(bool value) { if (host) host->setSnapping(value); }
    void fitFrame(const QRect &frame)
    {
        if (!host) return;
        UiWorkTiming timing("group.fit-frame",host);
        host->setGeometry(QRect(frame.topLeft() + host->geometry().topLeft() - host->frameGeometry().topLeft(),
            frame.size() - (host->frameGeometry().size() - host->size()))); keepOnDisplay(host);
    }
    std::function<void()> changed;
    std::function<void(QWidget *, Qt::Edge)> attachRequested;
    QString join(ManagedPanelSession *a, ManagedPanelSession *b, Qt::Orientation orientation, const QRect &destination = {})
    {
        if (!a || !b || !a->active() || !b->active()) return "Show both panels first.";
        const auto left = a->floatingGeometry(), right = b->floatingGeometry();
        return initialize({a, b}, splitGroupTree({{"panel", panelName(a)}}, {{"panel", panelName(b)}}, orientation),
            destination.isValid() ? destination : QRect(left.topLeft(), orientation == Qt::Horizontal ? QSize(left.width() + right.width() + 4, qMax(left.height(), right.height())) :
                QSize(qMax(left.width(), right.width()), left.height() + right.height() + 4)));
    }
    QString initialize(const QList<ManagedPanelSession *> &panels, const QJsonObject &tree, const QRect &position)
    {
        UiWorkTiming timing("group.initialize",nullptr);
        QStringList expected, actual;
        if (host || panels.size() < 2 || !groupTreeNames(tree, expected) || expected.size() != panels.size()) return "Invalid group layout.";
        auto a = panels.first();
        if (!a || !a->pageWidget() || !a->pageWidget()->isVisible()) return "Open Edit before grouping panels.";
        for (auto panel : panels) {
            if (!panel || !panel->active() || panel->grouped() || panel->pageWidget() != a->pageWidget() ||
                panel->nativeOwnerWindow() != a->nativeOwnerWindow() || actual.contains(panelName(panel))) return "Choose available panels from the same Edit page.";
            actual.append(panelName(panel));
        }
        expected.sort(); actual.sort(); if (expected != actual) return "Saved group contains different panels.";
        page = a->pageWidget(); host = reserve ? reserve->take(a->nativeOwnerWindow()) : new WorkspaceFloatingHost(a->nativeOwnerWindow());
        host->setObjectName(QStringLiteral("unboundFloating_group_") + actual.join('_'));
        host->setAttribute(Qt::WA_QuitOnClose, false);
        auto layout = new QVBoxLayout(host); layout->setContentsMargins(0, 0, 0, 0); host->setGeometry(position);
        for (auto panel : panels) { const auto error = addMember(panel); if (!error.isEmpty()) { dissolve(); return error; } }
        rebuild(tree); host->setChrome(WindowChrome::Standard);
        host->attachRequested = [this](QWidget *target, Qt::Edge edge) { if (attachRequested) attachRequested(target, edge); };
        host->installEventFilter(this); page->installEventFilter(this);
        refresh();
        for (auto panel : panels) panel->finishGroupTransfer();
        keepOnDisplay(host); report("group.create"); return {};
    }
    QString add(ManagedPanelSession *panel, Qt::Orientation orientation, bool before = false, const QString &target = {})
    {
        if (!host || !panel || !panel->active() || panel->grouped() || panel->pageWidget() != page ||
            panel->nativeOwnerWindow() != host->parentWidget()) return "Panel cannot join this group.";
        if (!target.isEmpty() && !groupTreeHasTarget(treeSnapshot(root),target)) return "Dock target no longer exists.";
        const auto tree = insertGroupTree(treeSnapshot(root), {{"panel", panelName(panel)}}, target, orientation, before);
        const auto error = addMember(panel); if (!error.isEmpty()) return error;
        rebuild(tree); refresh(); panel->finishGroupTransfer(); report("group.add"); return {};
    }
    QString merge(PanelGroup *other, Qt::Orientation orientation, bool before = false, const QString &target = {})
    {
        if (!host || !other || !other->host || other == this || other->page != page ||
            host->parentWidget() != other->host->parentWidget()) return "These groups cannot be merged.";
        if (!target.isEmpty() && !groupTreeHasTarget(treeSnapshot(root),target)) return "Dock target no longer exists.";
        const auto tree = insertGroupTree(treeSnapshot(root), other->treeSnapshot(other->root), target, orientation, before);
        parkHeaders();other->parkHeaders();
        QScopedValueRollback<bool> moving(other->releasing, true);
        for (auto member : other->members) if (member) {
            auto wrapper = other->leaves.value(member);
            member->window()->removeEventFilter(other); QObject::disconnect(member->window(), nullptr, other, nullptr);
            for (auto label : wrapper->findChildren<QLabel *>("unboundGroupHeader")) label->removeEventFilter(other);
            for (auto button : wrapper->findChildren<QPushButton *>("unboundDetachPanel")) QObject::disconnect(button, nullptr, other, nullptr);
            for (auto button : wrapper->findChildren<QPushButton *>("unboundHidePanel")) QObject::disconnect(button, nullptr, other, nullptr);
            wrapper->setParent(host); members.append(member); leaves[member] = wrapper;headers[member]=other->headers.take(member); wireMember(member);
        }
        other->members.clear(); other->leaves.clear();other->headers.clear(); other->retireHost();
        rebuild(tree); refresh(); if (other->changed) other->changed(); report("group.merge"); return {};
    }
    // Transfer a tab containing a split subtree without flattening its panels.
    PanelGroup *detachSection(QWidget *section)
    {
        if (!host || !section || !root || section == root || !root->isAncestorOf(section)) return nullptr;
        const auto tree = treeSnapshot(section); QStringList selectedNames;
        if (!groupTreeNames(tree, selectedNames) || selectedNames.size() < 2) return nullptr;
        const auto originalTree = treeSnapshot(root); const auto originalFrame = host->geometry();
        const QRect frame(section->mapToGlobal(QPoint()), section->size());
        QList<ManagedPanelSession *> selected, all;
        for (auto candidate : members) if (candidate) {
            all.append(candidate);
            if (selectedNames.contains(panelName(candidate))) selected.append(candidate);
        }
        if (selected.size() != selectedNames.size() || selected.size() == all.size()) return nullptr;
        for (auto candidate : selected) if (contains(candidate)) detachMember(candidate);
        auto detached = new PanelGroup(parent(),reserve);
        const auto error = detached->initialize(selected, tree, frame);
        if (!error.isEmpty()) {
            delete detached; dissolve();
            const auto rollback = initialize(all, originalTree, originalFrame);
            logEvent(QStringLiteral("group.section.rollback"), {{"error", error}, {"rollback_error", rollback}});
            return nullptr;
        }
        if (sectionDetached) sectionDetached(detached);
        logEvent(QStringLiteral("group.section.detach"), {{"panels", QJsonArray::fromStringList(selectedNames)}});
        return detached;
    }
    std::function<void(PanelGroup *)> sectionDetached;
    void dissolve(bool dockMembers = false,bool deferMemberShow = false)
    {
        if (releasing || (!host && members.isEmpty())) return;
        parkHeaders();
        QScopedValueRollback<bool> guard(releasing, true);
        QList<ManagedPanelSession *> current;
        for (auto member : members) if (member) {
            if(auto header=headers.value(member))header->releaseNativeHeader();
            current.append(member); member->releaseGroup = {};
            if (member->window()) member->window()->removeEventFilter(this);
            member->detachFromGroup(dockMembers || deferMemberShow);
        }
        members.clear(); leaves.clear();headers.clear(); retireHost();
        if (dockMembers) for (auto member : current) if (member->active()) {
            const auto error = member->dock(); if (!error.isEmpty()) logEvent(QStringLiteral("group.dock.failed"), {{"error", error}});
        }
        logEvent(QStringLiteral("group.release"), {{"dock_members", dockMembers}}); if (changed) changed();
    }
    QJsonObject snapshot() const
    {
        if (!host) return {};
        auto value = WorkspaceSettings::displayRectangle(host->isMaximized()?host->normalGeometry():host->geometry(),placementRecord); const auto tree = treeSnapshot(root);
        value["maximized"]=host->isMaximized();
        value["pinned"] = host->pinned();
        value["hidden"] = windowHidden();
        value["tree"] = tree; value["panels"] = QJsonArray::fromStringList(names());
        value["orientation"] = tree["orientation"]; value["sizes"] = tree["sizes"]; return value;
    }
    static QJsonObject savedTree(const QJsonObject &value)
    {
        if (value["tree"].isObject()) return value["tree"].toObject();
        const auto panels = value["panels"].toArray(); if (panels.size() != 2) return {};
        auto tree = splitGroupTree({{"panel", panels[0]}}, {{"panel", panels[1]}}, value["orientation"] == "vertical" ? Qt::Vertical : Qt::Horizontal);
        tree["sizes"] = value["sizes"]; return tree;
    }
    bool restore(const QJsonObject &value)
    {
        if (!host) return false;
        const auto tree = savedTree(value); QStringList expected;
        if (!groupTreeNames(tree, expected)) return false;
        auto current = names(); current.sort(); expected.sort(); if (current != expected) return false;
        placementRecord=value;const auto position = WorkspaceSettings::rectangle(value); if (position.isValid()) host->setGeometry(position);
        host->setPinned(value["pinned"].toBool());
        host->setProperty("unboundGroupDoorHidden",value["hidden"].toBool());
        rebuild(tree); refresh(); keepOnDisplay(host);if(value["maximized"].toBool())host->showMaximized();
        if(placementRecord.contains("display")&&DisplayPlacement::match(placementRecord,workspaceDisplays())<0)
            placementRecord["display_fallback"]=DisplayPlacement::rect(host->isMaximized()?host->normalGeometry():host->geometry());
        return true;
    }
    bool selected(ManagedPanelSession *member) const
    {
        auto wrapper = leaves.value(member);
        if (!wrapper || member->hiddenByUser()) return false;
        for (auto parent = wrapper->parentWidget(); parent && parent != host; parent = parent->parentWidget())
            if (auto tabs = qobject_cast<QTabWidget *>(parent))
                if (!tabs->currentWidget() || (tabs->currentWidget() != wrapper && !tabs->currentWidget()->isAncestorOf(wrapper))) return false;
        return true;
    }
    void syncVisibility(ManagedPanelSession *select = nullptr)
    {
        if (!host || rebuilding || releasing || syncingVisibility) return;
        bool transition = visibilityState.size() != members.size();
        for (auto member : members) if (member &&
            (!visibilityState.contains(member) || visibilityState.value(member) != member->hiddenByUser())) transition = true;
        // The session notification and command completion can report the same
        // transition. Do not resize/repaint the whole tree twice for one door.
        if (!transition && (!select || selected(select))) { refresh(); return; }
        UiWorkTiming timing("group.visibility",host);
        QScopedValueRollback<bool> guard(syncingVisibility, true);
        if (root) visibility.remember(root);
        for (auto member : members) if (member && member->hiddenByUser() &&
            (!visibilityState.contains(member) || !visibilityState.value(member)))
            visibility.closePath(leaves.value(member),host);
        for (auto member : members) if (member) {
            auto wrapper = leaves.value(member); if (!wrapper) continue;
            const bool visible = !member->hiddenByUser();
            for (auto tabs : host->findChildren<QTabWidget *>()) for (int i = 0; i < tabs->count(); ++i)
                if (tabs->widget(i) == wrapper) visibility.tabVisibility(tabs, wrapper, visible, select == member);
            if (!wrapper->parentWidget() || !qobject_cast<QStackedWidget *>(wrapper->parentWidget())) wrapper->setVisible(visible);
            member->window()->setVisible(visible);
        }
        if (root) applyPanelLayoutVisibility(root,visibility,[this](const QString &id) {
            for (auto member:members) if (member && panelName(member)==id) return !member->hiddenByUser();
            return false;
        });
        if (host->layout()) host->layout()->activate();
        if (root) visibility.restore(root);
        if (select && !select->hiddenByUser()) visibility.selectPath(leaves.value(select),host);
        visibilityState.clear();
        for (auto member : members) if (member) visibilityState[member] = member->hiddenByUser();
        refresh();
        QJsonArray states;
        for (auto member : members) if (member) states.append(QJsonObject{{"panel",panelName(member)},
            {"hidden",member->hiddenByUser()},{"selected",selected(member)},
            {"surface_visible",member->window() && member->window()->isVisible()}});
        logEvent("group.door.complete",{{"window",host->objectName()},
            {"native_id",host->testAttribute(Qt::WA_WState_Created) ? nativeWindowNumber(host) : 0},
            {"pinned",host->pinned()},{"visible",host->isVisible()},{"panels",states},
            {"frame",WorkspaceSettings::rectangle(host->geometry())}});
    }
    void detachMember(ManagedPanelSession *member,bool deferHostShow = false)
    {
        parkHeaders();
        if (!contains(member) || releasing) return;
        const QRect groupFrame = host->frameGeometry(); auto wrapper = leaves.value(member);
        if (!wrapper || !member->window()) { dissolve(); return; }
        QScopedValueRollback<bool> guard(releasing, true);
        const QRect detachedFrame(wrapper->mapToGlobal(QPoint()), wrapper->size());
        const auto tree = removeGroupLeaf(treeSnapshot(root), panelName(member));
        member->releaseGroup = {}; member->window()->removeEventFilter(this);
        QObject::disconnect(member->window(), nullptr, this, nullptr);
        if(auto header=headers.value(member))header->releaseNativeHeader();
        member->detachFromGroup(deferHostShow); members.removeAll(member); leaves.remove(member); headers.remove(member); wrapper->hide(); wrapper->deleteLater();
        if (members.size() == 1) {
            auto remaining = members.first(); remaining->releaseGroup = {}; remaining->window()->removeEventFilter(this);
            if(auto header=headers.value(remaining))header->releaseNativeHeader();
            remaining->detachFromGroup(); place(remaining, groupFrame); members.clear(); leaves.clear(); retireHost();
        } else if (members.isEmpty()) retireHost(); else rebuild(tree);
        place(member, detachedFrame);
        logEvent(QStringLiteral("group.detach"), {{"panel", member->window()->objectName()}, {"remaining", members.size()}});
        if (changed) changed();
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (!host || releasing || rebuilding) return false;
        auto header = qobject_cast<QLabel *>(object);
        if (header && header->objectName() == QLatin1String("unboundGroupHeader")) {
            if (event->type() == QEvent::MouseButtonPress) {
                auto mouse = static_cast<QMouseEvent *>(event); if (mouse->button() != Qt::LeftButton || !panelTitleHit(header,mouse->pos())) return false;
                press = mouse->globalPos(); headerPressed = true;
                pressedHeader=header;
                headerOffset = press - header->parentWidget()->parentWidget()->mapToGlobal(QPoint()); return true;
            }
            if (event->type() == QEvent::MouseMove) {
                auto mouse = static_cast<QMouseEvent *>(event);
                if (!headerPressed || pressedHeader!=header || !mouse->buttons().testFlag(Qt::LeftButton) || (mouse->globalPos() - press).manhattanLength() < QApplication::startDragDistance()) return false;
                headerPressed = false; QPointer<ManagedPanelSession> member;
                for (auto candidate : members) if (candidate && panelName(candidate) == header->property("groupPanel").toString()) member = candidate;
                if (!member) return true;
                detachMember(member); auto dragHost = static_cast<WorkspaceFloatingHost *>(member->window());
                dragHost->move(mouse->globalPos() - headerOffset); dragHost->raise(); dragHost->activateWindow();
                dragHost->beginPointerDrag(mouse->globalPos(), true); return true;
            }
            if (event->type() == QEvent::MouseButtonRelease || event->type()==QEvent::Hide || event->type()==QEvent::UngrabMouse) {headerPressed = false;pressedHeader=nullptr;}
        }
        if (object == host && event->type() == QEvent::Close) {
            setWindowHidden(true,false); static_cast<QCloseEvent *>(event)->accept(); return true;
        }
        if (object==host && event->type()==QEvent::DynamicPropertyChange &&
            static_cast<QDynamicPropertyChangeEvent *>(event)->propertyName()=="unboundPinned") timer.start();
        if (object == host && (event->type() == QEvent::Move || event->type() == QEvent::Resize)) timer.start();
        if (event->type() == QEvent::Show || event->type() == QEvent::Hide) {
            if (object == page && event->type() == QEvent::Hide) host->hide();
            QTimer::singleShot(0, this, [this] { refresh(); });
        }
        return false;
    }
private:
    QJsonObject placementRecord;
    static QString panelName(ManagedPanelSession *session)
    { return session->window()->objectName().mid(session->grouped() ? QStringLiteral("unboundGroupCell_").size() : QStringLiteral("unboundFloating_").size()); }
    QStringList names() const { QStringList result; for (auto member : members) if (member) result.append(panelName(member)); return result; }
    QString addMember(ManagedPanelSession *member)
    {
        const auto name = panelName(member), title = member->panelTitle();
        auto wrapper = new QWidget(host); wrapper->setProperty("groupPanel", name);
        auto layout = new QVBoxLayout(wrapper); layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(0);
        auto header=new GroupPanelHeader(wrapper,title,[member] { return member->interfaceScale(); },
            [member](double factor) { member->setInterfaceScale(factor); },
            [this,member] { detachMember(member); },[this,member] { member->setHiddenByUser(true);syncVisibility(); },
            [this](QMenu *menu) { hiddenCallers->populate(menu); });
        header->label->setProperty("groupPanel",name);header->label->setProperty("groupMember",members.size());header->label->installEventFilter(this);
        auto content = new QWidget(wrapper);content->setObjectName(QStringLiteral("unboundGroupCell_")+name);content->setWindowTitle(title);
        layout->addWidget(header->bar);layout->addWidget(content,1);headers[member]=header;
        if (name=="unboundWorkspaceToolbar") header->bar->hide();
        const bool clips = setNativeGroupSurfaceClipping(content);
        logEvent("panel.boundary.native-clip",{{"panel",name},{"enabled",clips}});
        // Keep old hosts until the destination is ready. Hiding the active host
        // earlier makes the window system activate another window in the middle of transfer.
        const auto error = member->attachToGroup(content,true); if (!error.isEmpty()) { delete wrapper; return error; }
        header->borrowNativeHeader(content,name);
        if (name=="unboundWorkspaceToolbar") new CompactToolbar(wrapper,content->findChild<QStackedWidget *>() ? content->findChild<QStackedWidget *>() : content);
        members.append(member); leaves[member] = wrapper; wireMember(member); return {};
    }
    void wireMember(ManagedPanelSession *member)
    {
        member->releaseGroup = [this, member] { detachMember(member,true); }; member->window()->installEventFilter(this);
        const auto wrapper = leaves.value(member);
        for (auto label : wrapper->findChildren<QLabel *>("unboundGroupHeader")) label->installEventFilter(this);
        if (auto header=headers.value(member)) {
            header->detachAction=[this,member] { detachMember(member); };
            header->hideAction=[this,member] { member->setHiddenByUser(true);syncVisibility(); };
            header->extrasAction=[this](QMenu *menu) { hiddenCallers->populate(menu); };
            auto button=header->button;
            button->drag=[this,member](QPoint pointer) { detachMember(member);if (auto window=dynamic_cast<WorkspaceFloatingHost *>(member->window())) { window->move(pointer-QPoint(80,12));window->beginPointerDrag(pointer,true); } };
        }
        connect(member->window(), &QObject::destroyed, this, [this] { if (!releasing && !rebuilding) dissolve(); });
    }
    void parkHeaders() { for (auto header:headers) if (header) header->park(); }
    void syncHeaders() {
        parkHeaders();
        for (auto member:members) if (member && headers.value(member)) {
            headers.value(member)->presentFor(leaves.value(member),panelName(member)=="unboundWorkspaceToolbar");
        }
    }
    QJsonObject treeSnapshot(QWidget *widget) const
    {
        return panelLayoutSnapshot(widget);
    }
    QWidget *buildTree(const QJsonObject &node)
    {
        PanelLayoutCallbacks callbacks;
        callbacks.leaf = [this](const QString &id) -> QWidget * {
            for (auto member : members) if (member && panelName(member)==id) return leaves.value(member);
            return nullptr;
        };
        callbacks.title = [this](const QString &id) {
            for (auto member : members) if (member && panelName(member)==id) return member->panelTitle();
            return QString();
        };
        callbacks.changed = [this] { if (!rebuilding) { syncHeaders();timer.start(); } };
        callbacks.tearOff = [this](QWidget *leaf,QPoint pointer) {
            QPointer<ManagedPanelSession> member;
            for (auto candidate : members) if (leaves.value(candidate)==leaf) member=candidate;
            QWidget *window=nullptr;
            if (member) { detachMember(member); window=member->window(); }
            else if (auto section=detachSection(leaf)) window=section->window();
            if (!window) return;
            auto floating=static_cast<WorkspaceFloatingHost *>(window);
            floating->move(pointer-QPoint(80,12)); floating->raise(); floating->activateWindow();
            floating->beginPointerDrag(pointer,true);
        };
        return buildPanelLayoutSurface(node,host,this,callbacks);
    }
    void rebuild(const QJsonObject &tree)
    {
        UiWorkTiming timing("group.rebuild",host);
        QScopedValueRollback<bool> guard(rebuilding, true);
        parkHeaders();
        visibility.reset();
        visibilityState.clear();
        for (auto member : members) if (member) leaves.value(member)->setParent(host);
        if (root) { host->layout()->removeWidget(root); root->hide(); root->deleteLater(); }
        root = buildTree(tree);
        if (!root || !host->layout()) {
            logEvent(QStringLiteral("group.rebuild.failed"), {{"tree", tree}, {"panels", QJsonArray::fromStringList(names())}, {"root", !root.isNull()}, {"layout", host->layout() != nullptr}});
            return;
        }
        static_cast<QVBoxLayout *>(host->layout())->insertWidget(0,root);
        for (auto member : members) if (member) {
            auto wrapper = leaves.value(member);
            // QStackedWidget owns tab-page visibility. Showing an inactive page
            // here makes it overlap the selected native panel and receive input.
            const bool visible = !member->hiddenByUser();
            if (auto stack = qobject_cast<QStackedWidget *>(wrapper->parentWidget())) {
                if (auto tabs = qobject_cast<QTabWidget *>(stack->parentWidget())) visibility.tabVisibility(tabs,wrapper,visible,false);
            } else wrapper->setVisible(visible);
            member->window()->setVisible(visible);
        }
        root->show();syncHeaders(); host->layout()->activate();
        restorePanelLayoutProportions(root);
        applyPanelLayoutVisibility(root,visibility,[this](const QString &id) {
            for (auto member:members) if (member && panelName(member)==id) return !member->hiddenByUser();
            return false;
        });
        QStringList titles; for (auto member : members) if (member && member->window()) titles.append(member->panelTitle());
        host->setWindowTitle(titles.join(QStringLiteral(" + ")));
        QList<QWidget *> surfaces;
        for (auto member : members) if (member) surfaces.append(member->window());
        registerDockSurfaces(host,surfaces);
        // Visibility is established before the initial layout/show. Scheduling
        // another visibility/layout pass after presentation repeats that work.
    }
    void retireHost()
    {
        timer.stop(); if (page) page->removeEventFilter(this);
        if (host) { host->removeEventFilter(this); host->hide(); host->deleteLater(); }
        host = nullptr; root = nullptr;
    }
    void refresh()
    {
        if (!host || releasing || rebuilding) return;
        QList<HiddenPanelCallers::Entry> hiddenEntries;
        for (auto member : members) if (member && member->hiddenByUser()) {
            QPointer<ManagedPanelSession> alive(member);
            hiddenEntries.append({panelName(member),member->panelTitle(),[this,alive] {
                if (!alive || !contains(alive)) return;
                alive->setHiddenByUser(false); syncVisibility(alive); report("group.hidden.reveal");
            }});
        }
        hiddenCallers->setHidden(hiddenEntries);
        bool visible = false;
        for (auto member : members) { if (!member || !member->active()) { dissolve(); return; } visible |= !member->hiddenByUser(); }
        visible &= page && page->isVisible() && !windowHidden();
        if (host->isVisible() != visible) {
            UiWorkTiming timing("group.host.visibility",host);
            host->setAttribute(Qt::WA_ShowWithoutActivating); host->setVisible(visible);
        }
    }
    void report(const QString &event) {
        auto record=snapshot(); record["native_id"]=host && host->testAttribute(Qt::WA_WState_Created) ? nativeWindowNumber(host) : 0;
        logEvent(event,record); if (changed) changed();
    }
    static void place(ManagedPanelSession *session, const QRect &frame)
    {
        if (!session || !session->window()) return; auto window = session->window();
        session->setFloatingGeometry(QRect(frame.topLeft() + window->geometry().topLeft() - window->frameGeometry().topLeft(),
            frame.size() - (window->frameGeometry().size() - window->size())));
    }
    GroupVisibility visibility;
    QHash<ManagedPanelSession *,bool> visibilityState;
    QHash<ManagedPanelSession *,QPointer<GroupPanelHeader>> headers;
    HiddenPanelCallers *hiddenCallers = nullptr;
    QPointer<GroupHostReserve> reserve;
    QPointer<WorkspaceFloatingHost> host;
    QPointer<QWidget> root, page;
    QList<QPointer<ManagedPanelSession>> members;
    QHash<ManagedPanelSession *, QPointer<QWidget>> leaves;
    QTimer timer;
    bool releasing = false, rebuilding = false, headerPressed = false, syncingVisibility = false;
    QPoint press, headerOffset;
    QPointer<QLabel> pressedHeader;
};
}
