# Color Manager update flow — November 5, 2026

## Recovery checkpoint and next native gate

### November 6 live continuation

Runtime `4d743ff` is installed, transaction
`75C34AC4-D42A-4349-B9E9-FD05994B0CBC`; all 25 managed payload hashes were
verified. The receipt is in
`build/native-color-disposable-render-20261105/closed-install-20261105-214604/`.
Protected-settings verification happened after Jordan reopened and tested:
`UI.preset`, `activedb.conf`, `recentprojects.conf`, `config.user.xml` and
`dblist.conf` differed, as did the trial profile's `Workspace.json`. Those live
changes were preserved, with a second profile snapshot. Do not describe this
installation as a 197-file unchanged-settings verification.

The first owned disposable property test stopped at media import. The documented
`ImportMedia([{'FilePath': path}])` call returned `None`; the harness's `len(None)`
then masked that failure. Its receipt confirms that only the owned project was
closed. No clip identity/property round trip, custom metadata history, save/reopen
assignment or DRP acceptance was established. The generated media decodes
offline. The replacement test must record the import phase explicitly and run
mailbox waits outside a connector SDK script; it must preserve the original
failure separately from cleanup failures and retain the existing ownership gate.

Jordan supplied a working native dropdown baseline with two loaded projects:
`second test` opens a menu containing `New Project 1`. Jordan reports that entering
OpenGate and returning to Original Resolve UI stops that previously working menu.
This is a transition regression, not evidence of an invalid single-project test.
Read-only inspection found a native `fileSwitchProject` menu attached with two
entries and unblocked button/menu signals; it does not establish that real mouse
input or popup presentation survives the transition. On November 6 Jordan
explicitly lowered this issue's priority. Keep the regression open; stop the live
dropdown investigation and prioritize color persistence, audio clip precedence
and thumbnail paint coverage.

The recorded 226×228 palette rejection has a transparent center and is correlated
with the preceding `UiTiVideoClip` member receipt. It is not an audio body receipt.
Do not relax transparent-skin rejection or claim it explains audio precedence.

The replacement external phased runner passes five offline guard/phase tests.
Its November 6 read-only run connected through Resolve's bundled Python in
18 ms and verified the installed `4d743ff` binary and declared scratch inputs.
Receipt: `build/native-color-disposable-render-20261105/phases-20261106-075640-2f9b7e/receipt.json`.
It found Jordan's `WK 29 110526` project open and recorded zero project mutations
and zero property-write requests. Full testing remains gated on Project Manager;
the working project was not closed or switched. Loaded-dylib identity, actual
SDK/native UUID equality and property/history acceptance are not established by
this read-only result.

Priority is now the end-to-end assignment foundation: actual clip identity and
reversible metadata access, then project save/reopen and DRP storage, native
history, and an OpenGate → stock Resolve save/export → OpenGate return. The last
flow must preserve embedded definitions/assignments and a usable native fallback,
and respect native recipient edits. Passing a reversible property test alone
does not close persistence or handoff. Editor/paint polish and dropdown work must
not substitute for this acceptance.

The bounded rejected-skin cache repair was verified separately on November 6.
`ClipSkinPaint` remains the sole owner of native skin classification and now
retains at most 16 rejected-source classifications. Complete current-capture
nine-piece ownership and target validity are still checked before a cached
rejection is used. The repeated image-conversion/classification path was removed
for retained rejected keys; no native cache/effect setting or acceptance threshold
changed. Control/candidate instrumentation reduced 100 palette calls to one for
the same rejected source, with unchanged native output and working mutation/key
eviction. Artifacts are under
`timeline-content-candidate/build/rea-clip-colors/skin-rejection-cache-20261106/`.
The optimized assembled timeline paint fixture compiled and then ran in the main
conversation with `timeline red preview failures=0`. Its earlier worker run never
launched because the approval remained pending and eventually aborted. This
source-only repair is not installed; it does not establish persistence, native
audio precedence, handoff or Windows live acceptance. Responsibility review found
one bounded cache added to the existing owner, no parallel model/controller or
project-state writer, and focused checks added to the existing skin fixture.

The paragraphs below are retained checkpoints; their pre-install state is
historical. Project assignments remain temporary despite the newer installation.

After the tool-server restart, source is clean on
`integration/native-color-repair-20261105`, runtime source `4d743ff`. Installed
runtime is still `e9f11fe`; no newer installation transaction exists. The final
local package is `build/native-color-disposable-render-20261105/`, binary SHA256
`d7e726b3a7b5bb0abd222f0a6f0e5e5370e83f2b1bce344bdfee4fe8c3da97af`.
Universal O2 build/signature, unchanged declared scratch-source hashes and exact
installation/rollback rehearsal pass. The added cache/effect inspection fixture
passes and verifies that the inspected settings and effect ownership are unchanged.
The assembled entry fixture passed on the preceding production-equivalent entry
flow; the final addition only records explicit diagnostic facts. The numbered
entry contract was reread; native clicks/GPU and page/platform acceptance remain
open. Nothing was merged to main or published.

The native API audit establishes that public TimelineItem.SetClipColor really
submits a native color action to UiUndoStack. Absence of a public Undo method must
not be mistaken for absence of native history from SDK color calls. This is the
first disposable control to test. It does not establish custom metadata history.
The disconnected private history prototype is unsupported for execution: Boolean
submission failures must be handled and its void custom callback has no verified
atomic failure contract. No private history calls are compiled into the package.

Native UI parity scratch tests rule out a per-clip effect for the batched body.
A viewport prototype succeeds only with the tested uncached paths; native
CacheBackground/warmed proxy caches prevent interception. Existing cache settings
were preserved. An isolated earlier nested fixture crash remains unexplained;
the bounded ASAN run completed without a sanitizer report. No production effect
hook was installed. The package now captures actual view/proxy/item cache modes,
viewport native/GL flags and existing effects for the next native decision.

The generated-media disposable property harness is prepared beside that package.
It requires an empty Project Manager state before creating its owned project,
checks the exact installed test runtime and SDK IDs, verifies audio/video property
round trips and restoration, and closes only its owned project. It has not run.
The last recovery SDK read reports `WK 29 110526` open on Edit. The pending test
window request remains unanswered; the working project was not closed/switched
and no native property or history test was performed against it.

## Native repair continuation — November 5

Jordan explicitly requires completing persistence, native history/parity, timeline
tab restoration, project selector behavior and thumbnail paint coverage. Manual
audio clip colors overriding track color is also a required live regression.
The installed runtime remains `e9f11fe`; the following source is not installed.

- `339ec78` repairs a verified partial-well primitive. A native monochrome skin
  identifies its owning clip's exact body palette for that capture; the later
  partial solid fill uses that identity instead of lowering the size threshold.
  Preset and RGB routes share the same skin cache. The prior per-capture palette
  association is cleared on begin. Gold/Blue scratch cases passed 20/20 with zero
  protected foreground/neighbor differences, and the optimized assembled paint
  fixture passes. Audio raster rejection is separately bounded and logged; this
  does not establish manual audio precedence or native thumbnail acceptance.
- `0dfda7b` adds an opt-in exact-image ARM64 identity inspector. It uses bounded
  current-task memory copies to validate actual UI/model/proxy ownership, and
  emits the candidate ID. It neither constructs native proxies nor writes a
  project. Unsupported platforms/architectures reject explicitly. The focused
  guard fixture passes; SDK/native equality is still a required runtime result.
- `5a33d18` records the real native project button's public menu/ownership/input
  state. A null menu must be distinguished from a failure to show an attached
  menu; static native code deliberately assigns null when its open-project list
  contains fewer than two entries. No substitute dropdown or private refresh was
  implemented. The header fixture passes, without claiming a live selector fix.
- `06bdefe` extracts native clip selection traversal to TimelineClipSelection.
  Actual color command call sites now use that resolver and the old private
  traversal is removed. It owns no assignment, renderer or persistent data.
  Existing paint/selection regressions pass after extraction.
- `b921178` fixes broad signal blocking in IsolatedNativeAction. Previously a
  custom hover blocked all native menus/menubars. The same fixture reproduced
  four failures for an unrelated native command/lifecycle and now passes all of
  them. The implementation follows the action's actual associated menu chain,
  preserves original signal state and restores leases before native Hide
  handling. This removes the global allWidgets menu-blocking path; it is not yet
  proof of the reported native project dropdown's cause or repair.
- `c1933ed` adds an explicitly compiled local probe factory hook. ControlService
  only wires it; the normal build includes no scratch service. The ignored local
  service has bounded requests, fresh disposable SDK/project guards, weak model
  tokens and no polling. It can inspect, select an exact SDK-matched disposable
  clip, bind/read metadata and test a reversible Qt property round trip. Native
  history functions are **not compiled** into this candidate. No working-project
  property writes or private native calls were performed.

The final local test package is staged at
`build/native-color-disposable-select-20261105/`, runtime `c1933ed`, SHA256
`e9d464b1088a9be492b3ae56af0b46e682f96b388cb03a9be2344204bb11f761`.
It includes the declared ignored scratch headers whose hashes were verified
unchanged across compilation; those inputs are recorded separately from Git
ancestry. Mac arm64/x86_64 O2 compilation, strict signature, all 25 managed-file
hashes, exact install/rollback rehearsal and final assembled entry fixture pass.
The ARM64 property probe is a local test component, not Windows/Intel parity.

Required next acceptance is a controlled Project Manager/disposable-project
window: compare native and SDK item IDs, verify reversible property writes and
foreign metadata preservation, then separately review/test actual native
history/save/DRP before enabling any persistent assignment writer. Native UI/GPU,
audio track precedence, project dropdown and Windows live acceptance remain open.

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

## November 6: captured DRP, native identity and menu reachability

Jordan's manually exported `Downloads/color pallette.drp` was copied unchanged
into `build/native-color-disposable-render-20261105/user-test-state-20261106-092742/test-project.drp`.
Original and copy SHA256 are
`2fac3ee841b1c117587bf5ae60fe095bb51b1a1f3004f13bd1081705171bca7d`.
The valid archive contains 22 video and 2 audio instances, with no nonempty
per-instance `ImportExportMetadataBA` values. Palette/profile and diagnostic files
are captured separately. This preserves reproduction material; it does not prove
that the installed visual trial saved custom assignments. The user can continue
editing; subsequent destructive/storage tests require an owned imported copy.

Actual diagnostics rejected `UiTiVideoClip` as `unsupported-ui-class`. The
exact-image identity adapter now recognizes the evidenced primary QObject layouts
of `UiTiVideoClip` and `UiTiAudioClip`, in addition to `UiTiItem`. The same profile
drives class and vtable acceptance. Unknown/secondary vtables, changed lifetime,
proxy/owner mismatch and identity changes remain rejected. The bounded identity
fixture passes; SDK/native equality and saved assignments still need live proof.

The color menu now leases Qt's scrollable-menu hint through a focused reversible
style lease. `ColorPresetMenu` still owns only its committed-palette rows; the
lease owns only its scoped rule/property, preserves the native foreign style and
unrelated stylesheet changes, and releases on hide/rebuild/destruction. No
replacement menu, assignment writer, timer or global style was added. Optimized
assembled menu fixture passes (`clip color menu failures=0`), including reaching
and invoking the final row once, native rows/checkmarks/style lifetime and
reacquisition after a foreign stylesheet replacement. This shared core change
uses the existing Mac/Windows command caller; Windows runtime remains unverified.

Diff review confirms these are separate presentation and identity boundaries;
neither introduces saved-assignment or history state. The installed runtime is
still `4d743ff`; these fixes and the rejected-skin cache repair are source-only.
Private history remains excluded from the storage candidate. A disconnected
native-compound snapshot experiment has static ABI evidence and syntax checks,
but has not executed or established metadata Undo/Redo.

### Candidate built for the next owned storage test

Clean source `12f18b4` compiled universal O2; binary SHA256
`fce367e1a25dc96b5da395eac08bc18b2c33dec890d6d27e0a11b205808e4fdb`.
Staged bundle strict signature and exact temporary install/rollback pass: 25
managed files across 9 paths. Stage: `build/native-color-storage-gate-20261106/`.
It is **not installed**. Existing `4d743ff` and rollback checkpoints are unchanged.
The six declared scratch headers are hashed before/after compilation. The new
guarded probe can write/remove only its owned test namespace after native/SDK
identity equality and a reversible property control. It never automatically
writes assignments. No private native history calls or snapshot experiment are
compiled. Ordinary paints remain temporary in this candidate.

The external storage coordinator reuses `ScratchProjectScope`, mailbox transport
and the extracted `bind_item_control` from the existing property fixture. It
refuses a named/nonempty editing project before import, retains real SDK proxies,
and performs filesystem-only waits after SDK calls return. The Qt property and
SDK color tokens are compared to their own baselines, avoiding the incorrect
comparison of native `ClipColorBlue` against SDK `Blue`. Five original offline
checks and four storage failure/codec checks pass. The real Qt codec decodes the
seeded audio DRP and identifies the two unrelated fixture entries. These checks
do not access Resolve and are not persistence acceptance.

Next native sequence: import the owned generated-audio DRP; verify actual
instance identity and reversible metadata control; write an exact payload while
preserving unrelated entries/native color; save/export; close/reopen and compare;
import the exported DRP under a second owned name and verify regenerated IDs plus
the same payload; remove only the hash-matched test namespace and verify foreign
entries remain in the native export. This is deliberately a storage foundation
test, not custom-paint restoration or native Undo/Redo acceptance. Jordan's
24-clip capture remains preserved unchanged for the later assembled regression.

The final-source optimized ASAN assembled entry fixture passes with Color Manager
enabled (`native entry flow failures=0`). Numbered Native-entry contract reread:

| Contract | Actual entry/control and available evidence | Native acceptance gap |
| --- | --- | --- |
| 1 | `PanelControls` startup, immutable Resolve Layout inventory, hidden manual background control; clean/restart fixture paths | Actual fresh-profile first launch and native content/GPU |
| 2 | `unboundSaveWorkspaceAs`, arrangements inventory, built-in protection paths; assembled Save As | Full native Save/Update protection and failure recovery |
| 3 | `unboundRestoreNativeWorkspace`, saved workspace action; fixture restores native slots and returns | Actual native owners/rendering after real edits |
| 4 | Existing command/backtick path; prior focused command/context fixtures, no change here | Physical/remapped keys and temporary Native maximize |
| 5 | Native Source/Program/Timeline drag callbacks; fixture moves/group/reclaims without default load | Real pointer gestures from Native and unchanged unrelated placements |
| 6 | `PinnedWorkspaceHeader`/`FooterPanel`, native project/button stand-ins, Home/Settings fixture invocation | Actual AppKit title strip, project dropdown regression remains open/lower priority |
| 7 | Header/footer visibility actions and maximize viewport checks; assembled band combinations | Actual native painting for every combination |
| 8 | Direct drag, native return, retained hide/show controls in assembled fixture | Native title clicks, X/group recall and exact constraints |
| 9 | `PanelCallerMenu`, `WorkspaceHeaders::populate`, flat `openGateShowPanel`, narrow overflow checks | Real native panel inventory/navigation; Mixer independent behavior |
| 10 | Source/Timeline/Program maximize and floating group viewport assertions | Actual GPU, Mixer and resize minima |
| 11 | Existing per-page owners unchanged; this run exercises Edit | Actual multi-page and Windows integration |
| 12 | One assembled entry/rearrange/bands/maximize/save/Native/return/restart fixture with Color Manager | Real pointer/keys, native GPU, page switching and Relay/Deliver |

No deliberate contract departure is introduced. This candidate is a local
diagnostic installation, not completion of the numbered experience or the color
checklist. A fresh Resolve process is required to load it; no user editing project
will be closed/switched by the coordinator. Closed-session state capture and
verification must finish before reopening, so later user activity cannot be
misreported as an installer settings change.
