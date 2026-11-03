# Custom clip colors: native rendering experiment

Historical first proof below. The latest native failures, removed paint paths,
current module boundaries and installation hold are tracked in
[the November 3 native fill investigation](opengate-native-clip-fill-20261103.md).

Branch: experiment/timeline-red-clip-rendering. No merge or publication.
Installed baseline: 503a969, preserved in its install staging/transaction records.
This experiment is not installed or live accepted by this source note.

## Retained feature contract

The target is additional real custom clip colors, including Red, with any RGB
color, saved names, a picker, rename/edit/remove and assignable commands. These
are persistent palette options, not aliases that globally replace an existing
native color. Palette removal must not silently recolor previously assigned clips.

Each assignment must retain the actual clip, identity, media, edits, effects and
ranges. Write the closest valid native palette value on that same clip using
verified native swatch colors and a documented perceptual distance (CIEDE2000).
Keep the exact hue, versioned palette ID and verified per-instance association in
additive project-portable data. Without OpenGate, the real native fallback must
remain usable. Save/reopen, DRP export/import on another system and return with
OpenGate must be verified before promising portability. MediaPoolItem metadata
alone does not prove per-timeline-instance storage. Never write invalid native
palette enums or undocumented database fields. The earlier compatibility contract
still applies: macos-custom-color-compatibility.md.

One shared picker/palette must serve clips, future track backgrounds and future
markers. Provide hue/saturation/lightness plus RGB/hex editing with one shared
color-definition model. Keep surface-specific application, native fallback and
project association in adapters; do not duplicate pickers or store clip/track/
marker responsibilities in the picker. Future surfaces opt into that interface;
they are not implemented by this rendering experiment.

Timeline behavior must remain native: selection, clip/edit-point hovers, trim,
roll, transitions, dragging, shortcuts, Undo/Redo, scaled coordinates, native
return and playback. Exact hue rendering is insufficient evidence by itself.

## First proof and deliberate limitation

This source implements a **temporary paint preview only**. It does not write a
palette, metadata, fallback or clip color. It cannot yet satisfy saved custom
colors. Default sessions have no preview object/filter/effect. Opt-in launch:
`OPENGATE_EXPERIMENTAL_RED_PREVIEW=1`.

When a focused Timeline's native `markSetClipColor` menu opens, the preview adds
clearly labeled temporary Red and Clear actions. Selected real UiTiClipContainer
objects are found in the native root/nested scenes. Existing effects are skipped.
All normal native actions remain untouched; no new key interception or command
registry is introduced. Clearing/destruction removes only the exact effect still
owned by that preview. Menu actions hide outside their owning Timeline scope.

ClipFillPaintDevice forwards public Qt paint primitives and state. It identifies
the largest chromatic solid rectangular fill and can substitute that exact fill
color in rectangular primitives. It forwards image/pixmap pixels, text runs/pens,
waveform paths, selection outlines, transforms and clipping. This is a hypothesis
about actual Resolve paint commands; native gradients, textured fills, GPU paths,
other body shapes or different menu/selection mechanisms may be unsupported.
A hue merely appearing somewhere is not native acceptance.

TimelineClipColorPreview owns the temporary effect/menu leases. The source is
inspected and painted directly, avoiding serialized text reshaping. It performs
an extra native paint pass for a previewed clip; performance is unverified and
this is not a production rendering/performance promise. Diagnostics report
requested clip count, candidate source fill, changed fill-command count and target.
No thumbnail-wide image tint or global native-palette substitution is used.

## Verification and live gate

The focused Qt fixture tests 70/100/147/213/300%, Red and another arbitrary RGB,
unchanged thumbnail pixels of the same native hue, unchanged waveform/text
regions, native identity/model color, selection clicks, command/shortcut delivery,
actual Undo/Redo position plus pixels, geometry edits, clear, deletion, skipped
existing effects and the scoped native-menu action. Tests reuse the Timeline
fixture; they do not mutate the open Resolve project.

Before a live trial, build and stage the candidate and preserve installed 503a969.
Native painting, selection/menu compatibility, edit-point/transition behavior and
playback remain unverified. Use generated/disposable media first. Stop rather
than replace existing clips/effects or infer clip identity from filenames/rows.
After the rendering route is accepted, implement and verify the persistent palette,
per-instance project association, nearest native fallback and interchange as
separate responsibilities. Existing metadata probes remain research only.

## Source checkpoint evidence

Optimized red preview and optimized AddressSanitizer runs pass, including the
captured native ItemIsSelectable + ItemHasNoContents container/painted-child
topology and removal after a foreign effect replaces the preview. The existing
optimized Timeline suite passes when run separately. A concurrent run reported
four cursor-sensitive queued-coordinate failures; the unchanged compiled suite
passed on an isolated rerun. No native acceptance is inferred from either run.

Organization review: the viewport only constructs the opt-in preview service.
The paint adapter owns primitive/state forwarding; the preview owns temporary
effect/menu leases and wires its two paint passes. No new project/persistence
writer, key handler, duplicate native scene, color enum or command executor.
Windows compilation/runtime and actual Resolve rendering remain unverified.
