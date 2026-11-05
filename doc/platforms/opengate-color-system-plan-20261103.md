# OpenGate color workflow — agreed task list, November 3, 2026

This is the consolidated color task list from Jordan's discussion. Checked items
describe observed results, not completion of the whole feature. Existing Timeline,
input, scaling, repaint and workspace requirements remain in the
[repair checklist](opengate-repair-checklist-20261024.md) and
[Timeline plan](opengate-timeline-content-plan-20261030.md).

## Current evidence and priority

- [x] Jordan demonstrated Red painting a Timeline clip in OpenGate with its
  thumbnails and text intact. See the
  [native fill investigation and installation evidence](opengate-native-clip-fill-20261103.md).
- [ ] Save/reopen must retain the custom color. Jordan reports that Red currently
  disappears after reopening the project.
- [ ] Original UI and OpenGate must display the same assignment without a mode
  switch, maximization or other repaint workaround.
- [ ] The selected color must receive the correct menu checkmark. Current native
  Blue/Teal checkmarks do not represent the Red assignment.
- [ ] Native keyboard color commands must replace a custom color immediately,
  just as choosing a native color in the context menu should. Jordan reports
  that the keyboard route currently leaves Red behind.
- [ ] An explicitly selected OpenGate clip paint takes precedence over its
  track color, as a native explicit clip color does. Later track-color changes
  must preserve that clip assignment. Clear removes the clip override and lets
  Resolve resolve the current inherited appearance; do not copy the track RGB
  into a new explicit clip color. This is Jordan's November 5 clarification.
- [ ] Finish ordinary clip colors, persistence and commands before track lane
  painting. A successful paint probe is not a finished saved-color feature.

The standalone HTML palette builder is a design tool. Its gradients, texture
controls and organization editor are not evidence of runtime implementation,
native Undo, portable project storage or playback performance. No build or
installation is authorized by this planning update.

November 4 source update: native Qt editor/entry fixtures, scoped preset-menu
rendering, stock-definition protection, native QAction replacement, local texture
controls and immutable paint caches are implemented in the isolated candidate.
See the [implementation/verification ledger](opengate-color-manager-implementation-20261103.md).
The feature is **not installed or ready for full live acceptance**: saved project
assignments, actual native Undo/Redo, native keymap persistence, fallback and mode
parity remain open. Jordan's export takes priority; disposable-project checks
wait until Project Manager is available. No task below is considered complete
solely because its component fixture passes.

## Palette, picker and menu workflow

Clarification, November 4: **native Resolve swatches are immutable**. Their
definitions, names and presence cannot be edited, removed or replaced by the
custom palette editor or import. Custom/OpenGate swatches remain editable.
Assigning a custom swatch to a clip does not redefine a native swatch. Preserve
native thumbnail, waveform and clip-display settings throughout.

- [ ] Preserve the stock Resolve colors for reference and selection, alongside
  an expanded set of included OpenGate colors. Jordan is using the preview to
  choose the shipped presets; proposed names/values are not a final palette.
- [ ] Use one shared color definition and picker for supported Timeline clip,
  Media Pool, track and future marker uses. Filter unsupported paint types by
  surface rather than creating separate palettes or pickers.
- [ ] Provide a central Color Manager dialog/editor, reachable through a pinned
  **Color Manager…** entry at the top of color submenus. Context menus remain
  quick selectors and display a swatch pill, name and accurate selection state.
- [ ] Put clip color choices in the Timeline and Media Pool submenus and command
  catalog. Keep expanded lists scrollable and reachable near screen edges.
- [ ] Support named categories, editable swatch names, drag handles for categories
  and individual swatches, moving colors between categories, and saved ordering.
  Removing a category must preserve/move its colors unless deletion is explicit.
- [ ] Provide Add, Edit, Duplicate and Remove with an easy correction/Undo flow;
  editing must not require deleting and recreating a swatch. Protect native stock
  definitions and allow duplication into an editable color.
- [x] Standalone preview: color/texture edits preview the selected preset only;
  **Update Preset** commits that preset and **Revert** restores its saved
  appearance. Local backup retains pending edits separately from saved presets;
  JSON export retains pending work separately as draftPresets. Existing drafts
  migrate without losing prior edits. Production editor integration remains open.
- [ ] Give colors stable IDs independent of labels or menu positions. Renaming or
  reordering must not change assignments or key mappings. Define whether editing
  a shared swatch updates all references before exposing that operation.
- [ ] Support hue, saturation, lightness, RGB/hex and relevant alpha controls with
  a live clip preview alongside native comparison swatches. Keep text, waveform,
  thumbnail and editing-state visibility intact.
- [ ] Support gradients with 2–5 stops, add/remove and position controls, a clearly
  selected stop, and color/lightness/alpha controls that identify the stop being
  edited. Prefer left-to-right; Jordan dislikes vertical gradients visually
  because the visible clip footer does not communicate them well.
- [ ] Preserve current HTML draft edits, local autosave, import/export and preset
  names/values while extending the builder. Keep the same index.html location;
  do not replace Jordan's live draft with defaults or automatically refresh it.
- [x] Standalone preview: selected/imported records refresh editor fields, and
  selected-stop fields follow the active stop. Checks pass at 736/360 px for
  draft migration, categories, ordering, stop editing, duplication and JSON
  round-trip. The narrow-layout test explicitly edits an interior stop.

## Texture design and display controls

- [x] Standalone preview offers Fine, Medium and Coarse variants of Grain,
  Stipple, Crosshatch and Brushed with texture strength. Each produces a distinct cached
  static tile; Flat removes texture. Glass, Inner Glow and Soft Sheen are removed
  from offered choices; existing draft parameters remain recoverable. User
  acceptance and production implementation of these options remain open.
- [ ] Keep texture separate from base color/gradient. Opacity dims paint and is
  not a substitute for a convincing texture or glass effect.
- [ ] Add a **Textures** on/off setting in Color Manager and an accessible quick
  toggle in the Timeline toolbar, with a visible state and assignable command.
  Both controls must use the same state owner.
- [ ] Turning textures off preserves every preset's texture parameters and every
  assignment, displaying its base solid/gradient. Turning them back on restores
  the appearance. This is a local display preference, not a project color edit,
  and must not dirty the project or add color Undo entries.
- [ ] Keep this distinct from **Preview Resolve-Compatible Colors** (working
  label): that view shows the native fallback colors a non-OpenGate user receives
  without destroying or rewriting the custom definitions. Decide its precise
  surface scope and naming before implementation.
- [ ] Keep the toolbar compact and the toggle reachable at narrow widths and
  different scales; preserve toolbar overflow, focus and existing commands.

## Commands, events and Undo/Redo

- [ ] Every included or user-created swatch becomes an assignable command as the
  palette expands. Refresh command labels after renaming while preserving IDs;
  deleting a color must not silently retarget its shortcut to a different color.
- [ ] Route context menus, mapped commands, picker commits, Clear and native
  color actions through the same assignment operation. Cover native→custom,
  custom→native and custom→custom transitions, including OpenGate-to-OpenGate.
- [ ] Respect focused Timeline/Media Pool/track context and text entry. Typing in
  a bin name or another editor must not trigger unrelated panel commands.
- [ ] One committed color operation must participate in the actual editing Undo
  history. Undo/Redo restores exact paint, assignment provenance, native fallback,
  checkmark and repaint together; it must not consume or break adjacent edits.
- [ ] Picker previews are reversible; Cancel restores the prior state and one
  accepted edit produces one operation rather than one per slider movement.
- [ ] Verify mixed sequences of native edits, color commands, moving/trimming,
  Undo and Redo. A private color-only Undo stack or a delivered key event alone
  does not meet this requirement.
- [ ] Use existing event/damage mechanisms for immediate repaint after insert,
  source drag, place-on-top, edit, command and Undo/Redo. Retain correct mouse
  positions, trim/roll cursors and native editing behavior below and above 100%.
- [ ] Guard native playhead rendering from custom clip paint and stale capture
  pixels. Jordan's November 3, 10:37:39 screenshot shows a red ruler head with a
  dark/doubled-looking line through the Timeline; the cause is not established.
  Preserve the native head/line color, geometry, stacking and temporal alignment;
  erase old positions without trails or duplicate lines during playback, scrub,
  seek, zoom, scroll, scaling and repaint. Check clips, gaps, transitions and lane
  boundaries with custom paint enabled/disabled and against native UI. Do not
  recolor the scene globally or conceal a stale frame by painting another line.

## Saved project state and non-OpenGate interchange

- [ ] Persist exact swatch definitions, stable IDs, names, gradient stops,
  texture parameters and assignments with the project, not only local browser
  state, user preferences or an unshared sidecar.
- [ ] Investigate a namespaced, versioned metadata record that survives DRP
  export/import and a stock Resolve round trip. Hidden marker fields and Media
  Pool third-party metadata are candidates, not proof of reliable per-Timeline
  clip identity or storage. Do not invent a hidden track as a storage requirement.
- [ ] Store the nearest valid native Resolve swatch on the actual native object
  when assigning a custom color, so a non-OpenGate recipient gets a usable color
  immediately. Keep the exact OpenGate definition independently recoverable.
  Follow the [compatibility contract](macos-custom-color-compatibility.md),
  including verified stock values and perceptual nearest-color mapping.
- [ ] Decide and document how gradients/textures map to a single native fallback;
  their fallback rule has not yet been agreed.
- [ ] Track assignment provenance: explicit Timeline instance color, inherited
  Media Pool/source color, track contribution and native fallback materialization
  must not be confused with one another.
- [ ] **Clear Color** removes an assignment at its own scope and restores the
  appropriate inheritance; it must not freeze a copy of the inherited RGB or
  erase unrelated source/instance overrides. Verify Resolve's actual inheritance
  precedence and Clear behavior in disposable projects before finalizing it.
- [ ] Verify OpenGate A→stock Resolve→save/export→OpenGate B preserves exact
  custom definitions and assignments. Define conflict behavior if a stock user
  changes the fallback rather than silently restoring a stale custom override.
- [ ] Receiving OpenGate users must see the project swatches with their names and
  menu entries without overwriting unrelated personal palettes or keymaps.
  Define project-versus-personal palette ownership and update scope explicitly.
- [ ] Verify stable identity through project reopen, Save As, project/timeline
  duplication, clip copy/paste, trim, relink and Undo/Redo. Do not use only file
  names, visible row order or screen coordinates as identity.
- [ ] Test project sharing, schema migration and supported Resolve version
  upgrades; preserve metadata safely when a capability is unavailable. No blanket
  future-version compatibility claim follows from the current paint proof.

The Resolve 21.1 reference manual describes Timeline-instance clip colors and
explicit clip colors overriding track colors (printed pp. 1026–1028); it does not
establish every proposed inheritance/metadata rule above. See the
[official manual](https://documents.blackmagicdesign.com/UserManuals/DaVinci_Resolve_21_Reference_Manual.pdf)
and [metadata investigation](macos-clip-color-metadata.md).

## Track colors — later priority

- [ ] Keep track color independent from explicit clip color. Provide a deliberate
  track-header/subtle lane-tint operation that does not overwrite clip colors.
- [ ] Extend tint beyond the pinned header into the empty lane while retaining
  clear gaps, boundaries and clip contrast. A gap must not look like a filled clip.
- [ ] Consider right-click and multi-track selection for audio/video track paint.
  Solids should share the common palette; gradient support for lanes is undecided.
  Automatic scene-color management is not part of this requirement.

## Module boundaries, performance and acceptance

- [ ] Keep palette definitions/ordering, assignment and inheritance, project
  persistence, command/history integration, paint generation/cache and editor UI
  as focused responsibilities with explicit interfaces. Reuse existing owners
  and remove replaced paths; coordinators only wire these components.
- [ ] Keep portable behavior in shared core and native storage/history/input in
  platform adapters. Check all existing callers, including Media Pool, and report
  macOS/Windows verification gaps rather than assuming parity.
- [ ] Cache static texture tiles and gradient resources after definition changes;
  reuse them during drawing. No animated grain, per-frame noise generation,
  repeated full-image recoloring, new polling loop or full-Timeline color pass.
  Texture-off must bypass texture work early. Persist on commits, never in paint.
- [ ] Compare against a matched baseline for playback, scrolling, dragging,
  trimming and command/Undo latency at scales below/above 100% through 300%, with
  varied frame rates, track density and clip/media types. HTML responsiveness is
  not proof of native Timeline performance or zero cost.
- [ ] Require assembled live acceptance: assign via menu and shortcut; see paint
  and checkmark; replace with native/custom colors; Undo/Redo around real edits;
  save/reopen; switch UI arrangements; round-trip a DRP; restore exact colors in
  another OpenGate installation; toggle textures without losing settings.
- [ ] Before proposing installation, review the actual diff for state ownership,
  duplicate logic and module growth, reread the numbered native-entry contract,
  and separate fixture, compilation, installation and live acceptance evidence.
  Preserve isolated feature branches and the best known installation.
