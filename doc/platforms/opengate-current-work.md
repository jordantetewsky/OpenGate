# OpenGate current work — October 29, 2026

**Development work, not a release. Audio-channel commands are not working end
to end. The unified native UI is a recorded implementation direction, not a
completed feature.** This page brings the current discussion together. The
full task list and evidence ledger (internal reference retained locally) retains
the detailed reports, historical results and other outstanding work.

## One native UI enhanced by OpenGate

the maintainer’s intended experience is one native Resolve workspace with OpenGate's
capabilities. The user should not manage Native versus OpenGate modes, perform
activation steps, or enter a reduced feature set after selecting Native. The
internal ownership and recovery mechanisms may remain; simplifying the visible
workflow must not silently replace the native behavior the user relies on.

- [ ] Make the OpenGate default an arrangement of this native workspace. Returning
  to it should be as simple as selecting the default in the workspace/layout list.
- [ ] Apply the same panel/container functions to native-origin, default, modified
  and saved arrangements: dragging, floating, neighboring splits, grouping, tabs,
  show/hide/recall, shortcuts, temporary maximize and per-panel scaling.
- [ ] Reclaim vacated panel space automatically without shrinking the user's core
  window or resetting unrelated positions.
- [ ] Detect placement/docking targets at the content edges of the core window,
  including otherwise empty native space. Preserve band clearance, occlusion and
  the agreed hold-to-bypass docking/snapping behavior.
- [ ] Provide minimum-width overrides in native-origin arrangements as well as
  custom arrangements. Preserve reachable controls, exact restoration and correct
  native painting; use appropriate reflow/overflow where needed.
- [ ] Reuse the resolved OpenGate header consistently. Panels must not overlap
  Inspector. Header and footer remain independently hideable/pinnable, release
  their height when hidden, and belong to the primary workspace.
- [ ] Preserve named and unsaved per-page layouts, retained hidden placements,
  group membership, pinning, display recovery and reliable underlying native
  restoration. No manual recovery sequence should be required for normal use.
- [ ] Reconcile the [numbered assembled entry-flow contract](opengate-native-entry-flow-acceptance.md)
  with this one-UI experience before claiming implementation complete. Component
  fixtures alone cannot establish the full user flow.

The October 28 investigation (internal reference retained locally)
documents the actual root-layout, input, sizing and docking boundaries. There is
no second Resolve editing engine. There is additional ownership/layout/input
coordination, and removing the mode label alone would not remove that work.

## Commands currently in progress

| Command | Required behavior | What exists / current gap |
| --- | --- | --- |
| Previous Audio Channel — All Source Channels | Cycle the selected audio instances backward through embedded source channels, wrapping | Registration and read-only planning exist. No accepted native execution/Undo workflow |
| Next Audio Channel — All Source Channels | Cycle forward through all embedded source channels, wrapping | Latest live attempt reached dispatch but failed context capture; no native action ran |
| Previous Audio Channel — Skip Excluded Channels | Cycle backward through configured channels, skipping excluded ones | Planning/fixture cases exist; same native execution and Undo gaps |
| Next Audio Channel — Skip Excluded Channels | Cycle forward through configured channels, skipping excluded ones | Planning/fixture cases exist; same native execution and Undo gaps |
| Reveal in Finder / File Explorer | Reveal the source file for one selected Timeline or Media Pool audio/video clip without requiring Find in Media Pool | Agreed command remains queued; no completed implementation is claimed |
| Toggle Fixed Playhead | Operate Resolve's actual Timeline setting without popup or focus prerequisites | Existing native menu-preparation signal/action route; local regression passes; preserve existing behavior |
| Stabilize Selected Clips | Operate on selected Timeline video clips without revealing Inspector | Existing selected-clip API route; local dispatch regression passes; preserve existing behavior and report live gaps separately |

Audio changes must affect the selected instances in the current active Timeline,
preserve other instances and Media Pool mappings, leave the UI/focus unchanged,
and provide a valid native Undo. The agreed unsupported/selection/error cases,
repeated invocation, wrap, scope switches and platform checks remain in the full
ledger. Native keyboard customization is the assignment authority.

### Why the audio commands remain unfinished

Fixed Playhead has an identified native preparation signal that can be invoked
without presenting its menu. Stabilize has a direct `TimelineItem.Stabilize()`
API. The earlier audio scripting setter changed a mapping but did not create the
required native Undo entry: Undo affected the preceding edit instead. the maintainer’s
manual native channel-menu operation did support Undo, motivating the native route.

The audio menu actions are created for a particular clip context. We have not
established an equivalent reliable, silent preparation entry point for each new
selection. Sending a synthetic right-click can also run native popup/focus/menu
tracking behavior. Earlier popup automation crashed Resolve and is not an
accepted implementation. This is not proof a silent native route is impossible.

### Latest live result and local correction

- Installed diagnostic runtime **[checkpoint]** on October 29, with a rollback to
  [checkpoint]. All 22 payload hashes and both signatures verified; all 232 protected
  preference/settings/unrelated workflow files were unchanged.
- The diagnostic needed a special process-launch setting, which explains the
  requested restart despite the correct project already being open. That is
  diagnostic setup, not the required finished command experience.
- Verified the designated disposable project, Timeline and selected mono polywav
  at source channel 3/Channel A. Captured a project-wide instance and source-mapping baseline for comparison, without project/selection changes.
- the project maintainer opened/canceled Audio Channel and invoked commands. Both workflow and
  preset routes reached the command registry. Requests were rejected with
  `fresh-manually-bound-context-required`: zero bound-context records, zero native
  channel invocations, and an unconsumed one-use permit.
- Readback by the original clip identity confirmed channel 3/Channel A and unchanged
  source configuration. No Undo was requested. The unused permit was archived,
  leaving execution disabled; do not keep repeating the same failed test.
- Local branch `fix/audio-context-receiver` reproduces capture failure when
  keyboard focus is absent before the context handler. Resolving the Timeline
  from the actual event receiver makes that regression pass for all four variants.
  Existing clip/hit/identity/expiry/one-use checks remain. Native-edit regression
  and arm64/x86_64 production syntax checks pass. No new input filter, selection
  writer or visibility owner was added.
- That correction is **not installed or live accepted**. The old logs do not
  establish focus loss as the sole cause of all attempts. The fixture does not
  prove real native effect, audio/repaint, Undo or fresh silent preparation.

Remaining gates are: (1) actual native action, intended-instance isolation,
audio/repaint and valid Undo; (2) fresh silent preparation without manual menu
setup; (3) the full four-command, wrap/repeat/error/context/platform matrix.
Passing a manually prepared one-shot test would not close gate 2.

## Findings about speed and Relay

the project maintainer reported Relay working better in a native-origin view, then also working
better in a heavily rearranged view. That later view still used OpenGate grouping.
The logs show a persistent gesture silently re-entered managed state after Native
was selected, but did not remount the full replacement native root layout. Mode
label, root replacement and floating/scoped arrangements are distinct facts.

Recorded current-build operation times identify optimization targets: menu/state
refresh within save had a median 27.722 ms across 265 samples, compared with
1.299 ms for persistence and 0.166 ms for capture. Those are scoped wall times,
not removable-cost estimates, total CPU usage or predicted speedups. Menu/state
substage timing is missing. Keep functionality while measuring redundant work.

Relay logged three recent jumps with matching requested/read-back timecodes, but
no request-start timestamps; this proves API readback success, not latency or
displayed-frame timing. Earlier rendering/uploads are a workload confound.
The existing responsiveness monitor excludes the main Resolve window, so its
pooled statistics cannot fairly rank native and floating layouts.

- [ ] Measure matching Relay request stages and UI operations with comparable
  layouts, media, workload and instrumentation before claiming a gain.
- [ ] Reduce demonstrated redundant menu/state/layout work through existing
  owners; preserve native commands, visibility, persistence and rendering.
- [ ] Report measured callback, visible-paint and end-to-end results separately.

## Bug reports and acceptance gaps

These are open reports or unresolved acceptance items; they are not a claim that
each was freshly reproduced in the current runtime.

| Report / gap | Evidence or finding | Current status |
| --- | --- | --- |
| Audio channel command does nothing | Latest live command dispatch reached registry, then context capture rejected; mapping unchanged | Open; local receiver correction uninstalled, silent preparation/Undo still unverified |
| Timeline loses its native toolbar | Recurring user report; recovery reset also loses Display Stacked Timelines choice; Timeline includes toolbar and tabs as one panel | Open; existing diagnostics are not a repair |
| Panels overlaps Inspector in native-origin view | October 28 screenshot; installed accessory does not reserve a Qt toolbar region | Open; unified header/clearance requirement recorded |
| Blank native layout areas remain | Screenshot and conservative native vacancy-collapse boundary | Open; identify the matching reserved branch and reclaim without hiding useful controls |
| Core-window edges/empty space lack docking targets | Source explicitly registers actual native leaves and excludes empty native space | Open; reuse existing target/layout mechanisms |
| Minimum-width overrides unavailable/inconsistent | User reports missing capability; selected overflow/width leases are not a uniform policy | Open; native, embedded and floating sizing parity required |
| Native selection still permits confusing custom arrangement | Runtime silently changes managed state on gesture; container title can be misleading in screenshots | Open workflow/presentation gap; remove user-facing mode burden |
| Narrow/maximized contents clip or fail to paint correctly | Numbered contract includes blank Timeline below width thresholds and Mixer content-fill requirements | Live acceptance remains required; proxy fixtures cannot prove native GPU painting |
| Floating visibility/blinking and retained recovery | Historical reports and specific earlier navigation repairs are documented separately | Keep independent presentation/recovery gates; do not infer all fixed from command work |
| Explicit Dock rejects a changed original slot | Existing full-layout report: original panel slot changed | Separate from Close/hide; remains in detailed repair ledger |
| Startup, unsaved restore, bands, display changes, hold-to-bypass, Windows parity | Full Panel/Container/Layout contract and existing task list | Continue tracked work; no blanket completion claim |

See the full task list (internal reference retained locally),
native-workspace findings (internal reference retained locally),
and [development branch index](../../WORK_IN_PROGRESS.md). These development
branches do not publish a release or update the installed runtime.
