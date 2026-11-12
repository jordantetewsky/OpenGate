# OpenGate keyboard and Speed Editor customization plan

Preserve Resolve’s existing Keyboard Customization editor as native panel content.
Add a Speed Editor tab with the same visual organization, an accurate device
diagram, the full Resolve command catalog, and independent device profiles.
Reuse OpenGate’s panel and persistence mechanisms. This document records the
agreed experience and the implementation gates; it does not establish runtime
support for native device remapping.

## Agreed interface

| Area | Keyboard Customization | Speed Editor Customization |
| --- | --- | --- |
| Top preset selector and adjacent menu | Existing native controls and behavior | Same placement and styling; independent device profile selection |
| Device diagram | Existing native keyboard, unchanged | Accurate Speed Editor physical layout, labels, key groups, dial and selected-button indication |
| Selected control | Existing Active Key area | Selected device key, its behavior type and applicable gesture or mode assignments |
| Commands area | Existing native category tree, search, filters and command list | Same organization and complete command catalog, with device assignment state |
| Save and close controls | Preserve native behavior | Save device profile changes; no keyboard-preset writes |

Jordan’s November 11 screenshots are the visual reference. This is an extension
of that interface, not a redesigned settings application. Preserve the keyboard
editor’s current appearance, proportions, controls and minimum content scale.
Larger hosts can provide more space using its native layout behavior. A smaller
outer host must keep all content reachable at that scale through OpenGate’s
existing overflow mechanism; it must not shrink or rearrange native controls.

The whole customization surface is a normal OpenGate panel: flat Panels access,
floating, resizing, tab/group placement, retained hide/reopen, saved geometry and
unsaved workspace restoration. Its Keyboard and Speed Editor tabs share a host;
their selected presets and editing state remain independent. Existing panel and
group ownership must remain authoritative. Follow the numbered
[native entry contract](opengate-native-entry-flow-acceptance.md).

## Speed Editor behavior model

### Native device commands in both editors

Expose a **Speed Editor Commands** category in the existing command browser,
including the Keyboard Customization tab. This is an additive category in the
native interface, not a replacement keyboard editor. Device-native operations
remain distinct from similarly named menu commands: ROLL must not be represented
as Go to Nearest Edit or an approximating navigation macro.

Each exposed operation needs its verified invocation contract: discrete action,
paired press/release, held operation with dial updates, or persistent mode. A
keyboard assignment to a held operation must preserve its complete lifecycle;
one-shot native shortcut activation alone does not establish that support.
Shared command identity and native category registration must be established
before adding displayed rows. Registering only an OpenGate PanelCommands entry
does not make an operation appear in Resolve's native keyboard database.

Each key has a complete native default and an optional custom override. A custom
override specifies one of these behavior types:

- **Gesture button:** single press, double press, hold, and hold plus dial in
  either direction. The editor exposes the applicable assignments for that key.
- **Mode selector:** selects dial behavior such as Shuttle, Jog or Scroll. This
  changes persistent device interpretation rather than merely firing a shortcut.

Represent these as distinct behavior types. Resolve native mode selection,
momentary hold-plus-dial behavior, and ordinary command execution need explicit
semantics. Press/release, mode LEDs, dial deltas and page context are part of the
device contract where applicable. Do not add synthetic double-press waiting to
keys that have no double-press assignment. Define precedence for hold versus
double press and ensure one physical gesture produces one intended action.

Profile changes and resets during a held key or active dial gesture need an
atomic transition with no stuck key, delayed command replay or leaked mode.
Test this behavior before enabling hardware dispatch.

## Profiles and native reset

The first profile is always **Native Speed Editor**, pinned above custom profiles.
It is protected from editing, renaming, deletion and replacement by import.
Creating a custom profile from it creates overrides; it does not alter the native
baseline. Provide the familiar preset selector and Save as New, Import, Export
and Delete operations, plus the agreed create/duplicate/rename controls within
that established menu pattern.

**Reset this key to native** removes the key’s complete override, including its
behavior type and gesture/mode assignments. Other custom keys remain unchanged.
Selecting Native Speed Editor restores the full native device behavior. Native
defaults can be page- and state-dependent; do not flatten them into a list of
keyboard shortcuts or silently emulate them with approximate macros.

Persist device profiles and their active selection independently of the native
keyboard preset and OpenGate workspace preset. Changing any of these selectors
must not change the other two. Hiding the panel or switching tabs must not discard
pending edits. Match the native editor’s Save and unsaved-change behavior once
observed; draft recovery must not implicitly overwrite saved profiles.

## Reuse and ownership

Source review found these existing boundaries:

| Existing component | Responsibility and planned reuse |
| --- | --- |
| `ManagedPanelSession`, `PanelGroup`, overflow and workspace persistence | Own placement, grouping, geometry, retained visibility and restoration; use their existing production paths |
| `NativeKeyboardNavigation` and `keyboardlauncher.h` | Discover/navigate the real editor and invoke the native menu command; extend this path rather than add another editor launcher |
| Native command views and their Qt models | Investigate a guarded reference to command identity/category data, preserving native ownership and notifications |
| `ArrangementStore` | Own saved workspace entries, IDs, validation and atomic writes; reuse applicable persistence mechanisms after separating workspace-specific policy |
| `ArrangementControls` | Own workspace preset UI operations; reuse operation conventions while keeping device selection independent |
| `WorkspacePresetPolicy` | Protect built-in workspace presets; apply the same protection principle to Native Speed Editor below the UI |
| `NativePresetBindings` | Read native keyboard bindings; it is neither the complete command catalog nor a device-profile writer |
| `ColorPalette` and `ColorPaletteDrafts` | Own color definitions and color draft recovery; reference established mechanisms, without storing device mappings in color state |

The reviewed code has workspace- and color-specific stores, not an established
generic device-profile model. The active color agent confirmed there is no newer
generic multi-profile interface. PanelCommands owns a single Shortcuts.json
binding object, not profile selection; settingsProfilePath selects a legacy
directory, not an independent profile model. Reuse does not mean passing a device
tab the workspace’s active selection object or bending color persistence to own
device state. Coordinate any bounded shared extraction before changing callers.

Inject a reference to one Speed Editor profile owner into its tab and input
dispatcher. Use the existing persistence mechanism through a separate domain
instance. If a shared preset repository boundary must be extracted, migrate the
existing workspace caller to it in a bounded, behavior-preserving change and
remove the superseded implementation. Preserve existing file formats and errors.
Do not leave a duplicate save engine or a model used only by tests.

The native command catalog should be borrowed through a lifetime-safe interface.
The Speed Editor view needs its own selection and device-assignment projection;
assigning a device key must never call native keyboard-binding mutation methods.
Keep labels, categories and command identities authoritative. The Assigned and
Unassigned filters on the device tab must describe device assignments. Investigate
stable IDs and dispatch for commands with no keyboard shortcut; listing a command
does not prove it can be invoked. Do not silently restrict the catalog to the
commands currently assigned in a keyboard preset or to OpenGate’s own commands.

## Native discovery and evidence

The communication protocol exists, but this does not establish a public API.
Resolve’s installed binary contains `SyBMDKeyboardManager`,
`SyBMDKeyboardDeviceMac`, `SyBMDKeyboardDeviceBase`, `EditorKeyboardController`,
and an `OnDeviceInReportRecieved` handler name. It imports macOS HID report and
device APIs. These observations identify a private device/controller path;
they initially identified the search boundary. Subsequent offline arm64 analysis
recovered the HID callback, raw-to-logical key translation tree, native paired
dispatch and separate mode/dial handling. See the
[native trace ledger](opengate-speed-editor-native-trace-20261111.md) for exact
binary identity, addresses, evidence and remaining unknowns. These static results
do not establish a working runtime hook or a native catalog registration API.

A November 11 search of the connected main Resolve scripting API for Speed Editor,
keyboard, shuttle, jog, HID and key mapping returned keyboard-preset operations,
including listing/loading/importing/exporting presets, but no Speed Editor
mapping interface. That is a scoped search result, not proof that no extension
or private interface exists.

Blackmagic’s [keyboard product documentation](https://www.blackmagicdesign.com/uk/products/davinciresolve/keyboard)
describes keys that modify dial behavior while held, and Shuttle/Jog/Scroll keys
that select dial behavior. It documents user behavior rather than a remapping API.

Investigate the boundaries in this order:

1. **Device communication:** reports, key up/down, dial values, connection,
   authentication and output/LED state for USB and Bluetooth.
2. **Interpretation:** button identities, gesture recognition, mode transitions,
   page/context dependencies and the location of native defaults.
3. **Command dispatch:** stable command identities, native invocation, availability
   and undo behavior, including commands without keyboard shortcuts.
4. **Selective override:** establish whether custom keys can be intercepted while
   unchanged keys and dial modes still reach the real native handler exactly once.
5. **Restoration:** prove a per-key reset and the pinned native profile restore the
   same native behavior, including stateful modes and held-button interactions.

Prefer an exposed interface if discovered. Otherwise inspect the private path
with explicit version/ABI checks and a reversible adapter. A pointer or symbol
name alone is insufficient evidence of safe invocation. If exclusive device
ownership prevents mixed native/custom handling, that is a design blocker to
resolve before claiming per-key native reset. Recreating similar macros is not
an acceptable substitute for that requirement.

## Implementation sequence and module boundaries

1. **Record and inspect:** freeze the visual/behavior contract above, coordinate
   with the color agent, inspect native dialog lifetime, command models and the
   existing preset persistence boundaries. Make no speculative preset writes.
2. **Native editor hosting:** a focused adapter borrows/restores the real editor
   while preserving its complete appearance and behavior. Test modal/modeless
   ownership, native Save/Close, unsaved state, keyboard handling, tab hide/reopen,
   resize and teardown. The exact borrowing technique is not selected yet; no
   new editor UI substitutes for a failed native hosting experiment.
3. **Shared persistence boundary:** perform the bounded extraction only if needed,
   then inject an independent device-profile instance. Preserve the built-in
   native profile, versioning, atomic saving and corruption recovery.
4. **Speed Editor view:** use the accurate physical device layout in the keyboard
   diagram’s position and native-consistent command/profile controls. The view
   edits the shared device model; it does not own a second store or dispatcher.
5. **Behavior and dispatch:** one device interpreter implements gesture/mode
   selection; a separate Resolve adapter owns native command invocation and
   native fallback. Platform device APIs stay in platform adapters. The panel
   coordinator only wires these components into existing sessions and menus.
6. **Assembled acceptance:** verify the actual experience below before integration
   or installation claims. ShuttleGlide integrations follow through these same
   interfaces after its relevant controls and existing owners are inspected.

## Acceptance checks

- Native keyboard tab retains the actual editor, preset behavior and minimum
  content scale. All controls remain reachable in smaller hosts and larger hosts
  use available space correctly. No replacement shortcut database or redesign.
- First open, resize, tab switch, grouping, X-hide, Panels recall, restart,
  unsaved workspace restoration and Native return follow the numbered contract.
- Accurate Speed Editor diagram selects the correct physical key and shows its
  complete gesture or mode assignment. Both tabs expose the same command catalog.
- Speed Editor Commands appears in the existing native command browser on both
  tabs. Keyboard bindings to native held operations preserve begin/update/end
  behavior. Native ROLL and nearest-edit navigation remain separate operations;
  test edit-target selection and playhead position independently.
- Create, save, duplicate, rename, import, export and delete device profiles;
  restore selection and pending edits. Native Speed Editor remains protected and
  first. Keyboard preset and workspace selections are unchanged by these actions.
- Change one key, reset it to native, and verify other custom mappings persist.
  Compare reset behavior against actual native input on the same Resolve page.
- Verify single/double/hold/hold-plus-dial, mode transitions, release handling,
  reconnects, profile switching during input, unavailable commands, and commands
  without shortcuts. Check USB and Bluetooth, duplicate events and latency.
- Verify with a live Resolve project as explicitly authorized. Opening, resizing,
  tabbing and inspecting the panel must preserve timeline and shortcut assignments.
  Any dispatch test that edits media needs a bounded reversible test scope.
- Preserve the known live installation, other agent’s color work and quit repair.
  Record source, compilation, fixtures, installation and live acceptance separately;
  report Windows verification gaps rather than infer parity from shared code.

## Current checkpoint

Branch: `feature/keyboard-customization-panel-20261111`, based on `809c747`.
The native editor was opened and closed normally in project `WK 30 111126` without
editing assignments. Its panel-hosting lifetime has not been verified. No helper
was installed, no device takeover occurred and no input dispatch was changed.

The initial standalone profile-store draft was moved out of `src/` into ignored
`build/design-drafts/` pending the reuse decision. It is not integrated, tested or
the approved persistence architecture. The first UI candidate now exists in source and compiles. Full device-profile and
input-dispatch implementation remains outstanding; see the checkpoint below.


## First visible candidate — November 11, 17:30 checkpoint

Jordan prioritized a live-testable native keyboard panel with existing panel
scaling and a rough, accurately styled Speed Editor tab before finishing hardware
remapping. The device diagram now matches the physical key layout and white,
space-gray, dark-gray, red FULL VIEW and black-dial color groups.

Implemented boundaries:

- `NativeKeyboardEditor` platform adapter constructs the actual native editor for
  the exact inspected arm64 image. Native opener is stack-owned/modal, so it is
  not reused as a retained pointer. An owned object runs native initialization
  with a temporary instance-local exec override; the original vtable is restored
  before hosting. Native Save/reject methods remain authoritative.
- `InputCustomizationPanel` owns one editor and sibling device tab parked in the
  existing workspace. `PanelControls` only wires registration/entry/close.
- `SpeedEditorDiagram` owns physical drawing and selection.
  `SpeedEditorCustomization` owns the device-tab preview; mapping/profile writes
  are visibly disabled. Native Speed Editor is the sole current device profile.
- `NativeCommandBrowser` borrows native source models through independent
  read-only projections. Native runtime catalog discovery/completeness is not yet
  verified. The new Speed Editor Commands category is not registered yet.
- `ManagedPanelSession` continues to own placement/grouping/scaling/persistence.
  The new surface opts into existing overflow and a 100% minimum content scale.
- Exact-image verification moved out of the clip inspector into
  `MacNativeImageVerification`; the old duplicate implementation was removed and
  both real callers use the same verifier. No color state or swatch wiring changed.

Evidence: universal Mac runtime build passes; hidden input-customization fixture
passes native-like lifetime/Save callback retention, tab switching, hide/recall,
group attach/detach, explicit session parking, metric scale/reset, restored minimum
and narrow-host overflow. Diagram fixture and rendered layout checks pass.
These use a fixture dialog; they do not validate actual native construction,
painted keyboard scaling, model coverage, Save/Cancel or app teardown in Resolve.
A review found and repaired callbacks surviving the plain owner's abandonment
and restored scales bypassing the native minimum.

Actual entry points in the candidate: flat Panels entry and OpenGate's Keyboard
Shortcuts command route to the same panel owner. Resolve's original menu action
still opens its ordinary native dialog; intercepting that entry is not complete.
Current inner tab is retained in the live object but not yet restored across
restart. Pending native edits survive hide/tab changes in the retained editor;
restart recovery for those edits is not implemented. Complete device profile
persistence and remapping are deferred to the next milestone, not claimed done.

No candidate installation, live native factory invocation, project edit, merge,
push or publication has occurred. Windows is explicitly unavailable for native
editor construction; Mac x86 compiles but the factory rejects an unsupported
image. The color thread owns the current live test window. Preserve its runtime
and incorporate its current source before any combined installation.


### Numbered native-entry contract mapping for this preview

| Contract | Actual path / present evidence | Remaining live or integration gap |
| --- | --- | --- |
| 1. First/restored launch | Existing PanelControls inventory; new panel gated by experiment flag | Actual first open and restored native editor not tested |
| 2. Workspace presets | Existing WorkspacePresetPolicy untouched | New panel placement saved/reloaded in real preset not tested |
| 3. Original Resolve UI | Input panel is retained independently during Native return; same session/store | Actual native editor return/lifetime not tested |
| 4. Native hotkeys/temporary maximize | Existing command routing retained; own shortcut editor route targets panel | Ordinary Resolve menu still opens its modal editor; live key contexts untested |
| 5. Native-origin dragging | Existing session/group callbacks retained | Direct dragging from original native editor not implemented |
| 6–7. Pinned bands | Existing workspace owners untouched | New panel with every band combination not exercised live |
| 8. Title drag / retained X | Existing host and retained hide/recall; fixture passes | Actual title/pointer interactions and native Close prompt untested |
| 9. Flat Panels / grouping | New panel registered in existing flat inventory; group fixture passes | Live menu appearance, nested selection and original-menu parity untested |
| 10. Scaling / maximize / overflow | Existing metric scale and overflow; reset/minimum/overflow fixture passes | Native painted keyboard geometry/input and maximize require live check |
| 11. Page state / platform | Existing Edit session; universal Mac compilation | Other pages unverified; Windows native factory explicitly unavailable |
| 12. Assembled experience | Hidden native-like tab/session sequence passes | Real first-open through Save/restart/Native flow remains open |

This is the first visible prototype milestone. It is not full acceptance of the
numbered contract or of Speed Editor remapping/profile persistence.

### Size persistence in either mode — November 11 follow-up

Jordan explicitly requires the keyboard panel's size and scale to persist in
both OpenGate and Native modes. For this owned customization window, opening
and moving it must leave Native mode selected. Explicit docking/grouping with a
workspace surface still enters OpenGate. This is a deliberate scoped exception
to contract item 5, which continues to apply to native workspace panels.

`PanelControls` coordinates the existing retained session across mode changes;
`LayoutIndependentPanelState` transforms an incoming workspace preset around
the panel's existing saved record and removes conflicting preset group/native
membership. It owns no state and writes no settings. Existing workspace settings
remain the only persistence path. Native return detaches an explicitly grouped
customization panel back to its saved floating frame instead of parking it.

The caller fixture exercises actual PanelControls entry, Native return, close /
recall, workspace selection and coordinator recreation with a fixture editor.
These checks do not replace actual Resolve restart and native-editor acceptance.
The preview remains Edit-page scoped; both modes does not yet mean every page.

Expanded caller result: `panel caller flow failures=0`. It covers conflicting
preset size/scale, floating and native preset membership, Native close/recall,
explicit docking and Native extraction, and actual coordinator teardown /
recreation. The internal title-drag path uses a synthetic header wired by the
existing decoration lifecycle; real native decoration remains a live check.
Universal arm64/x86_64 build and strict signature verify, with preview enabled.
Runtime SHA-256: `2a893bbff7be300acf1eebecbae51a04b15165d9debcc85828c4d2f0935bdebe`.
The existing input-customization regression and assembled native-entry regression
also pass: `input customization failures=0`, `native entry flow failures=0`.

Organization review: incoming-preset projection lives in the stateless policy
module; PanelControls retains coordination only. The former direct overwrite of
every panel record is replaced by the policy-transformed workspace. No settings
writer, profile store or native command dispatcher was added. Shared behavior is
exercised on Mac; Windows runtime remains unverified and its native factory is
unavailable.

The explicit local build option `OPENGATE_INPUT_CUSTOMIZATION_PREVIEW=1` enables
the existing experiment at process startup and is recorded in the build
manifest. This makes an installed preview reachable on ordinary relaunch without
modifying global launch settings. Default builds retain the opt-in gate.

### Live preview corrections — November 11, 18:30 follow-up

Jordan's screenshots established that the native editor and device tab open in
Resolve. They also exposed cropped tab text, an embedded dialog that can move
inside its tab, and selection highlights that cannot be cleared. Those are bugs,
not accepted preview behavior.

The agreed correction keeps two entry points (Resolve's native Keyboard
Customization action/assigned shortcut and Panels) for the same retained editor.
Dockability is not being redesigned in this pass. Floating tilde expansion uses
the current screen's usable area and restores the original frame. The owned
editor gets a screen-bounded baseline host minimum; enlarged content continues
using bounded overflow. Position, size, scale and selected tab use the existing
session/settings path in either UI mode.

The owned tab bar measures complete labels and retains native tab chrome. A
focused embedded-dialog guard prevents native header movement from displacing
content within the page while leaving native preset/command/Save input intact.
The device diagram allows empty selection: a second click or empty-background
click clears the selection. This is editor selection, not active hardware mode.

The button-behavior terminology is Action button and Dial mode button. The
latter persistently chooses what the jogwheel controls, permits only one active
mode, and excludes other button gesture assignments. Current Native profile
remains read-only in this UI preview; input dispatch/profile writing is still
outstanding. A lazy, optionally visible device activity/mode monitor is a later
part of this feature and is not included in these UI corrections.

Correction verification before build/install: `input customization failures=0`,
`panel caller flow failures=0`, `native keyboard entry route failures=0`.
The tests exercise a deliberately narrow tab style at multiple font scales,
embedded native-dialog displacement, empty selection on repeated/background
clicks, current-screen expansion through the real coordinator, exact restore,
minimum sizing, selected-tab persistence through coordinator recreation, native
menu/assigned-shortcut routing, fallback, and queued requests after quit.
These are fixture results; the replacement is not yet installed at this entry.

Organization review: InputCustomizationTabBar owns label measurement/painting;
EmbeddedDialogAnchor owns only embedded positioning. PanelTabSelection binds to
existing session snapshots; applyPanelFloatingMinimum supplies only screen-bounded
host constraints. NativeKeyboardEntryRoute leases the existing native menu row
and handles its assigned shortcut, restoring original connections on normal
teardown and abandoning UI restoration on quit. PanelControls wires these
components. No new profile/settings writer, color reader, assignment/history
implementation or hardware input dispatcher was added. Shared callers compile
on Mac; Windows runtime and actual second-display behavior remain unverified.

The broader native-entry regression caught three old-panel expansion assertions:
applying screen bounds to every standalone floating panel changed their existing
workspace behavior. Screen expansion is now an explicit Target policy enabled
only for the layout-independent input panel. Ordinary native/workspace/group
panels retain their existing content-viewport expansion behavior.

### Profile persistence before live remapping

Jordan explicitly set the next milestone order after the UI test: establish
independent device-profile saving, loading and import/export before activating
hardware remapping. Profile round trips must preserve stable control/command IDs,
gesture assignments, dial-mode definitions and native-reset semantics. Validate
versions and unavailable commands without silently replacing valid profiles;
use atomic writes and a tested recoverable prior version. Keep the pinned Native
Speed Editor profile immutable and keyboard presets independent.

Live remapping remains disabled until the native event ownership boundary is
verified: one physical gesture must produce one intended action, without both
OpenGate and Resolve executing it. Establish native pass-through, temporary
hold/dial versus persistent exclusive mode behavior, release/cancel handling,
disconnect/reconnect, and return to native operation before enabling overrides.
This is host-side input/command adaptation; firmware updates, firmware writes,
and persistent device reprogramming are outside this feature. The current UI
build performs no device remapping or firmware writes.

Corrections installed at clean source `954a467`, with all four fixture suites,
universal build, signature and install/rollback rehearsal passing. Resolve loaded
that build after installation. Real UI acceptance is still pending; main merge
is conditional on that acceptance, not on compilation or fixtures alone.
