# Native Color Manager implementation checkpoint — November 3

The explicit builder/native transfer now preserves existing saved presets and
their separate pending edits, including gradient stops and per-color textures.
It preflights the combined palette before writing, rejects conflicting local
drafts, and persists draft recovery before committing the palette. If the second
write fails, the UI reports the retained draft backup rather than claiming an
atomic two-file transaction. Native export includes pending edits too, with a
256 KB bound and schema validation. An entirely new builder color that has never
been updated has no saved base; import rejects that file with an instruction to
Update Preset in the builder, preserving both files. It does not silently drop
or commit that new draft. Native draft-only creation/import remains a separate
flow gap. Temporary-file transfer/collision/reopen/write-failure checks and the
full optimized ASAN manager fixture pass. No frame-paint path was added.

November 4 integration: rename context and scaled popup repairs are included in
`226f3ab`, `9288f8e` and `c2e6811`. Integrated optimized sanitizer workflow,
panel-scale and clip-color menu fixtures pass; assembled manager entry passes.
These remain source/fixture results, not a new installed or accepted runtime.
Active native timeline ID/displayed-tab agreement is now an explicit gate.

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
project data. A separately gated preset-to-Timeline **rendering trial** is now
available with both `OPENGATE_EXPERIMENTAL_COLOR_MANAGER=1` and
`OPENGATE_EXPERIMENTAL_COLOR_PAINT=1`; it does not provide saved clip assignments
or native color-history integration. Existing experimental Red remains available
independently. Neither development path has been installed by this work.

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
| `colormanagerwidget.h` | Wire editor components to the supplied palette/draft owners |
| `colormanagerpanel.h` | Own the feature lifetime and wire one shared palette to the parked widget |

File/thumbnail I/O was extracted from the coordinator, gradient gestures from the
appearance editor, and draft persistence from its transient widget map. Those
replaced local implementations are removed. Committed definitions have one
writer; recovery has its own explicitly uncommitted scope. `PanelControls` adds
registration/reveal wiring only. `WorkspaceArrangements` adds presentation and
its existing queued action route only. Existing native menu isolation and menu
origin tracking are reused. No new native scene, clip replacement/deletion,
project writer, global key filter, polling timer or private Undo stack is added.

The feature owner lazily creates the shared palette independently of the editor
window. The editor receives references; its former palette/draft construction and
owned fields are removed. Opening the manager cannot create a second definition
store, and future Timeline/Media Pool callers can use the same owner while the
manager is closed. Fixture coverage checks that palette identity survives editor
creation. The owned widget is destroyed before its referenced models.

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

## November 4 source checkpoint: paint, commands and display controls

This remains an uninstalled development candidate. The following are fixture or
source results, not acceptance in Jordan's editing project:

- Shared presets now reach the existing native menu/selection adapter under the
  separate color-paint flag. Category order, names, gradient/textured pills and
  the custom row's checkmark come from the one committed palette. Updating a
  preset replaces its stable registry ID; deleting one retires its operation.
  Existing assigned render snapshots are not silently rewritten by palette edits.
- Native stock definitions stay outside the editable model. Reserved native IDs
  and the stock category are rejected by updates/imports. Choosing a stock
  comparison cannot edit a custom draft; removing a custom color preserves every
  native comparison/action. The stock comparison is visibly labelled locked.
- A native action observer replaces the former menu-only cleanup callback.
  Recognized native color QAction shortcuts and menu actions release only the
  selected Timeline/Media Pool render bindings; the original action executes
  once. Typing and unrelated actions retain their behavior. Actual Resolve
  shortcut identifiers/routes beyond these fixture cases still need validation.
- The paint-device hooks delegate background/label decisions to `ClipStylePaint`.
  Verified opaque clip bodies and monochrome nine-piece skins can use solids,
  2–5-stop gradients and twelve static texture variants. Unknown photos and
  waveforms pass through. Semantic native labels get contrast backing while
  retaining native ink. Fixtures cover direct/cached labels and a crossing
  playhead at 70%, 100%, 147%, 213% and 300%. This does not prove every real
  native cache role or the reported moving-playhead capture defect.
- Immutable gradient resources have a 128-entry cache; positions/sizes use
  transforms rather than another stop table. Diagonal gradients retain the
  preview's projection on wide, shallow clips. All clips in one selection share
  an immutable definition. Binding updates use indexed lookup and a repaint-only
  batch; native command label invalidation uses a generation counter. No native
  history transaction is implied by that repaint batch. Geometry preparation
  still visits the bound clips; matched real-project performance remains open.
- `ColorDisplaySettings` owns a local, atomic `ColorDisplay.json` texture setting.
  The manager and compact Timeline toolbar control share it. Off bypasses tile
  drawing and retains definitions/assignments; On restores identical pixels.
  These operations make no project/history writes. The explicit NoFocus toolbar
  route permits a click while a field retains its caret, while keyboard dispatch,
  modal/menu and workspace guards remain in force. The toolbar is available
  before opening the manager and uses the existing toolbar layout/overflow.
- `build-workflow-commands.py --color-manager` generates native keymap targets
  for manager opening and texture toggling through the existing bridge on both
  platforms. Per-preset **native Keyboard Customization registration and mapping
  survival after rename/restart remain unfinished**. A stable Qt registry ID is
  insufficient evidence for that requirement.
- Wide/360-pixel manager layouts are captured in
  `build/macos/color-manager-preview.png` and its `.narrow.png` companion. Sample
  labels elide when necessary; category icons use the shared gradient/texture
  renderer. No current-project names or media are used.

Additional focused modules have one responsibility each:

| Module | Responsibility |
| --- | --- |
| `clipstylepaint.h` | Adapt verified native bodies/labels to shared preset paint |
| `cliplabelregions.h` | Discover semantic label bounds without retaining child pointers |
| `clipopaquemask.h` | Derive opaque body coverage of an already verified skin |
| `clipcolorpresetcommands.h` | Route committed preset IDs through the existing command registry |
| `colorpresetmenu.h` | Own only the added category/swatch menu rows |
| `nativeclipcoloractions.h` | Observe actual native color actions without executing them |
| `colordisplaysettings.h` | Own the local texture-display preference |
| `colortexturetoolbar.h` | Present that preference in the native tools layout |

Removed paths: the coordinator's broad QMenu-trigger cleanup was replaced by the
shared action observation route; Media Pool all/selected cleanup share one lease
release implementation; the manager checkbox no longer owns a separate texture
preference. Native controls, menus and action definitions are not replaced.
`PanelControls` adds feature wiring only. Shared command/paint interfaces are
used by the existing callers; macOS fixtures pass. Windows generated-script tests
run on this host, but Windows Qt compilation/runtime acceptance remains open.

Open acceptance requirements include: saved project assignments; real native
Undo/Redo and mixed edit sequences; a stable native graphics-to-SDK identity join;
one canonical checkmark (the underlying native fallback check is still present);
Original UI paint parity; verified stock fallback values/rules; project palette
ownership/conflicts; per-swatch native keymap registration/rename persistence;
real cached-label/playhead coverage; and matched playback/edit performance.
Native alpha blending is deliberately unavailable in the rendering trial rather
than approximated across overlapping body layers. The editor retains alpha data.

The installed SDK declares Timeline `GetSelectedClips` and
`GetMediaPoolItem`; the existing stabilization route already uses selection.
The bounded selection probe reads IDs/colors only, verifies a repeated snapshot,
and never substitutes a playhead clip. A live attempt could not connect to
Resolve, so no new native identity observation is claimed. Source and Timeline
Media Pool metadata carriers are both supported by the scratch probe; neither
has passed the new save/reopen/DRP experiment yet. Scratch lifecycle checks now
verify current project ID/name before save/close and skip cleanup if ownership
changes. Twelve offline probe scope/identity/carrier tests pass. The later live
storage pass is recorded below; no build has been installed.

### Live scratch storage evidence, November 4

Jordan subsequently relaunched Resolve without opening a project. The SDK still
reported its empty `Untitled Project` placeholder: zero timelines, root clips and
subfolders. A separate explicit probe flag accepts only that verified empty
placeholder; normal editing projects still cause refusal. The first attempt
created/closed its scratch project but failed to import media because relative
paths were interpreted in Resolve's working directory. Probe output/media/archive
paths now resolve absolutely.

The subsequent generated-audio-only Timeline-carrier experiment completed:

- namespaced third-party metadata was accepted and read back exactly;
- it survived project save/close/reopen without changing clip range;
- it survived DRP export and import into a separately named scratch project;
- the native Orange control assignment survived;
- **Timeline, clip and source SDK IDs all changed on DRP import**. Opaque JSON
  references to those original IDs were not remapped. The experiment correctly
  reports `instance_association_portable: false`.

Receipt: `build/color-timeline-metadata-native-20261104-absolute/receipt.json`.
Archive SHA-256: `c4c3b0ccb19e648cdfa4a0b79decdf31a01712bf128c7f5abf9c9dcb01bd9379`.
Its XML identifies Resolve `21.1.1.0010`. The nested `drp_roundtrip` result is the
actual import evidence; the older metadata subrecord's initial
`drp_import_verified: false` was not updated by that run. Test projects were
closed; their library entries and artifacts were retained. No user editing
project was saved/modified, no build was installed, and no native Undo result is
claimed. Resolve was released for Jordan's work; subsequent investigation uses
the captured archive offline.

## Retained native-history evidence

The later bounded metadata-property pass is retained in
`build/rea-clip-colors/import-export-metadata-20261104/manifest.json`, SHA256
`5309737d65916133a3ba23c897e5666dc430c8d1e6a0bb6def15c911770a0877`.
Nine bounded disassemblies and five metadata/vtable slices identify
`Sm2TiItem::ImportExportMetadataBA` as a read/write/reset QByteArray Qt property
with a serialized string-to-variant map. It already contains Resolve interchange
data; any future namespaced entry must preserve those entries. The setter is
deserialization machinery and no native history submission was observed in it.
No safe live selected-model join, SDK write route, automatic color-action payload
attachment or stable identity/remap route was established. These are offline
shell observations, not new REA provider evidence or runtime behavior. No private
property writes have been added to production.

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
