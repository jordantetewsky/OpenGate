// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QTemporaryDir>
#include <QLineEdit>
#include <QMenu>
#include <cstdio>
#include "../../../src/core/panelcontentscale.h"
#include "../../core/workspace-callers/timelineviewportfixtures.h"
#include "../../core/workspace-callers/timelineattachedchecks.h"
#include "../../core/workspace-callers/timelineoverflowchecks.h"
#include "../../core/workspace-callers/timelinepointerchecks.h"
#include "../../core/workspace-callers/timelinescaledinputchecks.h"
#include "../../core/workspace-callers/timelinerendercachechecks.h"
#include "../../core/workspace-callers/timelinecursorchecks.h"

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QTemporaryDir temporary;
    qputenv("UNBOUND_DIAGNOSTICS_DIR", temporary.path().toUtf8());
    qputenv("UNBOUND_SHORTCUTS_FILE", (temporary.path() + "/Shortcuts.json").toUtf8());
    int failures = 0;
    auto check = [&](bool value, const char *message) {
        if (!value) { ++failures; std::fprintf(stderr, "FAIL: %s\n", message); }
    };
    using namespace OpenGateTest;
    TimelineViewportFixture fixture;
    QApplication::setActiveWindow(&fixture.timeline);
    fixture.view->setFocus(Qt::OtherFocusReason);
    auto view = fixture.view;
    const auto originalParent = view->parentWidget();
    const auto originalGeometry = view->geometry();
    const auto originalTransform = view->transform();
    const auto originalScene = view->scene();
    const auto originalBodyWidth = view->tracksProxy->size().width();
    Unbound::PanelContentScale scale;
    int saves = 0; scale.changed = [&] { ++saves; };
    check(scale.apply(&fixture.timeline), "100 percent installs the real header controls without an opt-in flag");
    TimelineViewportFixture::settle();
    auto presentation = dynamic_cast<Unbound::TimelineContentViewport *>(
        fixture.timeline.findChild<QWidget *>("openGateTimelineContentViewport"));
    check(presentation && view->scene() == originalScene && view->transform() == originalTransform,
          "presentation retains original scene, native handlers and world transform");
    if (!presentation) return 1;
    auto layer = presentation->presentation();
    const auto baseline = layer->grab().toImage();
    auto divider = dynamic_cast<Unbound::TimelineHeaderDivider *>(layer->findChild<QWidget *>("openGateTimelineHeaderDivider"));
    const auto start = divider->mapToGlobal(QPoint(3, 10));
    QMouseEvent down(QEvent::MouseButtonPress, QPointF(3, 10), start, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(divider, &down);
    QMouseEvent move(QEvent::MouseMove, QPointF(-97, 10), start + QPoint(-100, 0), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(divider, &move);
    QMouseEvent up(QEvent::MouseButtonRelease, QPointF(3, 10), start + QPoint(-100, 0), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(divider, &up);
    TimelineViewportFixture::settle();
    check(qAbs(presentation->headerWidth() - 160.) < 2. && view->tracksProxy->size().width() > originalBodyWidth + 90,
          "dragging the right edge narrower gains actual clip drawing width");
    auto bar = layer->findChild<QScrollBar *>("openGateTimelineHeaderScroll");
    check(bar && bar->isVisible() && bar->maximum() >= 98, "narrow header exposes a real draggable overflow scrollbar");
    bar->setValue(bar->maximum()); TimelineViewportFixture::settle();
    TimelineViewportFixture::click(layer, QPointF(30, 50));
    check(view->blue->clicks == 1, "blue track button stays pinned and routes to the original item after header scrolling");
    TimelineViewportFixture::click(layer, QPointF(70, 50));
    check(view->red->clicks == 1, "red track button stays pinned and uses the original native action");
    TimelineViewportFixture::click(layer, QPointF(230 - presentation->headerOffset(), 50));
    check(view->lastControl->clicks == 1, "rightmost header control remains reachable through overflow");
    TimelineViewportFixture::click(layer, QPointF(presentation->headerWidth() + 40, 65));
    if(view->clip->clicks!=1) {
        auto tracks=qobject_cast<QGraphicsView *>(view->tracksProxy->widget());
        const auto at=tracks->mapToScene(QPoint(40,25));
        std::fprintf(stderr,"clip diagnostic clicks=%d root=%.1f,%.1f track=%.1f,%.1f hscroll=%d contains=%d item=%p expected=%p\n",view->clip->clicks,
            view->lastMouse.x(),view->lastMouse.y(),at.x(),at.y(),tracks->horizontalScrollBar()->value(),
            view->clip->contains(at),static_cast<void *>(tracks->itemAt(QPoint(40,25))),static_cast<void *>(view->clip));
    }
    check(view->clip->clicks == 1, "narrowed clip area routes pointer selection to the original nested native scene");
    const auto remembered = scale.snapshot();
    check(remembered["timeline_header_width"].toDouble() > 0 && remembered["timeline_header_scroll"].toDouble() > 0 && saves >= 2,
          "header gestures use the existing retained state and save callback");
    scale.setValue(2.); check(scale.apply(&fixture.timeline), "coherent content scaling applies at 200 percent");
    TimelineViewportFixture::settle();
    const auto enlarged = layer->grab().toImage();
    check(paintedInkHeight(baseline,Qt::red) > 0 && paintedInkHeight(enlarged,Qt::red) >= paintedInkHeight(baseline,Qt::red) * 1.8,
          "actual composed clip-label glyphs enlarge with their positions instead of fracturing");
    check(fixture.toolbar->height() == 31 && view->transform() == originalTransform,
          "content scaling preserves compact toolbar and never modifies the native world transform");
    TimelineViewportFixture::click(layer, QPointF((presentation->headerWidth() + 40) * 2., 130));
    check(view->clip->clicks == 2, "scaled and narrowed presentation preserves native clip hit positions");
    QWheelEvent wheel(QPointF((presentation->headerWidth() + 40) * 2., 130), QPointF(500, 500),
                      QPoint(0, 20), QPoint(0, 120), Qt::NoButton, Qt::NoModifier, Qt::ScrollUpdate, false);
    QApplication::sendEvent(layer, &wheel);
    check(qAbs(view->lastWheel.x() - 300.) < 2., "native wheel routing uses the same header/scale coordinate map");
    timelineScaledInputChecks(presentation,view,check);
    timelineCursorChecks(presentation,view,check);
    QMenu nativeMenu(view);
    nativeMenu.setAttribute(Qt::WA_DontShowOnScreen);
    nativeMenu.addAction("Native clip action");
    view->clip->showContext=[&](QPoint point){nativeMenu.popup(point);};
    QContextMenuEvent context(QContextMenuEvent::Mouse, QPoint(400, 130), QPoint(777, 555));
    QApplication::sendEvent(layer, &context);
    TimelineViewportFixture::settle();
    if(view->clip->contexts!=1 || (nativeMenu.pos()-QPoint(777,555)).manhattanLength()>=20)
        std::fprintf(stderr,"context diagnostic hits=%d native=%d,%d menu=%d,%d visible=%d active=%d\n",
            view->clip->contexts,view->lastContextGlobal.x(),view->lastContextGlobal.y(),nativeMenu.x(),nativeMenu.y(),
            nativeMenu.isVisible(),QApplication::activePopupWidget()==&nativeMenu);
    check(view->clip->contexts==1 && (nativeMenu.pos()-QPoint(777,555)).manhattanLength()<20,
          "native context hit testing and physical menu placement both follow the displayed clip");
    nativeMenu.hide();
    auto editor = new QLineEdit(view->viewport()); editor->setGeometry(280, 90, 100, 22); editor->setText("Original"); editor->show();
    QApplication::setActiveWindow(&fixture.timeline);
    TimelineViewportFixture::click(layer, QPointF((presentation->headerWidth() + 30) * 2., 190));
    editor->selectAll();
    QKeyEvent key(QEvent::KeyPress, Qt::Key_X, Qt::NoModifier, "x"); QApplication::sendEvent(editor, &key);
    check(editor->text() == "x" && QApplication::focusWidget() == editor,
          "inline editing stays in the original native editor without focus theft");
    delete editor;
    const QJsonValue retainedWidth=scale.snapshot().value("timeline_header_width");
    fixture.timeline.resize(100,600);TimelineViewportFixture::settle();
    check(presentation->width()>=presentation->minimumWidth() && divider->geometry().right()<=layer->width() &&
          bar->geometry().right()<layer->width(),
          "narrow window respects pinned-button/native content minima without overlapping or losing the divider and scrollbar");
    TimelineViewportFixture::click(layer,QPointF(60,100));
    TimelineViewportFixture::click(layer,QPointF(140,100));
    check(view->blue->clicks==2 && view->red->clicks==2,
          "both pinned track buttons remain fully reachable at the narrowest supported scaled width");
    if(scale.snapshot()["timeline_header_width"]!=retainedWidth)
        std::fprintf(stderr,"retained width expected %.3f actual %.3f request %.3f visible %.3f\n",
            retainedWidth.toDouble(),scale.snapshot()["timeline_header_width"].toDouble(),
            presentation->retainedHeaderWidth(),presentation->headerWidth());
    check(scale.snapshot()["timeline_header_width"]==retainedWidth,
          "responsive width clamping never overwrites the user's retained divider setting");
    fixture.timeline.resize(900,600);TimelineViewportFixture::settle();
    auto pin=layer->findChild<QToolButton *>("openGateTimelinePinTrackControls");
    pin->click();TimelineViewportFixture::settle();
    check(!scale.snapshot()["timeline_header_pinned"].toBool(true),"explicit pin preference is retained through the existing state owner");
    const double footerX=(presentation->headerWidth()+(layer->width()/2.-presentation->headerWidth())*.5)*2.;
    TimelineViewportFixture::click(layer,QPointF(footerX,layer->height()-9.));
    check(qAbs(view->bar->lastClick.x()-view->bar->bounds.center().x())<3.,
          "native timeline scrollbar remains operable beside the independent header scrollbar");
    auto stacked = new TimelineSourceView(&fixture.timeline); fixture.layout->addWidget(stacked); stacked->show();
    TimelineViewportFixture::settle();
    check(fixture.timeline.findChildren<QWidget *>("openGateTimelineContentViewport").size() == 2,
          "new stacked Timeline view automatically receives the same controls and scaling");
    check(scale.suspend(), "native return releases all Timeline content leases");
    TimelineViewportFixture::settle();
    check(view->parentWidget() == originalParent && view->scene() == originalScene && view->transform() == originalTransform &&
          fixture.timeline.findChildren<QWidget *>("openGateTimelineContentViewport").isEmpty(),
          "native return restores original ownership and removes all presentation widgets");
    check(originalGeometry.width() > 0 && scale.snapshot()["timeline_header_scroll"] == remembered["timeline_header_scroll"],
          "native return retains header width and offset rather than erasing them");
    Unbound::PanelContentScale restored; auto saved = scale.snapshot(); restored.restoreState(saved);
    check(restored.apply(&fixture.timeline), "fresh state owner restores saved and unsaved Timeline content state");
    TimelineViewportFixture::settle();
    check(restored.snapshot() == saved, "scale, pinning, header width and offset serialize together");
    check(restored.suspend(), "restored configuration returns cleanly to native ownership");
    timelineAttachedChecks(check);
    timelineOverflowChecks(check);
    timelinePointerChecks(check);
    timelineRenderCacheChecks(check);
    std::printf("timeline content viewport failures=%d\n", failures);
    return failures ? 1 : 0;
}
