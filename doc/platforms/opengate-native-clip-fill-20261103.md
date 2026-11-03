# Native clip-fill investigation — November 3

Branch `fix/native-view-clip-color`. Installed OpenGate remains `7b191f8`.
Jordan requires visible Red painting before another installation. The work below
does not satisfy persistent custom colors, the shared picker, native fallback,
portability, or final native behavior/performance acceptance.

## Native evidence and failed hypotheses

- Installed per-clip effects found cached pixmaps but no body fill. Qt device
  cache leases were zero. Changing item-cache handling did not fix native Red.
- OS sample of process 78704 during the accepted Red command shows native
  `UiTiDrawUtil::DrawRects<QPainter>` and `DrawRectsFromMap<QPainter>` beneath
  `QGraphicsView::paintEvent`, outside the selected clip's effect subtree. That
  effect instead sampled `UiTiGraphicsLabelItem::Draw<QPainter>` and text drawing.
  Evidence: ignored `build/rea-clip-colors/native-red-paint-sample-focused.txt`.
- REA successfully analyzed the smaller, same-install `libBMDDavUI` after whole
  Resolve imports timed out. Its generic rectangle painter can forward RGBA to
  QPainter or its graphics renderer. This does not establish custom clip storage
  or that the visible clip body uses that specific route. Fill evidence
  `ev_84712b11639e046a7815ed02aa3d6c033c9eb00290e08e5da9e474ad1a98a41e`;
  constructor evidence
  `ev_be65575ae914497833a8afbf2804054b70ce5dc9427fc53f02bc6674c55f608b`.
- A QWidget viewport effect passed at 100% after device-basis correction but
  failed larger scales. It was removed, not retained as a parallel paint path.

## Candidate production boundaries

`TimelineClipColorBindings` is the sole root-view owner of weak clip/color
associations and native-to-root geometry. `clipfillregionmap.h` matches eligible
rectangles to those regions. `ClipFillPaintDevice` forwards paint state and
primitives, changing only mapped solid rectangles; optional raster observation
reports geometry without changing image pixels. `TimelineClipColorPaint` scopes
one capture using the bindings. The existing `TimelineSceneRenderer` still owns
capture/damage/pixels through a generic callback wired by TimelineContentViewport.
TimelineClipColorPreview resolves native selection and applies/clears bindings.

Removed production paths: per-clip graphics effects, device-cache leases, and the
two-pass cache paint buffer. Media Pool retains its existing delegate caller of
the shared paint device. No project/color-enum writer, duplicate command executor,
native scene, workspace controller, input handler, or persistence owner was added.
The extra temporary Clear menu row is removed in source; native Clear Color and
native palette choices clear the selected override. Compatibility command IDs
remain. Native menu checkmarks are unfinished; a checked action is not paint proof.

## Verification and first live view probe

Optimized ASAN production candidate fixtures pass at 70/100/147/213/300%, including
a view-background body batch outside the clip subtree, an unselected same-colored
neighbor, native thumbnail/text/waveform pixels, cached child details, long exposed
clips, native clicks, commands, Undo/Redo pixels, geometry edits and cleanup. The
view capture uses one native pass. Menu and shared Media Pool caller fixtures pass.
Logs: `scene-fill-final.log`, `scene-fill-menu-final.log`, `scene-fill-pool-final.log`
under ignored `build/rea-clip-colors`. These are fixture results, not native success.
Windows runtime/compilation is unverified.

Jordan authorized a one-session development probe, then closed Resolve normally.
The universal, signed zero-effect OFX probe was loaded from the build directory
through OFX_PLUGIN_PATH. It creates no workspace/controller, replaces no installed
files, and does not invoke private native methods. Its test-only effect encloses
the complete nested view proxy so the existing installed renderer can exercise
the new binding/matcher/paint-device components without installing a replacement.
This tests native paint delivery, not the candidate renderer integration itself.
The independent optimized ASAN probe fixture passes at all five scales.

Live process 81642, installed runtime session
`9ce57214-de42-44b3-ac34-e7b95d4d0941`: Red found one native clip, one region and one
view proxy. The probe observed 65 rectangles, 181 pixmaps and one changed fill
command. **The before/after images still show a blue body, confirmed by Jordan.**
This is a failed native paint test. A nonzero changed-command count is explicitly
insufficient acceptance. The visible fill may be a smaller rectangle rejected by
the matcher or a later raster draw; neither cause is established by this result.
Raw evidence and images remain local under `build/native-fill-proof`.

## Next discriminating observation

The revised development probe records bounded, ordered rectangle/raster geometry
intersecting the selected clip, plus read-only color-related Qt method signatures.
It does not recolor rasters or invoke setters. Build outputs are separate from the
loaded probe; the build refuses to overwrite an existing probe while Resolve runs.
Loading a revised probe requires a fresh process, not another installed payload.

Binary strings include UiTiActSetClipColor, SetClipColor(QAction*), Sm2ClipColor,
SetClipColorWithColor, distinct audio/video track-control references, and all 16
native palette actions. These names alone do not establish arbitrary RGB support.
The SetClipColorWithColor string references a cold assertion; a read-only ARM64
reference scan locates its calling function at 0x104044598. Analysis remains
partial; no private-ABI call or native binary modification is authorized by a name.

Remaining gates: identify the actual visible body operation; prove selected-only
Red and arbitrary RGB in Resolve; preserve images, editing, cursor/command routing
and repainting; then implement checkmarks, saved palette/picker, verified per-clip
association, native fallback and interchange. Subtle independent track-lane tint
remains lower priority and must not overwrite clip colors. No new installation,
main merge, package publication or native color-model write is claimed here.

## Jordan's mode-switch discovery and skin candidate

Exact user sequence: choose Clip Color > Red in OpenGate, then choose Use Original
Resolve UI to see whether the command affected that presentation. Jordan did not
disable thumbnails or change the clip display style. His screenshots show Red in
Original UI with foreground thumbnails absent while OpenGate retains its different
presentation. Treat this as a successful diagnostic discovery of a native paint
effect, not complete custom-color acceptance. Do not attribute the display change
to a user setting or infer its mechanism from the screenshots alone.

Process 82246 / session dc1644c5-4852-4d7a-b7eb-25388ff7ff13 recorded the assignments
and later managed -> native -> managed transitions. The probe changed the rectangle
under the thumbnails, but changed zero skin pixmaps. Detailed raster evidence
identifies a shared 244x164 physical-pixel background at DPR 2, drawn as nine pieces,
and a separate 226x228 selection skin. The former exceeded the experimental 128px
guard. A preceding clip's right-shadow pieces also entered the selected clip's
three-pixel margin. These are concrete candidate matcher gaps, not proof of why
the two complete presentation paths differ. Logs/images remain local under
`build/native-clip-skin-proof`.

The source now separately recognizes nine-piece geometry (`clipninepatch.h`),
recolors a verified skin palette while retaining alpha/neutral shading
(`clipskinpalette.h`), and caches source-key/color variants (`clipskinpaint.h`).
The paint-device pixmap callback passes all unrecognized images through. Native
geometry is tested explicitly at DPR 2, including preceding shadow fragments.
Only tiles whose centers belong to the selected clip can enter its group. Cache
limits are eight source images, four variants each, and 512px per image dimension;
there is no repeated pixel recoloring for a cached source/color pair. This is not
a native playback performance guarantee or complete support for every native skin.

Optimized ASAN Timeline, proxy-probe and shared Media Pool fixtures pass. Latest
exact-geometry result: `clip-skin-native-geometry-tests.log`; developer bootstrap
singleton/order/shutdown checks: `development-entry-tests.log`, both zero failures.
No native skin success is claimed. The production candidate changes no clip effects;
the diagnostic proxy effect is confined to the test helper.

Jordan closed the probe process. Normal reopening restored installed 7b191f8 in
process 82307 / session 9d9939e4-86fc-497c-afdb-6d8b48625f97, without the developer
probe or experimental Red launch flags. No installation changed.

The next proposed proof must use the actual OpenGate renderer. A compile-time
development entry reserves the existing startup singleton name before constructing
the real candidate ControlService after the ordinary delay. The installed entry
yields through its existing singleton check; if it already owns a service, the
developer entry refuses to start. The same production controller and renderer run,
with no proxy effect and no concurrent controllers. This candidate is not yet loaded
or live accepted; a one-session launch requires Jordan's explicit go-ahead.

Prepared runtime: clean source `929758a`, universal O2, with the compile-time
development entry. Strict/deep OFX bundle verification and final binary hashes
pass. Stage: ignored `build/native-renderer-proof`; no installer or library
replacement is involved. The ordinary entry variant also compiles for both Mac
architectures after correcting Bash's empty-array handling with `set -u`. The
builder no longer creates an empty legacy OFX timestamp file in custom bundles.
Source backups `fix/native-view-clip-color` and `fix/native-clip-skin-color` are
pushed to Jordan's personal repository; no main merge or package publication.

Next live acceptance actions: confirm exactly one helper.start and loaded build
929758a; select a real Timeline clip in OpenGate; use its native Clip Color > Red
entry; verify the visible body beneath unchanged foreground content, neighboring
clips, displayed pixels after edits/Undo/Redo, and Original UI return. Checkmarks,
saved palette/picker and portability remain separate unfinished feature work.

## Authorized normal installation — November 3, 13:45 UTC

After pausing, Jordan explicitly requested installation and testing, superseding
the prior hold for this trial. The normal (non-development-entry) clean-source
`660cdfd` universal O2 runtime was built and its native bootstrap checks passed.
The staged installation and exact rollback rehearsal passed. All 23 installed
managed-file hashes and the strict/deep signature verify; all 197 protected paths
were unchanged before launch. Transaction
`FB425AC4-6D8E-4719-82B2-0E2364CAF993` preserves the previous `7b191f8`; earlier
best-known/fractured rollback checkpoints remain available.

Fresh Resolve process 83799 / session d12bbcf5-cd83-495c-9d04-7ca9a65d2dbb reports
loaded build `660cdfd`, with a single helper.start. Launch enables Timeline Red
and leaves Media Pool colors disabled. No development OFX path, proxy effect or
development singleton reservation is used. Receipt:
`build/timeline-clip-skin-install/installation-receipt.json`.

Numbered native-entry contract was reread before installation. Existing normal
startup, workspace/native return, title dragging, flat Panels, bands, maximize,
persistence and page entry points remain the inherited assembled paths. No new
live acceptance for those flows is inferred. The changed color action enters via
the native Clip Color submenu / existing mapped command, resolves native selection,
and paints through TimelineContentViewport -> TimelineSceneRenderer ->
TimelineClipColorPaint. Scale, foreground, command/Undo/Redo and native-geometry
fixtures are recorded above. Original UI comparison, real OpenGate Red pixels,
editing/playback, checkmarks and saved custom colors remain unaccepted.

Jordan was told ready only after installation and the loaded build were verified.
First test: choose Red on one Timeline clip in OpenGate and inspect its background
while preserving thumbnails, text and waveforms. Installation is complete; native
Red success is not yet claimed. No main merge or package publication.
