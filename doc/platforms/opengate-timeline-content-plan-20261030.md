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
