# Source-to-Timeline drag delivery repair

Branch: `fix/timeline-native-edit-repaint`, based on the uninstalled palette
isolation repair. Installation remains on hold. Installed build remains `0480ac1`.

## Evidence and cause

Jordan's 29-second recording (Desktop, Screen Recording 2026-11-02 at 9.01.17 AM)
shows Native UI insertion followed by an OpenGate attempt without the same visible
insertion. Decoded review frames are in `build/source-drop-repaint-review`.
The matching session `cba22de1-02e7-434e-89bc-3f1e379b8b8f` has no clip-color
preview events. Existing logs do not record native drag acceptance or resulting
model insertion. They cannot distinguish missed delivery from stale pixels.

The native Qt regression reproduced a delivery failure before the repair: entry
was accepted, but the native scene received zero moves and zero drops at every
tested scale. Qt 5.15.2 QApplication::notify retains the outer display widget as
the drag manager's current target. Subsequent sendEvent(nativeViewport, mapped)
calls are retargeted to that display widget, bypassing the intended receiver.
This code predates the Red experiment. The evidence supports an independent
scaled input regression; it does not prove every reported edit failure has the
same cause or that Red works.

## Responsibility boundaries

- TimelineViewportDragDispatch owns mapped drag-phase construction, delivery and
  acceptance. It calls the original scroll area's virtual viewportEvent through
  an ordinary base-class member pointer. No private Qt ABI or global drag-manager
  mutation is used. Native scene/proxy dispatch, MIME ownership and drop action
  remain native.
- TimelineInputRouter delegates drag phases to that module. The replaced event
  construction cases and old drop helper are removed. Other input stays there.
- TimelineContentViewport wires input completion to existing scene damage and
  completed-drop frame evidence. Move/preview phases do not add frame-diagnostic
  output. TimelineSceneUpdates remains the sole scheduler.

No second renderer, state owner, model mutation, command replay, pointer warp,
playback timer or repaint polling was added. Diagnostics distinguish delivery
acceptance from model insertion and displayed scanout.

The direct virtual-handler path does not replay root viewport QObject filters
or application filters for the mapped copy. Application filters already saw the
outer event; native scene/proxy filters remain active. Resolve's private root
filter behavior needs live acceptance. This limitation is explicit, not replaced
by a passing synthetic scene policy.

## Verification

The optimized AddressSanitizer drag/source suite passes at 70/100/147/213/300%.
It reuses TimelineViewportFixture and verifies native scene state and pixels:

| Action | Evidence |
| --- | --- |
| Drag entry/move/drop | Original MIME pointer, mapped local/global event coordinates, exactly one native insertion and accepted copy action |
| Preview/cancel/repeat | Correct native preview pixels before drop, canceled pixels cleared, next drag receives a fresh preview |
| Rejected/disabled target | Native scene policy retained, no extra insertion, disabled viewport rejects entry |
| Source Insert | Native QAction/QShortcut transport creates a clip and shifts existing model geometry; new pixels visible without click/resize/zoom |
| Source Place on Top | Native transport creates an upper clip and displays its pixels while Source retains focus |
| Deferred edits | Same model/pixel checks after 0, 3 and 7 queued Qt turns |

Fixture F9/F12 assignments are test transport, not claims about the user's native
Resolve assignments. The source fixtures use Qt scene edits, not Resolve's private
Insert/Place on Top API.

One isolated broader Timeline run passed all assertions before final fixture
pointer setup refinement. Other runs failed pointer-sensitive assertions while
the physical pointer moved during execution (observed deltas included hundreds
of pixels). The independent QWidget shortcut now gets fresh pointer setup after
unrelated edit checks; call-count, coordinate and pixel assertions are unchanged.
Latest retained broader result, `build/source-drop-repaint-review/viewport-regression.log`,
passes command/coordinate/pixel assertions and fails one stationary-OS-pointer
assertion. An idle-pointer rerun is pending; this is not recorded as a full pass.

## Acceptance gates

The numbered native-entry contract was reread. This local repair concerns ordinary
native input (8/9), Timeline painting and overflow (10), page/native return (3/11),
and assembled native gestures/remapped commands (12). Existing fixture coverage is
retained; the recording is failure evidence, not acceptance of this candidate.

Fresh Resolve testing must cover Source and Media Pool drags, Insert and Place on
Top, cursor/trim/roll gestures, Undo/Redo pixels, stacked timelines, page return,
Native return and actual GPU rendering. Existing `503a969` and fractured rollback
checkpoints stay preserved. No install, live project edit, rollback, main merge,
push or publication is part of this repair checkpoint. Red native paint remains
unproven; the installed old Red/Clear actions remain unsafe to use until replaced.

Both platform callers compose this shared core through PanelContentScale; the
edited callback has one production wiring site. macOS fixture compilation uses
Resolve's Qt. Windows native compilation/runtime acceptance remains unverified.
Diff review found one focused extracted responsibility, removal of its old path,
and no new persistent state writer or parallel controller.
