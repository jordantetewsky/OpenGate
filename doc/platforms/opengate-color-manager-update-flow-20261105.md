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
