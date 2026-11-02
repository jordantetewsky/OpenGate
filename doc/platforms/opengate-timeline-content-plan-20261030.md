# Timeline content, width, toolbar and focused commands

Agreed with Jordan on October 30, 2026. Status: partial source implementation and
verified build candidate; native width/overflow, toolbar/tab repair and live
acceptance remain open. This refines the
[current work](opengate-current-work.md) and
[repair checklist](opengate-repair-checklist-20261024.md), preserving other tasks.

## Required experience

1. Drag the right boundary of Timeline's pinned left track-header area left or
   right to give clips more or less horizontal space. This is an internal divider,
   not merely the outer panel's minimum width. Blue/red track buttons remain
   pinned on the left, readable and usable. Other track controls must fit or have
   a visible, draggable horizontal scrollbar when they overflow; gesture-only
   scrolling is insufficient. No overlaps, spill into the clips, inaccessible
   clipping or lost controls. Header rows remain aligned with their tracks throughout resizing,
   scrolling and scaling. Widening reveals controls naturally. Horizontal scrolling
   of header controls must not scroll the clip timeline or move the pinned buttons.
2. Timeline's scale/resolution control primarily changes its content: track
   headers, ruler, clip labels, waveforms and related native drawing. The toolbar
   keeps a compact, stable height and reachable horizontal overflow. Growing the
   toolbar alone does not satisfy content scaling. This is not temporal timeline
   zoom, viewer-image magnification or process/display-wide scaling. An optional
   independent toolbar scale is deferred; it is not required for this work.
3. Map increase/decrease/reset content scale once and apply the same keys to the
   focused panel, with each panel retaining its own setting. Timeline focus scales
   Timeline; Media Pool focus scales Media Pool; Inspector focus scales Inspector.
   The change never scales all panels together. Support each panel through its
   actual content adapter, with explicit coverage gaps for unsupported surfaces.
4. Panel context also governs shortcut eligibility for Fixed Playhead, Stabilize
   and, when functional, audio-channel commands. Timeline focus permits those
   Timeline operations; disjoint panel contexts can reuse a key. Media Pool rename
   and other text editing retain their keys, selection and focus. Text editing,
   dialogs and popups take priority. A Timeline-only mapping must leave that key
   available for a different native or OpenGate command outside Timeline; this
   is command eligibility by firing area, not a global reservation. Do not infer the target from hover or move
   focus to execute. An unsupported/unknown target cannot silently scale another
   panel or fall through to an OpenGate command in another context.
5. Preserve Toggle Maximum Viewer's user-reported correct behavior and existing
   intentional hover-based maximize bindings. Focus policy for the commands above
   must not silently convert unrelated bindings to a different targeting model.
6. Repair the reported sequence: Timeline toolbar and stacked-timeline tabs
   visible -> switch Resolve page -> return -> toolbar and/or stacked tabs missing. Keep Timeline, its toolbar and native timeline
   tabs as one arranged element. Preserve Display Stacked Timelines, active
   timeline, selection, playhead, grouping and saved/unsaved layout. No reset or
   manual recovery is an acceptable normal page-return path.
7. Set it once and OpenGate remembers it: retain the user's pinning choices,
   track-header divider width, header-control horizontal scroll position and
   independent panel content scale. Page departure/return, hide/show/recall,
   float/dock/group/tab transitions and temporary maximize/restore must not reset
   these settings. Restore the last working values after restart, including
   unsaved workspace changes. Saved workspaces retain their own values through
   the existing Save/Update/recall model. Original Resolve UI restores exact native
   owners/metrics while retaining OpenGate preferences for re-entry. Clamp a saved
   width/offset only when the current viewport requires it to keep controls
   reachable; ordinary layout changes must not silently replace the user's setting.

Shortcut scope refines the older focus-independent keyboard requirement for Fixed
Playhead. It does not require restricting explicit menu invocation or changing the
native operation itself. Stabilize must still operate on selected Timeline clips
with Inspector hidden. Audio command execution, silent preparation and native Undo
remain separately open; adding focus eligibility cannot claim those solved.

## Isolated branches and dependencies

Planning seed: `5e472349830ed9a6b4921c523785f125eb490781`, the clean October 29
command/current-work checkpoint. The older root checkout is not the seed. Feature
branches start at the documentation commit on `plan/timeline-content-20261030`.
All branches are local; no runtime changes are included at setup.

| Branch | Worktree under `Developer/OpenGate-features/` | Job and dependency |
| --- | --- | --- |
| `plan/timeline-content-20261030` | `timeline-content-plan` | Own this agreed plan and cross-links |
| `fix/timeline-toolbar-page-return` | `timeline-toolbar-page-return` | Diagnose and repair page-return lifecycle; review/reuse existing `diagnose/timeline-toolbar` evidence through `2e4e4a8` rather than duplicate its probe |
| `feature/timeline-track-header-width` | `timeline-track-header-width` | Establish native header boundary, implement divider and reachable overflow; no fabricated replacement track UI |
| `experiment/timeline-content-scale` | `timeline-content-scale` | Establish a real renderer capability, then content-only scaling and restoration; native capability discovery is a prerequisite |
| `feature/focused-panel-command-context` | `focused-panel-command-context` | Shared focus/text scope, contextual key reuse, and scale command routing through existing command infrastructure |

The toolbar diagnostic branch and newer audio receiver branch remain preserved.
Do not merge an audio experiment or install a diagnostic merely to begin this
work. Review the exact dependent changes before integration. Width, scale and
focus branches stay independently reviewable; share interfaces through bounded
commits when demonstrated dependencies require them. Integrate after their checks,
then run the assembled flow. Do not treat branch creation as implementation.

## Production responsibilities and interfaces

These are module jobs before coding, not instructions to create empty files.
Confirm native boundaries before choosing final APIs. Keep shared policy in
`src/core/`; keep Cocoa/Win32/native OS calls behind platform adapters. Mac headers
that forward to core are compatibility entry points, not a second implementation.

| Module | Single job and proposed interface/boundary |
| --- | --- |
| Proposed `TimelineTrackHeaderViewport` | Own reversible header divider width and control overflow against discovered native header/pinned/control surfaces; attach, set/get width and offset, restore exact native state |
| Proposed `TimelineContentScale` | Apply/report/restore supported native Timeline drawing scale through a verified renderer capability; expose capability, apply(factor) and restore without owning preferences, commands or toolbar geometry |
| Existing `PanelMetricScale` | Apply and restore baseline-derived Qt widget metrics for supported content; retain this adapter for ordinary widgets and keep Timeline toolbar outside content scaling |
| Proposed `PanelCommandContext` | Resolve one current stable panel ID and editing/modal eligibility from native/managed focused descendants; return a read-only context and scope decision using existing session identity, not a second panel registry |
| Existing `PanelCommands` | Dispatch registered commands and bindings exactly once; delegate context eligibility and conflict policy instead of growing another input filter or command registry |
| Existing `PanelCommandCatalog` | Present shared command identities and resolve native preset assignments; use the same context decision as dispatch and keep native keyboard customization authoritative |
| Existing `NativePresetBindings` | Read/watch/decode native assignments without writing Resolve keyboard preferences; reuse it for scale commands and contextual conflict analysis |
| Existing `NativeEditCommands` | Invoke established native Fixed Playhead/Stabilize operations; leave shortcut targeting to command context and keep Inspector/selection/focus unchanged |
| Existing `PanelScaleControls` | Present scale value/actions and delegate to the existing session setter; menu and shortcut must use the same operation |
| Existing `ManagedPanelSession` | Own one panel's transfer/lifecycle and existing scale preference; wire focused adapters into attach/restore instead of implementing their policies |
| Existing `TimelineToolbarViewport` | Lease and restore the attached toolbar's layout slot and overflow; correct/rebind its lifecycle here if evidence identifies that boundary |
| Existing `TimelinePresentationProbe` on diagnostic branch | Observe bounded read-only toolbar/tab/parent/geometry transitions; retain evidence without making it a visibility controller |
| Existing `PanelControls` | Coordinate existing panels, sessions and callers; wire dependencies without accumulating header, renderer, input or persistence implementations |

Use the existing stable panel IDs, `interface_scale`, session setter and workspace
save/checkpoint path. Do not create another scale store. Persist header width,
pinning choices and overflow offset through that same owner as required state;
do not add a separate settings writer. Restore after native content/layout is ready
so an initial empty scrollbar range cannot overwrite a retained scroll position.
One generic focused-panel scale command set is preferable to duplicate per-panel
commands. Exact native registration remains to be established, not bypassed with
hard-coded keys or a second shortcut editor.

Current warning sizes at the seed: `panelcontrols.h` 1,288 lines, `panelsession.h`
690, `panelcommands.h` 173, `panelcommandcatalog.h` 93. Extract a touched independent
responsibility first in a bounded behavior-preserving commit; move production call
sites and delete the replaced path. No parallel state owners, arbitrary part files,
line compression, forwarding-only scaffolds or unrelated architectural rewrite.

Specific existing paths to replace if touched: whole-subtree Timeline metric
application in `ManagedPanelSession::refreshMetricScale`; command-specific text
editing exceptions in `PanelCommands`; page-only/global collision decisions in
`PanelCommandCatalog`; any demonstrated obsolete toolbar reveal route in
`PanelControls`. Inspect all callers before removal; these are review targets,
not a claim that every one caused the reported defect.

## Execution order and evidence

1. Reproduce/trace page departure and return using existing diagnostic boundaries.
   Identify native parent/layout/visibility/height changes and the responsible
   production call site before adding a repair. Avoid timers that repeatedly force
   visibility or reset native preferences.
2. Inspect actual native track-header/viewport and renderer boundaries read-only.
   Establish whether pinned controls can be separated and header drawing/hit tests
   support a narrower viewport. Public QWidget metrics currently do not scale
   Resolve's custom-painted Timeline; no verified body/ruler scale hook exists.
3. Implement the smallest supported width/overflow adapter and content-scale
   adapter separately. Preserve the renderer and input coordinates. No screenshot
   enlargement, duplicate Timeline engine, second scroller around the full track
   body or silent substitute of ordinary time zoom. If native capabilities block
   the agreed experience, record the exact gap and discuss departures first.
4. Extract/reuse focused-panel context, apply text-first scope to dispatch and
   binding conflicts, then connect scale commands through the existing session
   setter. Resolve context again at queued execution so a focus change cannot
   retarget a pending command. Detect overlapping scopes; do not broadly whitelist
   native shortcut collisions whose scope is unknown.
5. Review each real diff for moved responsibility, removed path, state writers,
   duplicate logic, dependencies and file growth; then integrate verified pieces.

| Acceptance action | Required evidence (all currently open) |
| --- | --- |
| Drag divider narrow/wide, then drag its horizontal scrollbar | Actual native divider and scrollbar hit targets; pinned left buttons stationary; all controls reachable; row alignment and clip-area gain; matching behavior fixture where representable |
| Set pinning, header width/offset and scale; leave/return/restart | Same settings after page switches, hide/show, float/dock/group/tab, maximize/restore and fresh-process unsaved restore; saved workspace recall restores its own values; late layout/range initialization does not erase the retained offset |
| Scale Timeline repeatedly, reset, resize | Content changes visibly/sharply; toolbar height stable; ruler/labels/waveforms and pointer hits correct; no cumulative drift or blank painting |
| Use the same scale keys in Timeline, Media Pool, Inspector | Only focused content changes; correct per-panel readback; independent settings survive grouping, page return, save/unsaved restart and native return |
| Rename a Media Pool clip; type in native fields | Typing/editing preserved, zero unintended scale/Stabilize/Fixed Playhead/audio dispatch, no focus/selection movement |
| Reuse a key in disjoint panels; remap/clear/switch preset | Same-context conflict rejected, disjoint scope accepted, unknown native collision retained as conflict, old binding removed, exactly one invocation |
| Queue key then change focus or hide/destroy target | No command runs on a different panel or stale target; text/modal guards reevaluated |
| Switch Edit -> other pages -> Edit repeatedly | Toolbar/tabs and stacked-timeline choice survive in native-origin, embedded, floating, grouped and maximized placements; inactive page stays hidden |
| Stabilize / Fixed Playhead / viewer toggle / future audio | Timeline context eligibility, unchanged native operation, hidden Inspector preserved, correct viewer targeting; audio operation and native Undo verified separately |
| Native pointer editing and playback at narrow/scaled sizes | Click/select/trim/drag/scrub/scroll, waveform/ruler alignment and actual GPU playback remain correct; synthetic widgets alone insufficient |

Reuse focused setup from existing panel-scale, timeline-overflow, panel-command,
native-preset, native-edit-command and native-entry-flow fixtures. Extend shared
fixture support only where behavior requires it; do not clone an entire harness
or grow one test file indefinitely. Windows shared callers need review and build/
fixture/live evidence; unavailable Windows checks remain explicit gaps.

## Numbered contract mapping and completion gate

Reread [the numbered entry-flow contract](opengate-native-entry-flow-acceptance.md)
before implementation completion or an installation proposal. Relevant mappings:

- Items 1–3 and 11: scale/width settings and named/unsaved state survive page return,
  restart, default selection and exact native restoration through existing owners.
- Items 4–5 and 8: focused commands remain usable in native-origin and managed
  arrangements; dragging titles, detaching, retained recall and native gestures
  keep their actual entry points and behavior.
- Items 6–7: compact Timeline toolbar does not alter independent workspace bands,
  primary ownership or hidden-space/maximize accounting.
- Items 9–10: Timeline/toolbar/tabs stay one element; reachable overflow, flat
  Panels access, native painting and real content fill remain mandatory.
- Item 12: run the assembled first-open -> title drag -> vacancy reclaim -> band
  combinations -> maximize/restore -> Save/Update -> restart -> Native -> temporary
  maximize -> persistent gesture -> workspace-selection sequence, plus real page
  switches and remapped keys. Map each action to its production entry point and
  evidence; list remaining gaps instead of relying on isolated fixtures.

Record implementation, compilation, fixtures, installation and live acceptance
separately for each branch. Initial setup changed documentation only. The implementation checkpoint below
records subsequent source/build evidence; it does not close the full requirements.
The installed runtime, user settings, projects and best-known rollback are unchanged.


## Implementation checkpoint and actual gaps

Integrated source is on `integration/timeline-content-20261030` in
`timeline-content-candidate`. The independent feature histories remain intact:
focused commands `4934fb5` (plus guard extraction `4c3749d`), scale-state extraction
`7f1c07d` and graphics experiment `8bf5f93`, and read-only Timeline topology
inspection `ef7db1f` (reusing the earlier presentation probe).

| Requirement | Actual source/build status |
| --- | --- |
| Command mappings restricted to their firing panel | Implemented in the existing dispatcher/catalog with read-only `PanelCommandContext` and `NativeShortcutContext`; native-preset fixture verifies Timeline Stabilize sharing a key with native Media Pool, yielding outside Timeline, same-context collision rejection, text priority and queued target stability |
| Shared increase/decrease/reset panel content commands | Registered through the existing registry, workflow generator and native editor route; integrated fixture uses two actual isolated sessions to verify independent values, one existing save callback per change, reset, native metric restoration and retained scale on re-entry |
| Scale responsibility extraction | `PanelContentScale` now owns the existing value and reversible application; session removes `metricFactor`/direct metric-adapter ownership and delegates lifecycle application; existing `interface_scale` and workspace save remain the only persistence path |
| Timeline body scale, compact toolbar | Opt-in `TimelineContentScale` uses actual Qt scene transforms with inverse input mapping; fixture verifies native scene geometry/hits and unchanged toolbar height/font. It is disabled by default with `OPENGATE_TIMELINE_GRAPHICS_SCALE_EXPERIMENT`; custom Resolve painting, complete ruler/header coverage, native temporal zoom interplay and GPU behavior are unverified. Missing capability does not grow only the toolbar or claim success |
| Pinned left buttons, draggable width and visible scrollbar | Not implemented: a safe native track-control/scene boundary has not been established. The width branch remains isolated. No invented width state or test-only adapter was added |
| Toolbar and stacked tabs surviving page changes | Not repaired: current logs establish page-return transitions through zero-sized ancestors, not the full cause. Reused lifecycle probe plus bounded graphics inspection now record native scene/proxy/view topology without visibility, input, layout or project writes |
| Remembered header width, pinning and header scroll position | Still open with the width adapter. Existing scale retention is exercised; this is not evidence of completed header-state persistence or fresh-process native live acceptance |

The combined universal Mac helper and IO bootstrap build against Resolve's Qt
libraries with warnings as errors; both contain arm64/x86_64 and ad-hoc signatures
verify. The native bootstrap handshake self-test and integrated hidden entry-flow,
command-menu and panel-scale suites pass. Command/preset and read-only topology
fixtures also pass on their source branches. Generated Mac and Windows workflow
wrappers pass; Windows native compilation and live acceptance are not performed.
Exact artifact identity, hashes and checks belong in ignored
`build/macos/timeline-content-build-receipt.json` rather than a moving claim here.

Organization review: text editing moved out of `InteractionDiagnostics`; conflict
inventory/validation moved out of `PanelCommands` and its original implementation
was removed; panel scale state/application moved out of `ManagedPanelSession`.
Panel targeting, native scope translation, scale-command registration and native
scene inspection have distinct modules. No new panel registry, input event filter,
settings file/writer, project writer or toolbar visibility controller was added.
`PanelControls` gains only the registration include and lookup wiring. Review sizes
at this checkpoint: dispatcher 173 -> 159 lines; coordinator 1,288 -> 1,293;
session 690 -> 691. Related scene/scale logic stays together; no arbitrary part files.

Remaining external gate: actual renderer capabilities and toolbar/tab loss need a
fresh Resolve session loading this candidate. The currently running process cannot
load a replaced helper merely by rebuilding it. Do not quit the user's project or
install over the preserved runtime without authorization. Native keyboard editor
save/conflict presentation and actual scoped key coexistence also require that live
check. This candidate is not a completed width feature, toolbar repair, release or
live acceptance. Reread the numbered contract and record every assembled gap before
an install proposal; the hidden entry-flow fixture does not satisfy native painting.

Native registration caveat from final source review: the policy fixtures exercise
OpenGate's preset-input route. Resolve still owns original Workflow action
registration and its native shortcut dispatcher. Yielding a key in our filter does
not alone prove that an original native Workflow binding cannot fire or reserve it
outside Timeline, nor that the native editor accepts/displays the desired scope.
Those native-dispatch and editor checks are mandatory before calling panel-aware
mapping complete. Do not replace them with another synthetic fixture or mutate
native action owners/connectors without establishing the supported boundary.

Fixture isolation correction: the first integrated focused-scale test lacked its
`UNBOUND_SHORTCUTS_FILE` override and created a 105-byte legacy Shortcuts.json in
Application Support/Unbound. The initial directory inventory contained only
Arrangements.json and Workspace.json, so this was a newly created test file.
Its three exact F17/F18/F19 scale entries were verified and the file was moved,
not deleted, to ignored `build/macos/fixture-isolation-incident/` to restore the
prior absence. Native Resolve preset files were not written. The harness now
sets its shortcut file inside its existing QTemporaryDir; verify absence before
and after the corrected run. Do not describe the earlier run as fully isolated.


## Content viewport correction — October 31

Jordan's live screenshot rejects the first candidate: Media Pool headings grew
while rows/clip text stayed small, and Timeline reported unavailable. This is a
failed content-scale acceptance result. Scaling must enlarge actual native
content, keep chrome compact, and leave panel width independent with reachable
scrollbars; native minimum geometry must not lock the outer panel wider.

Isolated `feature/panel-content-viewport` replaces Media Pool's root-wide widget
metric path with a live Qt canvas viewport around the existing bins/clip splitter.
It reuses the native layout-slot lease; original widget/model/action identities
remain. The native title and toolbar remain outside that viewport. A focused
`ScalableContentViewport` owns rendering/inverse input/scroll geometry;
`MediaPoolContentScale` owns reversible canvas attachment and retained viewport
state; `PanelContentScale` retains the scale/canvas-width/scroll schema. Session
and workspace persistence delegate to that state owner. No second settings store,
project model, global font change or screenshot-based renderer was introduced.

The captured native Timeline is `UiTiSequenceView : QGraphicsView` with a
`BmdDavUI::UiGraphicsViewportWidget` native paint surface. The renderer adapter now
accepts that observed combination instead of rejecting every native viewport.
This test candidate enables its scene scaling on ordinary launch (explicit `0`
remains an opt-out), restores its original transform/scroll policies, and applies
the retained factor to newly created stacked views through local deferred events.
This still needs actual Resolve GPU, clip/track/ruler and pointer acceptance.
The independent pinned-header divider/frozen controls are not implemented by this
whole-scene scaling correction; do not call that width requirement complete.

`NativePanelPosition` gains an explicit covered-content acquisition option,
defaulting off for every existing caller. The Media Pool adapter uses it to
prepare a covered group/tab without revealing it. Semantic ancestry crosses Qt's
embedded-widget boundary for command focus and native Media Pool navigation.
Original broad widget metric scaling is removed from the production Media Pool
path; no parallel scale owner or extra global input filter was added.

Behavioral checks cover fixed-size native delegate drawing at larger physical
size, a narrowed panel with draggable overflow, pointer selection of the original
item, inline rename protection, focused key targeting, exact owner/layout/metric
restoration, serialization into a fresh state owner, explicit zero-scroll reset,
covered-tab/editor preservation, lazy stacked-view scale, and zero native-window
recreation warnings. The assembled entry-flow fixture is also required. These
are Qt integration fixtures, not actual Resolve acceptance. Windows callers are
reviewed through shared core; Windows native compilation/live verification remain
open.

Installation checkpoint: corrected source `6c3ffb4` built as a signed universal
helper and installed under Jordan's existing approval after Resolve exited.
The staged install/rollback rehearsal restored its baseline byte-identically.
All 21 installed hashes and signatures verified, and all 197 protected settings
and unrelated plugin files matched their pre-install hashes. Ordinary launch
loaded `6c3ffb4` in PID 34040, session
`3d22e456-2a42-423a-975c-8b626fed2a2c`; helper start and controls-ready events
confirm loading, not content rendering acceptance. Receipts are ignored local
artifacts in `build/content-correction-install/installation-receipt.json` and
`build/macos/content-correction-build-receipt.json`. Transaction
`0E71A8E7-4288-4E2D-9806-F67CA9047082` retains the previous 8149657 installation;
its earlier transaction still retains 684bc2f.

The frozen-source panel-scale and assembled native-entry fixtures pass. Diff
review confirms that Media Pool's old root-wide metrics call is replaced by
canvas routing; the session and persistence coordinator only delegate state.
No second model, settings writer or global input filter was added. Actual
Timeline GPU painting/input, Media Pool custom content, native shortcut editor
scope behavior, pinned header width and toolbar/stacked-tab page-return loss
remain open. A project must be open and the actual scaling/page-return gestures
exercised before those native acceptance results can be recorded.

### Live rejection: fractured Timeline drawing

Jordan's October 31 10:06:10 and 10:06:23 screenshots reject Timeline scaling
in installed `6c3ffb4`. At 227%, track-button labels/borders, glyph size/spacing,
and the surrounding layout do not enlarge coherently. This is not acceptable
content scaling, even though clip thumbnails and waveforms visibly enlarge.
The Qt rectangle/inverse-hit fixture does not model these native drawing paths
and cannot establish their correctness.

Runtime events confirm the experimental transform was applied at 1.25 through
2.34, ending at 2.27. The captured native topology contains nested track-control
and clip `QGraphicsView` surfaces inside the outer sequence scene. The last
available topology capture precedes the scale change and shows no items with
`ItemIgnoresTransformations`; do not claim that flag explains the corruption.
The exact native drawing/cache mismatch remains unproven. Source applies one
outer-view transform; this approach has now failed native acceptance and is not
a supported Timeline scaling capability. Preserve this evidence and the prior
rollback. No installed binary or user scale preference was changed during this
read-only review.

Renderer investigation found a concrete mismatch in the installed 21.1.1 ARM64
library. `UiGraphicsPaintEngineEx::drawStaticTextItem` maps each cached glyph
position through the painter transform, then passes the existing text item to
`GraphicsRenderer::DrawStaticTextItem`. The world-transform bypass helper also
explicitly disables the painter's world matrix. This supports the screenshot's
stretched advances/fixed glyphs and inconsistent UI borders; a generic Qt view
transform is not a coherent renderer scale. No native functions were invoked or
patched during this static inspection.

`fix/timeline-native-scale-capability` removes the observed-native-class exemption
and makes ordinary Qt scene transforms opt-in again. The regression fixture
rejects native paint surfaces even with the experiment flag, restores previously
applied transforms on rejection, and retains preference/chrome. This is
containment, not implementation of the requested Timeline scaling. It passes the
panel-scale fixture; the first sandboxed attempt aborted because macOS window
services were unavailable, and the authorized unsandboxed rerun passed. The live
installation remains 6c3ffb4.

The isolated coherent-render experiment takes a different rendering approach:
native scene drawing remains at 1:1 world coordinates, with backing-store device
pixel ratio enlarging glyphs and positions together. `TimelineRenderProbe` owns
bounded opt-in comparison capture only; the existing `TimelinePresentationProbe`
triggers it at its existing next-turn content observation boundary. It is not
an interactive renderer, input forwarder, new Timeline model or replacement
viewport. No native renderer pointers/private offsets are invoked. Ordinary
launch does not capture project imagery; `OPENGATE_TIMELINE_RENDER_PROBE=1`
explicitly enables at most two visible native views, each at 1x and 2x, in the
existing local diagnostics directory. No user state is changed.

The coordinate experiment reproduces the observed failure contract (transformed
glyph positions with fixed glyph size) and verifies that backing-store scaling
enlarges glyphs coherently without changing the registered views/scene geometry.
That models the mismatch; it is not a native Resolve test. Actual native probe
images must be inspected before an interactive approach is selected. Playback,
native input/dragging, focus, scrollbars, sharpness and pinned width all remain
unverified for this approach. Do not install or describe this diagnostic probe
as the requested working content-scaling fix.

Probe checkpoint: frozen source `08ff5f7` builds for arm64 and x86_64; helper
signatures, bootstrap self-test, render-probe fixture and assembled native-entry
fixture pass. The staged install/rollback rehearsal passes and restores its test
baseline byte-identically. Local receipt:
`build/coherent-render-probe-install/build-receipt.json`. Not installed; Resolve
PID 34040 is still running 6c3ffb4. Loading the opt-in native comparison probe
requires Jordan to save/quit and a fresh process; the running helper has no
entry point for this new capture code. No claim of interactive scaling repair.

Organization review: the failed native-class capability exemption is removed
from the existing renderer adapter. The new 66-line probe owns visual diagnostic
capture, and the existing presentation observer only invokes that component at
its established observation boundary. No duplicate Timeline/state/input owner,
new preference writer, native pointer offset access or unrelated coordinator
implementation was added. PNGs are bounded, explicit opt-in local diagnostics.
Shared Qt code compiles in the macOS adapter; Windows native compilation and
native rendering remain unverified.


## November 1 complete implementation candidate

This supersedes the diagnostic-only checkpoint above. The feature branch
`feature/timeline-content-viewport` implements the interactive content viewport.
The native scene paints at 1:1 world coordinates into a backing-resolution image;
there is no outer native-view scale transform. Media Pool uses the same painting
principle while retaining Qt proxy input, focus and popup handling. Its actual
transferred content surface now receives the scale adapter.

Timeline has a draggable internal header divider, blue/red button pinning, a
separate header scrollbar, and its existing time scrollbar beside it. Narrowing
the header increases native clip-body space. Scale, preferred width, pin choice
and scroll position use the existing session/workspace checkpoint, including
unsaved state. Responsive width clamping does not overwrite the preferred width.
The toolbar retains its compact dimensions. Page-return reconciliation retains
independent native toolbar and stacked-tab choices without invoking the native
Display Stacked Timelines action or revealing a user-hidden host.

Production responsibility review: TimelineContentScale owns reversible native
view leases and retained configuration; TimelineContentViewport composes one
presentation; TimelineSceneRenderer draws the native scene; TimelineTrackHeaderLayout
owns geometry and inverse mapping; TimelineInputRouter forwards native input;
TimelineTrackHeaderControls owns divider/pin/scroll gestures; and
TimelineAttachedPresentation owns native band visibility intent and page recovery.
PanelContentScale delegates adapters and ManagedPanelSession only wires them into
existing ownership/persistence. ScalableContentViewport owns Media Pool's Qt proxy
presentation. The replaced transform implementation and diagnostic-only render
probe were removed. There is one native scene/model, one saved-state path and no
new global input observer. Fixtures reuse the existing native-window harness.

The assembled Timeline fixture passes: divider/body allocation, pinned native
buttons, reachable rightmost header control, separate time scrollbar, coherent
custom glyphs, clip hit testing, context-menu placement, inline rename/focus,
stacked view discovery, narrow native minima, fresh-owner state restoration,
page-return bands, explicit user choices and exact native return. The final
persistence assertion previously held a dangling reference into temporary JSON;
it now copies QJsonValue and passes. Earlier panel-scale and assembled entry-flow
results are being rechecked against frozen source before installation.

Contract mapping: entry, title dragging, vacancy reclaim, native return and
workspace transitions remain on the existing assembled entry-flow harness;
this change specifically exercises clauses 7/9/10/11 for content geometry,
reachable controls, Timeline bands and page return. Installation hashes/build
checks are separate from live Resolve acceptance. Actual native GPU playback,
editing gestures and restart acceptance still require the installed candidate;
macOS fixtures cannot establish those or Windows integration. No live acceptance
claim is made by a passing fixture.


## November 1 live regression follow-up

Jordan tested c92fe3b and reported correctly placed clicks but missing reachable
header scrolling after narrowing, cursor glitches, native commands not behaving,
and audio painting over video at 300%. The build is not live-accepted. Logs confirm
that session loaded c92fe3b and that Tab is the native controlGoToMousePointer
assignment; its native Shortcut events fired. Existing topology captures do not
contain the actual 300% overlap, so its precise live cause is not established.

At Jordan's explicit request, restored 6c3ffb4 at 19:50 UTC while repairs continue
on `fix/timeline-header-overflow-input`. All 21 managed files match its checkpoint,
signatures verify, and 197 protected files stayed unchanged. Receipt:
`build/timeline-complete-install/rollback-receipt.json`. The c92fe3b payload and
source branch remain preserved. Do not silently replace the restored checkpoint.

The repair candidate reserves an always-painted 18-pixel header footer independently
of native content height and projects the native time scrollbar beside it. Scaled
painting is bounded by section raster devices, including native paint paths that
drop their clip. Native hover cursors come from the actual nested scene item or
viewport, retaining mouse-grab continuity. Existing mouse forwarding stays on the
same original scene/model and is exercised across the header/footer boundary.

Native Qt shortcut delivery uses the existing command filter's optional adapter,
without a key/command whitelist or another binding registry. The focused Timeline
adapter aligns the original view's coordinate origin during synchronous native
shortcut delivery, then restores it immediately. This lets a native handler query
QCursor::pos() through its original viewport at the displayed location. Qt chooses
the command and retains its assignments/context. Text editing, other panels,
ambiguous shortcuts and ordinary native return keep their previous dispatch.
A lifetime guard tolerates command-owner retirement during native dispatch.
Asynchronous native handlers that query coordinates later still need live checking;
fixtures establish synchronous delivery, not every Resolve command's internals.

Both title and native-panel scale menus share one live 1-percent slider and typed
percentage editor, covering 70–300%. Intermediate values are not restricted to
preset menu entries. Updates coalesce at 16 ms; release/menu dismissal flushes the
exact final value. Reset cannot be overwritten by a stale pending update. Menu and
focused-key increments preserve custom precision, for example 263% -> 273%.

| User action | Production entry and local regression |
| --- | --- |
| Narrow the pinned header and reach hidden controls | Divider -> shared header layout -> real bar; bounds and actual painted thumb tested even when native content minimum height exceeds the display |
| Scale and seek to the pointer with the native key | Existing Qt Shortcut -> TimelineShortcutCoordinates -> input router; original handler receives mapped point once, view position restores and OS pointer does not move |
| Drag clips across header/footer and hover clip controls | Input router -> original nested scenes; continuous grab coordinates and native cursor tested |
| Scale audio/video content without cross-section paint | Section-backed scene rendering; regression reproduces an escaping native-style painter and contains it |
| Choose 147%, 213% or 263%, drag continuously, then Reset | Shared scale-menu row -> existing setter; coalescing, typed values, exact final flush, reset and focused-key precision tested |
| Return to native ownership and reload saved state | Existing view leases/session checkpoint; assembled native-entry and viewport fixtures retained |

Interaction checkpoints include 70%, 73%, 85%, 100%, 125%, 147%, 200%, 213%, 263%
and 300%. The production diff keeps geometry, rendering, input, command coordination
and control editing in their existing focused boundaries. The old outer-cursor
lookup and duplicated title-menu slider path are removed. The only added command
coordination module connects existing focus ancestry to existing presentation;
there is no new scene/model, global event filter or settings writer. The coordinator
changes are interface wiring, not implementation growth. Both scale-menu callers
are exercised. Shared Qt code is built in the macOS adapter; Windows native builds,
actual GPU playback, every native shortcut and ordinary restart still require their
own live validation. Frozen build/fixture evidence belongs in the repair receipt;
it is not native acceptance of contract clauses 9–12.

## November 1 performance gate before the next installation

The input/header repair is built and preserved separately; `6c3ffb4` remains the
installed checkpoint. Jordan requested performance checks before replacing it.
`perf/timeline-render-work` separates native pixel damage from geometry work,
reuses bounded section raster devices and removes the extra full-frame copy.
The matched optimized synthetic workload improves playhead and pan timings across
70–300%; full-pan p95 at 300% remains about 24 ms. This is not a guarantee of native
playback at every frame rate. See the [performance review and live gaps](opengate-timeline-performance-review-20261101.md).
Native functionality, persistence, GPU rendering and playback acceptance remain
open until the completed candidate is loaded into a fresh Resolve process.

### Edit-point cursor feedback

Jordan also reports edit-point/transition operations succeeding while the pointer
remains an arrow. The old router sampled cursor state only after mouse events and
could choose static scene-item state over a later native viewport cursor change.
`TimelineCursorRelay` now owns native hover/grab cursor mirroring through local
CursorChange subscriptions along the hit path. The router keeps event forwarding;
its previous cursor resolver is removed. There is no polling, second command path
or synthetic trim/roll mode. Native QCursor artwork and hotspots are preserved,
and leaving/hiding the presentation clears its cursor and subscriptions.

The regression reproduces missed viewport cursor changes before the repair.
Native edit-item hover changes, native viewport changes without another mouse
move, custom bitmap/hotspot preservation and leaving the edit point pass at 70%,
100%, 147%, 213% and 300%. The full Timeline and AddressSanitizer fixtures pass.
Actual Resolve trim-in/out, roll, transition and modifier cursor acceptance is
still required; these fixtures do not establish the cause of every live report.

### Authorized installation of the performance/cursor candidate

Jordan confirmed Resolve was quit and requested installation. Installed runtime
`61aceef` at 2026-11-01 23:01 UTC. All 21 managed files match the staged hashes,
the installed bundle signature verifies, and its runtime manifest records clean
source and O2 optimization. The previous `6c3ffb4` runtime is preserved in rollback
transaction `6DBDAE32-41E6-4F7A-9C64-D8F83E16D23E`. User project/preferences are not
installer write targets. Receipt: `build/timeline-performance-install/installation-receipt.json`.
This supersedes the earlier installed-checkpoint statements. Native playback,
GPU/editing/cursor behavior and restart acceptance remain pending live testing;
there is no merge, push or publication.

### Live rejection and verified rollback — November 1, 23:17 UTC

Jordan rejected `61aceef`: Undo/Redo do not reliably update the visible Timeline,
Go to Mouse Pointer fails, and paint/command/cursor feedback can recover only
after backtick/maximize or a maximized window. Blue/red track controls staying
pinned during scrolling are positive partial evidence, not acceptance of the
Timeline. Native commands and immediate feedback must remain correct at every
scale and ordinary window geometry; maximizing cannot be a required workaround.

Preserved raw/rotated logs and the post-quit log tail in
`build/timeline-live-undo-redo-review/20261101T231332Z/`. Runtime `61aceef`, PID 48189,
session `1f3f59d9-1cb4-4ba0-ab4c-f0986806d07f` logged 11 Undo/Redo shortcuts and
9 native `editUndo` plus 2 native `editRedo` QAction signals with Timeline focus.
Those establish delivery/action signals, not model changes or correct pixels.
The existing diagnostics do not record frame/damage evidence around Undo/Redo.
Recovery after maximize suggests missed damage/stale geometry, but does not
establish the exact cause or justify a speculative command-specific workaround.

At Jordan's request, restored the exact `6c3ffb4` checkpoint from transaction
`6DBDAE32-41E6-4F7A-9C64-D8F83E16D23E`. All 21 installed hashes and the bundle
signature verify; all 197 protected paths are unchanged. Receipt:
`build/timeline-performance-install/rollback-receipt.json`. The candidate and
source remain isolated and rejected for live use. This supersedes the preceding
installed-runtime statement. No new repair is claimed by this rollback/log review.

## November 2 native command and independently sized header repair

All controls, including blue/red buttons, now scroll together inside the left
area. Native row geometry keeps track alignment. Independent control/timecode
size uses existing persistence and the stated fit policy limiting control size
to body scale; automatic row growth is not implemented. Queued native pointer
reads and paint-only native edit/Undo/Redo have before/after result tests that
check actual model state and pixels without maximize. See the
[repair, module review and evidence](opengate-timeline-native-command-repair-20261102.md).
Timeline/ASAN and relevant regressions pass; actual native GPU/private command,
preview and playback acceptance remain open. Installed 6c3ffb4 is preserved.
