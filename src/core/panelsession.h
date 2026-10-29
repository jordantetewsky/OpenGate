// SPDX-License-Identifier: MIT
#pragma once
#include "panelscope.h"
#include "compacttoolbar.h"
#include <QBoxLayout>
#include <QCloseEvent>
#include <QDynamicPropertyChangeEvent>
#include <QGridLayout>
#include <QLabel>
#include <QPointer>
#include <QSplitter>
#include <QStackedWidget>
#include <QTimer>
#include <QScopedValueRollback>
#include <QWidget>
#include <functional>
#include "diagnostics.h"
#include "viewport.h"
#include "floatinghost.h"
#include "panelwidth.h"
#include "paneloverflow.h"
#include "timelinetoolbarviewport.h"
#include "panelmetricscale.h"
#include "nativegroupslot.h"
#include "nativepanelposition.h"
#include "nativerootlayout.h"
#include "vacatedcontents.h"
#include "panelplacement.h"
#include "groupvisibility.h"
#include "cinemaviewer.h"
#include "uiworktiming.h"
#include "sourceplayerpresentation.h"

namespace Unbound {
// Managed native-content session. Each instance owns one host and preserves one slot.
class ManagedPanelSession : public QObject
{
public:
    explicit ManagedPanelSession(QObject *owner = nullptr) : QObject(owner)
    {
        arrangeTimer.setSingleShot(true);
        arrangeTimer.setInterval(180);
        connect(&arrangeTimer, &QTimer::timeout, this, [this] {
            if (constrain && window()) keepOnDisplay(window()->window());
            if (arrangementChanged) arrangementChanged();
        });
    }
    ~ManagedPanelSession() override { changed = {}; arrangementChanged = {}; dock(); }
    // active() is retained for compatibility: it means original content is leased.
    bool active() const { return !host.isNull(); }
    PanelPlacement placement() const {
        if (!active()) return PanelPlacement::NativeSlot;
        const auto surface=window();
        return surface && surface->window()==originalMain ? PanelPlacement::NativeSurface : PanelPlacement::FloatingSurface;
    }
    bool floating() const { return placement()==PanelPlacement::FloatingSurface; }
    bool dockPending() const { return pendingDock; }
    void resumePagePresentation() { queueRecovery(); }
    QWidget *window() const { return grouped() ? groupContainer.data() : host.data(); }
    std::function<void()> changed;
    std::function<void()> expandedClosed;
    std::function<void()> arrangementChanged;
    std::function<void()> releaseGroup;
    // Workspace bands belong to the outer host, never to a donated panel layout.
    std::function<QString()> beforeLayoutTransfer;
    std::function<QString()> releaseOriginalPosition;
    std::function<void(QWidget *, Qt::Edge)> attachRequested;
    QString panelTitle() const { return host ? host->property("unboundPanelTitle").toString() : QString(); }
    bool grouped() const { return !groupContainer.isNull(); }
    QRect floatingGeometry() const { return grouped() ? detachedGeometry : host ? (host->isMaximized()?host->normalGeometry():host->geometry()) : QRect(); }
    bool windowMaximized() const {return !grouped()&&host&&host->isMaximized();}
    void restoreWindowMaximized(bool value) {if(!grouped()&&host&&value)host->showMaximized();}
    QWidget *nativeOwnerWindow() const { return originalMain; }
    QWidget *pageWidget() const { return originalPage; }
    QWidget *nativeVacancy() const { return transferredLayout ? originalParent.data() : placeholder.data(); }
    bool adoptOriginalPosition(NativePanelPosition &position) {
        if (!active() || transferredLayout || !placeholder || content!=position.content() ||
            originalParent!=position.contentParking() || originalLayout || originalSplitter || originalStack) return false;
        return position.handoff(placeholder,[this](const NativePanelPosition::Destination &destination) {
            disconnect(parentDestroyed);
            originalParent=destination.parent; originalLayout=destination.layout;
            originalSplitter=destination.splitter; originalStack=destination.stack;
            splitterSizes=destination.sizes; stackIndex=destination.stackIndex;
            geometry=destination.geometry; flags=destination.flags; hidden=destination.hidden;
            parentDestroyed=connect(originalParent,&QObject::destroyed,this,[this] {
                if (originalPage) originalPage->removeEventFilter(this);
                if (host) host->deleteLater(); clear();
            });
            logEvent("panel.native-position.adopt",{{"panel",content ? content->objectName() : QString()},
                {"parent",originalParent ? originalParent->objectName() : QString()}});
        });
    }
    QString attachToGroup(QWidget *container, bool deferHostHide = false)
    {
        UiWorkTiming timing("panel.attach-to-group",host);
        const bool floatingContainer=container && container->window()->parentWidget()==originalMain;
        const bool nativeContainer=container && container->window()==originalMain && NativeRootLayout::contains(originalPage,container);
        if (!host || !host->layout() || grouped() || !container || container->layout() || (!floatingContainer && !nativeContainer))
            return QStringLiteral("Panel cannot join this group.");
        if (beforeLayoutTransfer) {
            const auto error=beforeLayoutTransfer();if(!error.isEmpty())return error;
        }
        metricScale.restore();
        detachedGeometry = host->geometry();
        QScopedValueRollback<bool> moving(recovering, true);
        {
            UiWorkTiming transfer("panel.group.layout-transfer",host);
            container->setLayout(host->layout());
        }
        if (host->layout() || !container->layout()) return QStringLiteral("Could not transfer panel layout into group.");
        groupContainer = container;
        if (relaxedWidth) groupWidth.apply(container);
        groupContainer->installEventFilter(this);
        if (!deferHostHide) {
            UiWorkTiming hide("panel.group.hide-old-host",host);
            host->hide();
        }
        refreshMetricScale();
        groupContainer->setVisible(!userHidden);
        reconcileOwnedSourcePresentation("join-group");
        return {};
    }
    void finishGroupTransfer()
    {
        if (!grouped() || !host) return;
        UiWorkTiming hide("panel.group.hide-old-host",host);
        host->hide();
        reconcileOwnedSourcePresentation("group-ready");
    }
    void detachFromGroup(bool deferHostShow = false)
    {
        endGroupedExpansion();
        if (!grouped()) return;
        if (!host) return;
        QScopedValueRollback<bool> moving(recovering, true);
        metricScale.restore();
        groupWidth.restore();
        if (groupContainer->layout()) host->setLayout(groupContainer->layout());
        groupContainer->removeEventFilter(this);
        groupContainer = nullptr;
        refreshMetricScale();
        static_cast<WorkspaceFloatingHost *>(host.data())->setChrome(chrome);
        setFloatingGeometry(detachedGeometry);
        if (!deferHostShow) host->setVisible(!userHidden && originalPage && originalPage->isVisible());
        reconcileOwnedSourcePresentation("leave-group");
    }
    QWidget *expandedWindow() const { return expandedHost; }
    bool beginGroupedExpansion()
    {
        if (!grouped() || expandedHost || !groupContainer->layout()) return false;
        QScopedValueRollback<bool> moving(recovering, true);
        metricScale.restore();
        expandedHost = new WorkspaceFloatingHost(originalMain);
        expandedHost->setObjectName(QStringLiteral("unboundFloating_expanded_") + groupContainer->objectName());
        expandedHost->setWindowTitle(groupContainer->windowTitle());
        expandedHost->setAttribute(Qt::WA_QuitOnClose, false);
        expandedHost->setLayout(groupContainer->layout());
        static_cast<WorkspaceFloatingHost *>(expandedHost.data())->setChrome(chrome);
        expandedHost->setGeometry(QRect(groupContainer->mapToGlobal(QPoint()), groupContainer->size()));
        refreshMetricScale();
        expandedHost->installEventFilter(this); expandedHost->show(); return true;
    }
    void endGroupedExpansion()
    {
        if (!expandedHost) return;
        metricScale.restore();
        QScopedValueRollback<bool> moving(recovering, true);
        if (groupContainer && expandedHost->layout()) groupContainer->setLayout(expandedHost->layout());
        expandedHost->removeEventFilter(this); expandedHost->hide(); expandedHost->deleteLater(); expandedHost = nullptr;
        refreshMetricScale();
    }
    void setChrome(WindowChrome value) { chrome = value; if (host && !grouped()) static_cast<WorkspaceFloatingHost *>(host.data())->setChrome(value); }
    bool pinned() const { return host && host->property("unboundPinned").toBool(); }
    void setPinned(bool value) { if (host) static_cast<WorkspaceFloatingHost *>(host.data())->setPinned(value); }
    void setSnapping(bool enabled) { snapping = enabled; if (host) static_cast<WorkspaceFloatingHost *>(host.data())->setSnapping(enabled); }
    double interfaceScale() const { return metricFactor; }
    bool setInterfaceScale(double value) {
        if (!qIsFinite(value) || value<0.7 || value>3.0) return false;
        metricFactor=value; refreshMetricScale();
        if (arrangementChanged) arrangementChanged();
        return true;
    }
    bool hiddenByUser() const { return userHidden; }
    QPoint overflowOffset() const { return overflow.offset(); }
    void restoreOverflowOffset(QPoint position) { overflow.restoreOffset(position); }
    void setConstrained(bool enabled) { constrain = enabled; if (enabled && window()) keepOnDisplay(window()->window()); }
    void bringOnScreen() { if (window()) keepOnDisplay(window()->window()); if (arrangementChanged) arrangementChanged(); }
    void centerOnCurrentDisplay()
    {
        if (!window()) return;
        setHiddenByUser(false);
        auto top = window()->window(); top->showNormal();
        auto screen = QGuiApplication::screenAt(QCursor::pos()); if (!screen) screen = QGuiApplication::primaryScreen();
        if (!screen) return;
        QRect frame(QPoint(), top->frameGeometry().size().boundedTo(screen->availableGeometry().size()));
        frame.moveCenter(screen->availableGeometry().center());
        const auto decoration = top->frameGeometry().size() - top->size();
        top->resize(frame.size() - decoration); top->move(frame.topLeft()); keepOnDisplay(top);
        top->raise(); top->activateWindow();
        logEvent(QStringLiteral("panel.center"), {{"host", widgetState(top)}});
        if (arrangementChanged) arrangementChanged();
    }
    void setFloatingGeometry(const QRect &r) { if (grouped()) { if (r.isValid()) detachedGeometry = r; } else if (host && r.isValid()) { host->setGeometry(r); if (constrain) keepOnDisplay(host); } }
    void setHiddenByUser(bool hide, bool activate = true)
    {
        userHidden = hide;
        if (host) {
            auto surface = expandedHost ? expandedHost.data() : window();
            if (hide) surface->hide();
            else if (originalPage && originalPage->isVisible()) {
                if (activate) {
                    auto window = surface->window();
                    if (!window->isVisible() || window->isMinimized()) window->showNormal();
                    surface->show();
                    // A door changes space inside its group; it must not move
                    // the group's outer frame or repeatedly reactivate it.
                    if (!grouped()) keepOnDisplay(window);
                    if (QApplication::activeWindow() != window) { window->raise(); window->activateWindow(); }
                } else {
                    // Viewer-mode commands change presentation without stealing
                    // editing focus or reordering an already-visible group.
                    auto window = surface->window();
                    const bool prior = window->testAttribute(Qt::WA_ShowWithoutActivating);
                    window->setAttribute(Qt::WA_ShowWithoutActivating);
                    window->show(); surface->show();
                    window->setAttribute(Qt::WA_ShowWithoutActivating, prior);
                }
            }
        }
        if (!hide) reconcileOwnedSourcePresentation("door");
        auto actualSurface=window(); auto actualHost=actualSurface ? actualSurface->window() : nullptr;
        logEvent(hide ? QStringLiteral("panel.hide") : QStringLiteral("panel.show"), {{"widget", widgetState(host)},
            {"placement",panelPlacementName(placement())},{"stage","session-state"},{"requested_hidden",userHidden},
            {"actual_surface",actualSurface ? actualSurface->objectName() : QString()},
            {"surface_visible",actualSurface && actualSurface->isVisible()},{"actual_host",actualHost ? actualHost->objectName() : QString()},
            {"native_id",actualHost && actualHost->testAttribute(Qt::WA_WState_Created) ? nativeWindowNumber(actualHost) : 0},
            {"host_pinned",actualHost && actualHost->property("unboundPinned").toBool()}});
        if (changed) changed();
        if (arrangementChanged) arrangementChanged();
    }

    QString undockContents(QWidget *slot, QWidget *page, const QString &title,QWidget *managedOwner=nullptr)
    {
        if (active()) return QStringLiteral("This panel is already floating.");
        if (!panelInManagedScope(slot,page,managedOwner) || (!slot->isVisible() && !NativeRootLayout::ownsParkedWidget(slot,page)) || !slot->layout())
            return QStringLiteral("Show this panel on the Edit page first.");
        originalParent = slot;
        originalPage = page;
        originalMain = page->window();
        originalLayout = slot->layout();
        ++generation;
        host = new WorkspaceFloatingHost(originalMain);
        static_cast<WorkspaceFloatingHost *>(host.data())->attachRequested = [this](QWidget *target, Qt::Edge edge) { if (attachRequested) attachRequested(target, edge); };
        static_cast<WorkspaceFloatingHost *>(host.data())->setSnapping(snapping);
        host->setAttribute(Qt::WA_QuitOnClose, false);
        host->setObjectName(QStringLiteral("unboundFloating_") + slot->objectName());
        host->setWindowTitle(title); host->setProperty("unboundPanelTitle",title);
        host->resize(slot->size().expandedTo(QSize(320, 240)));
        transferredSurface=new QWidget(host);
        transferredSurface->setObjectName(slot->objectName());
        transferredSurface->setMinimumSize(slot->minimumSize());
        transferredSurface->setSizePolicy(slot->sizePolicy());
        transferredSurface->setLayout(originalLayout);
        if (slot->layout() || transferredSurface->layout() != originalLayout) {
            if (!slot->layout() && transferredSurface->layout() == originalLayout) slot->setLayout(originalLayout);
            delete host.data(); clear(); return QStringLiteral("Could not transfer this panel's content layout.");
        }
        transferredLayout = true;
        auto presentation=new QVBoxLayout(host); presentation->setContentsMargins(0,0,0,0);
        presentation->addWidget(overflow.wrap(transferredSurface,host));
        relaxedWidth = true;
        host->setProperty("unboundRelaxedWidth",true);
        placeholderLayout = new QVBoxLayout(slot);
        auto marker = new QLabel(title + QStringLiteral(" is floating."), slot);
        marker->setAlignment(Qt::AlignCenter);
        placeholder = marker;
        placeholderLayout->addWidget(marker);
        vacatedContents.hide(slot);
        host->installEventFilter(this);
        page->installEventFilter(this);
        parentDestroyed = connect(slot, &QObject::destroyed, this, [this] {
            if (originalPage) originalPage->removeEventFilter(this);
            if (host) host->deleteLater();
            clear();
        });
        host->show();
        static_cast<WorkspaceFloatingHost *>(host.data())->setChrome(chrome);
        if (constrain) keepOnDisplay(host);
        refreshMetricScale();
        logEvent(QStringLiteral("panel.contents.undock"), {{"panel", title}, {"slot", widgetState(slot)}});
        if (changed) changed();
        return {};
    }

    QString undock(QWidget *widget, QWidget *page, const QString &title, bool retainHiddenSurface = false,bool deferHostShow = false,QWidget *managedOwner=nullptr)
    {
        logEvent(QStringLiteral("panel.undock.request"), {{"panel", title}, {"widget", widgetState(widget)}});
        if (active()) return QStringLiteral("This panel is already floating.");
        if (!panelInManagedScope(widget,page,managedOwner) ||
            (!widget->isVisible() && !NativeRootLayout::ownsParkedWidget(widget,page) && !(retainHiddenSurface && page->isVisible())) || widget->isWindow())
            return QStringLiteral("Show this panel on the Edit page first.");
        QWidget *parent = widget->parentWidget();
        auto splitter = qobject_cast<QSplitter *>(parent);
        auto stack = qobject_cast<QStackedWidget *>(parent);
        QLayout *layout = parent->layout();
        if (splitter || stack) layout = nullptr;
        if (layout && !qobject_cast<QBoxLayout *>(layout) && !qobject_cast<QGridLayout *>(layout))
            return QStringLiteral("This panel's layout is not supported yet.");
        if ((layout && layout->indexOf(widget) < 0) || (splitter && splitter->indexOf(widget) < 0))
            return QStringLiteral("The panel is not in its expected layout.");
        timelineToolbar.attach(widget);

        content = widget;
        originalParent = NativeRootLayout::logicalParent(page,parent);
        originalLayout = layout;
        originalSplitter = splitter;
        originalStack = stack;
        if (stack) stackIndex = stack->indexOf(widget);
        originalPage = page;
        originalMain = page->window();
        geometry = widget->geometry();
        flags = widget->windowFlags();
        hidden = widget->isHidden();
        if (splitter) splitterSizes = splitter->sizes();
        // QSplitter automatically inserts new children; replaceWidget needs an outsider.
        auto marker = new NativePanelSlot((splitter || stack) ? nullptr : parent);
        // Retain a reversible slot identity, not the detached panel's footprint.
        // Hidden placeholders do not reserve splitter/layout space.
        marker->setObjectName("unboundVacatedSlot_" + widget->objectName());
        marker->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Ignored);
        marker->setMinimumSize(0,0);
        placeholder = marker;
        if (stack) {
            const bool selected = stack->currentWidget() == widget;
            stack->removeWidget(widget);
            stack->insertWidget(stackIndex, marker);
            if (selected) stack->setCurrentWidget(marker);
        } else if (layout) {
            auto item = layout->replaceWidget(widget, marker, Qt::FindDirectChildrenOnly);
            if (!item) { delete marker; clear(); return QStringLiteral("Could not preserve the panel slot."); }
            delete item;
        } else if (splitter) {
            const int index = splitter->indexOf(widget);
            if (splitter->replaceWidget(index, marker) != widget) {
                delete marker; clear(); return QStringLiteral("Could not preserve the splitter slot.");
            }
            splitter->setSizes(splitterSizes);
        } else {
            marker->setGeometry(geometry);
        }
        ++generation;
        host = new WorkspaceFloatingHost(originalMain);
        static_cast<WorkspaceFloatingHost *>(host.data())->attachRequested = [this](QWidget *target, Qt::Edge edge) { if (attachRequested) attachRequested(target, edge); };
        static_cast<WorkspaceFloatingHost *>(host.data())->setSnapping(snapping);
        host->setAttribute(Qt::WA_QuitOnClose, false);
        host->setWindowTitle(title); host->setProperty("unboundPanelTitle",title);
        host->setObjectName(QStringLiteral("unboundFloating_") + widget->objectName());
        host->resize(widget->size().expandedTo(QSize(320, 240)));
        auto box = new QVBoxLayout(host);
        box->setContentsMargins(0, 0, 0, 0);
        const bool scrollInspector = widget->objectName()==QLatin1String("widgetEditInspector");
        const bool scrollToolbar=widget->objectName()==QLatin1String("frameEditToolbarTop") ||
            widget->findChild<QWidget *>("pageSingleScreenModeTools");
        const bool scrollMediaPool=widget->objectName()==QLatin1String("frameEditPageMediaPool");
        // Diagnostic comparison against public v4: retain the native viewer's
        // direct presentation parent, as before 50f7498. Other overflow paths,
        // native minima, ownership/recovery and command behavior are unchanged.
        // This is not evidence that native footage loading has been repaired.
        const bool scrollContent=scrollInspector || scrollToolbar || scrollMediaPool;
        box->addWidget(scrollContent ? overflow.wrap(widget,host,scrollToolbar) : widget);
        relaxedWidth = widget->objectName() == QLatin1String("widgetEditInspector") ||
            widget->objectName() == QLatin1String("frameEditPageMediaPool");
        // Relax the viewport, never the native controls inside it. Ignoring the
        // Media Pool's minimum width makes its toolbar overlap before it scrolls.
        if (relaxedWidth && !scrollContent) widthAllowance.apply(widget);
        if (scrollContent) host->setProperty("unboundRelaxedWidth",true);
        // A detached tools row must not leave a second empty tools row in Timeline.
        marker->hide();
        logEvent("workspace.native-slot.vacated", {{"panel",widget->objectName()},
            {"parent",parent->objectName()},{"placeholder_hidden",marker->isHidden()},
            {"main_min_width",originalMain->minimumWidth()},{"main_min_height",originalMain->minimumHeight()}});
        widget->show();
        widget->installEventFilter(this);
        host->installEventFilter(this);
        page->installEventFilter(this);
        parentDestroyed = connect(originalParent, &QObject::destroyed, this, [this] {
            // The original owner is gone; its displaced content ends with it.
            if (originalPage) originalPage->removeEventFilter(this);
            if (host) host->deleteLater();
            clear();
        });
        if (widget->findChild<QWidget *>("pageSingleScreenModeTools")) host->setProperty("unboundToolbarStrip",true);
        if (!deferHostShow) host->show();
        static_cast<WorkspaceFloatingHost *>(host.data())->setChrome(chrome);
        if (widget->findChild<QWidget *>("pageSingleScreenModeTools")) new CompactToolbar(host,widget);
        if (constrain) keepOnDisplay(host);
        refreshMetricScale();
        reconcileSourcePlayerPresentation(content,"undock");
        logEvent(QStringLiteral("panel.undock.complete"), {{"panel", title}, {"widget", widgetState(content)},
            {"original_parent", originalParent->objectName()},
            {"layout", originalLayout ? originalLayout->metaObject()->className() : "none"}});
        if (changed) changed();
        return {};
    }

    QString dock()
    {
        if (releaseOriginalPosition) {
            const auto release=releaseOriginalPosition; const auto error=release();
            if (!error.isEmpty()) return error;
        }
        endGroupedExpansion();
        if (releaseGroup) { auto release = releaseGroup; release(); }
        logEvent(QStringLiteral("panel.dock.request"), {{"widget", widgetState(content)},
            {"active", active()}, {"original_parent", originalParent ? originalParent->objectName() : QString()}});
        if (!active()) return QStringLiteral("This panel is already docked.");
        if (!transferredLayout && content && originalPage && !originalPage->isVisible() &&
            content->parentWidget() != host && content->parentWidget() != originalParent && nativeOwner(content->parentWidget())) {
            pendingDock = true;
            logEvent(QStringLiteral("panel.dock.deferred"), {{"widget", widgetState(content)}});
            if (changed) changed();
            return {};
        }
        QScopedValueRollback<bool> restoring(docking, true);
        if (transferredLayout) {
            if (!originalParent || !originalLayout || !placeholderLayout ||
                originalParent->layout() != placeholderLayout || !transferredSurface || transferredSurface->layout() != originalLayout)
                return QStringLiteral("Resolve changed this panel's content layout.");
            metricScale.restore();
            widthAllowance.restore();
            delete placeholderLayout.data();
            delete placeholder.data();
            originalParent->setLayout(originalLayout);
            if (originalParent->layout() != originalLayout)
                return QStringLiteral("Could not restore this panel's content layout.");
            // This replacement no longer represents native content. Discovery
            // can run again before deferred host deletion (nested Mixer is one
            // such caller); only the restored native slot may keep its name.
            transferredSurface->setObjectName(QString());
            originalLayout->activate();
            vacatedContents.restore();
            disconnect(parentDestroyed);
            if (originalPage) originalPage->removeEventFilter(this);
            auto oldHost = host.data();
            oldHost->removeEventFilter(this);
            logEvent(QStringLiteral("panel.contents.dock"), {{"slot", widgetState(originalParent)}});
            clear(); oldHost->hide(); oldHost->deleteLater();
            if (changed) changed();
            return {};
        }
        if (!content || !originalParent || !placeholder)
            return QStringLiteral("Resolve changed the panel hierarchy; restoration was stopped.");
        auto destinationParent=NativeRootLayout::restorationParent(originalPage,originalParent,originalLayout);
        if (content->parentWidget() != host && content->parentWidget() != originalParent && !overflow.owns(content) && !nativeOwner(content->parentWidget()))
            return QStringLiteral("This panel belongs to an unexpected window; restoration was stopped.");
        if (overflow.owns(content)) {
            // Validate the destination before removing the retained viewport.
            if ((originalStack && originalStack->indexOf(placeholder)<0) ||
                (originalSplitter && originalSplitter->indexOf(placeholder)<0) ||
                (destinationParent->layout() && !originalStack && !originalSplitter && destinationParent->layout()->indexOf(placeholder)<0))
                return QStringLiteral("The original panel slot changed.");
            metricScale.restore();
            overflow.releaseInto(host);
        }
        metricScale.restore();
        if(!timelineToolbar.restore())return QStringLiteral("Could not restore the attached Timeline toolbar safely.");
        const bool reclaimed = content->parentWidget() == destinationParent;
        QLayout *currentLayout = destinationParent->layout();
        if (reclaimed && (!currentLayout || currentLayout->indexOf(content) >= 0 || originalStack || originalSplitter)) {
            if (currentLayout) currentLayout->removeWidget(placeholder);
            logEvent(QStringLiteral("panel.dock.native-reclaimed"), {{"widget", widgetState(content)}});
        } else if (originalStack) {
            if (originalStack->indexOf(placeholder) < 0)
                return QStringLiteral("The original viewer slot changed.");
            const bool selected = originalStack->currentWidget() == placeholder;
            originalStack->removeWidget(placeholder);
            originalStack->insertWidget(qMin(stackIndex, originalStack->count()), content);
            if (selected) originalStack->setCurrentWidget(content);
        } else if (originalSplitter) {
            const int index = originalSplitter->indexOf(placeholder);
            if (index < 0 || originalSplitter->replaceWidget(index, content) != placeholder)
                return QStringLiteral("The original splitter slot changed.");
            originalSplitter->setSizes(splitterSizes);
        } else if (currentLayout) {
            if (currentLayout->indexOf(placeholder) < 0)
                return QStringLiteral("The original layout changed.");
            auto item = currentLayout->replaceWidget(placeholder, content, Qt::FindDirectChildrenOnly);
            if (!item) return QStringLiteral("Could not restore the panel slot.");
            delete item;
            content->setParent(destinationParent, flags);
        } else {
            if (destinationParent->layout()) return QStringLiteral("Resolve added a layout to this panel slot.");
            content->setParent(destinationParent, flags);
        }
        if (content->parentWidget() != destinationParent || content->isWindow())
            return QStringLiteral("Resolve did not retain the restored panel in its original parent.");
        widthAllowance.restore();
        content->setGeometry(geometry);
        content->setVisible(!hidden && (!originalStack || originalStack->currentWidget() == content));
        if (currentLayout) currentLayout->activate();
        // replaceWidget inherits the hidden vacancy's state. Apply proportions
        // after showing the restored pane, otherwise QSplitter assigns it zero.
        if (originalSplitter) originalSplitter->setSizes(splitterSizes);
        disconnect(parentDestroyed);
        if (originalPage) originalPage->removeEventFilter(this);
        auto oldHost = host.data();
        oldHost->removeEventFilter(this);
        delete placeholder.data();
        const auto restored = content;
        const QPointer<QWidget> destination = destinationParent;
        const auto nativeDestination=originalParent;
        const auto nativeDestinationLayout=originalLayout;
        const bool retainedNativeSlot=destinationParent!=originalParent;
        logEvent(QStringLiteral("panel.dock.complete"), {{"widget", widgetState(content)}});
        clear();
        oldHost->hide();
        oldHost->deleteLater();
        QTimer::singleShot(0, this, [restored, destination,nativeDestination,nativeDestinationLayout,retainedNativeSlot] {
            const bool nativeRootRestored=retainedNativeSlot && nativeDestination && nativeDestinationLayout &&
                nativeDestination->layout()==nativeDestinationLayout && restored && restored->parentWidget()==nativeDestination;
            logEvent(QStringLiteral("panel.dock.verify"), {{"widget", widgetState(restored)},
                {"native_root_restored",nativeRootRestored},
                {"passed", restored && !restored->isWindow() && ((destination && restored->parentWidget()==destination) || nativeRootRestored)}});
        });
        if (changed) changed();
        return {};
    }
protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (event->type() == QEvent::Close && (object == expandedHost ||
            (object == host && host->property("unboundExpanded").toBool()))) {
            // Temporary expansion returns to its previous placement, including
            // the original native slot. It is not an ordinary visibility door.
            if (expandedClosed) expandedClosed();
            else if (expandedHost) endGroupedExpansion();
            else static_cast<QCloseEvent *>(event)->setAccepted(dock().isEmpty());
            return true;
        }
        if (object==host && event->type()==QEvent::DynamicPropertyChange &&
            static_cast<QDynamicPropertyChangeEvent *>(event)->propertyName()=="unboundPinned" && arrangementChanged)
            arrangementChanged();
        if (object == host && event->type() == QEvent::Close) {
            setHiddenByUser(true);
            logEvent(QStringLiteral("panel.close.result"), {{"success", true}, {"action", "hide"}});
            static_cast<QCloseEvent *>(event)->accept();
            return true;
        }
        if ((object == host || object == groupContainer) && (event->type() == QEvent::Move || event->type() == QEvent::Resize)) arrangeTimer.start();
        if (object == content && !docking && !recovering &&
            (event->type() == QEvent::Hide || event->type() == QEvent::ParentChange)) {
            logEvent(QStringLiteral("panel.native-change"), {{"widget", widgetState(content)},
                {"event_type", int(event->type())}, {"page_visible", originalPage && originalPage->isVisible()}});
            queueRecovery();
        }
        if (object == originalPage && host) {
            if (event->type() == QEvent::Hide) {
                logEvent(QStringLiteral("panel.page.leave"), {{"host", host->objectName()}});
                host->hide();
                if (grouped()) groupContainer->hide();
            }
            if (event->type() == QEvent::Show) {
                logEvent(QStringLiteral("panel.page.return"), {{"host", host->objectName()}});
                queueRecovery();
            }
        }
        return false;
    }
private:
    void reconcileOwnedSourcePresentation(const QString &boundary)
    {
        // A native callback can explicitly hide the outer Source stack while
        // its layout is being transferred (recovery is intentionally suspended
        // then). Showing the destination cell does not clear that hidden bit.
        // Repair only our leased content at completed ownership/door boundaries;
        // never reveal a hidden host, select a tab or borrow Cinema content.
        auto surface=expandedHost ? expandedHost.data() : window();
        if (!userHidden && content && surface && surface->isAncestorOf(content) &&
            content->objectName()=="stackedEditSourceOfflinePlayers" && !visibleCinemaViewer())
            content->show();
        reconcileSourcePlayerPresentation(content,boundary);
    }
    bool nativeOwner(QWidget *parent) const
    {
        return parent && originalMain && parent->window() == originalMain;
    }
    void queueRecovery()
    {
        if (recoveryQueued || docking || !host || expandedHost) return;
        recoveryQueued = true;
        const unsigned token = generation;
        QTimer::singleShot(0, this, [this, token] {
            if (token != generation) return;
            recoveryQueued = false;
            if (!host || !originalPage || !originalPage->isVisible() || docking) return;
            // Cinema Viewer temporarily borrows/hides native player content.
            // Reclaiming it during that command interrupts Resolve's presentation.
            if (visibleCinemaViewer()) {
                QTimer::singleShot(100, this, [this, token] { if (token == generation) queueRecovery(); });
                return;
            }
            if (pendingDock) {
                const QString error = dock();
                logEvent(QStringLiteral("panel.dock.deferred-result"), {{"success", error.isEmpty()}, {"error", error}});
                return;
            }
            QScopedValueRollback<bool> recovery(recovering, true);
            QWidget *surface = window();
            if(transferredLayout && surface==host && (!surface->layout() || surface->layout()->count()==0)) {
                logEvent("panel.recover.deferred",{{"reason","empty-transfer-host"},{"host",host->objectName()}});return;
            }
            if (!transferredLayout && content) {
                if (content->parentWidget() != surface && !overflow.owns(content)) {
                    if (!nativeOwner(content->parentWidget())) {
                        logEvent(QStringLiteral("panel.recover.rejected"), {{"widget", widgetState(content)}}); return;
                    }
                    if (!overflow.reclaim(content)) surface->layout()->addWidget(content);
                }
                content->show();
            }
            if (!userHidden && !surface->isVisible() && !panelCoveredByTab(surface)) {
                const bool previous = surface->testAttribute(Qt::WA_ShowWithoutActivating);
                surface->setAttribute(Qt::WA_ShowWithoutActivating, true);
                surface->show(); surface->setAttribute(Qt::WA_ShowWithoutActivating, previous);
            }
            reconcileSourcePlayerPresentation(content,"native-recovery");
            logEvent(QStringLiteral("panel.recover.complete"), {{"widget", widgetState(content)}, {"host", widgetState(host)}});
        });
    }
    void clear()
    {
        releaseOriginalPosition={};
        if (releaseGroup) { auto release = releaseGroup; release(); }
        metricScale.restore();
        timelineToolbar.restore();
        groupWidth.restore(); widthAllowance.restore(); vacatedContents.restore(); relaxedWidth = false;
        overflow.forget();
        ++generation;
        if (content) content->removeEventFilter(this);
        host = nullptr; content = nullptr; placeholder = nullptr;
        groupContainer = nullptr;
        originalParent = nullptr; originalLayout = nullptr;
        originalSplitter = nullptr; originalPage = nullptr;
        originalStack = nullptr;
        originalMain = nullptr;
        splitterSizes.clear();
        transferredLayout = false;
        transferredSurface = nullptr;
        placeholderLayout = nullptr;
        userHidden = false;
        arrangeTimer.stop();
        recoveryQueued = false; pendingDock = false;
    }
    QPointer<QWidget> host, content, placeholder, originalParent, originalPage;
    PanelWidthAllowance widthAllowance, groupWidth;
    void refreshMetricScale() {
        if (!active() || qFuzzyCompare(metricFactor,1.0)) { metricScale.restore(); return; }
        auto target=content ? content.data() : expandedHost ? expandedHost.data() : window();
        metricScale.attach(target,metricFactor);
    }
    PanelMetricScale metricScale;
    double metricFactor=1.0;
    PanelOverflow overflow;
    TimelineToolbarViewport timelineToolbar;
    VacatedContents vacatedContents;
    bool relaxedWidth = false;
    QPointer<QWidget> originalMain;
    QPointer<QLayout> originalLayout;
    QPointer<QSplitter> originalSplitter;
    QPointer<QStackedWidget> originalStack;
    QPointer<QLayout> placeholderLayout;
    bool transferredLayout = false;
    QPointer<QWidget> transferredSurface;
    bool userHidden = false;
    bool constrain = true;
    bool snapping = true;
    QPointer<QWidget> groupContainer, expandedHost;
    QRect detachedGeometry;
    QTimer arrangeTimer;
    WindowChrome chrome = WindowChrome::Standard;
    int stackIndex = -1;
    QMetaObject::Connection parentDestroyed;
    QList<int> splitterSizes;
    QRect geometry;
    Qt::WindowFlags flags;
    bool hidden = false;
    bool docking = false, recovering = false, recoveryQueued = false, pendingDock = false;
    unsigned generation = 0;
};
}
