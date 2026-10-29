// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QToolButton>
#include <QPainter>
#include "../platform/workspacewindowintegration.h"
#include "workspaceframepolicy.h"
#include "dockoverlay.h"
#include "droppolicy.h"
#include "dockdestination.h"
#include "dockfeedback.h"
#include "dragtiming.h"
#include "draggeometrytrace.h"
#include "uiworktiming.h"
#include "diagnostics.h"
#include <QScreen>
#include <QPointer>
#include <QTimer>
#include <QCursor>
#include <functional>

namespace Unbound {
enum class WindowChrome { Standard, Compact, Hidden };

inline QRect resizeFromEdge(const QRect &original, QPoint delta, Qt::Edges edges, QSize minimum, QSize maximum)
{
    QRect result = original;
    int width = original.width(), height = original.height();
    if (edges & Qt::LeftEdge) width -= delta.x();
    if (edges & Qt::RightEdge) width += delta.x();
    if (edges & Qt::TopEdge) height -= delta.y();
    if (edges & Qt::BottomEdge) height += delta.y();
    width = qBound(minimum.width(), width, maximum.width());
    height = qBound(minimum.height(), height, maximum.height());
    result.setSize(QSize(width, height));
    if (edges & Qt::LeftEdge) result.moveRight(original.right());
    if (edges & Qt::TopEdge) result.moveBottom(original.bottom());
    return result;
}

class PinLight : public QToolButton
{
public:
    explicit PinLight(QWidget *owner) : QToolButton(owner) { setCheckable(true); setFocusPolicy(Qt::NoFocus); }
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(isChecked() ? QColor("#59a6c7") : QColor("#929292")); p.setPen(Qt::NoPen); p.drawEllipse(rect().adjusted(1,1,-1,-1));
        p.setPen(QPen(Qt::black, 1)); const QPoint c = rect().center();
        p.drawLine(c + QPoint(-3,-3), c + QPoint(3,-3)); p.drawLine(c + QPoint(-2,-3), c + QPoint(-2,0));
        p.drawLine(c + QPoint(2,-3), c + QPoint(2,0)); p.drawLine(c + QPoint(-4,1), c + QPoint(4,1)); p.drawLine(c, c + QPoint(0,5));
    }
};

class WorkspaceFloatingHost : public WorkspaceHostWindow
{
public:
    explicit WorkspaceFloatingHost(QWidget *owner = nullptr) : WorkspaceHostWindow(owner, owner ? Qt::Tool : Qt::Window)
    {
#ifdef UNBOUND_NATIVE_TESTS
        if(qEnvironmentVariableIntValue("UNBOUND_TEST_HIDDEN_HOSTS"))setAttribute(Qt::WA_DontShowOnScreen);
#endif
        qApp->installEventFilter(this);
        setMouseTracking(true);
        setAutoFillBackground(true);
        pinButton = new PinLight(this); pinButton->setObjectName("unboundPinWindow");
        pinButton->setCheckable(true); pinButton->setAutoRaise(true);
        pinButton->setFocusPolicy(Qt::NoFocus); pinButton->setToolTip("Keep this window above the Resolve workspace");
        connect(pinButton, &QToolButton::toggled, this, [this](bool pinned) {
            setProperty("unboundPinned", pinned); pinButton->update(); updatePin();
            logEvent(QStringLiteral("window.pin"), {{"window", objectName()}, {"pinned", pinned},
                {"native_id",testAttribute(Qt::WA_WState_Created) ? nativeWindowNumber(this) : 0}});
        });
    }
    void setPinned(bool pinned) { pinButton->setChecked(pinned); }
    bool pinned() const { return pinButton->isChecked(); }
    QSize minimumSizeHint() const override
    {
        auto hint = QWidget::minimumSizeHint();
        if (property("unboundRelaxedWidth").toBool()) hint.setWidth(240);
        return hint;
    }
    ~WorkspaceFloatingHost() override { delete nativeMoveWatch.data(); delete releaseWatch.data(); if (qApp) qApp->removeEventFilter(this); }
    void setChrome(WindowChrome value)
    {
        if (chromeInitialized && chrome == value) { updatePin(); return; }
        UiWorkTiming timing("window.chrome",this);
        chromeInitialized = true;
        const bool visible = isVisible();
        const QRect position = geometry();
        chrome = value;
        setWindowFlag(Qt::FramelessWindowHint, value == WindowChrome::Hidden);
        const bool strip=property("unboundToolbarStrip").toBool();
        const int top = strip ? 0 : value == WindowChrome::Compact ? 28 : value == WindowChrome::Hidden ? 24 : 0;
        setContentsMargins(strip ? 110 : 0, top, strip ? 90 : 0, 0);
        if (visible) show();
        {
            UiWorkTiming native("window.chrome.native",this);
            applyNativeChrome(this, value == WindowChrome::Compact || strip);
        }
        if (position.isValid()) setGeometry(position);
        {
            UiWorkTiming pin("window.chrome.pin",this);
            updatePin();
        }
        update();
        delete nativeMoveWatch.data();
        nativeMoveWatch = watchNativeWindowMove(this, [this] {
            if (dragging) return;
            dragTiming.begin(); geometryTrace.begin();
            ++dragGeneration; nativeDragging = dragging = movedEnough = true; startGeometry = geometry();
            delete releaseWatch.data();
            releaseWatch = watchNativePointerRelease(this, [this](QPoint position, bool bypass) {
                if (nativeDragging) { dragPointer = position; previewSnap(geometry(), bypass); finishPointerDrag(bypass, false, "native-titlebar-mouse-up"); }
            });
            logEvent(QStringLiteral("drag.begin"), {{"window", objectName()}, {"native_titlebar", true}});
        }, [this](QPoint position) {
            if (nativeDragging) { dragPointer = position; previewSnap(geometry(), QApplication::keyboardModifiers().testFlag(Qt::AltModifier)); }
        }, [this](QPoint position, bool bypass) {
            if (nativeDragging) { dragPointer = position; previewSnap(geometry(), bypass); finishPointerDrag(bypass, false, "native-titlebar-release"); }
        });
    }
    void setSnapping(bool enabled) { snapping = enabled; if (!enabled) { attachmentTarget = nullptr; snapPosition = {}; hidePreviews(); } }
    std::function<void(QWidget *, Qt::Edge)> attachRequested;
    void beginPointerDrag(const QPoint &global, bool continuing = false)
    {
        if (dragging) return; // Native and custom events may report the same gesture.
        snapPosition = {}; attachmentTarget = nullptr;
        dragTiming.begin(); geometryTrace.begin();
        ++dragGeneration;
        dragging = true; movedEnough = continuing; start = global; startGeometry = geometry();
        delete releaseWatch.data();
        releaseWatch = watchNativePointerRelease(this, [this](QPoint position, bool option) {
            if (dragging) { movePointerDrag(position, option); finishPointerDrag(option, false, "native-mouse-up"); }
        });
        logEvent(QStringLiteral("drag.begin"), {{"window", objectName()}, {"start_x",start.x()}, {"start_y",start.y()},
            {"geometry_x",startGeometry.x()}, {"geometry_y",startGeometry.y()}});
        // Native capture can deactivate the window synchronously while capture is acquired.
        grabMouse();
        if (!dragging && QWidget::mouseGrabber() == this) {
            releaseMouse();
            logEvent(QStringLiteral("drag.capture.recovered"), {{"window", objectName()}, {"reason", "cancelled-during-grab"}});
        }
    }
protected:
    void changeEvent(QEvent *event) override
    {
        QWidget::changeEvent(event);
        if (chromeInitialized && event->type() == QEvent::WindowTitleChange) updatePin();
    }
    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        if (pinButton) updatePin();
    }
    void hideEvent(QHideEvent *event) override
    {
        if (dragging) finishPointerDrag(true, true, "hidden");
        QWidget::hideEvent(event);
    }
    void paintEvent(QPaintEvent *event) override
    {
        QWidget::paintEvent(event);
        QPainter painter(this);
        painter.setPen(palette().color(QPalette::WindowText));
        if (chrome == WindowChrome::Hidden)
            painter.drawText(QRect(0, 0, width(), 12), Qt::AlignCenter, QStringLiteral("···"));
    }
    bool eventFilter(QObject *object, QEvent *event) override
    {
        const auto type = event->type();
        if (type != QEvent::MouseButtonPress && type != QEvent::MouseButtonRelease && type != QEvent::MouseMove &&
            type != QEvent::KeyPress && type != QEvent::ApplicationDeactivate && type != QEvent::WindowDeactivate) return false;
        if (!dragging && QWidget::mouseGrabber() == this &&
            (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease ||
             event->type() == QEvent::MouseMove || event->type() == QEvent::KeyPress)) {
            releaseMouse();
            logEvent(QStringLiteral("drag.capture.recovered"), {{"window", objectName()}, {"reason", "orphaned-capture"}});
        }
        if (dragging && (event->type() == QEvent::ApplicationDeactivate || event->type() == QEvent::WindowDeactivate)) {
            if (event->type() == QEvent::ApplicationDeactivate || object == this) finishPointerDrag(true, true, "deactivated");
        }
        if (dragging && event->type() == QEvent::KeyPress && static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape) {
            setGeometry(startGeometry); finishPointerDrag(true, true, "escape"); return true;
        }
        if (dragging && event->type() == QEvent::MouseButtonRelease && static_cast<QMouseEvent *>(event)->button() == Qt::LeftButton) {
            auto mouse = static_cast<QMouseEvent *>(event);
            const bool option = mouse->modifiers().testFlag(Qt::AltModifier);
            if (nativeDragging) { dragPointer = mouse->globalPos(); previewSnap(geometry(), option); }
            else movePointerDrag(mouse->globalPos(), option);
            finishPointerDrag(option, false, "qt-mouse-up"); return true;
        }
        if (dragging && !nativeDragging && event->type() == QEvent::MouseMove) {
            auto mouse = static_cast<QMouseEvent *>(event);
            if (!mouse->buttons().testFlag(Qt::LeftButton)) { finishPointerDrag(mouse->modifiers().testFlag(Qt::AltModifier), false, "button-no-longer-down"); return false; }
            movePointerDrag(mouse->globalPos(), mouse->modifiers().testFlag(Qt::AltModifier));
            return true;
        }
        if (event->type() != QEvent::MouseButtonPress && event->type() != QEvent::MouseButtonRelease && event->type() != QEvent::MouseMove)
            return false;
        auto widget = qobject_cast<QWidget *>(object);
        if (!widget || widget->window() != this) return false;
        auto mouse = static_cast<QMouseEvent *>(event);
        const QPoint point = mapFromGlobal(mouse->globalPos());
        Qt::Edges edges;
        if (chrome == WindowChrome::Hidden) {
            if (point.x() < 7) edges |= Qt::LeftEdge;
            if (point.x() >= width() - 7) edges |= Qt::RightEdge;
            if (point.y() < 4) edges |= Qt::TopEdge;
            if (point.y() >= height() - 7) edges |= Qt::BottomEdge;
        }
        const bool grip = widget == this && point.y() >= 4 && point.y() < contentsMargins().top();
        if (event->type() == QEvent::MouseButtonPress && mouse->button() == Qt::LeftButton && (edges || grip)) {
            dragEdges = edges; beginPointerDrag(mouse->globalPos()); return true;
        }
        return false;
    }
private:
    void updatePin()
    {
        if (!pinButton) return;
        QPointer<WorkspaceFloatingHost> alive(this);
        const bool native = chrome != WindowChrome::Hidden && setNativePin(this, pinned(), [alive](bool value) {
            if (alive) { logEvent(QStringLiteral("input.button.clicked"), {{"label", "Pin window"}, {"window", alive->objectName()}, {"checked", value}, {"native", true}}); alive->setPinned(value); }
        });
        pinButton->setVisible(!native);
        if (!native) { pinButton->setGeometry(8, 4, 16, 16); pinButton->raise(); }
    }
    void movePointerDrag(const QPoint &global, bool bypass)
    {
        dragPointer = global;
        const QPoint delta = global - start;
        movedEnough |= delta.manhattanLength() >= QApplication::startDragDistance();
        if (dragEdges) setGeometry(resizeFromEdge(startGeometry, delta, dragEdges,
            minimumSize().expandedTo(minimumSizeHint()).expandedTo(QSize(80, 40)), maximumSize()));
        else {
            const QRect desired(startGeometry.topLeft() + delta, size());
            setGeometry(desired);
            geometryTrace.sample(this,global,desired,"after-set-geometry");
            if (movedEnough) previewSnap(desired, bypass);
            else { snapPosition = {}; attachmentTarget = nullptr; hidePreviews(); }
            geometryTrace.sample(this,global,desired,"after-preview");
        }
    }
    void finishPointerDrag(bool bypass, bool cancelled, const QString &reason)
    {
        if (!dragging) return;
        const auto target = attachmentTarget;
        const auto edge = attachmentEdge;
        const bool attach = !cancelled && movedEnough && !dragEdges && target && attachRequested && snapping && !bypass;
        if (!cancelled && !dragEdges && snapping && !bypass && snapPosition.isValid()) setGeometry(snapPosition);
        nativeDragging = dragging = false; dragEdges = {}; snapPosition = {}; attachmentTarget = nullptr;
        delete releaseWatch.data(); releaseWatch = nullptr;
        hidePreviews();
        if (QWidget::mouseGrabber() == this) releaseMouse();
        dragTiming.report(objectName()); geometryTrace.report(objectName());
        logEvent(QStringLiteral("drag.end"), {{"window", objectName()}, {"reason", reason}, {"cancelled", cancelled}, {"attach", attach}, {"target", target ? target->objectName() : QString()}, {"edge", int(edge)}});
        const auto generation = dragGeneration;
        if (attach) QTimer::singleShot(0, this, [this, target, edge, generation] {
            if (generation == dragGeneration && !dragging && target && target->isVisible() && attachRequested) attachRequested(target, edge);
        });
    }
    void previewSnap(const QRect &desired, bool bypass)
    {
        DragTiming::Sample timing(dragTiming);
        snapPosition = {};
        attachmentTarget = nullptr;
        if (!snapping || bypass) { hidePreviews(); return; }
        const QRect currentFrame = frameGeometry();
        const QPoint offset = currentFrame.topLeft() - geometry().topLeft();
        const QSize decoration = currentFrame.size() - size();
        const QRect proposed(desired.topLeft() + offset, desired.size() + decoration);
        QList<QRect> targets, displays;
        QList<QLine> alignment;
        const bool align = QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);
        if (align) {
            for (auto window : QApplication::topLevelWidgets())
                if (window != this && window->isVisible() && !window->isMinimized() &&
                    (window->objectName().startsWith("unboundFloating_") || window->objectName() == "UiMainWindow"))
                    targets.append(window->frameGeometry());
            for (auto screen : QGuiApplication::screens()) displays.append(screen->availableGeometry());
        }
        const QRect snapped = align ? alignWindowEdges(snapWindowFrame(proposed, targets, displays), targets, alignment) : proposed;
        for (int i = 0; i < 2; ++i) {
            if (i >= alignment.size()) { if (guideWidgets[i]) guideWidgets[i]->hide(); continue; }
            if (!guideWidgets[i]) {
                guideWidgets[i] = new QWidget(this, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus);
                guideWidgets[i]->setObjectName(QStringLiteral("unboundAlignmentGuide"));
                guideWidgets[i]->setAttribute(Qt::WA_ShowWithoutActivating); guideWidgets[i]->setAttribute(Qt::WA_TransparentForMouseEvents);
                guideWidgets[i]->setStyleSheet(QStringLiteral("background:#6582a0;"));
            }
            const auto line = alignment[i];
            const auto frame = QRect(line.p1(), line.p2()).normalized().adjusted(0, 0, 1, 1);
            if (guideWidgets[i]->geometry() != frame) guideWidgets[i]->setGeometry(frame);
            if (!guideWidgets[i]->isVisible()) { guideWidgets[i]->show(); guideWidgets[i]->raise(); }
        }
        snapPosition = align ? QRect(snapped.topLeft() - offset, desired.size()) : QRect();
        QRect outline = snapped;
        DockZone chosen;
        DockDestination destination;
        {
            DragTiming::Sample lookup(dragTiming,"destination_lookup");
            destination = resolveDockDestination(this, dragPointer,
                {preview, dropMarker, compass, guideWidgets[0], guideWidgets[1]});
        }
        auto surface = destination.surface;
        const auto surfaceFrame = destination.frame;
        const auto previous = surface == hoverDestination ? previousPlacement : DropPlacement::None;
        const auto drop = surface && attachRequested ? (destination.drop.placement!=DropPlacement::None?destination.drop:selectPanelDrop(surfaceFrame, dragPointer, previous)) : DropSelection{};
        hoverDestination = surface; previousPlacement = drop.placement;
        if (drop.placement != DropPlacement::None) {
            chosen.active = true; chosen.target = drop.region; chosen.preview = drop.preview;
            chosen.edge = drop.placement == DropPlacement::Left ? Qt::LeftEdge : drop.placement == DropPlacement::Right ? Qt::RightEdge :
                drop.placement == DropPlacement::Above ? Qt::TopEdge : drop.placement == DropPlacement::Below ? Qt::BottomEdge : Qt::Edge(0);
            attachmentTarget = surface;
        }
        if (compass) compass->hide();
        const QString selection = attachmentTarget ? attachmentTarget->objectName() + QString::number(int(chosen.edge)) : QString();
        if (selection != lastDockSelection) {
            lastDockSelection = selection;
            logEvent(QStringLiteral("dock.target.selection"), {{"source", objectName()},
                {"pointer_x",dragPointer.x()},{"pointer_y",dragPointer.y()},
                {"candidate",surface?surface->objectName():QString()},
                {"candidate_x",surfaceFrame.x()},{"candidate_y",surfaceFrame.y()},
                {"candidate_width",surfaceFrame.width()},{"candidate_height",surfaceFrame.height()},
                {"target", attachmentTarget ? attachmentTarget->objectName() : QString()}, {"active", bool(attachmentTarget)},
                {"kind", !dockSectionKey(attachmentTarget).isEmpty()?"stack-split":chosen.edge == Qt::Edge(0) ? "tab" : attachmentTarget && attachmentTarget->isWindow() && surfacesAreGrouped(attachmentTarget) ? "group-split" : "panel-split"},
                {"edge", int(chosen.edge)}});
        }
        if (chosen.target.isValid()) {
            if (!dropMarker) {
                auto label = new QLabel(this, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus);
                label->setObjectName(QStringLiteral("unboundDockTarget")); label->setAlignment(Qt::AlignCenter);
                label->setAttribute(Qt::WA_ShowWithoutActivating); label->setAttribute(Qt::WA_TransparentForMouseEvents);
                label->setStyleSheet(QStringLiteral("background:#34475b; color:white; border:1px solid #80a5c9; border-radius:4px;"));
                dropMarker = label;
            }
            dropMarker->setToolTip(chosen.edge == Qt::Edge(0) ? QStringLiteral("Add as tab") :
                attachmentTarget && attachmentTarget->isWindow() && surfacesAreGrouped(attachmentTarget) ? QStringLiteral("Split group") : QStringLiteral("Split panel"));
            const auto caption = dropCaption(drop.placement, !dockSectionKey(surface).isEmpty()?QStringLiteral("entire stack"):surface ? surface->windowTitle() : QString(),
                surface && surface->isWindow() && surfacesAreGrouped(surface));
            auto label = static_cast<QLabel *>(dropMarker);
            label->setText(caption); label->setToolTip(caption);
            const QSize labelSize(qMax(120, label->fontMetrics().horizontalAdvance(caption) + 24), 26);
            const QRect labelFrame(chosen.target.center() - QPoint(labelSize.width()/2, labelSize.height()/2), labelSize);
            if (dropMarker->geometry() != labelFrame) dropMarker->setGeometry(labelFrame);
            if (!dropMarker->isVisible()) { dropMarker->show(); dropMarker->raise(); }
            attachmentEdge = chosen.edge;
            if (chosen.active) outline = chosen.preview;
        } else if (dropMarker) dropMarker->hide();
        if (!attachmentTarget) { if (preview) preview->hide(); return; }
        if (!preview) {
            preview = new QWidget(this, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus);
            preview->setObjectName(QStringLiteral("unboundSnapPreview"));
            preview->setAttribute(Qt::WA_ShowWithoutActivating);
            preview->setAttribute(Qt::WA_TransparentForMouseEvents);
            preview->setStyleSheet(QStringLiteral("background: rgba(80,140,210,45); border: 2px solid #5099df;"));
        }
        if (preview->geometry() != outline) preview->setGeometry(outline);
        if (!preview->isVisible()) {
            preview->show(); preview->raise();
            if (dropMarker && dropMarker->isVisible()) dropMarker->raise();
        }
    }
    static bool surfacesAreGrouped(QWidget *widget) { return widget && widget->objectName().startsWith("unboundFloating_group_"); }
    void hidePreviews() { if (compass) compass->hide(); hoverDestination = nullptr; previousPlacement = DropPlacement::None; if (preview) preview->hide(); if (dropMarker) dropMarker->hide(); for (auto guide : guideWidgets) if (guide) guide->hide(); }
    QToolButton *pinButton = nullptr;
    WindowChrome chrome = WindowChrome::Standard;
    bool chromeInitialized = false;
    DragTiming dragTiming;
    DragGeometryTrace geometryTrace;
    bool snapping = true;
    DockCompass *compass = nullptr;
    QPointer<QWidget> hoverDestination;
    DropPlacement previousPlacement = DropPlacement::None;
    QWidget *preview = nullptr;
    QWidget *dropMarker = nullptr;
    QWidget *guideWidgets[2] = {nullptr, nullptr};
    QString lastDockSelection;
    QPoint dragPointer;
    QRect snapPosition;
    QPointer<QWidget> attachmentTarget;
    Qt::Edge attachmentEdge = Qt::LeftEdge;
    unsigned dragGeneration = 0;
    bool dragging = false, nativeDragging = false;
    bool movedEnough = false;
    Qt::Edges dragEdges;
    QPoint start;
    QRect startGeometry;
    QPointer<QObject> releaseWatch, nativeMoveWatch;
};
}
