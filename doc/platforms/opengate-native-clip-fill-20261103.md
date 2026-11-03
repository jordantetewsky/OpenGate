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
