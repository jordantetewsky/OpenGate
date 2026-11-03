# Native Color Manager implementation checkpoint — November 3

Branch: `feat/color-manager-paint`, based on preserved paint checkpoint
`fix/native-clip-skin-color`. This is development work, not an installed or
accepted project-color feature. The [agreed task list](opengate-color-system-plan-20261103.md)
remains authoritative. Installed build `660cdfd` is unchanged.

## Implemented editor scope

The native Qt content widget provides named categories and drag ordering,
per-preset Update/Revert, a selectable/draggable 2–5-stop gradient bar, selected
stop HSL/hex/alpha/position controls, direction, paint opacity, and Fine/Medium/
Coarse Grain, Stipple, Crosshatch and Brushed. It accepts the standalone builder's
palette JSON and exports saved definitions. Imported IDs cannot silently replace
different existing definitions. Invalid/corrupt input preserves the existing file.

Saved personal presets/order use `Colors.json` in the existing profile. Pending
appearance edits use a separate debounced recovery file, `Colors.json.drafts`;
they do not overwrite saved definitions. These are local editor files, **not
project-portable clip assignments**. Opening/painting the preview does not write
project data. No new preset-to-Timeline assignment command is exposed by this
checkpoint. Existing experimental Red remains its separate, unchanged adapter.

The preview uses generic sample video/audio labels, generated waveform geometry
and a neutral thumbnail placeholder. An explicit image chooser accepts a bounded
local thumbnail without embedding its path/image into the shared palette. It
does not inspect the current project, Don’t Tell media or its timeline names.
Stock comparison remains read-only. Closest-reference matching uses CIEDE2000
against sampled reference colors; this is **not verification of Resolve's actual
fallback palette or a production fallback write**. Gradient/translucent fallback
reduction remains undecided and is not guessed from the first stop.

The readability policy selects dark/light label ink for a uniform solid fill.
Gradients and patterns use an opaque neutral backing behind the sample label,
independent of texture phase or narrow high-luminance gradient intervals. The
paint definition itself is retained. Texture tiles are deterministic, static
64×64 images with a 32-entry cache; texture-off bypasses tile generation. This
does not establish native Timeline performance or native cached-label coverage.

## One panel, existing workspace machinery

`OPENGATE_EXPERIMENTAL_COLOR_MANAGER=1` adds the optional panel. Entry points are
**Clips & Tracks Color Manager…** immediately above Original Resolve UI, the flat
Panels inventory, `color-manager.open`, and a pinned **Color Manager…** item in
Timeline/Media Pool color menus. All reveal the same content widget. Right-click
entry does not invoke a native color handler. The existing managed session owns
floating, docking, close/recall, arrangement participation and native return;
there is no new QDockWidget/controller/input-filter implementation.

The owned widget parks hidden under the native page and is deleted with its
adapter. The manager reflows vertically at narrow widths and stacks header
actions; the preset editor scrolls without forcing an oversized Timeline neighbor.
The manager flag can be enabled without enabling Red or Media Pool paint trials.
Normal feature-off registration preserves the existing panel inventory.

## Responsibility review

| Module | Single responsibility |
| --- | --- |
| `colorpreset.h` | Validate/serialize bounded portable paint definitions |
| `colorpalette.h` | Own committed personal presets and category ordering |
| `colorpalettedrafts.h` | Own uncommitted editor recovery data |
| `colorpainttexture.h` | Generate/cache static pattern tiles |
| `colorpaintreadability.h` | Choose safe label foreground/backing |
| `colorpresetpaint.h` | Draw preset paint using those shared policies |
| `perceptualcolormatch.h` | Compute sRGB/Lab perceptual distance |
| `colorstockcomparison.h` | Present read-only stock reference comparisons |
| `colorpresetpreview.h` | Render generic sample clips and optional thumbnail |
| `colorgradientstopbar.h` | Present gradient handles and local pointer/key gestures |
| `colorpreseteditor.h` | Edit one transient appearance draft and expose Update/Revert |
| `colorcategoryeditor.h` | Present category/swatch selection and proposed ordering |
| `colormanagerfiles.h` | Bound explicit import/export/thumbnail file I/O |
| `colormanagerwidget.h` | Wire the editor components to their owners |
| `colormanagerpanel.h` | Own/park one content widget for existing panel sessions |

File/thumbnail I/O was extracted from the coordinator, gradient gestures from the
appearance editor, and draft persistence from its transient widget map. Those
replaced local implementations are removed. Committed definitions have one
writer; recovery has its own explicitly uncommitted scope. `PanelControls` adds
registration/reveal wiring only. `WorkspaceArrangements` adds presentation and
its existing queued action route only. Existing native menu isolation and menu
origin tracking are reused. No new native scene, clip replacement/deletion,
project writer, global key filter, polling timer or private Undo stack is added.

## Verification and remaining gates

- `test-color-manager.sh`: definition validation, personal save/reopen, invalid
  import preservation, distinct/cached textures, label safety, Update isolation,
  draft recovery, actual handle dragging, typing priority, shared menu entry and
  native-handler isolation, existing host acquire/park lifecycle. Optimized ASAN
  passes; screenshots additionally verify wide and 360-logical-pixel layouts.
- CIEDE2000 passes all 34 published supplementary reference pairs in both
  directions. This independently implements the equations; no restricted MATLAB
  implementation is included. [Reference notes and data](https://hajim.rochester.edu/ece/sites/gsharma/ciede2000/).
- `test-color-manager-entry-flow.sh` extends the existing assembled entry fixture
  rather than cloning its native tree: menu open, unchanged native owners, X hide,
  flat Panels recall with retained geometry, neighbor docking, Original UI return,
  mapped operation, singleton content, and existing restart flow pass.
- Existing clip-color menu regression passes. Workspace-menu and full native
  controller compilation pass. A universal arm64/x86_64 O2 candidate was built
  and signature-verified in `build/color-manager-candidate/Contents/MacOS`;
  subsequent source refinements require a matching final rebuild.
- New scratch-DRP import scope tests pass without connecting to Resolve. Actual
  metadata save/reopen/import and stable-ID survival have **not** run in this
  checkpoint. The probe refuses an already-open project and preserves receipts.

Before live/install acceptance: import Jordan's chosen preset JSON; verify actual
Resolve widget styling, image handling, narrow placement and panel lifecycle;
join native selection to verified stable Timeline-instance IDs; establish portable
metadata and closest native fallback writes; attach custom state to actual native
Undo/Redo; implement per-swatch mapped commands and checkmarks; prove actual clip
paint, cached foreground, playhead, native command replacement and mode parity;
measure playback/edit latency. Mac fixture success does not establish Windows
compilation/runtime parity. No installation, main merge or package publication.

## Native history investigation — observations and limits

REA provider tools were absent in the static-analysis worker. Read-only doctor
reported aligned registration; no setup/configuration changes were performed.
New results are ordinary offline Mach-O disassembly, not REA provider or runtime
evidence. Existing artifact evidence is
`ev_ce3fae3e86fbbb5e957cfa09defb562af0117500e91b207fa3993dc8dc2981f0`.

Retained ARM64 input SHA-256:
`801848888e276e40cefcc89011155702ddf67bb6ecfccff4de60ff505685e2a9`.
Local manifest `build/rea-clip-colors/color-history-static-manifest.json` indexes
13 bounded captures; SHA-256:
`d7911ebe103706bd9b675838639adde469f2a4fa7880884e78dee6c2c3fec165`.

Observed: `UiTiActSetClipColor` construction at `0x104b36c00` snapshots selected
objects' prior color strings. Apply at `0x104b377d0` and restore at `0x104b37ad0`
use the same setter. Submission at `0x104b36754` reaches shared execution
`0x103145e78`; successful execution can reach history append at `0x103140048`.
No desired/current-color comparison was found in the bounded constructor,
wrapper, apply or inspected caller. A same-enum native history entry is therefore
a hypothesis requiring controlled native Undo tests, not an established behavior.
The setter's underlying target remains unresolved. `SetClipColorWithColor` at
`0x104044598` still uses the fixed integer-key-to-string mapping; its name does
not prove arbitrary RGB support. No address is invoked by production code.

Installed SDK declarations expose `Timeline.GetUniqueId`,
`TimelineItem.GetUniqueId`, source-scoped third-party metadata and hidden marker
customData. They do not provide a documented general native Undo registration
endpoint, prove ID portability, or establish a join from selected native graphics
objects. The expanded scratch probe separately measures DRP metadata and ID
survival; it does not claim native history integration.
