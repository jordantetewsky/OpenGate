# OpenGate keyboard and Speed Editor customization plan

Preserve Resolve’s existing Keyboard Customization editor as native panel content.
Add a Speed Editor tab with the same visual organization, an accurate device
diagram, the full Resolve command catalog, and independent device profiles.
Reuse OpenGate’s panel and persistence mechanisms. This document records the
agreed experience and the implementation gates; it does not establish runtime
support for native device remapping.

## UI repair checkpoint after planning consolidation

Clean source `6cfb407` is now installed and loaded in Resolve PID 76286 after
Jordan quit for installation. Live UI acceptance remains pending. All 25 managed
file hashes and the strict bundle signature verify. The closed-session comparison
confirmed all 203 protected files unchanged before reopening, including custom
colors. Transaction `37150516-6721-40CB-84B6-AAF82B37314F` retains the exact
previous `954a467` installation; the installation/rollback rehearsal passed.
The feature branch is pushed; nothing is merged to main or published as a release.

Direct inspection of previously installed `954a467` reproduced the native macOS
application-menu entry opening a separate modal keyboard dialog. The Qt menu
lease did not cover that AppKit-owned route. A focused Mac target/action lease
now forwards that menu item to the same retained panel, retaining native
validation, shortcut display and original fallback. Normal teardown restores the
item; application quit disarms callbacks without rewriting the native menu.

Jordan requested **Command–Shift–K** as an entry. The inspected saved preset
currently assigns Command–Option–K to `resolveCustomizedActions`, while
Command–Shift–K belongs to a Fairlight timeline operation. The additional shortcut
is Edit-scoped and checks active-page conflicts/text/modal contexts; it does not
rewrite the keyboard preset or take over Fairlight's assignment. The existing
native assigned shortcut route remains in place.

The toolbar now registers Keyboard & Speed Editor immediately after the existing
Source Monitor / Program Monitor toggles, reusing their visibility callbacks,
checked-state synchronization and narrow-toolbar overflow. The existing expansion
controller retains the independent editor's expansion preference and normal frame
through the existing workspace record. Hiding releases the temporary expansion
slot; recalling reacquires it, so a hidden editor cannot intercept timeline tilde
or block another panel's expansion. WorkspaceSnapshot preserves that preference
when rebuilding the geometry record, and toolbar checks update on visibility
changes rather than waiting for a settings write. Ordinary native/group expansion policy is
preserved. Startup from a hidden expanded checkpoint is tested; actual Resolve
relaunch and second-display behavior remain live checks.

The device preview now exposes Button actions / Jogwheel mode, all five action
gestures, left/right mode directions, conditional mode help, and the Native
assignment preview per action gesture. Save/reset remain disabled; preview choices
are transient and do not edit the protected profile or send device input.

Verification: input customization, panel caller flow, native keyboard entry route
and native entry flow fixtures all report zero failures. Coverage includes the
actual AppKit target/action route in an isolated test process, native fallback and
validation, Command–Shift–K's page conflict policy, expand/X-hide/toolbar recall/
command recall/restore, Timeline expansion while the editor is hidden,
saved-expanded startup, and the agreed dropdown visibility.
These results do not establish the new route works in live Resolve yet.

Organization review: NativeKeyboardMenuRoute is the platform contract;
MacKeyboardMenuRoute owns only the AppKit menu lease. NativeKeyboardEntryRoute
coordinates native/menu/shortcut entry; PanelExpansionController owns expansion
state; ToolbarDoors supplies the existing visibility UI; SpeedEditorCustomization
owns the transient preview. PanelControls wires callbacks and startup restoration.
No new settings/profile writer, hardware interpreter or color implementation was
introduced. The replaced unconditional expanded-close/door-restore paths now use
the expansion owner's policy. Shared changes have Mac fixture coverage; Windows
runtime remains unverified, with the OS-menu adapter explicitly a no-op there.

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

### Button actions, Jogwheel mode, and Native per gesture

The current user-facing terms are **Button actions** and **Jogwheel mode**.
These supersede the earlier Gesture button / Action button / Mode selector /
Dial mode button wording in dated checkpoints. Jogwheel mode describes a
persistent change in what turning the wheel controls, not only Shuttle behavior.

- **Button actions:** separate assignments for Single press, Double press, Hold,
  Hold + dial left, and Hold + dial right. Single/double press mean the physical
  button interactions Jordan also calls single click/double tap.
- **Jogwheel mode:** pressing this button selects what the wheel controls;
  releasing the button leaves that mode active until another mode is selected.
  Only one persistent jogwheel mode is active. A button configured this way does
  not also carry independent single/double/hold action assignments.

**Native is an assignment choice for each applicable gesture**, not merely a
whole-button behavior option. It means preserve Resolve's actual Speed Editor
interaction for that gesture, including its held/release behavior and context.
For example, overriding Hold + dial left must not silently replace the key's
native single press, double press, hold, or rightward behavior. If a gesture has
no native action, Native preserves that absence; do not invent an action. The
ability to combine native and overridden gestures on the same physical key is
still a dispatch acceptance gate, not a verified runtime capability.

The editor must offer inspectable dropdowns for the behavior and applicable
gesture options. Show the Jogwheel mode explanation only when that behavior is
selected; hide it when no control is selected or when editing Button actions.
Keep the existing native visual style. Any UI-only preview must explicitly say
its selections do not save assignments or change hardware behavior.

Suggested explanation for Jogwheel mode:

> Jog, Shuttle, and Scroll change how the wheel navigates the timeline. A custom
> Jogwheel mode can instead control zoom, move between edits or markers, or adjust
> another directional value. Press the button to activate it; the mode stays
> active after release until you select another mode.

Represent the behavior types distinctly. Resolve native mode selection,
momentary hold-plus-dial behavior, and ordinary command execution need explicit
semantics. Press/release, mode LEDs, dial deltas and page context are part of the
device contract where applicable. Do not add synthetic double-press waiting to
keys that have no double-press assignment. Define precedence for hold versus
double press and ensure one physical gesture produces one intended action.

Profile changes and resets during a held key or active dial gesture need an
atomic transition with no stuck key, delayed command replay or leaked mode.
Test this behavior before enabling hardware dispatch.

### Separate the function from its activation

Timeline Zoom is a function, not inherently a persistent mode. The same function
can be used by separate button actions, by a temporary held interaction, or by a
persistent Jogwheel mode:

| Example assignment | Behavior |
| --- | --- |
| Single press -> Zoom In | One zoom-in action |
| Double press -> Zoom Out | One zoom-out action |
| Hold + dial left/right -> Zoom Out/In | Adjust zoom while held; release returns wheel control to the previously active mode |
| Jogwheel mode -> Timeline Zoom | Turn to zoom after releasing the button, until another mode is selected |

Do not create separate implementations of a function for held and persistent
activation. One verified operation adapter supplies the target and directional
adjustment; the shared interpreter owns activation and begin/update/end lifetime.
Profile data must distinguish behavior, gesture, command/function identity,
target, and any supported increment/sensitivity. Native inheritance is explicit
and distinct from an unavailable command. Round-trip these through save, export,
import and restart before enabling hardware overrides.

### Directional pairs and adjustable values

Jogwheel modes may drive either a decrease/increase command pair or a continuous
parameter adjustment. The following are requested examples, subject to verifying
Resolve's actual command/parameter access; listing them is not a support claim.

| Function | Left/right turning | Target and units to establish |
| --- | --- | --- |
| Timeline or viewer zoom | Zoom out/in | Identify timeline versus viewer; use its supported zoom steps or value |
| Edit navigation | Previous/next edit | Relevant timeline/context; movement threshold per edit |
| Marker navigation | Previous/next marker | Relevant timeline or clip markers; threshold per marker |
| Position X | Decrease/increase horizontal position | Selected image/clip transform; verify native position units and sign |
| Position Y | Decrease/increase vertical position | Selected image/clip transform; verify which sign moves up/down |
| Nudge selected clips | Earlier/later by frame increments | Selected timeline clips, frame rate, selection and track constraints |
| Audio level | Volume down/up | Explicitly distinguish selected clip level from track level; supported dB increments |

The target must be visible and unambiguous in the assignment. Do not silently
substitute track volume for clip volume, image position for timeline placement,
or playhead movement for nudging selected clips. Verify no-selection, multiple
selection, locked/invalid targets, supported page/context and parameter bounds.
Continuous adjustment is preferred where a verified native interface supports
it; repeated discrete commands are a distinct supported path, not proof of a
continuous parameter API. Both require sensible undo behavior and correct gesture
completion/cancellation. These same functions may be used with hold + turn.

### Preserve Jog's natural response

Jordan specifically wants the direct, natural feel of **Jog**, not a custom
Shuttle-style playback-speed control. Start by investigating reuse of native Jog
movement and its processing; do not require an invented acceleration curve or a
curve editor to enable each function.

The [native trace](opengate-speed-editor-native-trace-20261111.md) identified
signed dial movement and fractional-remainder accumulation into integral deltas
in the held-Jog path. This is evidence of a useful mechanism, not proof that the
full ordinary Jog response, acceleration, snapping or downstream processing can
already be redirected. Establish where that response is produced and measure
native behavior before claiming equivalent feel for custom modes.

Each function still needs an appropriate sensitivity or increment. One frame's
worth of movement must not automatically become one marker, edit, zoom step or
decibel. Movement per action/value unit is a sensitivity choice; it does not by
itself require a nonlinear acceleration curve. For discrete operations, preserve
fractional motion between reports and issue steps at an appropriate threshold.
Avoid reporting a successful continuous adjustment when only discrete commands
are available. More elaborate response tuning is conditional on measured need.

Acceptance covers small precise turns, fast turns, immediate direction reversal,
stop response, latency, repeat rate, target bounds and undo. Prevent an unbounded
command queue from continuing after the wheel stops. Reset residual motion at
defined mode/focus/target/connection boundaries so it cannot leak into the next
operation. Verify USB and Bluetooth and test native Jog alongside each custom
function. Do not claim identical feel from source inspection alone.

## Profiles and native reset

The first profile is always **Native Speed Editor**, pinned above custom profiles.
It is protected from editing, renaming, deletion and replacement by import.
Creating a custom profile from it creates overrides; it does not alter the native
baseline. Provide the familiar preset selector and Save as New, Import, Export
and Delete operations, plus the agreed create/duplicate/rename controls within
that established menu pattern.

**Reset this key to native** removes the key’s complete override, including its
behavior type and gesture/mode assignments. Other custom keys remain unchanged.
**Reset this gesture to native** removes only that gesture's override, preserving
the other assignments on the same button. Prove mixed native/custom gestures do
not both dispatch, swallow required lifecycle events or alter native timing.
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

## Focused customization and the optional device monitor

### Entry points and retained visibility

Jordan proposes a third entry beside the program/timeline viewer: a compact
on/off button for the same Keyboard & Speed Editor Customization panel. Its exact
placement needs inspection of the existing viewer controls before implementation;
do not cover native controls or invent a second panel instance. This is the
customization toggle, distinct from the optional activity monitor's visibility.
The native Keyboard Customization menu/assigned shortcut and flat Panels entry
remain the other two entry routes; their reported live parity issue stays open.

The viewer button's checked state reflects actual panel visibility, including
changes made through X, the editor's Close control and other entry points. Off
hides the retained panel; on recalls it and its selected tab, geometry, scale,
editing state and expanded/normal presentation. Route all entry points through
the existing session/visibility owner and persistence path, not a button-owned
boolean or another settings writer. Preserve native Save/unsaved-change handling.

**Closing while expanded must hide immediately.** It must not require a second
close after shrinking to the prior frame. Reopening should retain expansion;
tilde restores the exact pre-expansion normal frame, subject to available-screen
constraints. Preserve both the normal frame and expanded state rather than save
the screen-sized rectangle as the normal frame. Include restart restoration in
the acceptance checks; it is not established by same-session hide/reopen.

Source diagnosis of Jordan's November 11 report: ManagedPanelSession's Close
filter handles an expanded host before ordinary retained hide and invokes
expandedClosed; PanelControls currently wires that callback to restoreExpanded.
This explains the observed restore-instead-of-close path. Scope the correction
to the independent customization editor while preserving existing native/group
temporary-expansion contracts. No correction is implemented by this document.

Acceptance sequence: open through each entry -> resize/select tab/change scale
-> expand -> X-hide -> viewer toggle recall -> confirm expansion/tab/scale ->
tilde restore exact normal frame -> toggle off/on -> restart and recall. Also
exercise the native editor's Close/Save flow and both Native/OpenGate UI modes.

### Input focus and shared observations

November 11 clarification: pressing a physical control while customizing must
identify that control without performing its timeline action. Merely having the
customization window visible must not capture input after focus returns to the
timeline. Preserve the real keyboard editor's key recognition and assignment
behavior, including normal typing in its search and preset fields. Verify that
hosting the editor preserves native shortcut isolation; do not install a broad
keyboard handler that consumes text entry or duplicates native recognition.

With the Speed Editor tab active and the customization window focused, device
press/release and dial input belong to an inspection context. A press selects the
corresponding control and displays its physical state; release clears its held
indication. Hold-plus-dial can identify the combination to edit without running
the mapped command. Inspection must suppress both native and custom timeline
actions, including delayed gesture callbacks. This requires a verified event
ownership boundary; an observer alone cannot provide suppression. Do not claim
this mode works until native input can be isolated and safely resumed.

With the timeline focused, normal native or enabled custom actions execute once.
The optional Speed Editor monitor observes those same events and their dispatch
results, animating the pressed control and showing the active dial mode and the
command being performed. Opening the monitor must not enable inspection capture.
It is a learning aid modeled on ShuttleGlide's gesture monitor; inspect and reuse
applicable existing behavior before implementing its Qt view. Unsupported native
command identification must be shown as unknown, never inferred as executed from
a button press alone. Distinguish an attempted dispatch from confirmed execution
where the native adapter cannot report completion.

Both views subscribe to one authoritative device-state owner through a
lifetime-safe interface. Do not use a borrowed raw pointer as a lifetime policy
or let either view read the device independently. Keep these concepts separate:

- Editor selection: which control/gesture is being edited; clicking it again or
  the background clears selection without synthesizing a hardware release.
- Physical state: pressed/released controls, held combinations and dial motion.
- Operational state: the exclusive active dial mode and command activity. A mode
  can remain active after its selector is released, including on unlit buttons.

The monitor is an optional Qt panel with lazy construction and visible-only
animation/subscriptions. Hiding it stops presentation work without changing input
processing or losing the active mode. Reopening reads the current shared state.
Focus changes, closing/hiding the editor, tab changes, disconnect and teardown
must cancel pending inspection gestures without replaying them into the timeline.
Define and test handover while a key is held: native/custom handlers must receive
balanced lifecycles, never an orphan release, stuck hold or late press.

## Implementation sequence and module boundaries

This is the current forward order; the dated checkpoints below remain historical
evidence. Read-only native-path investigation may proceed alongside persistence,
but live remapping stays disabled until its prerequisites pass.

1. **Finish installed UI acceptance and entry repair.** Jordan reports the two
   entry points are not delivered. Keep this open despite routing fixture passes.
   Demonstrate the actual native Keyboard Customization menu/assigned shortcut
   and flat Panels entry opening the same retained editor, with native fallback
   only where explicitly unsupported. Verify labels, bounds, focus, scaling,
   selection clearing and persistence in both UI modes. Correct expanded Close
   to retained hide and add the proposed viewer toggle using that same owner;
   prove all three entry routes and synchronized visibility. Preserve native styling.
2. **Complete independent device-profile persistence.** Reuse the existing atomic
   persistence mechanisms through one device-profile owner; extract shared policy
   only if needed. Prove save/load, create/duplicate/rename/delete, import/export,
   restart, draft handling, version/error recovery and immutable Native Speed
   Editor before activating any live mapping. Profiles remain independent of
   keyboard and workspace presets.
3. **Establish device state and exclusive event ownership.** A platform adapter
   receives verified native device events; one shared state/interpreter owns
   physical state, gestures and exclusive dial modes. A focused routing policy
   chooses inspection versus normal native/custom handling. Verify native
   pass-through, focus transitions, balanced releases, disconnect/reconnect and
   no duplicate dispatch. No firmware writes or device reprogramming.
4. **Add physical-input selection in the focused editor.** The device tab projects
   shared state into selection/held feedback; it owns neither a second device
   reader nor command execution. Verify physical presses and hold-plus-dial select
   assignments without timeline actions. Separately verify the native keyboard
   editor still recognizes typing/key assignments without leaking timeline
   shortcuts. Returning focus to the timeline resumes normal input.
5. **Add the optional observation monitor.** A separate Qt presentation module
   subscribes to shared state and dispatch observations, with visible-only
   animations and mode/command feedback. Start with verified native behavior;
   use the same interface for later custom dispatch. It must not consume input,
   execute commands, or change profile selection.
6. **Enable selective remapping and verified native commands.** The interpreter
   resolves saved mappings; a separate Resolve adapter invokes native commands
   and preserves fallback. Verify action gestures, temporary hold/dial and
   persistent exclusive modes, commands without shortcuts, per-gesture and per-key
   native reset, and full native-profile return. Validate directional functions,
   explicit targets/units, Jog response and undo with the same operation adapters
   for held and persistent activation. Add native device commands to both catalogs
   only after their identities and invocation lifecycles are established.
7. **Accept the assembled flow before integration.** Exercise all three entry points,
   focused inspection, timeline use with monitor, save/export/restart/import,
   native return and rollback together. Retain separate source, fixture, install
   and live-acceptance evidence. Main merge remains conditional on Jordan's UI
   acceptance; do not treat this plan as acceptance or hardware support.

Panel coordinators wire these boundaries; persistence, event routing, device
interpretation, native invocation and monitor rendering retain their own jobs.

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
- Override and reset one gesture while keeping the same key's other gestures
  Native; verify both native/custom ownership and profile round trips. Inspect
  each dropdown and confirm mode help is conditional on Jogwheel mode selection.
- Use the same directional function through hold + turn and persistent Jogwheel
  mode; verify release restoration, exclusive mode state, explicit target/units,
  parameter bounds, undo, reversal and no queued action tail after stopping.
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
