# Color Manager update flow — November 5, 2026

## Installed limited trial — November 5 afternoon

Jordan closed Resolve and authorized installation. Runtime `e9f11fe` is now
installed; all 25 managed-file hashes match the staged payload. Transaction
`376BB1CC-9872-4A18-ADA5-C161CC91E9DF` preserves the previous installation.
The fresh closed-session inventory verified 197 protected files unchanged before
the explicitly authorized additive palette import. The real transfer model merged
34 saved presets and two pending drafts. The latest profile and original export
were backed up; the original export was not changed. Receipts are under
`build/color-toolbar-diagnostic-trial-20261105/closed-install-20261105-142829/`.

The final optimized ASAN assembled entry check initially failed two assertions
because its lookup still required the old QToolButton type. Production now uses
QAbstractButton for the native-sized vector control. Correcting only those two
fixture lookups preserved every assertion and the complete rerun passed with zero
failures. No production code changed after the staged runtime build.

The numbered native-entry contract was reread before installation. Its assembled
fixture covers default entry and workspace protections (1–2), Native return and
temporary/persistent gestures (3–5), native bands, flat Panels, visibility and
drag/reclaim paths (6–9), maximize/restore (10), and the assembled sequence (12).
Actual native pointer/GPU behavior, page/restart restoration and Windows parity
(1, 8, 10–12) remain live gates. The reported project dropdown regression is an
open native-control behavior gap (6, 8); it is not repaired by this trial.

This installation is for picker, saved-preset refresh, menu-scope and single-row
texture-control testing plus bounded diagnostics. Project color persistence,
timeline-tab restoration, native Undo/parity and thumbnail paint leakage remain
unfinished. Installation does not establish live acceptance or release readiness.

This is a source/fixture checkpoint on `feat/color-manager-update-flow-20261104`,
based on `6634ee9`. It does not describe a new installation or completion of
project-persistent colors. The running local visual trial remains separate.

## Observed workflow and changes

Jordan's supplied recording showed successful custom paint selection, then an
editor draft changing its sample while the actual assigned clip retained its
previous appearance. Update/Revert were below the visible form. Solid colors
also displayed labels for hidden gradient fields. Visual frames were reviewed;
the recording's audio was not transcribed.

The edit form now owns its scrollbar; Update/Revert and draft status remain
outside it. The manager's old outer editor scroll wrapper is removed. Gradient
labels hide with their fields. Saving a named preset refreshes its existing
OpenGate assignments; uncommitted edits affect the sample only. A saved update
creates one immutable definition per changed preset and one binding revision per
view. Native clip `update()` requests invalidate the nested clip/proxy pixels;
root-only invalidation reproduced stale gradient pixels in the fixture.

The color field has a clickable pill with a compact live picker. Its saturation/
brightness plane, hue control and hex field update the existing draft/sample as
the user changes them. The picker labels the selected gradient stop. Done keeps
the draft; Cancel/Escape restore the color present when that pick began. Update
Preset remains the save operation. The modal `QColorDialog::getColor` path is
removed. Changing preset/stop closes a stale picker before changing its target.

An explicit custom clip paint must take precedence over track color. Track
changes must preserve the explicit clip assignment. Native Clear releases the
custom display binding to current native paint. Actual Resolve inheritance,
native history and portable assignment storage remain live acceptance gates.

## Responsibility review

- `ColorPresetEditor` owns one draft and selected-stop controls, including its
  form scroll and fixed save actions. It does not save files.
- `ColorPickerButton` owns only RGB selection UI and its transient popup. It has
  no palette, native clip, command, project or history dependency.
- `ColorManagerWidget` composes existing owners and routes Update to the palette;
  its redundant scroll wrapper is removed.
- `TimelineClipColorPreview` observes committed palette definitions for its
  existing session. `ClipColorPreviewCommands` supplies that existing dependency.
- `TimelineClipColorBindings` refreshes its existing weak bindings from a vector
  of definitions and requests affected clip damage. It does not depend on palette
  file storage, add a second registry, or look up palettes during frame painting.

Removed presets retain their last assigned snapshot. Unsupported translucent
updates retain the last supported assigned paint; the editor explicitly reports
that transparency remains preview-only in this trial. Neither behavior is a
finished project assignment/history policy. New editor/picker tests reuse the
existing manager fixture; no parallel production model was introduced.

## Evidence and limits

Optimized AddressSanitizer fixtures passed for Color Manager, clip-color menus,
Timeline red/gradient/texture painting and the assembled Color Manager/native
entry flow. The latter exercises the existing contract's menu entry, flat Panels,
managed hosting, native return and workspace flow; this change does not replace
the numbered native-entry contract. Native dragging/GPU behavior, project restart,
real Undo, supported-page transitions and remapped native keys still need live
acceptance. Windows runtime is unverified.

The command fixture checks saved-preset pixel refresh without manual reapplication,
multiple assignments sharing one update, unrelated swatches avoiding invalidation,
track-driven native repaint preserving explicit paint, native Clear exposing
current native paint, and native shortcuts releasing only selected assignments.
This is not evidence of Resolve's underlying inheritance or native color history.

The manager fixture checks visible Update at both form scroll extremes and a
360-pixel dock. The picker fixture checks actual sample pixel changes while the
committed palette is unchanged, cancellation, selected-stop isolation, Done,
explicit Update and Escape from the picker text field. Wide/narrow manager and
picker screenshots were rendered and inspected. An initial added Escape test
held a raw pointer after the popup's correct deletion; its observer now uses
`QPointer`, and the sanitizer rerun passes.

Artifacts are retained locally in `build/color-manager-video-review-20261104/`.
No playback performance claim follows from these fixtures. The new picker only
runs inside the editor; saved-definition propagation runs on commits, with no
new playback timer, per-frame palette scan or animated texture generation.

## Jordan's actual presets

The original `Downloads/OpenGate Palette Presets.json` is unchanged, SHA256
`dc447ab67d2fe77a9db55fd11b7d961720d30812f18e99147b05e642cf106c98`.
It contains 16 saved presets and two differing pending drafts (Gray and Royal
Blue). The other pending records equal their saved definitions.

The compatible additive import at `Downloads/OpenGate Presets - Import into
Trial.json` only aliases the incoming empty `custom` category to `builder-custom`
to avoid the trial's same-ID/different-name category conflict. Preset IDs, names,
order and paint parameters are retained. The real native model validates a merge
to 34 committed presets and three pending drafts, preserving the existing trial
draft. This sandbox validation did not modify the running palette. Its last
read-only inventory still contained 18 presets; live import remains unconfirmed.
Cyan/Mint's transparency is preserved and remains unsupported for native painting.

Project save/reopen, DRP handoff, native fallback materialization, canonical menu
checkmarks, native per-swatch keymap registration, native Undo and Original UI
parity remain open in the consolidated color task list. The read-only SDK
selection probe last reported Resolve scripting unavailable. No private metadata
setter or private history address has been invoked in production as a substitute.

## Combined candidate and later read-only evidence

The isolated trial branch `trial/color-manager-update-flow-20261105` combines the
three focused changes with the already installed ordinary-launch trial adapter.
Runtime source `13ba354faf94eca96bc9d1d89d4f06aa011cb405` builds clean at O2 for
arm64/x86_64. The signed binary SHA256 is
`672541bc630737132d99c59bfe53183bdbb7f2b6099568f93102d2c959872042`.
Strict signature verification and an install/byte-identical rollback rehearsal
pass for 25 managed files across nine paths. The separate assembled entry-flow
fixture passes on this combined branch. This is staging evidence, not installation.

The fresh merge preflight retained the original export unchanged and validated
34 saved presets with two pending drafts from the latest trial files. The earlier
three-draft inventory above was an earlier snapshot; user edits remain authoritative.
Fresh input hashes must be checked/recomputed after Resolve closes, before any
actual merge. The running palette was not overwritten during this preflight.

On November 5, after Resolve had relaunched as a new process, the documented SDK
selection inspection succeeded: one selected item, consistent project/timeline/
item IDs across repeated reads, and no project writes. The receipt is retained
in the trial worktree's `build/color-selection-readonly-native-20261105/`.
The current connector's 21.1.1 API declarations still expose only stock clip
colors for SetClipColor and no TimelineItem third-party metadata or native Undo
transaction API. Marker customData remains a separate candidate, not verified
invisible assignment storage.

A further bounded static review found a private native graphics/item/model
association, but no safe Qt identity getter/property on the inspected container,
item or model superclass chains. It does not justify production raw-offset
access. Findings and hashes are retained in the original timeline-content
worktree's `build/rea-clip-colors/clip-instance-identity-20261105/`. The live SDK
item is not yet joined to a painted graphics object. That join and native history
attachment remain unresolved despite successful SDK connectivity.

The clip/track precedence follow-up reproduced a separate real menu-routing bug:
the old swatch-name fallback treated a titled Track Color palette as a clip
palette, appended clip commands and cleared clip bindings when a track swatch
was chosen. The detector now honors declared menu/parent-row scope and the
`changeTrackColor` identity before using its unnamed-palette fallback. Optimized
sanitizer checks pass for titled, separately parent-labeled and native-identified
track menus, while retaining late-populated clip submenus. No new global menu
controller or assignment owner was added. This repair is not yet in the staged
`13ba354` binary above.

Jordan's later thumbnail-view screenshots show an exposed gold block in the
thumbnail well while the custom footer remains teal/mint. This remains an open
paint regression; the screenshots do not determine whether the missed primitive
is a solid fill, partial skin or cached raster. Hold the prepared update while
investigating rather than claiming the existing full-body fixture covers it.

## Bounded native diagnostic follow-up

`ClipPaintTrace` is disabled by default. The local diagnostic trial can enable
`OPENGATE_COLOR_PAINT_TRACE=1`. It records at most 64 distinct large primitive
receipts per binding revision: kind, original fill color or pixmap cache identity/
size/source rectangle, mapped/pass-through result, clip/primitive geometry and
an explicitly ephemeral QGraphicsObject address. It never captures image pixels
or alters replacement decisions. Native callsites are not captured. Disabled
capture retains no regions and installs no raster observer. Repeated diagnostic
frames use bounded numeric hashes without JSON allocation/serialization.

`TimelineContentInspection` adds a focused public graphics-child walk, separate
from QObject parent/child lookup. It cross-casts graphics members to QObject and
records type/parent/scene/liveness, capped at 64 members and eight selected clips
per explicit paint command. It reads no private offsets, invokes no private
methods and retains no native pointer across event-loop turns. This supports a
scratch check of graphics reachability; it does not produce a persistent ID.
The fixture models the actual separate QObject+QGraphicsRectItem inheritance and
checks both public cross-casting and reported truncation.

The optimized sanitizer paint regression passes with primitive diagnostics
enabled, retaining metadata receipts while preserving the existing native text,
thumbnail, waveform, playhead and scale assertions. Diagnostic fixtures verify
deduplication, capture limits, new-revision reset and disabled behavior. These
are fixture receipts, not a native capture of Jordan's thumbnail leak.

The supplied installed app is now independently identified as Resolve 21.1.1,
build 21.1.10010. Its ARM64 slice hashes exactly to the retained analysis input
`801848888e276e40cefcc89011155702ddf67bb6ecfccff4de60ff505685e2a9`.
This matches static input identity; it does not validate calling private methods.

Further static evidence establishes native compound Undo grouping, but the Qt
method table does not expose Begin/Commit/AbortTransaction. An actual per-instance
metadata child action remains missing. The SDK TimelineItem ID getter's QString
return path is verified on a real native script proxy; that function cannot be
called with a model pointer or fabricated receiver. Public graphics traversal
is a useful next scratch probe, not a complete identity adapter. Evidence lives
under `build/rea-clip-colors/native-color-transaction-20261105/`,
`thumbnail-well-paint-20261105/` and `native-identity-adapter-feasibility-20261105/`
in the original timeline-content worktree. Tab hopping, project persistence and
the gold thumbnail-well leak remain open.

Final staging checkpoint: local diagnostic runtime
`6fc09b421d3d6ab2f0c3c261cf42ba0cf1e616a2` builds clean at O2 for arm64/x86_64.
Its binary SHA256 is
`a6b2109b849c7af6b06a4f5f51358eaac0b3dc4832323c31a625f71c2a17cee7`.
Strict signature, 25 managed-file hashes, exact install/rollback rehearsal and
the assembled entry-flow fixture pass. Startup/isolated-profile sanitizer checks
also pass. This candidate includes the clip/track menu guard, editor/picker fixes
and bounded diagnostics. No application paint-matching threshold was loosened.

The diagnostic stage is retained in the trial worktree at
`build/color-manager-diagnostic-trial-20261105/`. A read-only process check still
found Resolve PID 5130, so no installation or live palette merge was performed.
Before installing, recheck absence, recapture the 197 protected-path hashes and
merge from fresh closed-session palette/draft inputs with a backup. Keep both
the preceding visual trial and stable `660cdfd` rollback transactions.

Next native evidence requires that install window, then reapplying a custom
paint before changing thumbnail modes and exercising A → B → A. The paint receipt
can classify the exposed native background path; graphics-member receipts can
test public traversal and object recreation. These are diagnostic checks, not
acceptance of persistence, native Undo or a corrected thumbnail path. All those
user requirements remain release blockers.

## Branch integration and platform gates

Jordan asked which changes eventually reach main and how module boundaries,
persistence and Mac/Windows parity will be checked. These are acceptance gates,
not a declaration that all existing modules or platforms are complete.

The inspected main head is `f915bfc`. None of the relevant feature/fix/trial heads
below is an ancestor of that main head. Some fixes have already been cherry-picked
into the color candidate under different hashes; do not merge them twice.

| Branch | Scope | Integration decision |
| --- | --- | --- |
| `fix/rename-command-context-20261104` (`8ed4ab0`) | Shared text-entry and deferred-command context guard | Candidate for a focused integration after native rename/keymap acceptance; already included in the color lineage as `226f3ab` |
| `fix/scaled-context-menus-20261104` (`9ea066d`) | Native popup ownership above/below 100% and coordinate regression | Candidate for focused integration after real Media Pool/menu acceptance; equivalent work already in color lineage as `9288f8e`/`c2e6811` |
| `fix/timeline-switch-presentation-20261104` (`35998d7`) | Nested-scene presentation regression and investigation record | Tests may be harvested; this branch is not a persistent timeline-ID or color-restoration fix |
| `feat/color-manager-paint` (`6634ee9`) | Shared palette/editor, paint rendering trial and transfer | Hold full feature acceptance for persistence, history, native parity, fallback and platform gates; do not equate personal palette save with project assignment save |
| `feat/color-manager-update-flow-20261104` | Visible Update, live picker, saved-swatch refresh, menu guard and diagnostics | Review focused production commits against the feature dependency set; diagnostic code remains opt-in and is not evidence of repaired native failures |
| `fix/texture-toolbar-row-20261105` | One-row texture control and bright/muted state | Integrate the focused change into the local test candidate first; shared source does not prove Windows/native layout acceptance |
| `trial/color-texture-startup-20261104`, `trial/color-manager-update-flow-20261105` | Local profile paths, ordinary-launch opt-in and bounded diagnostic activation | Test artifacts only; exclude local profile activation/forced diagnostics from the eventual main/release integration |

No main merge or publication is requested by this discussion. Integration should
be a reviewed dependency set, not a blind merge of every experimental ancestor.
Preserve the accepted installation and test rollback before installation.

The source organization gate requires one job per module, one authoritative
owner per kind of state, and removal of the superseded path. Current ownership:
committed personal definitions in ColorPalette; pending editor recovery in
ColorPaletteDrafts; local texture-display preference in ColorDisplaySettings;
transient UI editing in ColorPresetEditor; RGB selection in ColorPickerButton;
native graphics bindings in TimelineClipColorBindings. Persistent project/clip
assignment ownership is still unfinished; graphics bindings cannot substitute
for it. The large shared PanelControls remains a coordinator: this toolbar
repair adds no implementation there. Existing oversized modules are not a blanket
organization pass; extract a touched responsibility in a bounded change.

Mac and Windows use shared PanelControls (`windows/managedworkspace.cpp` creates
it), while bootstrap, native window behavior and paths remain platform adapters.
The current color changes have Mac compilation/fixture evidence. There is no
current-branch Windows compile/runtime receipt establishing color-feature parity,
and earlier Windows workspace CI is not a substitute. An arm64/x86_64 Mac binary
also does not prove live Intel-Mac or Windows GPU behavior.

Required assembled edge cases before enabling the finished feature:

- A → B → A with duplicate timeline names, recreated clip objects, same source
  used in multiple instances, linked/multi-selection and correct active tab ID.
- Project save/reopen, Save As, timeline/project duplication, copy/paste/split,
  relink and DRP round trip through stock Resolve and another OpenGate installation.
- Explicit clip paint over track color; track changes preserve clip overrides;
  Clear restores verified inheritance. Native/custom/custom transitions include
  different custom presets sharing the same closest native fallback.
- One native Undo operation restores exact paint, provenance, fallback and menu
  state without disturbing neighboring move/trim/edit operations or native keys.
- All thumbnail modes, clip heights, partial exposures, scrolling and zoom,
  Native/OpenGate switching, cached labels, waveform pixels and moving playhead.
- Native swatches remain immutable; preset rename/reorder retains IDs/keymaps;
  removal, invalid imports, pending drafts and write failure preserve prior data.
- Native versus mapped versus context-menu routes; bin/clip rename, picker fields,
  delayed commands after editor exit, floating/docked focus and scaled menus.
- Narrow/wide one-row toolbar, on/off state and shared manager synchronization;
  supported scales and DPI/display changes on both OSes, preserving native owners.
- Matching-project performance comparison with texture on/off and many clips;
  no animated noise, per-frame file writes or unbounded paint/image caches.
- Actual packaged Windows startup/DLL integration and Mac arm64/Intel checks,
  then install/rollback with native projects, presets and preferences preserved.

The immediate native session should exercise the smaller targeted UI/diagnostic
test plan. It is not acceptance of the full matrix or a substitute for native
persistence/history work.

## Single-row texture control correction

`fix/texture-toolbar-row-20261105` changes only texture-control presentation.
The previous code appended a 24px font glyph to the outer QBoxLayout without
checking direction; the real `frameEditToolbarTop` owns a vertical layout, so
this created the extra row Jordan reported. The control now joins the known
native middle-buttons horizontal row beside position lock. Missing/not-ready
vertical structure does not create a fallback row. Native constraints, controls,
command registration and the existing overflow owner are unchanged.

The font-dependent glyph and standard toolbutton frame are replaced by a small
Qt icon button: bright checked texture grid, muted unchecked grid, native-sized
hit area, subtle hover/press feedback and explicit On/Off tooltip/accessibility
state. ColorDisplaySettings remains authoritative and the existing scoped command
still handles the click. There is no new preference or assignment writer, global
event filter or timer; PanelControls receives no additional implementation.

The old outer-layout append path is removed. The existing owned-button list now
retains weak toolbar/button pairs so idempotent discovery works when the button's
parent is the inner native row. Both platform callers retain the same interface.
The focused fixture reuses the existing display/manager checks and existing
PanelOverflow, verifying one row, native-sized alignment, wide/narrow widths,
reachable horizontal scrolling, one invocation, shared setting synchronization,
immutable palette data and distinct On/Off pixels. Wide-state screenshots were
rendered and inspected. Stock Aqua fixture buttons demanded42px layout height
inside the captured30px Resolve row; only the test stand-ins were corrected to
use their actual widget rectangles. Native controls were not changed to make the
fixture pass. Actual Resolve appearance and Windows runtime remain live gates.
