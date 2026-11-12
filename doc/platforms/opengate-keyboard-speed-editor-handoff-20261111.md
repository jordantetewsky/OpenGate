# Handoff prompt: OpenGate keyboard and Speed Editor

Continue Jordan's keyboard/Speed Editor feature. Read this entire handoff, the
local AGENTS.md, and the linked plan before editing. Preserve the live installation
and custom colors. Do not redesign Resolve's keyboard editor. Execute agreed work
persistently, and distinguish source, fixtures, build, install and live acceptance.

## Repository and scope

- GitHub: https://github.com/jordantetewsky/OpenGate (remote `personal`).
- Main integration worktree:
  `/Users/jordantetewsky/Developer/OpenGate-features/keyboard-speed-editor-main-20261111`
- Integration branch: `integration/keyboard-speed-editor-main-20261111`, based on
  `personal/main` at `eeb884a`. This is the keyboard-only port for main.
- Tested/live combined checkout:
  `/Users/jordantetewsky/Developer/OpenGate-features/keyboard-color-preview-20261111`
  on `fix/input-customization-panel-20261111`, source `cee2ef8`.
- Original feature branch: `feature/keyboard-customization-panel-20261111` at
  `5a1a0fa`; original panel implementation `9488a55`.
- The combined checkout and original feature ancestry contain non-keyboard work.
  Never merge those entire histories to main under keyboard-only authorization.
- Jordan explicitly authorized the main merge despite the remaining native-menu
  bug. Check current remote/main and the integration commit; do not assume a local
  branch name means a push completed. Never force push.

The main port preserves main's existing `PanelMetricScale`/session scale value
instead of importing the later Timeline/Media Pool content renderer. It adds the
keyboard-specific minimum and tab persistence to that same owner. It includes
shared non-restoring quit handling needed to keep expanded state through quit.
It does not bring in the clip-color implementation or change the live installation.
New generic text guard is an unchanged extraction from main's diagnostics; there
is one definition, with existing callers retained. Keyboard code still needs the
opt-in preview build flag; merging source does not enable a finished product.

## Read these documents

1. `doc/platforms/opengate-input-customization-plan-20261111.md` — authoritative
   behavior, gesture/state terminology, feature order and module boundaries.
2. `doc/platforms/opengate-native-entry-flow-acceptance.md` — numbered assembled
   native/OpenGate experience. Component fixture passes are not live acceptance.
3. `doc/platforms/opengate-speed-editor-native-trace-20261111.md` — existing native
   investigation; distinguish addresses/static findings from runtime proof.
4. `doc/platforms/opengate-task-audit-20261020.md` — current acceptance and gaps.

## Installed runtime and rollback

Currently installed and observed loaded: clean `cee2ef82ab8ff6446259c92642c699b6cede6a7d`.
Runtime SHA256: `553154b5e374bddeeec72c4969cdd3dc756470917d93394bdc3d7ffb0b0223d1`.
Last observed Resolve PID77111, session `056e9acc-2944-4110-b56f-3c73fde5a8ab`;
recheck live identity before using a PID.

Installed helper:
`/Library/Application Support/Blackmagic Design/DaVinci Resolve/IOPlugins/Unbound.dvcp.bundle/Contents/MacOS/UnboundWidgetProbe.dylib`

Stage in the combined checkout: `build/input-menu-responder-20261111/`.
All 25 installed file hashes and strict bundle signature verified; install and
exact rollback rehearsal passed. Transaction:
`/Library/Application Support/CineWaltz/OpenGate/Transactions/7B7012B0-3391-4F11-8B3A-D3B660B3C2B7`
restores prior `6cfb407`. Earlier transaction `37150516-6721-40CB-84B6-AAF82B37314F`
restores `954a467`. Preserve these and all older checkpoints.

203 protected files were captured with Resolve closed. Jordan reopened before
post-install closed verification. A subsequent read-only comparison found only
Workspace.json changed; the palette and other captured files matched. Do not claim
closed-session equivalence for this installation. Receipts are in the stage.

Trial profile and diagnostics (currently live):
`/Users/jordantetewsky/Developer/OpenGate-features/timeline-content-candidate/build/color-texture-trial-20261104/`
Read `diagnostics/active-session.json` and narrowly filter `events.jsonl*`; do not
print entire feature manifests or pointer streams.

## Live acceptance and remaining menu defect

Jordan says everything else opens the tabbed editor and the other UI fixes work.
The unresolved entry is specifically **DaVinci Resolve > Keyboard Customization**.
The toolbar toggle, Panels and shortcut entry are accepted by the user. Do not
argue that a callback proves the native menu works.

`6cfb407` skipped NSMenuItem entries whose target was nil. Local Qt Cocoa source
shows `QCocoaMenuItem::resolveTargetAction()` intentionally sets a nil target for
responder-chain dispatch. `cee2ef8` accepts nil, resolves native validation through
the original selector, preserves fallback, and adds two events:
`keyboard.native-menu.leased` and `keyboard.native-menu.invoked`.
The isolated fixture covers explicit and nil targets, refresh, validation,
fallback and teardown; it passes. Live logs show nil-target leases and one
invocation at 2026-11-12T02:50:17.426Z with `panel_opened:true`; Jordan still reports
failure. This may involve lease timing/native refresh or actual reveal; these are
hypotheses, not findings. Trace actual menu activation and visible host outcome.
Do not blindly add more hooks or disconnect Resolve's original QAction handlers.

REA Frida attach to PID77111 failed: unable to access process from current user.
No script was loaded. Prior CUA attempts stalled for minutes and returned Computer
Use inactive; avoid retrying unchanged failed calls. User quit normally for installs.
Use existing supported read-only diagnostics first. Read REA skill if continuing
native/runtime work. No firmware or device configuration has been written.

## Product contract

Keep Resolve's native keyboard editor, command hierarchy, search, profile selector,
menus and styling. It lives in a retained resizable panel with Keyboard and Speed
Editor tabs. Native content baseline is the minimum scale; use reachable overflow.
Fix clipped labels, prohibit dragging the embedded dialog inside its panel, allow
selection clearing by clicking the selected key again or blank device area.
Tilde expands a standalone floating editor on its current display. Close hides;
recall retains expansion; normal frame remains available for restore. A hidden
editor must release the global expansion slot so Timeline tilde still works.
Panel state persists across native/OpenGate modes, workspace changes and restart.

Device drawing follows actual hardware: white IN/OUT and SHTL/JOG/SCRL keys,
space/dark gray groups, black wheel and red FULL VIEW. Use the existing command
catalog, not a separate invented list. Independent Speed Editor profiles must never
change the keyboard preset. Immutable Native Speed Editor profile is pinned first.

UI terms: **Button actions** versus **Jogwheel mode**. Button actions expose Single
press, Double press, Hold, Hold + dial left, Hold + dial right. Native is a choice
for each individual gesture, not only a whole-button setting. Jogwheel mode is a
persistent state chosen by pressing a key, lasting after release until another
mode is selected. Only one persistent mode is active; those keys have no separate
press/hold actions. Explain this only when the relevant mode is selected.
Timeline zoom can be a directional action pair, temporary hold+turn, or persistent
mode. Other intended operations include next/previous edit or marker, X/Y position,
selected-clip frame nudges and audio volume. Specify targets, units, undo and response.
Reuse measured native Jog movement/fraction behavior where appropriate; do not invent
curves and assume they match. Native ROLL is not Go to Nearest Edit and must not
silently move the playhead as an approximation.

## What is NOT implemented

The Speed Editor page is a preview. Behavior/gesture choices are transient; Save
and Reset are disabled. No independent device profile persistence, import/export,
hardware remapping, focused physical-input suppression, native device command
category or gesture-monitor implementation is complete. Native internal trace
findings are not a verified event-interception/dispatch mechanism.

## Next work order

1. Resolve the remaining native-menu entry using actual user-flow evidence.
2. Implement independent device profile model/persistence by reusing established
   profile mechanisms: create, duplicate, select, save, delete, import/export,
   restart round trips, invalid-data handling and immutable Native profile.
   Separate keyboard profiles and workspace state. No hardware activation yet.
3. Prove one physical button's event lifecycle and exclusive ownership: native
   fallback, custom action exactly once, no native/custom double execution,
   clean releases on focus/disconnect/profile changes. Avoid firmware changes.
4. Focused customization editor: physical controls select/show state without
   affecting Timeline. Shared device-state owner distinguishes UI selection,
   physically held keys, temporary hold+turn and persistent jogwheel mode.
5. Optional lazy Qt Speed Editor monitor, consuming the same owner, displays
   active mode and actual executed commands with learning animation. Hidden
   monitor does not perform unnecessary rendering/work. Timeline keeps operating.
6. Expand remapping to native operations and full gestures/modes only after the
   lifecycle contract works. Expose verified Speed Editor commands as a category
   in both command browsers; don't substitute lookalike keyboard operations.
7. Later append/reuse ShuttleGlide work. After reverse-engineering native wheel
   dispatch/response, investigate three-finger circular touchpad input reaching
   Timeline through that same path with a better response. This is planned only.

## Module map

- InputCustomizationPanel: owns native editor plus device tab, not placement.
- InputCustomizationTabs: native-style readable tab layout.
- EmbeddedDialogAnchor: prevents native dialog dragging within the host.
- SpeedEditorDiagram: drawing/hit testing/selection, not physical device state.
- SpeedEditorCustomization: transient UI preview and command binding display.
- NativeCommandBrowser: reads existing native command models.
- NativeKeyboardEditor platform adapter: verified-build guarded native creation;
  current Mac support is Resolve21.1.1 ARM64; Windows explicitly unavailable.
- NativeKeyboardEntryRoute: coordinates native menu and shortcut entry.
- MacKeyboardMenuRoute: per-item AppKit target/action lease and native fallback.
- ManagedPanelSession: retained placement, existing scale owner and interface state.
- PanelTabSelection: selected-tab state attached to existing workspace snapshot.
- PanelFloatingMinimum: floating minimum bounded to the current display.
- PanelExpansionController: one expansion transaction and hidden preference recall.
- LayoutIndependentPanelState: pure workspace projection around independent panels.
- WorkspacePersistence/WorkspaceSnapshot: existing settings writer/schema.
- PanelControls: wires these owners; do not accumulate their implementations here.
- Shutdown dependency modules: disarm their own leases/callbacks without restoring
  native UI during application teardown. Keep OS behavior in platform adapters.

## Verification and build

On the main integration, these passed:
- `bash infrastructure/macos/test-input-customization.sh`
- `bash infrastructure/macos/test-native-keyboard-entry-route.sh`
- `bash infrastructure/macos/test-panel-caller-flow.sh`
- `bash infrastructure/macos/test-native-entry-flow.sh`
- `bash infrastructure/macos/test-workspace-quit.sh` (native and floating)
- `git diff --cached --check`

Main port has compiled through these Mac fixtures, but has not had a universal
package build or live install. Windows runtime unverified. The installed combined
cee2ef8 has a completed universal build. Do not overwrite that color-preserving
installation with a main-only build: main intentionally lacks the unmerged color
work. Use an isolated combined test candidate when next installing, verify hashes,
checkpoint exact rollback and protected files, quit normally and confirm loaded ID.
The existing combined build command and installer scripts are in its build stage;
reuse them rather than writing another installer. Installation authorization does
not authorize changing custom colors or writing Speed Editor firmware.
