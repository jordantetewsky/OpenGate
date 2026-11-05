# Color Manager update flow — November 5, 2026

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
