# Native color investigation and uninstalled cache candidate

Branch: `fix/timeline-red-native-cache`. Installed OpenGate remains `a494785`.
Jordan authorized installing REA and investigating Resolve; OpenGate replacement
remains on hold. No live project, clip color, native executable or settings were
changed by this analysis; REA's own Codex configuration was changed as authorized.

## Priority and visual contract

First prove real Red/custom clip painting while preserving native editing and
commands, then implement the shared persistent palette/picker and verified
nearest-native fallback/portable per-instance association. A temporary preview
does not satisfy those saved-color requirements.

Track backgrounds follow later. Track assignments remain independent from clip
assignments. A muted lane tint extends beyond the pinned header into empty gaps,
with substantially lower intensity than solid clip fills so gaps stay readable.
Changing a track tint must not recolor its clips. No scene-association subsystem
is requested. Independent timecode scaling, text-safe Mixer shortcuts and the
Mixer gutter investigation remain queued separately.

## REA setup and evidence boundary

Installed official npm `rea-agents@6.8.0` under Developer/Tools/rea, with matching
skill version 49 and Node 24 launcher. Codex registration and Ghidra 12.1.4/Java
checks pass; an actual local MCP transport connects and advertises its tools.
Configuration backups precede REA registration and Node-launcher pinning.
Homebrew's Ghidra 12.2 is outside REA's accepted release line; an isolated official
12.1.4 archive was verified against published SHA-256
`ddac49f903da9d5bac833e5cc79395098b9c33cfd3279be5f31bd00387d2d4db`.
Its ARM64 native decompiler was built from its own bundled source.

REA Mach-O inspection Evidence
`ev_ce3fae3e86fbbb5e957cfa09defb562af0117500e91b207fa3993dc8dc2981f0`
identifies Resolve's universal executable, original SHA-256
`d14f64e789f214fbfccbd9cad54b2e61b90d4bd202a9eb0105a74011306cbea5`.
Ghidra explicitly rejects universal selection, so `lipo -thin arm64` created an
ignored analysis copy, SHA-256
`801848888e276e40cefcc89011155702ddf67bb6ecfccff4de60ff505685e2a9`.
The installed executable is unchanged. Native clip-color type references are
seeds for investigation; they do not establish arbitrary RGB storage/rendering.

The first targeted string query reached Ghidra's 330-second import/analysis
deadline. REA reports incomplete output, no confirmed out-of-memory error, and
supports a longer startup budget. Session readback confirms cleanup_pending=false;
close succeeds. A second local analysis reached its enlarged 1,200-second
deadline too. A Java thread sample places its running main thread in Ghidra's
CallFixupAnalyzer/ClearFlowAndRepairCmd. This is an analysis bottleneck, not a
Resolve performance measurement or proof of a Resolve bug.

REA's supported reduced-analysis setting disabled `Call-Fixup Installer` for a
third, 330-second attempt. The native pre-script confirms that exact analyzer
was disabled, but this run also timed out before the string query could return.
No completed native handler/decompilation finding is claimed. Each session
reports cleanup_pending=false and closes; no analysis is left running. Further
whole-program retries without a changed analysis strategy are not justified by
these results. Native handler tracing is currently blocked by provider import
completion; the installed color failure is established by logs, not by REA
decompilation. Raw proprietary binary/decompile output stays in ignored local
evidence, not in Git.

## Cached-paint candidate responsibilities

The actual installed log shows the Red request arriving but only cached pixmaps
reaching the paint hook. The candidate exposes Qt device-cache paint commands
through reversible weak leases, rather than tinting images or replacing clips.

- `ClipDeviceCacheLease` owns temporary DeviceCoordinateCache bypass/restoration
  for QObject items; item-coordinate caches and non-QObject cached items remain
  untouched. Translucent subtrees fall back to native painting.
- `ClipCachePaintBuffer` owns reusable backing pixels matching Qt's integer DPR
  and rounded device-cache origin. It owns no item policy or selection state.
- `ClipFillGraphicsEffect` composes the cache/backing services with the existing
  primitive fill remapper; it owns no project or persistence model.
- `TimelineClipColorPreview` owns effect/cache leases and selected-preview
  clearing, replacing duplicated teardown loops with one release path.
- `MenuActionColorSwatch` draws the non-interactive native-row swatch. Existing
  QMenu activation and the isolated native-action mechanism remain the owners.
- `ClipColorPreviewCommands` wires the same commands to the surface adapters,
  clears selected previews on native palette choices, and discards stale origins
  when a menu is reused outside supported panels.

The row reads Red and has a red swatch. Its tooltip still states the temporary
preview limitation; stable command IDs and existing preset labels are preserved.
No alternate command dispatcher, project writer, palette store or input layer
was added. Cache tests reuse the existing Timeline fixture and replace its older
single-scale no-contents check, rather than duplicating the harness.

## Verification and open gates

Optimized ASAN cache tests pass at 70/100/147/213/300%, including cached roots,
cached children under no-contents containers, arbitrary RGB, thumbnail/glyph/
waveform pixel preservation, selection/commands, Undo/Redo geometry and pixels,
cache restoration, native-effect replacement and deletion. A new same-color
opacity check failed ten cases before conservative fallback and passes afterward.
Unsupported raster-only sources restore native pixels and cache policies.
Logs: `build/rea-clip-colors/cache-opacity-before-native.log` (10 failures),
`cache-opacity-after-native.log` (zero failures).

Optimized ASAN menu tests pass for Red/swatch click routing, overflowing native
menu scrolling, native palette-handler isolation, native palette clearing and a
menu reused from Timeline in an unsupported panel. Log:
`build/rea-clip-colors/menu-origin-native.log` (zero failures).
An initial sandbox execution could not access macOS screen services; it is not
counted as a behavioral result.

Actual Resolve cache topology, native painting, all editing gestures, playback
cost, persistence, shared picker and interchange remain unverified. Windows
compilation/runtime are unverified. Review found focused responsibility boundaries
and no new persistent state writer; passing fixtures are not native acceptance.
No installation or main merge is proposed by this source checkpoint.
