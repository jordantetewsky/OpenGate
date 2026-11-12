# Resolve Speed Editor native trace — 2026-11-11

## Scope and identity

Offline analysis of the installed Resolve arm64 executable. No live input hook,
project edit, device takeover, firmware change or installation was performed.
Addresses below are unslid virtual addresses, not runtime callable pointers.

- Original: `/Applications/DaVinci Resolve/DaVinci Resolve.app/Contents/MacOS/Resolve`
- Universal SHA-256: `d14f64e789f214fbfccbd9cad54b2e61b90d4bd202a9eb0105a74011306cbea5`
- arm64 UUID: `423A1472-54B0-39CE-9124-697B0D504821`
- Thin SHA-256: `801848888e276e40cefcc89011155702ddf67bb6ecfccff4de60ff505685e2a9`
- Local evidence: `/private/tmp/opengate-speed-editor-trace/`

Method: Mach-O section/function-start parsing, chained-pointer rebasing, LLVM
arm64 disassembly and instruction-level data-flow review. Stripped functions
must not inherit llvm-objdump's misleading nearest exported symbol labels.
The first REA/Ghidra whole-image analysis timed out at 330 seconds; that failure
is not evidence about target behavior. A longer attempt reached the client's 450-second deadline without an analysis
result. The session was subsequently closed; no Ghidra decompilation result is
claimed. Focused LLVM evidence supplied the findings below.

## Findings ledger

| Question | Evidence | Status |
| --- | --- | --- |
| Where does input enter Resolve? | HID registration `0x1009afb5c`, callback `0x1009b7a84`, report handler `0x1009b2b8c` | Static path recovered |
| Where is key identity mapped? | Constructor `0x1009aa990` initializes 106 raw uint16→logical int entries in tree `0x11d8bee88` | Hardcoded initializer creates runtime translation tree |
| Where are native behaviors mapped? | Press/release switch paths plus mode and held-state handlers | Partially recovered; not one complete editable command table |
| Can input be intercepted after decoding? | Press `0x1009b30e0`, release `0x1009b0d64`, queued dial `0x1009b06bc` | Candidate boundaries identified; no runtime hook verified |
| Can arbitrary native commands be invoked? | SyManager queues selectors; UI consumer routes them into native editing controllers | Hardware command path recovered; complete arbitrary-command invocation not established |
| Can native keyboard categories be extended? | Existing OpenGate registry is separate from native UiActionManager/editor models | Unresolved; displayed rows alone would not suffice |
| Does ROLL select an edit without seeking? | Begin/update/end recovered; downstream edit-target selection not yet located | User-observed behavior remains to be independently verified |

## Transport and key translation

Device callback registration uses device+0x318 as IOHIDDeviceRef, a 128-byte
buffer at device+0x320, and the device as callback context. Report 4 contains
six little-endian uint16 key IDs. Resolve compares current and previous sets,
then invokes press for newly present keys and release for removed keys.
The translation tree stores raw key at node+0x1c and logical key at node+0x20.
It is a key-identity translation, not evidence of arbitrary command rebinding.

Report 3 carries dial data. Parser `0x1009a6e98` reads flags, signed 32-bit
movement scaled by 4096, and an additional byte. The handler posts a Qt event;
queued callback `0x1009b956c` verifies that the device remains registered before
calling `0x1009b06bc`. Report 10 uses a separate handler and must not be confused
with the main dial report.

Logical translation `0x1009b3f24` also consults cached debug configuration
Local.Debug.SimulateSpeedEditor / Local.Debug.SimulateReplayEditor. The replay
simulation branch remaps CLOSE_UP and PLACE_ON_TOP. Their live flag values have
not been observed; this does not diagnose Jordan's APPND issue.

## Native action and mode dispatch

Normal SMART_INSRT (raw1), APPND (2), CLOSE_UP (4), PLACE_ON_TOP (5) dispatch
selectors 308, 309, 311 and 312 respectively, with press/release arguments.
They are paired hardware operations; calling one selector once is not proof of
an equivalent keyboard command.

The common path resolves context through `0x102ad26f8` / `0x102ad8600`, gets
SiteManager through `0x102b0bcb0`, finds a receiver with `0x102b0e800`, and calls
that receiver's virtual slot+0x50. SiteManager identifies the owner, not the
concrete receiver implementation. Its receiver vector is at owner+0xa0. Subsequent
analysis identified the receiver as SyManager: virtual+0x50 at `0x1014869f4`
queues events and virtual+0x58 at `0x101486f3c` dequeues them. UI consumer
`0x102d06018` dispatches those selectors through its jump table.

SHTL (28), JOG (29), SCRL (30) have native mode behavior, not the same ordinary
press dispatch. Mode setter `0x1009b2240` tracks mode at device+0x90, updates
hardware via the device virtual slot+0x50, resets dial/controller state and
updates LEDs. That device virtual slot is a different object/interface from
the context receiver slot above. SCRL additionally has a contextual branch.

## ROLL lifecycle

Raw/logical ROLL key 11 enters press case `0x1009b37b8`, which calls
`0x1009b3b4c(device, 3, 11)`. This helper:

1. Checks the held-operation owner, rejecting a conflicting held key.
2. Stores owner key 11 at device+0xa4 and selects JOG mode 1.
3. Stores held-operation kind 3 at device+0xa0.
4. Dispatches selector 298 with operation kind 3 to begin.

Main dial handler `0x1009b06bc` recognizes the held state, accumulates movement
with fractional remainder at device+0x98, and dispatches selector 307 with
integral signed deltas. Its report-mode and additional counter gates still
apply. Release clears owner/held state and dispatches selector 306 with kind 3.
Thus ROLL is a stateful begin/update/end operation, not nearest-edit navigation.
The UI consumer reaches UiTiEditKeyboardEditor, independently confirmed by RTTI.
ROLL begins through `0x10469e634` / `0x10469e63c`, using a gesture named ROLL.
Callback `0x1046c2084` reaches `0x10469e800`, creating a Keyboard Roll transaction.
Empty edit-target state invokes `0x1046c7ea0(4)`: it reads an anchor from timeline
state, compares distances to candidate clip boundaries through `0x1045c5a20`,
constructs a trim-target object and selects it through `0x10493296c`. This is a
separate edit-target construction path. Absence of playhead movement remains
unproven because downstream selection has view-display side effects.
Detailed data flow: `roll-native-trace-report.json` beside the assembly evidence.

Relevant files: `device-key-press.asm`, `device-key-release.asm`,
`hold-state-audit.asm`, `main-dial-audit.asm`, `mode-set.asm`,
`report-handler.asm`, `queued-dial-callback.asm`,
`key-translation-initializer.json`, `rebased-pointers.json`, `index.json`.
Raw button identities were also independently corroborated against the local
Magic Automa 4.6.14 packaged HID decoder; that corroborates key numbering only.

## Implementation implications and remaining work

The native baseline should delegate to the real native path. Removing one
custom override should restore that path for the key, not install a substitute
macro. However, press/release handlers also maintain held-key and mode state;
suppressing an entire handler indiscriminately can break bookkeeping. Determine
the exact override boundary and paired lifecycle before installing a hook.

Expose verified native operations under Speed Editor Commands in both native
command-browser tabs, with distinct identities for ROLL and nearest-edit
navigation. Keyboard assignment requires correct release/continuous input
semantics in addition to a catalog row. The native registration and persistence
interface remains to be recovered. Coordinate with the color agent, whose UUID
swatch commands have the same native-catalog integration question.

Next evidence needed: concrete context-receiver method; ROLL edit-target and
playhead handling; native UiActionManager category/command registration and
lifetime; version/ABI validation; controlled runtime observation; assembled
panel/profile/input acceptance. No production implementation is claimed here.


## Native keyboard panel lifetime

Native opener `0x10553bf68` constructs UiActionsEditorDialogImp on the stack,
initializes/runs it, then destroys it. A retained panel cannot borrow that pointer
after the opener returns. The object span is 0x108 bytes, with 8-byte alignment
sufficient by the observed stack layout. Constructor `0x1052159a0` creates native
widgets/controller; initialization `0x1052189a4` populates current preset state
and finally invokes virtual exec at +0x1a8. The wrapper's exec performs focus
restoration around QDialog::exec; no further editor-specific initialization was
found there. Native Save and reject remain native methods.

The preview factory uses separately owned storage and native construction. Only
during initialization, its instance vtable is replaced by an RTTI-preserving
copy whose exec slot returns without entering the modal loop. RAII restores the
original table before reparenting the actual dialog as a tab child. Exact image,
code ranges, class metadata, and all primary vtable slots are checked first.
The existing clip-image verifier was extracted and reused. No global executable
or vtable patch is made. This is implemented and compiled, not live-validated.
Native finished queues the same app notification used by the ordinary opener.
Evidence and limits: `keyboard-dialog-lifetime-report.json`.
