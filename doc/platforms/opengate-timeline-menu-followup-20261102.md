# Timeline live-test follow-up

Branch: `fix/timeline-menu-scroll-followup`, isolated from installed `b516400`.
Jordan confirmed Source-to-Timeline dragging works in that installation. The
installed repair source was pushed to personal/fix/timeline-native-edit-repaint;
no package, main merge or publication was made.

## Agreed experience and changes

The color trial is included. Logs show enabled Red/Clear entries prepared in the
real unnamed Timeline Clip Color submenu. The screenshot and Jordan's follow-up
identify offscreen clipping, not a disabled experiment. Native QMenu resizes when
actions are inserted after Show but does not repeat initial screen fitting.

MenuOverflowScroll owns fitting and a visible vertical scrollbar for overflowing
menus. It uses public QMenu popup layout and row scrolling, retaining original
actions, native handlers, menu ownership and keyboard/wheel navigation. Its extra
right strip reserves space outside native action/icon rectangles. Fitting menus
hide the bar. NativeClipColorMenus only recognizes the palette, retains origin
and invokes the existing command preparation plus this presentation helper.

The existing header-controls module explicitly focuses Timeline when its
scrollbar is clicked; clicking after Source/text focus must not leave commands
targeted there. Existing wheel/scroll state and persistence remain the owners.
No alternate scrollbar model or settings writer was introduced.

TimelineSceneUpdates now finishes a pending layout before dispatching an earlier
queued presentation repaint. This removes a reproducible intermediate-layout
paint ordering. TimelineSceneRenderer clips each raster slice to its intended
destination. These changes address intermediate frames/edge leakage; Jordan's
small pinned-control artifacts and full-extent scaling bump still require live
visual comparison. A synthetic ordering check is not proof that every native
full-extent zoom transition is settled.

NativePanelNavigation recognizes Find Timeline in Media Pool and Find Current
Timeline in Media Pool, including native preset ID
timelineFindCurrentTimelineInMediaPool. Labels/ID were read from the installed
Resolve binary and read-only native preset. The command goes through the same
existing panel-door request as Find in Media Pool: reveal the retained floating,
drawer or grouped destination and select its tab, without replaying Resolve's
command or resetting placement. No second visibility owner was added.

Jordan also requested independent timecode sizing so its visibility can be
adjusted separately from track controls/divider width. He explicitly placed it
below making clip-color selection usable. This remains queued; current header
scaling is not presented as that new independent timecode control.

## Checks and remaining acceptance

Optimized ASAN standalone menu checks pass for visible scrollbar, screen fitting,
reserved icon space, reaching the last item, native action execution once and
reopening/returning to the first item. Initial spacing failure was fixed by
reserving an actual strip rather than changing the native menu stylesheet.
The clip-color menu/isolation fixture passes; an added stress case exercises
scrolling the actual late-populated submenu to Red before selection. Its final
run is pending alongside the final assembled native-navigation and broader
Timeline regressions.

Optimized ASAN header checks pass for explicit click focus after Source/text
focus, immediate horizontal wheel response and layout-before-paint ordering.
Final-source source-drop and full Timeline checks remain required before install.

Organization review: fitting/scrolling moved into one reusable presentation
module; the palette coordinator contains no scroll implementation. Header input,
native damage scheduling, raster clipping and navigation recognition remain in
their existing focused modules. New tests reuse existing Timeline and workspace
fixtures. No new command dispatcher, project mutation, polling timer, persistent
state owner or platform API was added. Shared-core Windows runtime remains
unverified; Mac fixture/build evidence does not close that platform gate.

The numbered native-entry contract was reread for the previous installation;
these changes retain native gestures/commands (8/9), overflow and painting (10),
retained placement (11) and separate assembled/live acceptance (12). Actual
Resolve Red paint and stored-color integrity remain unproven until Jordan can
reach and select the trial entry. Persistent arbitrary colors and the shared
picker are not claimed complete by this temporary trial.
