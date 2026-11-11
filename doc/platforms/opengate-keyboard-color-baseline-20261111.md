# Keyboard preview: preserved color baseline

The combined keyboard preview reconstructs the color behavior of the installed
local-startup checkpoint from retained source and that checkpoint's debug data.
It does not adopt the later failed color reader or claim a bit-identical rebuild.

Installed checkpoint SHA256:
`35ef8ec273b3bf11537cb814d0e85b6a43127bb5561ceb5e690cbcd6d9aa62ab`.

Evidence lives in the separate color worktree:
`/Users/jordantetewsky/Developer/OpenGate-features/color-manager-update-trial-20261105`.
Its base commit is `809c7479ece0f14495c38c6fd6003fac7d8326f8`.
The retained baseline source is
`build/color-assignment-wiring-20261110/source-snapshot/`, with hashes in its
sibling `verification-receipt.json`. That earlier snapshot's runtime has a
different hash; the following three deltas reconstruct the subsequent normal
startup and recreated-view correction.

1. `src/platform/clipcolorassignmentbackend.h` uses the retained session backend
   and the `OPENGATE_LOCAL_PERSISTENT_CLIP_COLORS` ordinary-startup gate from the
   color worktree. The installed debug information identifies the platform
   getter at source line 16 and its session-getter call at line 17, matching this
   file. Ordinary startup requires the local persistent-color build flag.
2. `src/core/clipcolorassignmentbackend.h` adds only
   `virtual bool isActive() const { return true; }` before `read`. Installed debug
   information places `isActive`, `read` and `commit` at lines 28, 29 and 30.
   Its `Read` structure is 24 bytes, containing `available`, `present`, `bytes`
   and `nativeColor`; it has no later `error` field.
3. `src/core/timelineclipcolorpreview.h` includes backend acquisition for new
   views, `isActive` checks, and clearing inactive document bindings. It excludes
   the later `hasSavedAssignments` method and early-failure error logging.
   The reconstructed source matches installed debug locations: `persistent`
   line 49, `clearInactiveDocumentBindings` line 161, the reload persistence
   check line 140, and the palette callback persistence check line 201.

The snapshot versions of the Mac assignment backend, native history adapter,
and `nativeclipcolorrecord.h` remain in place. Installed debug information
places `Target::capture` at line 39 with one argument and `Record::open` at
line 101, matching the snapshot. The retained November 11 color handoff dates
removal of the `BtString` metatype check, added read errors, and later backing
accessor experiments after this installed checkpoint. Those changes are not
part of this keyboard preview.

The inspected dSYM is under
`build/native-color-local-startup-20261110/runtime/Contents/MacOS/UnboundWidgetProbe.dylib.dSYM/`;
its sibling runtime matches the installed SHA above. The local-startup build
receipt and the November 11 handoff corroborate the session/startup correction
and subsequent rollback to this checkpoint. A machine-readable reconstruction
record is retained at
`/private/tmp/opengate-speed-editor-trace/installed-color-source-reconstruction.json`.

Limits: the DWARF v4 line tables contain no source content hashes and have zero
source timestamps and lengths. Matching symbols, layout and source locations
support this behavioral reconstruction; they do not prove identical original
source bytes or identical rebuilt machine code. This evidence is static only.
Compilation, installation and live acceptance must be recorded separately.
Known color limitations of the installed checkpoint remain limitations; this
additive keyboard work does not claim to repair or revalidate them.
