# Native workspace, feature parity and Relay timing — October 28

Read-only investigation on `investigate/native-workspace-parity`, based on the
installed source `[checkpoint]`. No production code,
installation, preferences, project, layout or running process was changed.

## the maintainer’s clarified experience

The user should work in one native Resolve UI enhanced by OpenGate, without
having to understand or repair an internal Native/managed mode distinction.
Selecting the default layout from the workspace/layout selector should suffice
to return to the familiar arrangement. Native-origin arrangements must offer
the same functions as other arrangements: panel/container dragging, grouping,
tabbing, visibility/recall, maximize, persistence, header/footer, and docking at
the edges of the core window. Vacated space should be reclaimed. Minimum-width
overrides must also be available; Native selection must not be a reduced feature
set. Controls must remain reachable and native painting must remain correct.

These are the target and investigation scope, not an implemented redesign or
permission to silently remove existing recovery/ownership safeguards. The
numbered entry-flow contract still documents the current implementation's
Native-return path. A future one-UI implementation must reconcile that contract
explicitly while preserving recovery underneath the simpler presentation.

Screenshots at [test time omitted] and [test time omitted] EDT show blank native space, Panels overlapping
Inspector, and a heavily rearranged floating container after Native was selected.
The latter container is titled Source Monitor + Program Monitor while other
panels are visible. Screenshots establish the reported presentation, not its
complete ownership state or the cause of a rendering/performance issue.

## What the existing runtime proves

The installed bundle manifest and active session identify build **[checkpoint]**,
test process, session `[test identifier omitted]`.
The preserved capture contains all 18,007 event sequence numbers from session
start at [test time omitted] through [test time omitted] EDT, with no gaps or duplicates.

- At [test time omitted], OpenGate mounted its full replacement root layout.
- At [test time omitted], it restored the original native root layout.
- Native was explicitly selected again at [test time omitted]; completion was recorded
  at [test time omitted].
- At [test time omitted], a Program Monitor gesture entered managed state without
  loading a default. The mode change was recorded at [test time omitted]. Subsequent
  group drops succeeded at [test time omitted], [test time omitted] and [test time omitted].
- No later full-root mount is recorded in this complete session capture.

Thus selecting Native is not a permanent exclusion of OpenGate management.
It also does not imply every subsequent custom arrangement parks the whole
native root. The mode flag, full-root replacement, scoped native arrangements,
and floating panel groups are distinct implementation facts. The user's later
report that Relay feels faster is preserved as user evidence; it is not a
controlled latency result. Grouping remains active in that faster-feeling view.

## Measured work, with limits

These are synchronous operation wall times in the current diagnostic build,
not CPU utilization, total app latency, removable-work estimates or speedups.
Nested spans must not be added together.

| Recorded work | Count | Median ms | p95 ms |
| --- | ---: | ---: | ---: |
| Menu/state refresh within save | 265 | 27.722 | 50.090 |
| Persist phase | 265 | 1.299 | 2.090 |
| Snapshot phase | 265 | 0.166 | 0.298 |
| Header scheduling/wiring phase | 265 | 0.204 | 0.395 |
| Panel menu wiring | 17 | 26.348 | 32.193 |
| Group drop | 14 | 58.834 | 85.609 |
| Native-arrangement detach | 9 | [baseline count].778 | 77.782 |
| Native-arrangement initialization | 4 | 16.726 | 21.999 |

`PanelControls::save()` calls `updatePresetState()` after persistence, which
updates arrangement status and wires the workspace menus. Existing caching does
not make that combined phase free. Substage timings are absent, so these records
do not identify which menu/state function dominates or how much can be removed.
This is a concrete optimization target independent of a one-UI redesign.

One 96-update Source drag measured destination lookup median 6.507 ms and p95
8.070 ms. That path includes current native window ordering and target selection.
The measurement does not isolate the OS query. Do not cache z-order through a
drag without accounting for other applications and changing occluders.

The previous viewer-restore experiment separately reduced synthetic grouped
callback time from 8.37 to 4.89 ms (split) and 7.15 to 4.76 ms (tabs), but did not
establish improved first-paint p95. See the existing viewer-expansion-latency
report. This illustrates why faster internal work is not automatically faster
visible response and is not evidence of this proposed design's improvement.

## Can the logs prove Relay is faster?

Not end to end. Relay's activity log records three recent successful jumps at
[test time omitted], [test time omitted] and [test time omitted] EDT. All requested/read-back timecodes
match and return true. Its installed `main.js::setTimecode()` logs only after
`getCurrentTimeline`, `SetCurrentTimecode`, and `GetCurrentTimecode` finish.
There is no matching request-start timestamp or request ID. Inter-result spacing
cannot substitute for per-request latency. Successful API readback also does not
prove that the correct video frame was already displayed.

Relay's log also records rendering and uploads earlier in this hour, with the last
captured render/upload event at [test time omitted]. This is a workload confound, not proof
that those jobs caused the earlier slowdown. The same-media/workload condition
was not controlled across the user's earlier and later observations.

`ResponsivenessProbe` filters out `UiMainWindow`, so pooled `ui.responsiveness`
results do not cover native and floating arrangements equally. Pointer-age and
next-event-turn logs have broader coverage, but their sampled widgets, gestures,
typing, sample counts and background work differ between intervals. Those
observational summaries are retained for diagnosis, explicitly not as a layout
speed ranking. No measured percentage improvement, resource reduction, or causal
claim about root replacement is justified yet.

## Existing boundaries and feature gaps

| Boundary | Current job and finding | Next bounded work |
| --- | --- | --- |
| `NativeRootLayout` | Sole lease/restoration owner for the original native root. Full replacement retains the real tree; native visibility is required by prior loading evidence. | Compare retained-native/scoped and full-root paths without disabling the loading safeguard. |
| `NativePanelToggles` | Routes managed native panel controls and synchronizes checked presentation using coalesced events, not a perpetual mirroring loop. | Preserve single execution and intent/presentation separation; measure before removing coordination. |
| `NativeDockSurfaces` / `DockDestination` / `DockScopeTargets` | Registers real native leaves and resolves panel/retained-section targets; empty native/core-window space is explicitly excluded. | Add explicit core content-edge targets through this shared target mechanism and existing layout tree, with correct band insets and occlusion/bypass handling. |
| `NativeWorkspaceSizing` / `NativeVacancyCollapse` | Reclaims known empty native branches and relaxes the outer main-window constraint while keeping surviving child minima. Unknown custom containers remain occupied conservatively. | Identify the screenshot's reserved branch from a matching widget capture; avoid treating all blank-looking native widgets as disposable. |
| `PanelWidthAllowance` / `PanelOverflow` / panel sessions | Existing width leases and overflow apply to selected presentation paths, not every native panel. Viewers and Timeline retain native content minima. | Provide one panel sizing policy across native, embedded and floating placement, with exact restoration; prove reflow/overflow and live narrow GPU painting per panel. |
| `WorkspaceHeaders` / macOS header adapter | Installs main-window Panels as a native chrome accessory; this installed source has no layout-mode gate and does not reserve space in Resolve's Qt toolbar row. | Reuse the resolved header presentation consistently and reserve a real region so Panels cannot collide with Inspector. |
| `WorkspacePersistence` / arrangement UI | Captures native/managed state through existing snapshots; coordinator also refreshes menu/status after saves. | Separate needed persistence from redundant presentation refresh without creating a second save or visibility owner. |

At [test time omitted] the logged floating group had a 1408 logical-pixel minimum width.
At [test time omitted] native sizing recorded a 1345-pixel main-window minimum from the
surviving native layout. These are different constraints. Neither is a general
minimum-width override, nor proof of which individual child sets the floor.

## Discriminating measurements before predicting a gain

1. Preserve the current working installation/settings. Use the existing group,
   native-root and persistence owners; do not introduce another layout engine.
2. Measure the same Relay operation with request ID and monotonic timestamps at
   renderer input, main IPC entry, API completion and renderer acknowledgement.
   Record root ownership, active/key window, focused native control and layout
   identity alongside it. Measure video presentation separately if that is the
   reported delay. Existing logs cannot reconstruct these missing timestamps.
3. Compare equivalent retained-native/scoped and full-root arrangements on the
   same timeline/media, size and background workload with equal instrumentation,
   warm repetitions and separate cold results. Include failures and timeouts.
4. Measure menu/status refresh substages, native toggle coordination and target
   lookup. Prototype only the demonstrated redundant boundary and compare against
   the same baseline. Retest visibility, native commands, saved/unsaved restore,
   bands, narrow panels and Relay before claiming an experience improvement.

These measurements require a candidate with the missing instrumentation; this
read-only investigation neither installs one nor asks the user for a layout tour.
No production fix, compilation, fixture pass, installation or live acceptance is
claimed. Shared-core findings apply to Windows callers in principle; no Windows
build or runtime verification was performed. Future production changes must pass
the organization gate and the assembled numbered user flow.

Local evidence is in ignored `[local evidence reference omitted]`: reproducible
analysis script, session events, source hashes, transitions, sanitized Relay
observations, and summary statistics. No credentials or authentication files were
read or copied. Report/task-list changes add no state writer or runtime path.
