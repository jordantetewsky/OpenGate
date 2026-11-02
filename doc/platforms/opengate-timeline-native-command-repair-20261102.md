# Timeline command, redraw and header repair

Branch: `fix/timeline-native-command-presentation`. Requested `6c3ffb4` remains
installed. Rejected `61aceef` and its live logs remain preserved. Implementation,
fixture evidence, compilation and native acceptance are separate.

## Command and input repair

Native/OpenGate commands must run their existing handlers once and immediately
show their result in an ordinary-sized Timeline at every supported scale.
Maximize/backtick cannot be a required workaround for stale feedback.

The old pointer adapter restored the native origin before a queued handler could
read QCursor. The failing regression checks both root and nested viewports.
`TimelinePointerCoordinates` retains alignment through queued work, reconciles
pointer/layout changes, and releases it on leave/hide/native return. This
supersedes the previous synchronous-origin-restoration test contract. The
transient widget origin is presentation state; the OS pointer, native scene/world
transform, model, fonts and saved geometry remain unchanged.

The old cache could retain stale pixels after paint-data changes without scene
item geometry changes. `NativeCommandActivity` observes QAction/QShortcut
execution and completed OpenGate dispatch without a key whitelist or handler
replay. Native action discovery/connections move out of InteractionDiagnostics;
its old registry is removed and diagnostics subscribes to the same observer.
The existing command filter also observes newly delivered shortcuts immediately,
so a lazy shortcut needs no discovery delay. Notifications coalesce at a Qt turn.

`TimelineSceneUpdates` requests fresh native rasters after command/edit input,
separately from layout reconciliation. It does not treat unrelated playhead damage
as proof that paint-only editing refreshed. Validated workflow results publish a
completion callback wired by both OS adapters; result files still cannot execute
commands. This covers the existing asynchronous selected-timeline Stabilize path.

`TimelineCommandPaintEvidence` records command sources, actual raster revision,
sampled fingerprint and body/header scales after render, using rotating logs.
There are no images or polling. Fresh raster evidence is explicitly not proof of
model correctness, native GPU playback or display scanout.

## Header behavior

All controls, including blue/red buttons, share the horizontal header scrollbar.
The frozen-button slices/hit map are removed. Whole-area pinning keeps the left
area fixed during native time panning; unpin allows that complete area to scroll
away. Footer pin/size controls remain available to recover it. The native time
scrollbar remains beside the independent header scrollbar.

The footer offers an independent controls/timecode size menu, typed percentage,
live slider and Match Timeline size. Actual UiTiTrackControlItem geometry keeps
rows aligned in height, position and vertical scrolling. Header foreground scales
uniformly within those rows; solid native raster background colors are used
instead of stretching strips that could contain glyphs.

The stated fit policy limits header content to the body scale instead of growing
track rows or clipping adjacent rows. At 300% body size, 70%, 100% and 147% header
sizes are independent choices. The menu exposes the current limit. Automatic row
growth was not implemented while the preference question remained unanswered.
Preferred header scale/width survive temporary fitting limits. A zero scale
preference follows body size. All settings use TimelineContentScale's existing
session/workspace checkpoint, including unsaved state.

## Functional evidence

The Timeline suite and AddressSanitizer pass. New focused modules reuse the
existing fixture and test model state plus rendered pixels, not just callbacks:

| Action | Evidence |
| --- | --- |
| Queued Go to Mouse Pointer | One handler; correct root/nested native coordinates; no OS pointer move |
| Native edit/Undo/Redo | Actual QUndoStack state and displayed pixels change without geometry changes or maximizing |
| Queued/dynamic actions and immediately created native shortcuts | One native handler and current pixels; no key whitelist or discovery delay |
| OpenGate menu and native-preset mapping | Actual scoped registry changes native state and visible pixels; another panel and inline text keep focus/typing |
| Asynchronous workflow result | Validated completion refreshes pixels; invalid results cannot publish completion or replay commands |
| Trim-in/out and roll | Native item boundaries, adjacent clips, live global cursor query and pixels change during the grab, before release |
| Paint-only drag plus unrelated playhead damage | Root repaint-only data remains current without resize/maximize |
| Header/menu/scroll/pin | Timecode grows, rows align, blue/red controls scroll out, whole area unpins/recovers, scrollbar remains usable |
| Native return/fresh owner | Ownership restores and scale/width/scroll/pin round-trip through existing state |

Command/result tests combine body sizes 70/100/147/213/300% with follow-body,
70% and preferred 100% headers. Existing tests retain 73/85/125/200/263% checkpoints,
exact fresh/incremental pixels, section containment, cursor artwork/hotspots and
no idle polling. Other local suites pass: panel scale, panel commands, native
preset bindings, native edit commands, workflow commands/results, command menu,
assembled native entry and Timeline presentation probe. Final artifact checks
are recorded separately in the build receipt.

## Matched synthetic performance

Both O2 builds use the same 2,880-clip fixture, 1440 x 850 presentation, Retina DPR 2
and 240-native-unit header (above both footer minima). Paint-only drags change
native data and check each rendered frame. Old means unchanged 61aceef headers.

| Scale | New playhead loop p95 ms | New pan p95 ms | New paint-only drag p95 ms | Current drag frames: old / new |
| --- | ---: | ---: | ---: | ---: |
| 70% | 4.922 | 16.881 | 17.205 | 10/20 / 20/20 |
| 100% | 2.950 | 14.047 | 13.694 | 10/20 / 20/20 |
| 147% | 1.671 | 14.335 | 12.557 | 10/20 / 20/20 |
| 213% | 1.052 | 11.268 | 12.066 | 10/20 / 20/20 |
| 300% | 0.908 | 23.485 | 23.382 | 10/20 / 20/20 |

Old paint-only drag p95 was 3.006–3.616 ms while half its frames were stale; that
is not equivalent successful work. New direct mouse dispatch p95 is 0.019–0.024 ms.
Unchanged exposure reuses native pixels. Raw outputs are under
build/timeline-performance-review/native-command-{baseline-61,candidate}.json.
Full redraw/drag at 300% still exceeds a 16.67 ms / 60 Hz budget. These are synthetic
UI costs, not decoding/native GPU or physical pointer-to-display measurements.
No playback rate/decoder setting is changed and arbitrary-frame-rate smoothness
is not established.

## Organization and native gate

Reviewed responsibilities, state writers, duplicate connections and dependencies.
Command observation moved out of diagnostics; pointer alignment moved out of
input dispatch; frame evidence has its own module. Geometry replaces frozen-button
logic rather than adding a parallel controller. Coordinators wire callbacks;
PanelControls/controller do not grow implementations. There is one native scene/
model and existing persistence owner. Both workflow-result callers are updated;
Windows native compilation/runtime remain unverified.

Contract clauses 9–12 still require actual Resolve painting, private command and
preview behavior, real remapped keys, trim/roll/transitions, toolbar/stacked tabs,
restart persistence and playback while editing. A fixture is not proof of every
proprietary native handler. Built/staged does not mean live accepted. No new
installation, merge, push or publication is implied.
