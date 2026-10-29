# OpenGate repair checklist

## Development status — October 29, 2026

**Work in progress: the audio-channel commands are not working end to end.**
The latest live attempt failed before a native action ran; channel mapping stayed
unchanged and no Undo was requested. A local receiver-context correction passes
focused tests but is not installed or accepted. Silent native preparation,
correct audio/repaint, native Undo and the full four-command matrix remain open.
The one-native-UI direction below is a recorded implementation target, not a
completed redesign. This branch is a development checkpoint, not a release.

[Current branch status and source branches](../../WORK_IN_PROGRESS.md) separates
local implementation, verification, installation and actual user acceptance.
Historical dated entries below retain their evidence and are superseded by newer
entries where they describe the same state.

## Native workspace clarification and investigation — October 28, 2026

the project maintainer requests investigation of one native Resolve UI enhanced by OpenGate,
with equivalent functions in native-origin and saved/default arrangements. The
user should not manage internal Native/managed states or perform recovery steps
to regain functionality. Selecting the default from the workspace/layout list
should return the familiar arrangement. Include automatic vacancy reclaim,
core-window edge placement/docking, minimum-width overrides with reachable
controls and correct painting, and the resolved header without Panels/Inspector
overlap. Preserve the complete existing Panel/Container/Layout task list.

Read-only findings and measured limits are recorded in
native workspace investigation (internal reference retained locally).
This investigation is on isolated `investigate/native-workspace-parity`, based
on installed source [checkpoint]; no runtime, settings or installation changed.
The current runtime silently enters managed state after a persistent Native
gesture, but a managed flag does not imply a full-root replacement. The latest
faster-feeling arrangement still uses grouping. Relay's recent jumps read back
correctly; its log lacks request durations, so no Relay speedup is established.

- [x] Preserve current-session evidence and inspect root ownership, input routing,
  native toggle coordination, save/menu work, edge targeting and width policies.
- [x] Identify measurement gaps: main-window responsiveness coverage, Relay
  request boundaries, unequal workloads and missing resource profile.
- [ ] Implement one native Resolve workspace enhanced by OpenGate, with no
  user-facing mode switch required to access the full feature set. Preserve
  dragging, floating, grouping, tabs, shortcuts, maximize, hide/recall, independent
  bands, and saved/unsaved per-page persistence in every supported arrangement.
- [ ] Make default-layout selection in the workspace/layout list sufficient to
  restore the familiar view; no separate activation or recovery sequence.
- [ ] Reclaim vacated space automatically in native-origin arrangements as well
  as saved/default arrangements, without resetting unrelated panel positions.
- [ ] Measure matching operations before predicting a design speedup; current
  median menu/state refresh is 27.722 ms per recorded save (265 samples), not a
  promised reduction or demonstrated cause of Relay delay.
- [ ] Reduce demonstrated redundant menu/state/layout coordination while reusing
  existing owners and preserving native actions. Compare matched Relay and UI
  timings before/after; report measured improvements separately from hypotheses.
- [ ] Supply core-window content-edge docking through the existing target/layout
  mechanisms; native empty-space targeting is currently explicitly excluded.
- [ ] Provide sizing/minimum-width parity across native-origin, embedded and
  floating panels, with per-panel native painting/reflow/overflow validation.
- [ ] Reuse the resolved header consistently with reserved space, and reconcile
  one-UI presentation with reliable underlying native recovery and the numbered
  assembled acceptance flow. No redesign implementation or acceptance claimed.

## Current priorities and progress — October 27, 2026

### Native-action attempt failed before invocation — October 29

the project maintainer reports nothing happens. Loaded [checkpoint] received both workflow and preset
command requests, including Next Audio Channel — All Source Channels. Each was
rejected with fresh-manually-bound-context-required. There are zero bound-context
records, zero native-action invocations, and no consumed permit. Read-only lookup
by the original item identity confirms the mapping remains mono channel 3/Channel A
and its Media Pool configuration is unchanged. The selected-item list was empty
at a later readback; that is not evidence the original selection was wrong.
Do not repeat the command or request Undo. The unused permit has been archived,
leaving the running experiment disabled. Evidence: audio-direct-action-test/
build/direct-action-install/{live-audio-evidence,live-noop-mapping,attempt-summary}-[checkpoint].json.

Isolated fix/audio-context-receiver reproduces one concrete weakness: capture
depends on Timeline keyboard focus before the native context event handler runs.
The live first click had no focused widget. The new unfocused-receiver regression
fails on [checkpoint] for all four variants, then passes after resolving the Timeline
from the actual QWidget receiver. Existing focus resolution reuses this shared
lookup; selected-item/hit/identity/expiry/one-use checks remain. Capture rejection
reasons now identify remaining early returns. This does not prove focus loss was
the sole live cause: the old capture has no per-stage context-event rejection
records, and later attempts also failed. The fixture cannot prove native menu
target binding, real QAction effect, audio/repaint, Undo or silent preparation.

Verification: hidden native-audio-action fixture fails before/passes after; hidden
native-edit-command regression passes; arm64/x86_64 production translation-unit
syntax checks pass. Initial sandbox fixture launch could not create a test window;
the permitted hidden run supplies the behavioral result. Mac and Windows callers
were reviewed; Windows compilation/live behavior remain unverified. Actual diff
review finds one reused read-only resolver, no parallel input/selection/visibility
owner, no new state writer, and no coordinator growth. Local source correction is
not installed, merged or released. Gate 1 remains open, and deterministic silent
preparation for arbitrary selection (gate 2) is still unestablished.

### Approved native-action test installed — October 29, [test time omitted] UTC

Test-session update: the project maintainer quit and the prepared launch-test.sh opened test process,
session [test identifier omitted] at [test time omitted] UTC. Loaded [checkpoint] and
the exact OPENGATE_AUDIO_ACTION_PERMIT process path are verified. the project maintainer opened
the designated disposable project/Timeline/item; read-only inspection confirms
sole selected audio instance, mono source channel 3 (Channel A), five embedded
channels, with Media Pool configured source channels 3/4/5. Saved a fresh baseline
of a project-wide instance and source-mapping baseline without changing
Local evidence and rollback details are retained privately.
Created one expiring, single-use permit for those exact identities; expiry is
recorded in test-ready-[checkpoint].json. No native channel action has run yet.
User test: open/cancel the selected clip's Audio Channel submenu, then invoke
OpenGate UI > Commands > Next Audio Channel — All Source Channels once. Expected
3/Channel A to 4/Channel B; inspect the actual result before requesting Undo. Do not repeat
or issue Undo on an unchanged mapping. This remains the diagnostic component gate,
not proof of silent preparation or a finished four-command workflow.

Resumed the already-approved [checkpoint] installation after a fresh process check
confirmed Resolve closed. Transactional installation completed with macOS
administrator authentication. All 22 staged/installed payload hashes and both
executable signatures verify. All 232 freshly snapshotted protected settings,
native preferences and unrelated workflow files are unchanged; only the four
intended audio scripts/helper/Current metadata were installation targets.
Local evidence and rollback details are retained privately.
Rollback to [checkpoint]: [local path omitted]
[test identifier omitted]/Restore Previous OpenGate.command.

the project maintainer reopened Resolve. Loaded [checkpoint] is confirmed in test process, session
[test identifier omitted], started [test time omitted] UTC. Read-only API
inspection confirms the exact designated disposable project `disposable test project`
([test identifier omitted]), `disposable test timeline`
([test identifier omitted]), and sole selected item
[test identifier omitted], [test media] - Channel A.
Process environment inspection confirms OPENGATE_AUDIO_ACTION_PERMIT is absent.
The active permit file is also absent. This ordinary-launch session therefore
cannot execute the experiment; no native channel operation or Undo was run.

Next external action: the project maintainer saves/quits normally so the existing launch-test.sh
can start a process with its narrowly scoped permit path. Then verify the same
target, create the one-use expiring permit, manually open/cancel Audio Channel,
and invoke Next Audio Channel — All Source Channels once. Inspect actual mapping,
instance isolation and audio/repaint before asking for exactly one native Undo;
no Undo if mapping did not change. Manual preparation remains diagnostic only.
The finished silent command lifecycle, four-command matrix and Windows evidence
remain open. No runtime merge, publication or product acceptance is claimed.

### One-shot native-action experiment [checkpoint] — staged, not installed

the project maintainer explicitly approved installation. Process check still finds Resolve PID
77381 running, so no files were changed and installation waits for the project maintainer to save
and quit normally. The active permit file is absent; the experiment remains unarmed.
Approval need not be requested again once Resolve is closed.

the project maintainer reports that the repeated probes/builds feel like a spiral and requests a
clear completion path. No working four-command feature has been delivered. Keep
that fact explicit; scaffolding, compilation and fixtures are not user acceptance.
Installed [checkpoint] remains read-only; no new native action has been performed.

Local experiment/audio-direct-action-test is clean at [checkpoint]. Its scoped test
invokes only QAction::trigger on an existing native channel action after both
manually opened menus have closed. It contains no popup creation/navigation,
setActiveAction, API mapping setter, automatic Undo or focus restoration. This is
deliberately a diagnostic gate, not the final no-manual-preparation command flow.
The native callback's actual effect, instance scope, repaint/audio and Undo are
unverified. Native cleanup on menu hide remains relevant to whether this can work.

Responsibilities: NativeAudioActionExperiment owns the observed context and one
native invocation; NativeAudioActionPermit owns identity/expiry validation and
atomic one-use consumption. Existing AudioChannelPlanRequest owns read-only API
planning transport. InteractionDiagnostics forwards events through its existing
observer. NativeEditCommands, PanelControls and both OS services only wire the
optional experiment. Without the explicit process permit path, it is not armed.
Focused Timeline-view lookup was extracted separately in [checkpoint], moving existing
callers and removing duplicate lookup loops. No second application input filter.

Actual user context-event coordinates must hit the one selected item. The context
is invalidated by selection changes, deactivation or intervening input, and both
native menu hide signals are required. A fresh API plan must match the allowed
disposable project, Timeline and item plus current menu channel/count and requested
direction/scope. A consumed marker prevents another invocation across process
restarts. The action-attempt receipt is flushed before triggering. No permit is
active in the staged package; its template is expired and references only the
previously designated disposable test timeline disposable item.

[checkpoint] versions planning transport as v3. Malformed, inactive, expired, mismatched
or late tickets cannot become new registry commands. The experiment retains a
consumed planning ticket to prevent late-script replay. Four updated read-only
audio wrappers must be installed with this helper. This diagnostic transport still
does not establish a final ordinary workflow-menu lifecycle or startup-availability
repair; those remain open. No change to existing Stabilize or Fixed Playhead logic.

Verification: hidden production-registry/planner/native-QAction fixture passes all
four direction/scope cases and refuses wrong project/Timeline/item/clicked target,
changed selection, deactivation, unclosed menus and repeated invocation. It asserts
no menu visibility/focus/selection changes in the fixture. Nine read-only planning
tests and Mac/Windows v3 wrapper/no-replay tests pass. Universal Mac compilation,
bootstrap lifecycle, both signatures and exact install/rollback rehearsal pass.
Windows native compilation/live operation and actual Resolve audio/Undo remain
unverified. Actual diff review leaves native execution/permit IO in focused modules;
coordinators gained only event/callback wiring and both platform callers were checked.

Local evidence and rollback details are retained privately.
files across helper/Current metadata/four audio scripts (six transaction targets).
Rollback baseline [checkpoint]. Existing settings and native keyboard presets are not
targets. Candidate [checkpoint] is approved but not installed, merged or published.

Bounded completion gates: (1) prove this real native action gives the requested
mapping, actual audio/repaint, intended-instance isolation and one valid native
Undo in the designated disposable project; no Undo if the mapping did not change.
(2) establish fresh native action preparation for an arbitrary newly selected clip
without manual menu setup or UI/focus changes. (3) only then complete the full
four-command matrix, repeat/wrap, excluded-channel mode switches, selection/error
cases, native keyboard behavior and platform evidence before accepted integration.
If a gate cannot be established, report the precise native/API limitation and stop
repeating unsupported popup experiments. A passing one-shot test cannot close gate 2.

### Connection probe installed and captured — October 28, [test time omitted] UTC

the project maintainer approved [checkpoint] installation and observation launch. Resolve was verified
closed; transactional installation completed with administrator authentication.
All 18 payload hashes and both executable signatures verify. All 198 snapshotted
settings/native-preference/workflow files are unchanged. Receipt: audio-menu-
Local evidence and rollback details are retained privately.
Rollback to [checkpoint]: [local path omitted]
[test identifier omitted]/Restore Previous OpenGate.command.
Earlier checkpoints remain. Installed/loaded session [checkpoint]-fc88-4bf5-b66a-
[checkpoint], test process, started [test time omitted] UTC, identifies [checkpoint]. This supersedes
not-installed statements below. Audio execution remains blocked. No runtime merge
or publication; actual audio operation/Undo acceptance remains absent.

the project maintainer performed the manual submenu open/cancel. Structured capture is in that
stage's live-capture.json. Qt connection output was captured by Resolve's own
ResolveDebug.txt handler, not the redirected console; preserved as live-connections.txt.
The audio context root's aboutToHide connects to
UiTiAudioTracksScene::OnContextMenuAboutToHide(). Channel actions have QMenu's
_q_actionTriggered plus two opaque functors; at least one functor is our existing
audio observer. Incoming QAction-to-QAction links also appear, so aliasing remains
possible. Do not identify opaque functors as the engine operation from counts alone.
Generic OpenGate PanelMenus also connects aboutToShow to menus; these observers
must be accounted for before claiming a native preparation callback. The trace
does not establish OnClipMenuAboutToShow as an audio preparer. Its presence in
metadata alone remains insufficient. The native Clip menu structure has an Audio
Operations submenu, but no direct channel action in the captured top level.

The next useful experiment is direct QAction invocation after native menu closure,
scoped to one manually prepared disposable clip and exactly one permitted operation.
Local work is isolated in experiment/audio-direct-action-test, worktree
audio-direct-action-test. This is a diagnostic component gate for no-UI execution
and actual Undo, not the complete command flow. It deliberately still requires
manual context preparation for the experiment; that prerequisite is unacceptable
for the final product and silent fresh preparation remains unresolved. No new
installation or native operation is approved or performed yet. The currently active
project is a working project, so no action test may run there. All native effects
require a separately approved candidate and an exact disposable project/Timeline/
item permit, with source context matched to the actual right-clicked selected item.

### Connection-trace follow-up [checkpoint] — built/staged, not installed

The [checkpoint] capture establishes declared methods and ownership, not actual signal
receivers. Follow-up [checkpoint] extends the same focused probe using Qt's public
QObject::dumpObjectInfo diagnostic on observed menus/actions and the selected/outer
scenes. It never invokes preparation or channel actions. At most 16 objects are
dumped; anonymous functor receivers may remain opaque. No private Qt connection
structures, native handles, message-handler replacement or hidden popup is used.
Structured diagnostic IDs bracket corresponding Qt debug output. The existing
observer also records menu titles/action structure and examines Clip-labelled menu
candidates once per process, so OnClipMenuAboutToShow is not treated as an audio
entry point based on its name. Candidate identity/ownership still need real evidence.

Hidden fixture passes discovery of Clip-menu connections/structure, passive audio
menu lifetime capture, unchanged UI/ownership, zero action/preparation
invocations by the observer and all four audio paths blocked. The test confirms Qt
prints connection details, including the explicit limitation for functor callbacks.
Universal Mac build, bootstrap lifecycle, executable signatures and exact install/
rollback rehearsal pass. Windows native compilation/live capture is unverified.
Current installed build remains [checkpoint]; no new software has been installed.

Local evidence and rollback details are retained privately.
files, helper/Current metadata only, rollback to installed [checkpoint]. Existing scripts,
settings and hotkey assignments are not installation targets. A staged observation
launcher refuses while Resolve runs and captures its console to a new local file;
default Qt debug output is enabled only in that launched process's environment.
No persisted logging/preferences change or process termination. Installation of
[checkpoint] and this observation launch need exact-candidate approval and Resolve closed.
Then the same single manual audio-submenu opening/cancel will collect receiver
information the first probe could not record. No audio hotkey/operation test is
requested, and no silent audio operation or native Undo acceptance is claimed.

### Native preparation capture received — October 28, [test time omitted] UTC

Loaded probe [checkpoint] session [test identifier omitted],
test process, started [test time omitted] UTC. the project maintainer manually opened/canceled Audio Channel.
Local evidence and rollback details are retained privately.
Two QMenus are observed: channel child ID 1 owned by context root ID 2; root has
no QObject parent or associated widget. Both emit aboutToShow/aboutToHide; no
destruction is recorded through the inspected capture. This establishes observed
survival after hiding, not safe reuse for a different selected clip. Their visible
flags at these preparation/hide phases do not prove how they were presented.

Selected scene UiTiAudioTracksScene is parented to AudioTracksView. Outer scene
UiTiSequenceScene exposes OnClipMenuAboutToShow(), OnTimelineMenuAboutToShow() and
UpdateClipMenuStates(). Those are now confirmed available native methods, not
verified audio-action preparation entry points. No such method was invoked.
The source's generic QObject method inventory does not reveal signal receivers
or captured action context; no fresh-action or Undo claim is justified from names.
Observed submenu checks 4 - Channel B among MixL/MixR/Channel A/Channel B/Channel C. Record that as the
current menu baseline only; the probe did not choose it and its origin is not
established by this capture. No audio action trigger was observed in this test.

Next investigation: establish which native menu preparation callback is actually
connected and whether it rebuilds/rebinds the audio actions to current selection.
Qt's public QObject::dumpObjectInfo can report connections to debug output without
invoking them; functor connections may remain opaque. Inspect this route and debug
output capture before adding further runtime experiments. Do not cache the surviving
context actions or invoke OnClipMenuAboutToShow based only on its name.

### Read-only preparation probe installed — October 28, [test time omitted] UTC

the project maintainer explicitly approved installation of [checkpoint]. Resolve was verified closed
without agent termination. Transactional installation completed with administrator
authentication; all 18 installed payload hashes match and both executable signatures
verify. All 198 snapshotted settings/native-preference/workflow files are unchanged.
Local evidence and rollback details are retained privately.
Rollback to [checkpoint]: [local path omitted]
[test identifier omitted]/Restore Previous OpenGate.command.
This supersedes not-installed/awaiting-approval statements below. New-process probe
loading and actual native preparation/lifetime evidence remain pending. All four
audio execution paths are intentionally blocked in this diagnostic candidate;
assignment labels/settings remain. Do not request another audio hotkey operation.
Next precise observation: normal launch, select designated disposable mono clip,
manually open Audio Channel submenu once and cancel without choosing a channel.
This diagnostic gesture does not relax the no-visible-UI command requirement.
No runtime merge, release publication or audio acceptance is claimed.

### Read-only preparation probe [checkpoint] — built and staged, not installed

the project maintainer explicitly asked for active implementation/probing of a Fixed Playhead-like
silent preparation route. Isolated diagnose/audio-menu-preparation-probe, worktree
audio-menu-preparation-probe, starts from recovery baseline [checkpoint]. Commit [checkpoint]
explicitly blocks all four audio execution entry points in this diagnostic build
and removes their transaction object/caller; assignments and labels remain. It is
not a command repair and must not be mistaken for lost assignments. Existing
Stabilize/Fixed Playhead paths are unchanged. No popup automation is used by it.

Focused NativeMenuPreparationProbe owns read-only menu lifecycle/preparation
evidence: parent ownership, associated widgets, declared relevant Qt methods,
first observation, about-to-show/hide and destruction. It never emits preparation
signals, invokes native methods/actions, displays menus or changes focus/selection.
NativeAudioChannelProbe supplies matching audio menus plus the selected audio and
outer Timeline scenes' owner chains. InteractionDiagnostics forwards ActionAdded
and existing Show events; no second application listener/polling controller.
The helper shares one bounded owner-description routine rather than duplicating
per-scene scans. Inventories are capped at 64 menus and 128 lifecycle transitions;
IDs are local observation IDs, not native pointer/handle payloads. Method presence
alone is not proof of callable semantics. Earliest record is first observation,
not a claim to have intercepted construction itself. Commits [checkpoint] and [checkpoint].

Exact-source hidden preparation fixture passes: zero native action/preparation
invocations by the observer; visibility, geometry, owner and active window remain
unchanged; root/child observed once; externally stimulated lifecycle is recorded;
all four diagnostic audio paths refuse execution. Existing audio/Clip Attributes
observer regression passes. Universal arm64/x86_64 build, bootstrap lifecycle and
both executable signatures pass. Windows compilation/live behavior and all native
preparation semantics remain unverified. No complete command experience is claimed.

Local evidence and rollback details are retained privately.
files, helper bundle and Current metadata only. Workflow scripts, settings and
native preferences are not targets. Install/rollback rehearsal restores current
[checkpoint] exactly and preserves the settings sentinel. Not installed/merged/published.
The actual installation remains verified recovery [checkpoint]. Installation requires
approval for [checkpoint] and Resolve closed. Proposed precise live observation: the project maintainer
manually opens the selected disposable clip's Audio Channel submenu once, then
cancels without choosing a channel. This collects native ownership/method/lifetime
evidence missing from older captures; it is investigation, not acceptance of visible
menus during command execution. No audio hotkey test is requested for this probe.

### Clarification: silent native menu preparation is a valid research direction

the project maintainer correctly compares Audio Channel to the accepted Fixed Playhead mechanism.
Source verification: Fixed Playhead invokes the native toolbar button's
MenuAboutToShow preparation signal, resolves its actual Fixed Playhead action and
triggers it once; it never calls popup/exec/show or changes focus. Internal menu
construction/preparation is distinct from displaying then concealing a popup.
The no-visible-UI contract does not rule out this preparation/action pattern.
Investigate a fresh selected-clip audio equivalent with verified action ownership,
selection/instance scope and Undo. No such equivalent is established yet. The
crashing popup-navigation implementation neither satisfies that pattern nor proves
it impossible. Do not use stale cached clip-context actions as a substitute for
fresh preparation, or infer a callable native operation from method names alone.

### Recovery completed — October 28, [test time omitted] UTC

After Resolve closed, the approved checkpoint restored [checkpoint] and removed the
crashing [checkpoint] runtime. All 22 restored payload hashes match the preserved
[checkpoint] stage; both executable signatures verify. Recovery receipt now records
Local evidence and rollback details are retained privately.
This supersedes the pending/refused recovery below. Resolve was never terminated
by the agent. Earlier checkpoints and crash evidence remain preserved.

Of 198 files in the earlier recovery baseline, keyboard/settings/workflow hashes
match except two Resolve-owned files (.wipluginslist.conf and UI.preset) changed
during the interval containing the reopened session and shutdown. The recovery
transaction does not target those files; their precise write timing was not
captured. Preserve them rather than restoring old user state or claiming every
native preference stayed byte-identical through that session.

Offline no-UI investigation: supplied 21.1.1 declarations expose the mapping
setter but no Timeline transaction/Undo API; Fusion composition Undo is a different
scope and must not be substituted. Static Qt metadata for UiTiAudioTracksScene and
UiTiTracksScene identifies no channel-apply slot. Exported symbols inspected do
not expose a usable native channel operation. Earlier menu inventories show
unnamed channel actions with no scalar data, unlike a stable global command ID.
This does not prove a no-UI operation is impossible, but none has been established.
No further popup automation candidate or hidden-dialog workaround is authorized
by the clarified experience. Audio commands remain unaccepted; next required work
is establishing native action preparation/execution without presentation and with
demonstrated Undo before proposing another live audio candidate.

### [checkpoint] crashed Resolve; menu-presentation approach rejected

the project maintainer reports pressing U crashes the project. Session [checkpoint]-6ad5-48ce-ac22-
[checkpoint] loads [checkpoint] (test process). Its trace maps U to edit.audio.next-channel
(Skip Excluded Channels), resolves the selected native audio-tracks item and invokes
the read-only planner. ResolveDebug's fatal Signal 11 stack identifies
NativeAudioChannelTransaction's queued callback calling QMenu::setActiveAction,
then Qt internalDelayedPopup. This is an OpenGate-triggered Resolve process crash,
not an assignment or user-selection error. No native channel-choice completion or
Undo acceptance is established. Preserve crash evidence under native-audio-track-
Local evidence and rollback details are retained privately.

**Explicit clarified acceptance requirement:** all four commands must happen with
no menu or dialog presented, no other panel opened and Timeline focus retained,
like Stabilize. A briefly presented/hidden popup is not acceptable. Retire the
menu-opening transaction approach from the production proposal; do not patch it
by concealing windows or keeping hidden popup controllers. Preserve the experiment
and crash receipts in isolation; do not merge it. Investigate an actual native
action/operation with demonstrated instance scope, repaint and native Undo.

Direction semantics remain numerical, independent of hotkey letters: All Source
Next 3→4, Previous 3→2; wrap at source ends. Skip Excluded uses the eligible list:
for configured 3/4/5, Next 3→4 and Previous 3→5. U currently invokes Next Skip
Excluded in the captured trace; no reversal or channel-change result is proved.

Recovery to previously approved [checkpoint] was prepared after an initial process check
found Resolve closed. The first administrator command stopped at a quoting error;
the corrected rollback script then refused because Resolve had reopened. No files
were restored. [checkpoint] therefore remains installed until Resolve is closed and the
checkpoint can be restored. Recovery receipt explicitly records restored=false;
all 198 snapshotted settings/workflow files remain unchanged. the project maintainer was asked to
save/quit when convenient and avoid audio commands meanwhile. Never terminate it.
[checkpoint] is only a recovery baseline for existing functions; its audio commands failed
live and are not accepted. Do not call the four-command task fixed or ready for
ordinary use. Continue independent offline investigation while recovery is pending.

### Installed scene-dispatch correction — October 28, [test time omitted] UTC

the project maintainer requested installation of [checkpoint]. Resolve was verified closed without
termination by the agent. Transactional installation completed with administrator
authentication. All 22 payload hashes match; both executable signatures verify.
All 198 snapshotted settings/native-preference/workflow files are byte-identical.
Local evidence and rollback details are retained privately.
Rollback to [checkpoint]: [local path omitted]
[test identifier omitted]/Restore Previous OpenGate.command.
Earlier checkpoints remain preserved. This supersedes installed-[checkpoint] and
unapproved/not-installed [checkpoint] statements below. New-process loading and actual
native scene menu/channel/audio/repaint/Undo remain unverified. No runtime merge
or publication. Next: normal launch, designated disposable selected mono polywav,
one Next Audio Channel — All Source Channels invocation, inspect result/phase
trace, then native Undo only if an actual channel change occurred. Preserve all
other explicit limitations and open checklist requirements.

### [checkpoint] live failure and native scene-dispatch correction

the project maintainer reports no command effect while the designated clip remains selected.
Loaded session [test identifier omitted], test process, started
October 28 [test time omitted] UTC, identifies [checkpoint]. Native preset invocations are accepted;
nine contained scenes are inspected and the selected item resolves in
UiTiAudioTracksScene. This confirms the contained-scene selection correction in
real Resolve. Both observed Next command scopes invoke enabled workflow planner
actions. Attempts then time out or cancel on intervening input/deactivation.
No native-choice completion is recorded. Do not blame selection or assignment.
Local evidence and rollback details are retained privately.
The plan job and result directories are empty after inspection, consistent with
plan consumption; old tracing does not independently establish the timeout phase.

Source review identifies a concrete dispatch defect: the operation sends the
context event directly to the selected QGraphicsItem, bypassing the owning scene's
handler. Qt's QGraphicsView::contextMenuEvent source instead sets the originating
viewport, scene/screen positions, modifiers/reason and sends the event to its scene.
Correction [checkpoint] uses that scene-delivery boundary and includes the viewport.
It also records plan receipt/context delivery and distinguishes plan/menu timeouts.
Same transaction responsibility; no new controller, native slot invocation, state
writer, cursor movement, API setter or fallback. All four commands retain the
shared planner/native transaction and their existing assignments.

The original fixture implemented menus on items and did not catch the scene-owned
case. Corrected fixture handles context at the actual Qt scene event boundary and
passes all four commands/mode switches, wraps, nested selection, ambiguity and
cancellation with the originating viewport supplied. An attempted test-only scene
subclass crashed during paint dispatch into its context handler; it was removed
in favor of an event-filter handler on a real QGraphicsScene. Resolve was untouched.
This fixture failure and correction are not a Resolve crash or runtime acceptance.
Actual native scene handler/menu execution, audible result and Undo remain unverified.
Installed [checkpoint] stays unchanged; no new installation approval has been given.

[checkpoint] universal arm64/x86_64 runtime compilation, both executable signature
checks and bootstrap lifecycle fixture pass. Staged separately at
Local evidence and rollback details are retained privately.
installation receipt/payload. Twenty-two hashed files across six transaction
targets; four audio scripts retain the same content as installed [checkpoint].
Transactional rehearsal passes exact baseline restoration/settings preservation.
Rollback baseline is installed [checkpoint]. Not installed, live-accepted, merged or
published. Exact-candidate approval and Resolve closed are the next installation
gate; native execution/audio/Undo remain the next actual acceptance gate.

### Installed four-command candidate — October 28, [test time omitted] UTC (October 27 local)

the project maintainer explicitly requested installation of [checkpoint]. Resolve was verified closed;
the agent did not terminate it. Transactional installation completed with macOS
administrator authentication. All 22 payload hashes match and both executable
signatures verify. Of 198 snapshotted settings/native-preference/workflow files,
194 remain byte-identical; exactly the four intended audio workflow scripts changed.
Local evidence and rollback details are retained privately.
installation-receipt.json.

Rollback to [checkpoint]: [local path omitted]
[test identifier omitted]/Restore Previous OpenGate.command.
Earlier rollback checkpoints are retained. This supersedes awaiting-approval and
installed-[checkpoint] statements below. New-process loading and live four-command
acceptance remain unverified; no runtime merge/publication. Next: reopen the
disposable project, select the designated mono polywav and invoke Next Audio
Channel — All Source Channels once. Confirm actual change before one native Undo;
if rejected, capture the exact scene/planner reason instead of repeated blind input.
Only after that gate passes expand to all four directions/scopes and wraparound.
The explicit limitations and other open tasks below remain in force.

### Four-command native candidate [checkpoint] — built/staged, awaiting approval

Independent follow-up while approval is pending: the installed CodecPlugin SDK
README describes supported Resolve Studio versions. This supports investigating
Free plugin discovery first, but does not establish the upstream maintainer’s failure cause.
The published Mac bootstrap's pluginInit calls the shared initializer, which
schedules startup immediately after a valid host handshake; msgPluginStart also
schedules the same idempotent path. Thus a missing msgPluginStart alone cannot
explain failure if pluginInit completed successfully. Separate never-discovered/
rejected handshake from bootstrap load failure and initialized-but-hidden UI using
actual Free logs before designing a fallback. No Free acceptance or fallback added.
Current [checkpoint] session's captured tab hide at [test time omitted] recovers in 291 ms; no
persistent toolbar-loss cause is established by it. Existing toolbar bug stays open.

the project maintainer requires continued execution toward one live-test candidate for all four
commands, rather than stopping at each component or proposing another all-source-
only install. Branch fix/native-audio-track-selection now has clean [checkpoint];
installed/loaded runtime remains failed candidate [checkpoint]. No installation or runtime
merge occurred during this follow-up. This supersedes the [checkpoint] staging proposal.

All four catalog commands now share NativeAudioChannelTransaction with direction
and scope parameters. It captures the contained-scene selection, requests a fresh
read-only source plan, retains selection guards throughout planning, then validates
the fresh native menu's checked channel/channel count against the plan before
activating the planned row through native QMenu keyboard handling. All Source and
Skip Excluded use their current mapping on every invocation; exclusions are read
from the source configuration and never changed. No mapping setter is called.

Boundaries: audio_channel_plan.py owns source/instance eligibility and target
planning; audio_channel_native_wrapper.py transports plans/registry requests;
AudioChannelPlanRequest owns correlated file IO and invokes the existing matching
workflow action; NativeTimelineSelection resolves the contained Qt scene;
NativeAudioChannelTransaction owns the native operation and selection/input
cancellation. NativeEditCommands wires all four entries to that transaction.
The replaced API executor audio_channel.py and both production API execution paths
were removed. The behavior-preserving planning extraction is separately committed
as [checkpoint] before native integration. No extra shortcut listener or visibility writer.

Workflow menu invocation first enters the existing registry; Qt then invokes the
matching wrapper in planning mode. The second phase performs reads only. Only the
native selected-item menu can commit a change. Correlation tokens, two-second
deadline, mapping/menu comparison and selection-change rejection prevent reuse of
an old plan. Canceled planning tickets suppress a late script from replaying a
command. Remaining transport quirk: a canceled ticket can consume a later direct
workflow-menu invocation; a fresh registered hotkey request replaces the ticket.
This needs lifecycle refinement and live characterization; do not conceal it.
Planning still depends on Resolve's workflow action being enabled. The earlier
disabled-action/ordinary-launch failure therefore remains relevant and unclosed.

Current supported test scope is one selected native mono audio item with an
available original file. No/multiple selection, linked AV selection and unsupported
mapping cases fail closed. Independent multi-clip native cycling remains required;
the planner's independent multi-item results are not an implemented batch executor.
No claim of complete four-command acceptance or a fixed overall product.

Verification for [checkpoint]: exact-source native registry/planning/menu fixture passes
all four commands and mode switches, both directions/wrap/repetition, nested scene
selection, other-item isolation, selection retention, cross-scene ambiguity and
cancellation. Nine read-only operation tests pass. Generated Mac/Windows wrapper
tests pass with 33 registry targets plus Stabilize; all four audio plans perform
zero media writes and canceled planning does not enqueue a retry. Existing native
Edit and native-preset/context regression fixtures pass. Universal arm64/x86_64
runtime, bootstrap lifecycle and both executable signature checks pass. Windows
native compilation/live acceptance, real Resolve contained-scene selection, actual
audible result, repaint, instance isolation and native Undo remain unverified.

Local evidence and rollback details are retained privately.
files, helper/Current metadata plus four audio scripts (six transaction targets).
Install/rollback filesystem rehearsal restores the current baseline exactly and
preserves the settings sentinel. Candidate [checkpoint] approval has been requested;
do not install until approval and a fresh closed-Resolve check. Preserve rollback
to current [checkpoint] and the earlier rollback to [checkpoint]. Published assets unchanged.
First live gate remains one native change on the designated disposable mono item,
then one Undo; expand to the four-command matrix only if that succeeds. If native
contained scenes still expose no selected item, stop that route and record the
remaining selection-adapter failure rather than guessing clip coordinates.

### [checkpoint] live test failed — native selection assumption disproved

Bounded follow-up branch fix/native-audio-track-selection, [checkpoint], extracts
NativeTimelineSelection from the menu transaction. It follows visible QWidget
children and actual QGraphicsProxyWidget ownership within the focused Timeline,
deduplicates scenes, and requires one selected item across the contained scenes.
It does not decode private handles or infer selection from pointer position.
The transaction watches all inspected scenes and revalidates the complete selection
before dispatch; menu placement uses the selected item's own view coordinates.
Old outer-scene-only resolution and its single-scene observer were replaced.
Traversal is bounded at 64 views/4096 items and fails closed at those limits.
The existing production registry/menu fixture passes nested proxy selection with
an empty outer selection, nested native commit, ambiguity across scenes and
cancellation when selection changes in a different contained scene, alongside
the prior direction/wrap/repetition cases. Real Resolve nested-scene selection
remains unverified; this does not yet establish a repair. Not installed or merged.
Universal arm64/x86_64 runtime compilation, both executable signature checks and
I/O bootstrap lifecycle fixture pass for clean [checkpoint]. Windows compilation/live
behavior remain unverified. A live test requires a separately approved candidate;
the installed failed [checkpoint] remains unchanged during this local work.

New session [test identifier omitted] (test process, started
[test time omitted] UTC) loads [checkpoint]. the project maintainer reports the channel command does not work.
Both resolve-preset-input and resolve-workflow-shortcut reach the command registry.
At each observed attempt the focused UiTiSequenceView / UiTiSequenceScene reports
zero QGraphicsScene selectedItems, so the transaction rejects before sending any
native menu event. This is a failed selection adapter, not a lost assignment or
evidence that the project maintainer selected incorrectly. Do not request re-selection as a repair.

Read-only scripting inspection independently confirms the designated disposable test timeline
item [test identifier omitted] selected alone, audio track 1, mono source
channel 3 (Channel A), five embedded channels. Standard Qt scene selection is therefore
not equivalent to Resolve Timeline selection at the inspected outer scene. Saved
Local evidence and rollback details are retained privately.
No native audio change or Undo acceptance occurred; no Undo is required for these
rejected attempts. Keep this experimental runtime unmerged.

Offline Qt metadata identifies separate UiTiTracksView / UiTiTracksScene classes
and native controller SelectedItemsChanged signals with Resolve-specific handles.
This suggests a nested-scene/custom-model boundary, but does not prove where the
selected clip is represented or authorize dereferencing private handles. The next
implementation must establish that association rather than scanning arbitrary
items, guessing a screen position, or dropping the single-selection guard. Existing
mono Clip Attributes control evidence remains available as a separate native route.

### Installed native execution candidate — October 27, [test time omitted] UTC

the project maintainer approved [checkpoint] specifically. Resolve was verified closed (not terminated
by the agent), then the staged transactional installer completed with administrator
authentication. All 20 installed payload hashes match. Of 198 snapshotted settings,
native preferences and workflow files, 196 remain byte-identical; only the two
intended All Source Channels workflow scripts changed. No unexpected changes.
Both installed executable signatures verify. Receipt: native-audio-menu-transaction/
build/native-menu-test/installation-receipt.json.

Rollback to [checkpoint]: [local path omitted]
[test identifier omitted]/Restore Previous OpenGate.command.
This supersedes the staged/not-installed and installed-[checkpoint] status below.
New-process loading, actual native audio output/repaint and Undo are not yet
verified. No runtime merge or publication. Next: reopen the disposable project,
select the designated mono polywav, invoke Next Audio Channel — All Source Channels
once, and verify the result before attempting one native Undo. Skip Excluded
Channels remains the older unaccepted API implementation; toolbar/stacked-tab and
Stabilize availability bugs remain unresolved.

### Continued read-only investigation after staging [checkpoint]

Loaded session [checkpoint] still identifies [checkpoint]. Deduplicated current-session
logs contain 326 Timeline presentation transitions through the available capture
(last session event [test time omitted] UTC). Three post-startup tab hides at [test time omitted],
[test time omitted] and [test time omitted] show timelineTabsHolder itself not explicitly hidden;
its native frameHeaderContainer is hidden. Each is followed by a Show within
194–239 ms. These transient recoveries do not reproduce or explain the maintainer’s
persistent toolbar/stacked-tabs failure. No late toolbar Hide is present in this
capture. Receipt: build/timeline-visibility-[checkpoint]/current-session-transitions.json.
Do not add a force-show loop or reset stacked-timeline preferences based on this.

Source ownership review: TimelineToolbarViewport leases only frameEditToolbarTop
through NativePanelPosition and PanelOverflow; it does not own timelineTabsHolder
or frameHeaderContainer. NativePanelPosition restores its captured visibility and
layout slot. The toolbar height is captured at attachment; this remains a geometry
investigation point, not an established cause. Next persistent recurrence must
distinguish native header visibility, toolbar content/overflow/slot visibility,
and the Timeline ancestor before any reset. No toolbar repair is claimed.

Reveal-file API investigation (Resolve 21.1.1 supplied scripting declarations):
Timeline.GetSelectedClips and MediaPool.GetSelectedClips expose separate selections;
TimelineItem.GetMediaPoolItem provides the source without navigating to Media Pool.
ClipProperties documents File Path as source path and Proxy Media Path separately.
These APIs do not establish which selection context owns a command invocation.
The implementation must obtain explicit context from existing command/focus routing,
reject ambiguous context, then read a single selected item's source. Do not prefer
whichever selection list is nonempty, replay Find, or silently substitute a proxy
when the original is offline. Compound/multicam, active takes, synced audio and
image sequences still need path-semantics investigation. No reveal command or OS
adapter is implemented by this inspection; no project or file-browser action ran.

### Native audio execution implementation — isolated, not installed

Branch feature/native-audio-menu-transaction ([checkpoint], worktree
native-audio-menu-transaction) replaces the two All Source Channels production
callers' API execution path with NativeAudioChannelTransaction. The focused shared
Qt component addresses one native-selected QGraphicsItem in the focused Timeline,
delivers a keyboard-reason context-menu event directly to that item, recognizes its
fresh Audio Channel submenu, and activates the next/previous native menu row through
QMenu keyboard handling. This supports native menu exec()-return handlers as well
as action-signal handlers. It never calls the mapping setter or force-enables an
action; it does not move the cursor or select another item. Actual Resolve support
for this QGraphicsScene selection/context-event route is still unverified. It must
fail closed when the native selection representation or menu is unsupported.

This is explicitly an intermediate single-item All Source Channels implementation.
It can briefly present the native menu; final presentation is not accepted yet.
Multiple native selections, dialogs/popups, unavailable focus/scene/items and
unsupported menus are refused. A changed selection or intervening input cancels
pending work; native menu choices are not cached across requests. A two-second
deadline bounds a failed attempt. Reporting distinguishes a delivered menu key
from verified mapping, audio, selection preservation and Undo. Mono/stereo/adaptive,
linked-selection and offline/native-menu semantics still require actual Resolve
verification; synthetic menu recognition does not establish those behaviors.

All four catalog labels remain. All-source workflow scripts now post through the
existing command bridge, removing their API setter invocation. Skip Excluded
Channels still uses the unaccepted experimental API route and is not an Undo fix;
native exclusion handling and independent multi-clip cycling remain required.
No new shortcut listener, mapping state owner or OS-specific selection code was
added. The transaction's temporary event observer handles only its pending native
operation; existing catalog, bridge, menus and platform callers remain shared.

Verification: production registry-to-selected-item-to-menu fixture passes both
directions/wrap, eight separate exec()-return commits, no other-item changes,
selection retention, no/multiple selection, single-channel/unsupported rejection,
and cancellation on selection change. Existing native Edit, preset-conflict and
four-command menu fixtures pass. Generated Mac and Windows wrappers pass with 31
bridge requests and three remaining API scripts. The native transaction code has
compiled universal x86_64/arm64 with bootstrap/signature checks, including final
[checkpoint]. Its exact-source native transaction fixture passes. Windows native build/live acceptance, actual
Resolve dispatch/Undo/audible result and complete user flow remain unverified.

Separate refactor/audio-channel-planning, [checkpoint], extracts selection/policy into
audio_channel_plan.py and keeps the experimental API executor in audio_channel.py.
Production executor and generated wrappers use that extracted logic; the old inline
planning path was removed. A read-only serializable native-edit plan includes project/
Timeline/item identities and per-item before/after channels; planning performs zero
writes and grants no authority to execute against a later selection. Eleven operation
tests plus both platform wrapper tests pass. This refactor is not installed or merged
as runtime, and does not establish native transaction integration by itself.

Next live gate, after exact-candidate approval: one All Source Channels invocation
on the designated disposable disposable test timeline mono polywav, confirm actual channel and
paint, then one native Undo. Stop if no supported native selection/menu or if Undo
does not restore that channel. Preserve [checkpoint] as the installed rollback baseline.

Local evidence and rollback details are retained privately.
[checkpoint]. Twenty hashed files across four transaction
targets: helper bundle, Current metadata, and only the two All Source Channels
workflow scripts. Existing Skip Excluded/Stabilize scripts and settings are not
installation targets. Both staged executable signatures verify. The staged installer
and rollback pass a filesystem rehearsal with exact baseline restoration and an
unchanged settings sentinel. Not installed, live-accepted, merged as runtime, or
published. Installation requires approval specific to this new execution candidate;
the earlier [checkpoint] approval does not cover it. Do not test it on working clips.

### Installed diagnostic checkpoint — October 27, [test time omitted] UTC

**Live native-control evidence at [test time omitted] UTC:** the project maintainer opened Clip Attributes from
the Timeline, explicitly not Media Pool. Loaded session [checkpoint] reports [checkpoint].
The native dialog is UiClipAttributesDialogImp / UiClipAttributesDialog. Its
m_TracksConfigWidget is a six-column QTreeWidget with one row (Audio 1, Mono).
The source dropdown is BmdDavUI::UiStyledComboBox, current index 3, with entries
<none>, 1 - MixL, 2 - MixR, 3 - Channel A, 4 - Channel B, 5 - Channel C. The contemporaneous
native Audio Channel submenu checks 3 - Channel A. General dialog capture is truncated
at 192 controls; do not treat it as a complete inventory or proof of dialog commit.
Local evidence and rollback details are retained privately.
timeline-attributes.json. No automated channel write or dialog acceptance occurred.

Separate diagnostic branch [checkpoint] adds a focused NativeAudioChannelEditor resolver,
used by the production Clip Attributes observer. It validates the source-channel
table cell rather than selecting the first combo (which controls track format).
Fixture passes mono identification, non-mono/multiple-output/unconnected/disabled/
malformed rejection and unchanged selection. Native table itemWidget association
still requires live verification; the generic inventory did not record that edge.
This is not native dispatch or Undo acceptance and is not installed. Verified
21.1.1 Resolve scripting stubs expose the mapping setter but no Timeline undo/redo,
Clip Attributes invocation, or command-execution entry matching this investigation.
Native dialog entry/commit and selection identity remain unresolved integration work.

the project maintainer explicitly approved candidate [checkpoint] and requested installation. Process
check confirmed Resolve closed; it was not terminated by the agent. Installed clean
[checkpoint] from integration/audio-native-route-inspection
using the staged transactional installer after administrator authentication.
All 18 payload files match; both executable signatures verify. All 198 snapshotted
OpenGate/native preference and workflow-script files are byte-identical after
installation. Only helper bundle and Current metadata were installation targets.
Staged install/rollback fixture passed before installation.

Rollback: [local path omitted]
[test identifier omitted]/Restore Previous OpenGate.command restores [checkpoint].
Local evidence and rollback details are retained privately.
Published 0.0.3 assets and runtime remain unchanged. This supersedes installed-[checkpoint]
statements below. New-process loading is verified by session [checkpoint]; the native
control capture above does not establish audio execution or complete live acceptance.

Candidate contains the narrow Timeline/Fusion shortcut conflict correction, fresh
native audio menu interpretation, Clip Attributes schema capture, workflow enabled/
disabled and menu-preparation capture, clearer dispatch failure reasons, and late
Timeline toolbar/tab transition capture. It does not fix native audio Undo/repaint,
Stabilize availability lifecycle, or missing toolbar/tabs. No runtime merge to main.
The normal launch and Timeline Clip Attributes inspection are complete. The cold
Stabilize test was not obtained before visiting native UI; do not ask the project maintainer to
restart merely to repeat it. Its action was observed enabled in this new session,
which does not close the earlier disabled-action failure. Next audio test is the
separately approved [checkpoint] gate above. No layout reset or stacked-timeline toggle
is required.

This section supersedes historical installed/pending statements below, without
removing their evidence. The whole product contract remains in scope. Maintain
this checklist throughout the work; a progress message is not a stopping point.

### Selected-instance comparison after restart — October 27

Read-only inspection after the project maintainer reselected the disposable test clip confirms
project [test identifier omitted], disposable test timeline
([test identifier omitted]), and selected item
[test identifier omitted]. Its stored mapping is mono source channel 1
(MixL); source configuration remains channels 3, 4, 5. Comparing all [baseline count] audio
instances with the pre-command baseline finds exactly one mapping difference:
this designated item. No added/removed instances or Media Pool mapping changes.
This is mapping-isolation evidence for the observed invocations, not audible,
paint, repeated-wrap, or Undo acceptance. Receipts in audio-undo-test's ignored
build/undo-test/live-after-restart.json and live-isolation-comparison.json.

Native Shift-Command-C opens the selected Timeline item's Clip Attributes without
Media Pool navigation. Its Audio table exposes one row/six cells through macOS
accessibility. Source-channel cell advertises AXPress but neither that action nor
an inspected-arrow click exposed a readable channel editor in the AX tree. Canceled
without applying changes. Do not implement coordinate-based production automation
or assume the API setter supplies native undo/repaint from this inspection.

Separate fix/native-timeline-shortcut-context branch extracts native preset owner
context classification from PanelCommandCatalog and tests a narrow exception for
the observed Fusion reverse-playback conflict when native Timeline has focus.
Unknown owners, text fields and genuine Edit conflicts must remain protected.
This is independent of the mapping/repaint/Undo failure; no replacement runtime
has been installed. Runtime [checkpoint] remains the installed diagnostic candidate.

### Offline command repair checkpoint — October 27, while the project maintainer edits

**New live failure: Stabilize does not fire ([test time omitted]–[test time omitted] UTC).** Installed [checkpoint]
accepts the maintainer’s shortcut as edit.inspector.stabilize from Timeline and main-window
contexts, then rejects native workflow dispatch. Logs report exactly one matching
action; Inspector diagnostic inventory independently shows workspaceWIPlugins_0,
OpenGate - Edit - Stabilize, disabled. This is not evidence of a lost assignment
or ambiguous action. Installed Stabilize script exists with the expected route
marker and remains the October 26 file; the same enabled-action guard existed in
published [checkpoint]. No proof yet of why Resolve disabled it, or whether the state is
stale. ResolveDebug also records a native stabilization attempt at [test time omitted] UTC
with reported frame size 0 x 0; do not infer a media fault from that log alone.
No Stabilize invocation, selection mutation, forced enabling, or Inspector opening
was performed during this read-only investigation. Track this regression alongside
the audio command; do not claim accepted Stabilize behavior is currently preserved.

**Recovery reported by the project maintainer:** clicking the native Workflow Integrations command
made Stabilize work again; this is an unacceptable prerequisite, not user error.
Logs corroborate a script result at [test time omitted] UTC (no selected video), followed
by an accepted hotkey at [test time omitted], native workflow invocation at .605, and
successful timeline-item stabilization completion at .906. Assignment survived.
Opening/preparing the menu versus executing its action has not yet been isolated
as the cause of recovery. Investigate stale native action availability and menu
initialization; do not force-enable the QAction or replay the operation.

The existing WorkflowCommandMenu observer connects QAction::changed but its
deduplication signature omitted enabled/visible state, concealing these transitions.
Branch diagnose/workflow-action-availability adds those fields to the existing
observer/signature with no new listener, state writer, controller, or dispatch path.
Acceptance for the eventual repair must include first hotkey after ordinary launch
without visiting Workflow Integrations, continued operation across page/project and
selection changes, disabled/busy cases, and exactly one selected-clip operation.
The shared route also serves the four audio commands, so test both callers. Current
recovery does not close the startup/availability bug or prove why the state changed.

**Command-tree clarity requirement:** the project maintainer objects to every assignable command
cluttering Workflow Integrations. Investigate separation of native shortcut
registration from reliable execution and visible menu presentation. Preserve native
Keyboard Customization, existing assignments and single dispatch. Do not claim
entries are unavoidable or hide/reparent them without proving those contracts.

Offline continuation adds bounded before-menu/next-event-turn snapshots to the
existing WorkflowCommandMenu observer ([checkpoint], fixture passed), alongside enabled/
visible transitions ([checkpoint]). No signal is emitted into Resolve, menu opened, or
action enabled by the observer. NativeWorkflowOperation now distinguishes missing,
ambiguous and disabled actions; native-edit-command fixture passes with unchanged
refusal behavior. Combined source [checkpoint] on integration/audio-native-route-inspection
includes these changes and the late Timeline capture correction. Universal Mac
helper compilation passed. This remains an isolated inspection candidate, not a
native audio Undo/repaint fix, Stabilize lifecycle repair or Timeline repair.
Installed runtime and published release are unchanged.

the project maintainer is working in Resolve; no further live input, installation, or project
changes are authorized implicitly by this offline continuation. The next native
inspection must wait for an agreed break and candidate-specific installation.

- Shortcut branch: [checkpoint] extracts existing native owner classification from
  PanelCommandCatalog; [checkpoint] recognizes only the observed Fusion reverse-playback
  owner as inactive while the native m_TiTimelineView QGraphicsView has focus.
  Text editing, unknown Fusion owners, other focus and real Edit conflicts remain
  protected. Shared code has one existing input filter and preset reader. Mac
  native-preset fixture passes; Windows compilation/live behavior is unverified.
- Native-route branch: [checkpoint] adds NativeClipAttributesProbe, a read-only bounded
  schema observer (16 captures, at most 192 controls, 16 rows/8 columns per model,
  64 combo entries). It forwards existing Show events, coalesces per window, checks
  lifetime and visibility next turn, and never sets a model value or invokes a
  native action. Production coordinator growth is four wiring lines. Fixture
  checks real production probe output, coalescing, destruction, hidden windows,
  and unchanged model/selection. Passing does not establish native schema support.
- Offline Resolve Qt metadata identifies UiClipAttributesViewImp and
  UiClipAttributesController. No private slot or opaque native handle was invoked;
  names alone do not establish a usable native channel operation.
- Combined source: integration/audio-native-route-inspection, based on installed
  [checkpoint], retains all four assignable commands and Timeline observer; includes
  fresh native-menu choice validation plus both changes above. Merge conflicts
  were resolved retaining both observers and both fixture sections. Universal
  x86_64/arm64 helper compilation and signature verification pass; combined native
  preset and audio-probe fixtures pass. No runtime merge to main, installation,
  live acceptance, or publication. This is an inspection candidate, not an Undo
  repair. The API mapping setter is unchanged and remains unaccepted.

Next precise evidence needed: native Clip Attributes widget/model inventory from
the designated Timeline test clip, then prove an actual native channel action
with timeline repaint, audible output and one Undo before expanding acceptance.
Do not manufacture a repaint, invoke private methods inferred from their names,
or treat successful API readback as completion. Toolbar/stacked-Timeline repair
and the rest of the checklist remain open.

### Recurring Timeline toolbar loss — October 27, current investigation

**Recurrence reported around [test time omitted] local time:** the project maintainer again reports both the
Timeline toolbar and stacked tabs missing. Read-only evidence saved under
Local evidence and rollback details are retained privately.
logs, active-session receipt, OpenGate JSON profiles and native config.user.xml.
Running receipt remains [checkpoint]/session [checkpoint]. Saved workspace marks Timeline
hidden=false; toolbar records floating=false/scale=1 with no explicit hidden flag.
This does not identify the native setting or presentation state. No reset, toggle,
layout write or project change was performed to collect evidence.

the project maintainer supplied visual evidence: Desktop/Screenshot 2026-10-27 at 5.38.42 PM.png
(actual filename uses a narrow no-break space before PM). The image shows Timeline
tracks/ruler present while the native editing toolbar and stacked timeline tabs
are absent. This establishes the visible failure, not its cause. the project maintainer is exporting;
do not interrupt export or perform live input. After export, an agreed testing break
can be used for the specifically identified combined diagnostic candidate [checkpoint];
installation still requires approval and the project maintainer saving/quitting Resolve. No automatic
layout reset or toggling Display Stacked Timelines as part of installation/testing.

Installed TimelinePresentationProbe exhausted its 256-event lifetime limit much
earlier in the session and missed the late transition. Previous log scans also
omitted rotations named events.jsonl.1 etc.; this capture includes them. Do not claim
the old probe captured this failure. Separate diagnostic commit [checkpoint] caps geometry
events independently and preserves event-driven Show/Hide/ParentChange capture
through long sessions using the existing rotating log. It includes timelineTabsHolder
and frameEditTimelineAndAudioView. The production-path fixture passes late toolbar/
tab Hide capture after 300 resize events and retains no-mutation/lifetime checks.
This fixes missing evidence, not the toolbar/stacked-timeline presentation bug.

the project maintainer reports the Timeline panel repeatedly loses its native toolbar. The toolbar
is part of the Timeline panel, not an independently optional panel. The reset used
to recover it then requires re-enabling **Display Stacked Timelines**. This clarifies
the earlier recovered-toolbar observation: recovery did not fix the recurring bug.
Do not require the project maintainer to distinguish widget visibility from checked-state mechanics.
The specific reset entry point and causal state writer are not yet established.

Investigate both incomplete Timeline presentation and loss of native preferences
through recovery. Preserve the complete toolbar/timeline-tabs contract (items 9–10),
stacked-timeline choice, native rendering and current arrangement across page return,
project/timeline changes and managed/native transitions. No periodic force-show,
replayed toggle, automatic reset or separate visibility controller is an accepted fix.
The audio diagnostic installation proposal is on hold while this report is assessed;
audio undo work and the broader queue remain open. No new installation performed.

Read-only source findings: TimelineToolbarViewport leases the native toolbar's
layout position and reparents it into PanelOverflow; it captures height once on
attachment. NativePanelPosition owns restoration of the original slot/visibility.
PanelControls still contains legacy attached-toolbar reveal routing, including an
unreachable branch after requestUndock's toolbar early return. These are investigation
boundaries, not proof of cause. Existing timeline-overflow fixture covers a fixed-height
synthetic toolbar and dock restoration, not Resolve's changing native timeline modes.
Do not claim that fixture verifies this reported experience.

Logs and profile files preserved read-only in main-release's ignored
`[local evidence reference omitted]`; latest captured event is October 27 [test time omitted] UTC,
runtime [checkpoint]. Current report is user evidence; those older events do not establish
the exact failure transition. No live project mutation, settings change or production
repair made during this investigation. Next: trace the toolbar ownership/geometry
transition and the reset path before choosing a bounded production correction.

### Separate branches and combined diagnostic candidate — October 27

the project maintainer requests concurrent audio-command and Timeline repair work on separate
branches, combined into one installation for live testing when ready. Refactor at
responsibility boundaries if needed; do not inflate coordinators. This supersedes
the temporary hold on combined diagnostic preparation, not the installation gate.

- Audio diagnostic: diagnose/native-audio-channel, [checkpoint]. The failed external-API
  audio implementation remains isolated on feature/selected-audio-channel; no native
  undo workaround has been accepted or included in this diagnostic candidate.
- Timeline diagnostic: diagnose/timeline-toolbar, [checkpoint]. Focused 51-line
  TimelinePresentationProbe records bounded before-handler/next-turn ownership,
  visibility and geometry evidence. It does not show/hide/reparent, change a setting,
  poll, or install an input filter. InteractionDiagnostics forwards existing events.
- Integration: integration/audio-timeline-diagnostics, clean [checkpoint], worktree
  audio-timeline-integration. Includes both diagnostics on current main runtime.
  Wiring conflicts were resolved by retaining each focused observer; the coordinator
  diff is 12 lines. No duplicate state owner or presentation implementation added.
- Verification: both focused production-observer fixtures pass; combined hidden
  interaction audit passes (hidden/dynamic/Fusion). Sandbox window-service failure
  on the first audit was rerun with permission and passed. Universal arm64/x86_64
  helper compiled; signature verification and I/O bootstrap fixture passed. Windows
  shares the observer caller but has not been compiled or live-tested here.
- Prepared ignored build/macos/combined-diagnostic-install: candidate hashes,
  installed-bundle/Current metadata snapshot, and established transactional install
  script with Resolve-running refusal and fresh checkpoint/rollback. Existing
  installation/settings/hotkeys are unchanged. Not installed, live-accepted, merged
  to main as runtime, or published; this is a diagnostic candidate, not a repair.
- Next gate: candidate-specific installation approval and user saving/quitting
  Resolve. Then capture the native audio submenu and a naturally occurring Timeline
  presentation failure. Do not reset the layout to manufacture recovery or mutate
  any working clip except the specifically authorized test clip after identity
  verification. Final audio acceptance still requires native undo, audible mapping,
  repeated wrap and instance isolation; Timeline acceptance requires complete native
  controls and preserved stacked-timeline choice through normal transitions.

### Approved combined diagnostic installation — October 27, [test time omitted] UTC

the project maintainer explicitly approved [checkpoint] and quit Resolve. Process check confirmed closed.
Installed clean diagnostic runtime [checkpoint] using
the prepared transactional installer after macOS administrator authentication.
All four bundle-file hashes match the candidate; both executable signatures verify;
Current runtime manifest matches the bundle. All 62 snapshotted OpenGate/native
Resolve preference files are unchanged, including the keyboard preset. No workflow
command scripts, layout reset, profile restore or project mutation was performed.

Fresh rollback transaction:
`[local path omitted]`
contains `Restore Previous OpenGate.command` and previous bundle/Current metadata.
Local evidence and rollback details are retained privately.
This supersedes earlier installed-[checkpoint] statements; published release remains
[checkpoint] and is unchanged. New process loading and live diagnostic usefulness are
not yet verified. The audio commands and toolbar repair remain incomplete.

Next precise user action: reopen Resolve normally, select the authorized test
polywav at the end of the timeline, and open its Audio Channel submenu. Merely
opening the menu should provide the first native action inventory; do not require
an audio change/Undo or layout reset for this initial evidence capture.

### First combined diagnostic live capture — October 27, [test time omitted] UTC

Loaded [checkpoint] confirmed in test process, session
[test identifier omitted]. the project maintainer opened the test clip's Audio Channel
submenu. Four inventories show five enabled/checkable native QActions: MixL,
MixR, Channel A, Channel B, Channel C; only Channel A/channel 3 checked. Action object names and
QVariant data are empty. The submenu and its parent are unnamed QMenus. Focus
ancestry includes UiTiSequenceView/m_TiTimelineView; inspected public meta-methods
expose customContextMenuRequested(QPoint), not a proven channel operation or menu
preparation method. No native action was invoked by OpenGate.

This confirms observable real channel actions, not safe fresh-menu construction,
automated native undo, instance isolation, audible output or working hotkeys.
Never cache an action from one right-click and assume it targets later selection.
A subsequent read-only scripting query returned no selected clips in current
project [test identifier omitted], timeline
[test identifier omitted]. The observations are not simultaneous:
context-menu target versus selection, or a changed selection, remain possibilities.
Requested one menu-closed single-click selection check; no mutations authorized
from this empty-selection result. Previous native Undo confirmation need not repeat.

Timeline observer captured 172 transition records during launch; toolbar reached
visible 50px content with a 61px overflow slot. Intermediate native/leased hierarchy
changes are recorded, but no recurring-loss transition has been identified yet.
No repair or full assembled-flow acceptance is claimed. Evidence is retained at
`[local evidence reference omitted]`.

### Menu-closed selection check — October 27

After the project maintainer single-clicked the test clip, read-only API inspection reports Edit,
same project [test identifier omitted], active timeline
[test identifier omitted], and exactly one selected audio item:
[test identifier omitted], [test media] — Channel A, audio track 1.
Mapping is mono channel 3, five embedded channels, mute=false, no linked source.
No setter or other project mutation was called.

The active timeline differs from the previous empty-selection read
([test identifier omitted]). Therefore the earlier observation does NOT
establish a context-menu/API selection mismatch. Single selected-clip resolution
passes for this reported context. Do not ask the project maintainer to repeat it. Native-menu
freshness/target binding, action dispatch with native undo, multi-selection,
audible channel output and toolbar loss remain unresolved. Do not invoke the
previous menu's cached action against this newly confirmed selection.

### Active-timeline contract and native dispatch research — October 27

the project maintainer explicitly requires only selected audio instances in the **current active
timeline** to change. Other instances in that timeline, other timelines, and Media
Pool source mappings must remain unchanged. Added production-operation regression
coverage on feature/selected-audio-channel ([checkpoint]): two timeline instances share a
source, active timeline changes between invocations, and only the new selected
instance changes; unrelated instances retain mapping. Seven operation tests pass.
This does not resolve the external API setter's native Undo failure.

Audio branch [checkpoint] adds NativeAudioChannelChoice: one focused shared interpreter
for freshly supplied native menu actions, reused by the production diagnostic
observer. It validates contiguous channel numbering, exactly one current channel,
enabled/checkable rows, direction and wrapping; it rejects ambiguous/no-current,
single-channel and unsupported menus. QPointer invalidates a destroyed target;
callers must still validate current context, not cache a live action for later use.
Existing native-menu fixture extended (no duplicate harness) and passes; universal
helper compilation passes. No action invocation or new state writer. Not installed
or integrated into the current [checkpoint] runtime; Windows compilation remains pending.

Bounded offline Qt metadata inspection of Resolve 21.1.1's timeline controller,
scene, view and list-context-menu mediator found no exposed channel method. This
is not proof that an internal method does not exist. It rules out treating the
previously guessed method names as established entry points. Local Qt source
qgraphicsview.cpp/contextMenuEvent and qgraphicsscene.cpp/contextMenuEvent show
that default context-menu routing uses position even when an event has a reason;
therefore a synthetic keyboard-context event at an arbitrary center/pointer is NOT
a proven selected-clip route. No such event or undocumented method was invoked.
Local evidence and rollback details are retained privately.

Toolbar investigation remains on its separate branch. Current [checkpoint] installation
contains observation only, not a repair for missing toolbar or stacked-timeline
preference loss. Latest captured session through [test time omitted] UTC contains 176 Timeline
transition records, without an identified recurring-loss transition. Do not force
visibility or change fixed-height layout code based solely on this hypothesis.
Next live evidence needed is the actual disappearance while this diagnostic is
loaded; source inspection continues at the existing toolbar lease/restore boundary.

### Native Clip Attributes route investigation — October 27

the project maintainer requests sustained implementation toward usable hotkeys, not further
report-only checkpoints. Investigating native Clip Attributes as an alternative
entry to an undoable selected-instance channel change; this route is NOT proven
or implemented. Do not assume its accept handler has the same scope/undo semantics
as the previously tested Audio Channel submenu. In particular, independent cycling
of mixed-channel multiselection remains required and must not be silently replaced
by one common channel applied to every item.

Read-only Accessibility inspection of Resolve's main Clip menu did not expose a
Clip Attributes item. At the same check, current timeline was
[test identifier omitted] with no selection. No shortcut, menu action,
channel setter or project navigation was issued. Requested that the project maintainer open the
known test clip's Clip Attributes Audio tab so its actual controls can be inspected
without another diagnostic installation. Window inventory afterward contained only
the main and OpenGate workspace windows, not that dialog. Do not invent widget IDs
or trigger an action against the newly active working timeline.

Toolbar investigation: current Workspace.json's frameEditToolbarTop records contain
floating=false and interface_scale=1, with no native_hidden field. This does not
support blaming a persisted explicit-hidden toolbar flag for the present report.
NativeSurfaceDoors restore, the toolbar lease and captured fixed height remain
separate possible boundaries; no cause or repair claimed. No profile mutation.

### Four agreed channel commands and source-configuration evidence — October 27

the project maintainer supersedes the initial two bare command names with these four exact names:
- Previous Audio Channel — Skip Excluded Channels
- Next Audio Channel — Skip Excluded Channels
- Previous Audio Channel — All Source Channels
- Next Audio Channel — All Source Channels

Both pairs modify only selected audio instances in the current active timeline.
“All Source Channels” broadens the eligible channel range, never the set of clips
modified. No Media Pool mapping writes, global source changes, other-timeline
changes, or changes to unselected instances are permitted.

Read-only live evidence: the selected test instance has mono channel 3 of 5;
its MediaPoolItem.GetAudioMapping reports three mono configured tracks mapped to
channels 3,4,5. Channels 1/2 still exist in the five-channel source. Media Pool Clip
Attributes exposes three output rows; timeline instance Clip Attributes exposes
one output row. This supports deriving exclusions from configured source mappings
for this mono example, rather than inferring exclusions from the instance's single
current channel or labels such as MixL/MixR. Nothing was changed in either dialog.
A later deeper accessibility inspection found the dialog closed; it did not inspect
or invoke the channel cell editor. Native Clip Attributes dispatch/undo remains
unverified, and multiselection must not be silently collapsed to a common channel.

Implemented on feature/selected-audio-channel: both pairs use one focused operation
with direction and explicit range. Skip Excluded reads fresh Media Pool configuration
and cycles its unique mapped mono source channel numbers in source order; All Source
cycles 1..embedded channel count. If the current channel is excluded, advance to
the next/previous eligible number with wrapping. A sole eligible current channel
is a no-op with reason; an excluded current channel may move into that sole eligible
channel. Muted mapped tracks remain eligible (mute is not an exclusion). Missing,
linked-source or non-mono source configurations are skipped, never silently treated
as all-source. Broader stereo/adaptive semantics remain open.

Verification: nine production-operation tests pass; generated Mac and Windows
wrappers pass (34 commands, five direct operations including Stabilize); Mac native
Edit routing, native preset assignment/remapping/conflicts, command-menu exact labels
and once-only dispatch, and workflow result checks pass. All four audio command
variants are covered by routing, menu and preset checks. Universal Mac helper build
passed with the then-uncommitted source; not an installed candidate or Windows C++
verification. Existing [checkpoint] diagnostic installation remains unchanged.

Organization review: no PanelControls edits or second controller/state owner.
Audio mapping/policy stays in audio_channel.py (129 lines, net +27); native Edit
routing stays in nativeeditcommands.h (97 lines, net +3). Shared workflow dispatcher
remains 40 lines, wrapper 31 lines. The prior Stabilize dispatcher extraction still
has real callers and no replaced parallel implementation. Existing fixture harnesses
were reused. This checkpoint remains isolated because the API setter's native Undo
failure is unresolved; passing tests/builds are NOT permission to ship broken Undo.

### Installed-hotkey Undo experiment prepared — October 27

the project maintainer requested a concrete candidate and bounded disposable-project test after
being told the API-based implementation still has an Undo blocker. Prepared clean
runtime **[checkpoint]** on integration/audio-undo-test: audio implementation [checkpoint] plus
the currently installed [checkpoint] diagnostics, on the main documentation baseline.
No uncertain floating Source blink work included. This is NOT a claimed Undo fix.

Exact installation/rollback and live test plan (internal reference retained locally)
requires a new disposable project, one actual assigned hotkey invocation, mapping/
selection/other-instance checks and one native Undo; stop immediately if it fails.
This tests an execution context not yet live-verified: actual installed Workflow
Integration hotkey versus the earlier external connector setter. Do not assume
that context changes native Undo behavior. Expanded acceptance follows only if it does.

Universal helper/bootstrap and targeted combined fixtures pass: nine operations,
Mac/Windows generated wrappers, native Edit, all four preset assignments, menu and
workflow completion. Windows native build/live absent. Candidate installation
staging contains helper/metadata and four exact-name scripts; hashes/signatures
verified. Local mock install/rollback passed byte-identical restoration of all six
managed paths, removal of new scripts, and untouched settings sentinel. Real
installation stays [checkpoint]. Candidate-specific approval and user saving/quitting
Resolve remain required before any real installation. No working-project mutation.

### Four-command Undo candidate installed — October 27, [test time omitted] UTC

the project maintainer explicitly approved installation of [checkpoint]. Confirmed Resolve closed,
installed via administrator-authenticated transaction, verified all 22 staged
payload files and both runtime signatures. All 62 snapshotted OpenGate/native
Resolve settings files are byte-identical afterward. Four exact-name workflow
scripts are installed; no keyboard assignment, layout/profile reset or project
mutation performed. Loaded runtime and actual command/Undo behavior remain untested.

Rollback to previous [checkpoint] diagnostic installation:
`[local path omitted]`.
The transaction covers helper, Current metadata and all four new scripts. Receipt:
`[local evidence reference omitted]`. Published release assets
remain unchanged. This supersedes previous installed-[checkpoint] statements only;
[checkpoint] is an experimental installed candidate, not accepted or merged runtime.

Next: user reopens Resolve normally, then use the documented new disposable-project
fixture and actual assigned-hotkey Undo test. Do not test Undo in the working edit.

### Installed Undo test baseline captured — October 27

the project maintainer confirms a disposable project/timeline. Loaded [checkpoint] verified in process
53459, session [test identifier omitted]. Read-only baseline identifies
project `disposable test project` ([test identifier omitted]), active
`disposable test timeline` ([test identifier omitted]), selected audio instance
[test identifier omitted] ([test media] — Channel A), mono channel 3/5.
Source [test identifier omitted] configures mono channels 3,4,5.
The disposable project contains copied timelines: baseline captures [baseline count] audio
instances across [baseline count] timelines for isolation comparison. No writes performed.
Local evidence and rollback details are retained privately.
backed up before live assignment; new commands currently have no saved assignments.
Next user step: assign Next Audio Channel — All Source Channels through native
Keyboard Customization, return to intended selected clip, invoke once. Inspect
readback/isolation before asking for one native Undo. No live command result yet.

### Context-sensitive shortcut ownership requirement — October 27

the project maintainer asks that timeline-specific commands leave their keys usable for native
commands in other contexts, distinguishing clip operations from show/hide/maximize.
Product choice pending: Audio Channel/Stabilize require Timeline keyboard focus,
or also work from Inspector/viewer focus while a valid timeline selection remains.
Do not silently change accepted Stabilize access from Inspector fields without
resolving that choice. Command target scope (selected active-timeline instance) is
separate from the keyboard context in which its shortcut should claim a key.

Actual installed [checkpoint] behavior: native preset matching is Edit-page gated;
NativeEditCommands availability checks the page, not focused panel or eligible
selection. Audio/Stabilize validate selected audio/video inside their scripts,
after shortcut interception. Thus an ineligible operation may already have claimed
the key before returning no-selected-clips. Text fields block audio shortcuts;
modified Stabilize/Fixed Playhead are explicitly allowed in text fields. Dialogs
and tracking menus are guarded. Native conflict matching conservatively rejects
other owners except the known FairlightTimeline exclusion; it does not implement
general focus-context reuse of the same key. No full context-aware behavior claim.

Repair should reuse command catalog/availability and existing input routing, with
one explicit context owner and validation before claiming a key; preserve native
commands and ordinary typing. Do not add a separate keyboard listener or assume
that a late script no-op returns a consumed key to Resolve. Scope decision pending;
no installed behavior/settings changed. Current disposable Undo baseline remains
captured; no test hotkey or Undo has been executed by the agent.

### Installed command dispatch and stale Timeline display — October 27, [test time omitted]–[test time omitted] UTC

the project maintainer reports U/J and Workflow Integrations appear not to work. Logs distinguish
several accepted native-preset Next/Previous dispatches and direct native workflow
actions, with script completion reporting one changed item. J initially triggered
OpenGate's conservative conflict rejection against
FusionWidget.fuHotkey_FuFrame_Playback_Mode{ play = true, reverse = true}; Resolve's
own workflow handler nevertheless subsequently ran Previous. This is inconsistent
routing evidence, not proof of a correct context-aware shortcut implementation.
Do not disable Fusion bindings or ignore all native conflicts to make the test pass.

Read-only API inspection first found the same selected test instance at Channel C/5.
After further user invocations, the native submenu inventory at [test time omitted] and the
supplied [test time omitted] screenshot check **1 — MixL**, while the painted timeline label
still says **Channel A**. The highlighted Channel A submenu row is hover, not the checked
channel. Subsequent API inspection of the same project/timeline/item returns
name [test media] — MixL and mono channel 1. Source mapping remains channels
3,4,5. No agent mapping write or Undo performed during diagnosis.

This establishes stored/native-menu versus painted-label inconsistency for this
candidate. Audible channel and waveform correctness remain unverified; script
success is not full user-experience acceptance. Installed-path Undo/Redo remains
untested. Do not conceal the mismatch by periodically showing controls, replaying
native commands, or inventing a forced repaint fix. Exact invocation sequence is
not a controlled single-step test: two Previous dispatches are recorded 241ms apart
at [test time omitted] with separate completion records; do not infer duplicate dispatch from
one physical press without matching input events. Preserve the evidence.

Directions clarified: Next increases source channel numbers with wrap; Previous
decreases with wrap. All Source: 1→2→3→4→5→1; Skip Excluded in this fixture:
3→4→5→3. Previous reverses each sequence. This is independent of menu hover.
Local evidence and rollback details are retained privately.
live-ui-mismatch-events.json; screenshot supplied by the project maintainer. Both the shortcut
context problem and native presentation/update problem remain open.

### Ordered work queue

1. **Previous Audio Channel / Next Audio Channel** — implemented on an isolated branch; local checks pass, live acceptance pending. These exact user-facing names must be assignable through
   the existing command system. Previous cycles 4 → 3 → 2 → 1 → 4 for a four-channel
   source; Next cycles 1 → 2 → 3 → 4 → 1. Primary use: a selected audio clip backed
   by multichannel media such as a polywav. Change the source-channel selection,
   not the selected timeline track, clip, mixer bus, volume or mute state.
   Discover native capabilities and actual selection/mapping semantics first.
   Do not silently change every timeline instance through a shared Media Pool
   attribute. Establish per-instance versus source-global effects before mutation.
   Record precise questions for linked selections, stereo/adaptive/multiple output
   mappings and selection context if evidence cannot settle them. No/multiple
   selection, offline/unsupported media and one-channel sources require safe,
   explicit behavior. Verify both wrap directions, repeated invocation, actual
   resulting channel/audio, unintended-instance isolation and native undo behavior.
   Do not declare completion from a displayed channel label or fixture alone.
2. **Reveal selected clip in Finder / File Explorer** — newly requested, not
   implemented. One assignable command resolves one selected audio or video clip
   from Timeline or Media Pool and reveals the underlying file. The user must not
   have to invoke Find in Media Pool first. Preserve selection, panel placement
   and hidden Media Pool state; do not replay navigation as a visibility workaround.
   Resolve ambiguous/multiple selections safely. Investigate offline media,
   compound/multicam/generated clips and proxy/original path semantics; record
   limitations rather than selecting an arbitrary file. Shared selection and
   command semantics; OS file reveal belongs in platform adapters.
3. **macOS Resolve Free startup** — the upstream maintainer reports 0.0.3 prototype.1 installs
   but provides no OpenGate UI menu or panel buttons. I/O-plugin eligibility is
   his hypothesis, not an established cause. Trace bootstrap/discovery boundaries
   and distinguish unsupported startup from initialized-but-hidden controls.
4. **Coherent workspace recall** — the upstream maintainer sees intermediate panel enumeration
   before the final workspace appears on Windows Studio. Track as a presentation
   defect under the existing coherent-open/restore contract. Preserve final layout,
   ownership and native initialization; no speculative keep-showing timers.
5. **Divider drag repaint** — the upstream maintainer reports costly live repaint on Windows
   and suggests his deferred-repaint approach. Investigate bounded presentation
   behavior, native rendering correctness and final repaint. This is a specific
   reported interaction issue, not authorization for broad speed optimization.
6. **Panel Manager tooltips** — clarify options the upstream maintainer found unclear without
   inventing new modes or exposing implementation details.
7. **Product attribution / independent repository** — the upstream maintainer requests OpenGate
   identity in the installer license presentation, suggests “Inspired by” if
   acknowledged, and recommends leaving the fork relationship. Preserve his
   correspondence. Review inherited notices and actual authorization before any
   license changes. Plan repository migration preserving history/releases/issues/
   links; no migration, deletion or publication is authorized merely by this report.
8. Continue the remaining numbered native-entry contract and this entire checklist,
   including assignable hold-to-ignore docking/snapping, Reverse Match Frame,
   native rendering/page-state/return/persistence and other unclosed requirements.

### Latest reconciled evidence

- Accepted ordinary-launch native double-click, Match Frame and Find load/select
  and retained reveal fixes were merged. the project maintainer later accepted tested Mixer,
  default proportions and nested-panel header behavior as a checkpoint. These are
  bounded live results, not blanket product completion.
- Main at this documentation update is `[checkpoint]`; release runtime is `[checkpoint]`.
  Release `v0.0.3-prototype.1` is published with Mac universal PKG and Windows x64
  EXE. Mac is signed/notarized/stapled; Windows unsigned. 54 Mac hidden suites,
  9 Mac displayed suites and 50 Windows fixture runs passed. Compilation,
  fixtures, installation and live acceptance remain separate evidence categories.
  See [release](https://github.com/jordantetewsky/OpenGate/releases/tag/v0.0.3-prototype.1)
  and checkpoint (internal reference retained locally).
- the upstream maintainer’s report, supplied by the project maintainer on October 26: Windows Studio installer,
  startup/menu/features, intuitive inline docking and saved-workspace recall work.
  Record this as external live evidence for those tested behaviors. It does not
  close every Windows contract item. Mac Free startup failed as described above.
- Floating Source blink remains open/deferred polish. Its ineffective trial stayed
  isolated and unmerged. Difficult docking target discovery remains flagged;
  the upstream maintainer’s positive Windows result does not invalidate the maintainer’s earlier report.
  Earlier Mixer native-rendering crash is not conclusively eliminated.
- the project maintainer requested tutorial preparation. Most recent recorded uninstall removed
  installed payloads and set aside the active OpenGate profile, preserving all 59
  hashed native preference files including keyboard.preset.xml. Recovery:
  `[local path omitted]`.
  Do not assume this is still current: inspect installation/process/profile state
  read-only. Do not reinstall or restore his profile during investigation.
- Updated optional the project maintainer Preset is shipped. Fresh profiles enter Resolve
  Layout; existing profiles preserve their last view. The custom user preset was
  set aside for the tutorial, not removed from its recovery backup.

### October 27 takeover observations (read-only)

- Verified main HEAD `[checkpoint]`, with only the expected checklist edit and untracked
  handoff at takeover. Both handoff additions are preserved. `origin` is the upstream maintainer’s
  repository; `personal` is the maintainer’s. Do not push development to `origin`.
- Tutorial uninstall state is no longer current: the I/O bundle, Current metadata
  and active Unbound profile exist. Installed runtime metadata identifies clean
  `[checkpoint]`; this metadata read is not a complete
  payload hash audit or proof of the loaded helper. The recovery folder still exists.
  Resolve's read-only API reports Studio 21.1.1.10, Edit, an open project/timeline.
  No installation, settings restore, hotkey edit or project mutation performed.
- Connector API documentation explicitly provides Timeline.GetSelectedClips and
  TimelineItem.GetSourceAudioChannelMapping/SetSourceAudioChannelMapping. The
  Audio Mapping guide says the timeline setter accepts exactly one track_mapping
  entry; channel_idx is 1-based, zero means unconnected, output count follows type.
  Embedded and linked source metadata are read-only to this setter. Media Pool
  SetAudioMapping is a different operation and can redefine source track structure;
  it will not be used for these commands. Runtime getter returns the actual selected
  audio item's stereo [1,2] mapping. Setter, audible output, other-instance isolation
  and undo are not tested. No documented timeline undo API was found.
- Product question pending: leave stereo/adaptive mappings unchanged initially,
  or route all their outputs to one channel. Mono wrap semantics are already clear.
  Linked selection, linked source audio, unavailable media and mapping validation
  remain explicit investigation/test items, not inferred support.
- Command architecture: PanelCommandCatalog owns identity/preset assignment;
  PanelCommands owns dispatch/input guards; NativeEditCommands owns Edit operation
  entry points; WorkflowCommandBridge transports registry requests; generated
  workflow scripts perform selected-clip API operations; WorkflowCommandResults
  reads completion diagnostics. PanelControls wires these, not operation logic.
  Planned bounded extraction: reusable native workflow-action dispatch from
  NativeEditCommands, preserving Stabilize behavior and removing its inline copy.
  Audio mapping will have one focused shared operation with a direction argument;
  reveal/path resolution and native OS reveal remain separate responsibilities.

### Agreed command flow and next implementation checkpoint

the project maintainer clarified the target with the October 27 [test time omitted] screenshot: the selected
Timeline audio clip's native **Audio Channel** submenu (MixL, MixR, Channel A, Channel B,
Channel C in the example). Cycle these source channels, preserving each clip instance.
Multiple selected audio items advance independently from their current channel
with their own wrap limits; selected video is unchanged. This supersedes the
initial single-selection-only interpretation. Single-channel and unsupported or
offline items are skipped with explicit reasons. Stereo/adaptive semantics remain
an investigation gate: never silently collapse outputs to mono.

All new OpenGate commands must use the existing assignment/dispatch system, with
stable IDs, exact names and the same native keyboard customization entry path.
Previous/Next are separate entries calling one audio operation with direction.
Catalog identity, native shortcut integration, workflow dispatch, audio operation
and completion reporting have separate jobs. PanelControls only wires components.
No second shortcut listener, visibility controller or audio implementation.

- Dispatcher extraction is implemented on `feature/selected-audio-channel`:
  NativeWorkflowOperation now owns the existing validated workflow-action invocation;
  NativeEditCommands' Stabilize caller delegates to it and its inline path is removed.
  Mac hidden native-edit-command fixture compiled and passed (zero failures).
  Initial fresh-worktree header setup and sandbox window-service failures preceded
  that passing run; they are not production failures. Windows execution is pending.
  Not installed, live-tested, merged or published.
- Next: shared audio selection/mapping operation, previous/next catalog entries,
  generated workflow targets and meaningful wrap/repeated/multiselection/failure
  tests; preserve Stabilize, Fixed Playhead and native keyboard behavior.
- Live mutation testing is NOT authorized yet: the disposable project proposal
  remains unanswered. No working timeline or selection is to be changed to test.
  Audible output, per-instance isolation and native undo remain live gates.
- Deferred follow-up: an OpenGate-created Qt **Selection** panel (working name),
  dockable/floatable/tabbable through existing panel mechanisms, showing selected
  video/audio counts and operation results. Hidden panels should do no rendering
  or panel-only refresh work. This is an explicit proposed addition to the usual
  native-content panel definition, not permission to replace native Resolve panels.
- Deferred discussion: ShuttleGlide-style optional command/result animation and
  possible future input integration. Feedback should follow actual operation
  results, not merely key presses. Neither animation nor Selection blocks the two
  commands; detailed presentation choices remain unaccepted.

### Audio candidate implementation and evidence — October 27

Worktree `selected-audio-channel`, branch `feature/selected-audio-channel`, based
on documentation checkpoint `[checkpoint]`; dispatch extraction checkpoint `[checkpoint]`.
The two command entries now share `src/core/operations/audio_channel.py`, with
fresh native selection and mapping reads on each invocation. Eligible mono clips
cycle independently across embedded source channels, preserving mute and track
format. No Media Pool setter, selection writer, panel door or native key replay
is used. Duplicate/missing selected-item identities reject before any mutation.
Video selection is unchanged; unavailable original files, single-channel media,
unconnected channels, linked-source audio and non-mono mappings are skipped.
A successful setter is checked by mapping readback; that is NOT audible proof.
Partial setter failures are counted; successful earlier changes are not silently
rolled back. Native undo grouping remains unknown and must be live-tested.

Production responsibilities reviewed:
- NativeWorkflowOperation owns validated action lookup/invocation; the replaced
  inline Stabilize path was removed, and the real caller uses the extraction.
- NativeEditCommands registers and routes Edit operations; no audio mapping lives
  there. PanelCommandCatalog and the existing preset/input ownership are unchanged.
- Shared audio_channel.py owns the selected-instance audio operation only.
- workflow_operation_wrapper.py owns direct-operation execution guards and atomic
  result transport for both Stabilize and audio; the old inline wrapper was removed.
- PanelCommandMenu presents registered non-panel operations using catalog labels,
  so adding an operation no longer requires another hand-written menu entry.
- WorkflowCommandResults consumes bounded diagnostics only; it cannot dispatch.
  Audio result counts/reasons are currently diagnostic, not a new visible panel,
  toast, or animation. User-facing result presentation remains unfinished.
- PanelControls, selection ownership and OS adapters were not changed. No new
  persistent state, shortcut listeners or visibility controllers were introduced.
  Windows and Mac generators use the same operation and wrapper implementation.

Verification executed locally:
- Six production-operation tests pass: both wraps/repeated fresh reads; independent
  multiselection/video/unselected isolation; skipped mappings/offline files/mute;
  duplicate identity rejection; setter refusal/readback mismatch; empty/page guards.
- Actual generated Mac and Windows wrapper tests pass, including repeated audio
  invocation and independent wrap, no recursive requests, and preserved Stabilize.
- Mac hidden native-edit-command, native-preset-bindings, workflow-command and
  panel-command-menu fixtures pass (zero failures), including audio assignment,
  removal of old bindings, conflicts, exact menu labels and once-only dispatch.
- Full universal Mac helper compilation/link/signing passed. This is a development
  build, not an installer, installation, Windows build, or live acceptance result.
- The first Mac wrapper test caught an extracted-module import-path failure;
  resolved through explicit file-based module loading, and its rerun passed.
- Shared operation tests are registered in the Mac wrapper suite and Windows
  release test entry. Windows script tests ran on Mac; Windows C++ compilation,
  native execution and live acceptance remain unverified for this candidate.

Next precise gate: obtain permission for a disposable live project and verify
native menu/API equivalence, actual distinguishable channel audio, other-instance
and Media Pool isolation, repeated/multiple selection and undo. No project switch,
live mutation, installation or publication has occurred. The new audio behavior
stays isolated until verified and accepted. The broader ordered queue is retained.

### Authorized live test and toolbar observation — October 27

the project maintainer approved disposable live testing, then identified the already-open
`DEMO OPENGATE` project as temporary. Added a separate `OpenGate Audio Channel
Verification [checkpoint]` timeline with two instances of generated four-channel
330/550/770/990 Hz audio. Existing `OpenGate_Demo` timeline was not edited.
The initial dictionary-form ImportMedia call returned None; a path-list import
succeeded. The test source's Media Pool mapping was configured mono during fixture
setup only; production channel cycling never uses that setter.

No-selection production operation passed in real Resolve 21.1.1.10. A baseline
WAV render job was added, but StartRendering returned false; no output/audio proof
was obtained. Render setup moved Resolve to Deliver; the agent returned it to Edit.
Do not describe this failed render attempt as successful signal verification.

the project maintainer reported the toolbar disappearing, then reported it returned. Installed
runtime logs identify `[checkpoint]`, session `[test identifier omitted]`.
Edit→Deliver occurred at [test time omitted] UTC and return at [test time omitted]; header controls logged
zero width during Deliver and positive width after return. Workspace snapshot says
header visible=true. Which toolbar disappeared and exact recovery trigger remain
unconfirmed; temporal proximity does not prove cause. Preserve this as an unresolved
page-return presentation observation, with recovered visibility, not a proven fix.
Logs/settings snapshot: `[local evidence reference omitted]` in the feature
worktree. No layout reset, installation or toolbar-state mutation was performed.

the project maintainer pasted a known five-channel polywav and selected its mono Channel A/channel-3
instance. The actual production operation was loaded from source through the
Resolve connector, then Next changed it to channel 4. Readback confirms exactly
one timeline item changed across the project's audio-item snapshots, selection
identity stayed the same and Media Pool mapping stayed byte-identical. Receipt:
`[local evidence reference omitted]`. This establishes the tested setter's
instance scope, not audible output, undo, installed shortcut execution or overall
acceptance. the project maintainer is checking playback and one native Undo before further writes.
New audio command binaries/wrappers remain uninstalled; live source-operation
execution is distinct from the assignment fixtures and installed user experience.

### Audio undo failure — candidate acceptance blocked

the project maintainer reports that native Undo after the source-API channel change undid the
preceding paste instead of restoring channel 3. Installed logs record editUndo
at [test time omitted] UTC and editRedo at [test time omitted]. Subsequent read-only inspection finds
the restored pasted clip still mapped to channel 4 (Channel B), with no selection.
This is evidence that this tested API invocation did not add the expected native
undo entry. Do not install/promote the candidate as a complete native command.
No more channel mutations are being made pending comparison with a direct native
Audio Channel menu change followed by Undo. Audible result remains unconfirmed.

Investigate the native menu route and actual undo scope before choosing a repair.
Do not manufacture a separate undo stack, intercept Cmd-Z, use Fusion composition
undo as though it were timeline undo, or silently accept undoing unrelated edits.
The API route remains isolated with its passing local tests and explicit live
limitation; those tests do not establish undo behavior.

### Native undo comparison and deferred channel colors — October 27

the project maintainer changed the test clip through Resolve's native Audio Channel UI and reports
that Undo works. Read-only API inspection afterward finds the same selected clip
at channel 4 (Channel B); the supplied Clip Attributes screenshot also shows mono,
Source Channel 4 — Channel B, Audio 1. This establishes a behavioral difference from
our tested external source-API setter. Investigate the native operation/transaction
route; do not claim a replacement implemented from UI labels or method names.
No new audio candidate is installed or accepted. Audible output is still a
separate unconfirmed check.

**Deferred requirement: assign colors to audio source channels.** the project maintainer wants
channel colors, illustrated with Clip Attributes' source-channel selection.
Record for roadmap discussion; do not expand the immediate cycling/undo work.
Open product choices: source-channel identity shared across timeline instances
versus per-instance clip color; where colors appear (native timeline, channel
menu, future Selection panel); project persistence, source renames/relinking and
collaborator behavior. Investigate native capabilities before promising coloring
native controls or replacing original content. Reuse existing clip-color research
where applicable, but source-channel color is not automatically native clip color.
No implementation, UI format, scope semantics or release commitment yet.

### Native Audio Channel diagnostic preparation — October 27

the project maintainer is exporting; no Resolve queries, project/page manipulation, clip mutation,
installation or live test is to run during the export. Work continues locally.
The channel-command API candidate remains isolated on feature/selected-audio-channel.

Separate branch `diagnose/native-audio-channel`, worktree `native-audio-channel-probe`,
starts from accepted main plus documentation ([checkpoint]), excluding the API candidate.
NativeAudioChannelProbe records Audio Channel QMenu actions, scalar data types/values,
checked/enabled state, ownership and bounded focus-ancestor method signatures.
InteractionDiagnostics forwards its existing Show observations; no extra application
input listener, action invocation, menu preparation/opening, selection write or
visibility controller is introduced. Custom native handle payloads are not decoded
or dereferenced. At most 32 inventories and 64 trigger observations per process.
The callback observes a Qt signal only; it does not claim actual mapping or undo.

Universal Mac helper compilation passed. Isolated diagnostic fixture passed with
zero failures: no invocation/visibility/geometry/ownership changes, once-only trigger
observation after repeated inspection, bounded captures. Existing hidden interaction audit
regression also passes (hidden/dynamic/page observations). Unrelated-menu and
OpenGate-owned-menu exclusion checks pass. Windows uses the same InteractionDiagnostics
caller but compilation/runtime are not verified. Diagnostic not installed, not merged
and not live-accepted. No new command is exposed by this diagnostic build.

Read-only static Qt string-table inspection found no channel setter in UiTiSequenceView;
UiConformerPanelImp exposes HandleAudioSelection/RefreshAudioMenu but their purpose
is unverified and they have not been invoked. Method names are not implementation
proof. Broader offline scanning was stopped to avoid unnecessary export contention.
Next evidence: actual native Audio Channel menu inventory on an approved diagnostic
build, followed by a bounded native operation test only after action/context semantics
are established. No fake undo history or Cmd-Z interception is planned.

### Working and integration discipline

- Before implementation, map each task to the relevant contract, production entry
  point and evidence. Keep a status of uninvestigated / implemented / compiled /
  fixture-verified / installed / live-accepted / merged / published as applicable.
  Record exact build, session and user observation; do not erase unresolved quirks.
- Before production edits, state each affected module's responsibility. Shared
  command registration/dispatch remains shared; channel selection and mapping
  belong in a focused audio-channel component; selected-media resolution should
  reuse native selection infrastructure; OS reveal belongs in platform adapters.
  These are responsibility boundaries, not preselected API or file-name designs.
- Inspect existing command catalog, bridge, native edit operations and wrappers.
  Extract a touched independent responsibility from oversized modules in a bounded,
  behavior-preserving change. Move actual callers and remove replaced paths. No
  duplicate shortcut listener, state owner, visibility controller or patch pile.
- Review actual diffs for ownership, dependencies, duplicate logic and file growth.
  Test meaningful assembled behaviors and both platform callers; disclose Windows
  verification gaps. Refactoring is part of the work when warranted, not deferred
  automatically and not an unrelated architecture rewrite.
- Keep clean, reviewed, accepted work eligible for main in coherent checkpoints;
  keep uncertain experiments isolated. the project maintainer wants good work merged while quirks
  remain explicitly tracked. Historical merge/push permission is not blanket
  permission to install/publish every future candidate. Document scope and remaining
  failures at each integration, and obtain candidate-specific installation approval.
- Never terminate Resolve, reset a layout to pass a test, alter hotkeys, discard
  another agent's work or restore tutorial settings without authorization.

# Product definitions

Use three product terms: **Panel, Container, and Layout**. These definitions reflect
the maintainer’s October 25 clarifications and agreement. They describe required product
behavior, not completed implementation or live acceptance.

## Panel

A Panel is a native Resolve element: the native grouping of UI and its functions.
Each Resolve page has its own panels. Timeline includes the timeline, its toolbar
above, and its native timeline tabs. Inspector, Source, Media Pool and Mixer are
other examples. Native tabs inside a panel are not OpenGate tabs between panels.

A panel can float independently, have its UI scaled independently, or be docked
or tabbed with other panels. Its native functions remain intact. Mixer starts next
to Timeline, not inside the Timeline panel. Mixer follows the same panel rules:
independent floating, neighboring panels, and tabs within a container. Its native
layout relationship must not limit those operations.

When a panel is maximized, its contents must use the available maximized content
area and remain accessible without clipping. Enlarging only the outer window
while retaining the old content bounds does not satisfy this requirement. Restore
must return to the previous placement and size. This applies to Mixer as well as
every other panel; native painting needs live verification. Panel-specific limits
must be documented rather than silently losing native functions.

## Edit-page panel inventory

These are the panel identities already registered in the current coordinator.
This list defines their product boundaries, not proof that every operation passes.

| Panel | What belongs to it |
| --- | --- |
| Timeline | Timeline editing area, its toolbar and native timeline tabs |
| Source Monitor | Native source viewer and its playback controls |
| Program Monitor | Native timeline viewer and its playback controls |
| Media Pool | Bin navigation, clip contents and native Media Pool controls |
| Inspector | Native inspection/editing controls for the current selection |
| Mixer | Native audio Mixer and its controls; a neighbor of Timeline, independently movable |
| Index | Native Edit Index and its controls |
| Effects | Native effects browser and its controls |
| Metadata | Native metadata display/editing controls |
| Sound Library | Native sound-library search and controls |
| Keyframes | Native keyframe editor and its controls |

Every listed panel follows the same float, neighbor, tab, hide/recall and maximize
rules. Different native widget structures require adapters, not different user
rules. The current code also registers Timeline Toolbar separately internally;
that is not permission to expose it as a separate arranged panel. Header and
Footer follow their band definitions below. Actual native content boundaries,
including Mixer sizing, must be checked against Resolve before changing adapters.

## Container

A Container holds one or more panels, arranged beside each other, above/below each
other, or as tabs. A panel floating independently occupies a one-panel container.
Panels retain their identities and individual UI scales when grouped.

Agreed container rules:

- A floating container has traffic lights and a pin. New floating containers start
  pinned. Pinning belongs to the container; preserve an explicit unpin choice when
  hiding, recalling or restoring a saved layout. Embedded groups do not gain extra
  OS traffic lights.
- The container's traffic-light X hides the whole container without destroying
  its arrangement or overwriting individual panels' hidden/available choices.
- A panel's X hides that panel while remembering its place in its container.
  Its tab remains present and dimmed; clicking it reveals and selects the panel.
- Closing the active tab switches to the next available tab in tab order, like a
  page switcher. It skips deliberately hidden panels. Closing an inactive panel
  does not change the currently selected tab.
- Closing the last available panel hides the container, even if remembered hidden
  tabs remain. Do not reopen a deliberately hidden panel to keep the container up.
  For a one-panel container, panel X and traffic-light X have the same visible
  result: the container disappears and remains recallable.
- Recalling a panel restores its retained container and selects that panel in its
  remembered position. Recall of the whole container restores its previous
  presentation; it does not unhide individually hidden panels.
- Hiding a split panel lets visible siblings fill its space; recall restores the
  remembered split placement. Switching tabs preserves the container's outer size.

Tabbed panels have three distinct states:

| State | Appearance | Clicking the tab |
| --- | --- | --- |
| Active | Highlighted | Keeps the panel selected |
| Available behind another tab | Normal | Selects the panel |
| Deliberately hidden | Dimmed but readable | Reveals and selects the panel |

Show/hide hotkeys use the same visibility rules. A hotkey for a hidden or covered
panel reveals/selects it in place. Invoking it while that panel is presented hides
it and selects the next available tab; when none remain, the container hides.
With two available tabs this behaves like flipping a two-sided card. Hidden and
merely unselected panels must remain distinct internally.

"Window" is an OS implementation detail, not another product object the project maintainer must
manage. Native window handling remains necessary underneath floating containers.

## Assignable free-drag command — agreed requirement

Provide an assignable **Ignore docking and snapping while held** command. While a
panel or container is being dragged, key-down suppresses docking, tabbing/grouping
targets and magnetic snapping and immediately clears previews. Key-up restores
target detection at the current pointer position within the same drag, without
requiring another mouse movement. The user can alternate these states repeatedly.
The command is hold/release, not a latched toggle or saved global setting.

Holding the command through mouse-up leaves the dragged item floating at the
chosen position. Releasing the command before mouse-up permits normal docking
again. Whole-container movement preserves its internal panel arrangement. Drag
cancellation, focus loss or a missed key-up must not leave bypass stuck on.

Option on macOS / Alt on Windows is the existing hardcoded bypass and a suggested
default, not a restriction on assignment. Shift already controls edge alignment.
Integrate assignment into the shared command system with platform input adapters;
do not add a competing shortcut listener. Show/hide and maximize commands must
remain unaffected, and normal typing outside a drag must retain its native use.

Current floating-host preview and release paths already check Option/Alt, and
existing tests cover unsnapped Option release. That does not establish assignable
hold/release or preview refresh without pointer movement. Verify native title,
panel-title, grab-zone and whole-container drags on both platforms. Clear drag
guidance should expose the command and its current assignment.

## Layout

Consolidate "Layout" and "Workspace" into one product concept: **Layout** (working
label). A layout records panel/container placement, sizes, grouping, tab order and
selection, visibility, per-panel UI scale, pinning, and master-bar visibility.

The current layout includes unsaved changes. A saved layout is a named copy that
can be recalled; it is not a different kind of workspace. Preserve per-page state
and the protected built-in Resolve Layout. Existing internal identifiers and file
schemas need not be renamed to simplify the interface.

## Header and footer

The Header shows which panels are currently presented and provides controls to
reveal/select or hide them. A header button reveals and selects its panel if hidden
or behind another tab; pressing it while the panel is presented hides it using the
same rules as panel X and the show/hide hotkey.

The Footer navigates Resolve pages. Both bands retain their independent hide/show
behavior; hiding a band releases its occupied space. One primary interface owns
these bands; they do not migrate to whichever floating container gains focus.

The far-right Panels dropdown provides access to every panel on the current page,
including hidden panels, plus a clearly labeled Containers section for recalling
existing groupings without hotkeys. Keep individual panels directly accessible.
Recalling a container restores that grouping; it does not load a different layout.
Saved Layouts remain clearly separate because loading one may rearrange the whole
interface. Panels access remains reachable when the Header is hidden.

## Recall after display changes

Recalling a panel must make it reachable on a connected display. If its floating
container was on a disconnected display or is now unreachable offscreen, bring
that whole container onto the display containing the primary OpenGate interface.
Preserve grouping and size where possible; constrain placement/size enough to keep
its title and controls accessible. Do not detach the requested panel or reset the
layout to achieve visibility.

A container deliberately spanning connected displays stays where the user put it
as long as it remains reachable. Reconnecting an external display does not move
containers automatically. Explicitly recalling a saved layout may restore its
external-display arrangement when that display is available.

# Repair status

Close/hide/recall has an isolated implementation and local checks. Installation
and live acceptance remain pending. Workspace capture/save extraction is implemented locally;
the seeded saved-output comparison and six affected hidden fixtures pass.
Clean universal candidate `[checkpoint]` and all nine displayed suites pass.
Verification and remaining gaps (internal reference retained locally)
separate those results from installation and live acceptance. the project maintainer does
not need to review the technical assessment line by line.

Reference: behavior review (internal discussion retained locally), the maintainer’s annotated HTML, and the source assessment of build `[checkpoint]` on October 24, 2026. The source assessment is complete. First-repair implementation and local evidence are recorded in retained window close (internal reference retained locally).

# First repair

**Required behavior:** X hides an OpenGate floating panel or whole group. Recall brings back the same host, size, position, pinning, and group membership. Explicit docking remains a separate operation. Match Frame must still load the correct source frame as well as reveal Source.

**Checkpoint defect:** In `[checkpoint]`, `src/core/panelsession.h` handled Close by docking and `src/core/panelgroup.h` handled Close by dissolving the group and docking its members. Tests explicitly expected this outdated behavior. The isolated repair replaces those paths with retained hiding.

- [x] Implement on `fix/retained-window-close` while preserving the installed build.

- [x] Route ordinary managed-window Close through existing hide mechanisms. Temporary expansion, native return and shutdown pass local checks; a separate existing full-layout Dock rejection remains recorded below.

- [x] Replace contradictory tests with checks of retained host identity, geometry, pinning, group membership, and hidden state.

- [x] Verify recall through existing panel callers, including a group with hidden members and nested tabs, in the assembled fixture.

- [x] Check named snapshot restoration, fresh-process unsaved restart, page departure/return, explicit dock to the native slot, and shutdown in fixtures. Live acceptance remains pending.

- [x] Produce a reviewable diff and separate compilation and fixture results; retain the intermittent displayed drag-suite failure as well as passing reruns.

- [ ] Obtain the maintainer’s explicit authorization before installing a candidate.

- [ ] Verify the real X button and recall in Resolve, including the correct Source picture for navigation commands.

Do not claim this repair fixes playback disappearance, blinking, or native loading. Those have separate acceptance checks.

# Upstream comparison and direction — October 25

the project maintainer agreed to continue OpenGate's bounded structural repairs after reviewing
Unbound v0.5.0. GitHub's latest-release endpoint identified v0.5.0, published
October 24. The local upstream checkout is `[checkpoint]`; its only difference from
the release tag is README text, so the inspected production source matches v0.5.0.

Upstream's `panecatalogue.h` centralizes native panel identities and policies.
That is a useful structural reference. Its sidebar and viewer sessions still
close by docking back into native slots. Edit viewers have their own lifecycle
and geometry adapters, including native single-viewer visibility handling.
Its catalogue does not register an independent Edit Timeline panel. It does not
provide OpenGate's split/tab container arrangement system.

Replacing the upstream button would be small; adding our agreed container,
hide/recall, band and full-layout behavior would require changes below that button.
There is no evidence that doing that again would be safer than repairing existing
OpenGate boundaries. Continue focused extractions with behavior checks; evaluate
specific upstream mechanisms for reuse individually. This source comparison is
not live acceptance of upstream or proof of the cause of our regressions.

# Live review and reopening priority — October 25

the project maintainer reported the installed `[checkpoint]` trial positively and requested merge to
main. Local merge `[checkpoint]` preserves main's release README and incorporates the
installed source history; no production changes were introduced by conflict
resolution. No push or new release. This does not close every historical defect.

the maintainer’s next priority is less latency and eliminating the visible sequence of
individual panels opening before the saved arrangement appears. Both cold launch
and in-session use matter; no specific in-session trigger was identified.

Three captured Resolve processes ran `[checkpoint]` (PIDs 18735, 18762, 18785). Edit
resume to native-plan assembly request took approximately 4.44, 4.01 and 4.72
seconds respectively. These intervals include readiness and other event-loop
work; they are not isolated CPU measurements. Each session recorded 78–88 menu
Local evidence and rollback details are retained privately.

Code review: restoration acquires one requested panel at a time because native
Media Pool/Index toggles can be mutually exclusive. Each pass also revisits UI
wiring, including a full native menu scan. Individual sessions are presented
before the final native arrangement/group mounts. SurfaceReadiness is already
event-driven: its timeout is a deadline, not a mandatory delay to remove.

Next optimization must distinguish required native initialization from repeated
UI preparation. Measure and reduce repeated wiring first, then evaluate final
presentation without intermediate floating hosts. Do not hide the native root
or skip readiness as a cosmetic shortcut: prior Source-loading evidence depends
on native visibility. Retained in-session recall must preserve host identity and
must not unnecessarily replay startup acquisition. Evaluate real elapsed time,
intermediate visibility and final layout/picture correctness separately. The
accepted installation stays unchanged during this work. No speedup is yet claimed.

## First reopening optimization: native menu preparation

Implemented separately on `perf/reopen-menu-preparation`. `PanelMenus` now
inventories attached native menu owners once per synchronous `wire()` pass,
instead of rescanning all window descendants for every menu. Parent ownership
and conflicting attached owners are still checked. Opening a menu gets a fresh
inventory; there is no persistent cache or second controller. A scoped
`panel.menus.wire` timing span records the actual operation in future live logs.

The 12-panel/1,800-extra-button hidden fixture measures five samples of ten passes.
Baseline median: 4.443 ms/pass. Changed median including timing: 0.563 ms/pass
(about 87% reduction for this operation only). Logs: `[local evidence reference omitted]`
and `menu-after.log` in the optimization worktree. This does not establish a
Resolve reopening speedup or removal of the visible assembly sequence.

Ownership checks pass for ambiguous attachments despite a known parent, conflict
removal, changes between wiring and menu opening, parent-only custom menus, late
menu creation and exactly one command dispatch after repeated preparation.
First-open and delayed-Index startup flows pass. All five complete saved-output
checkpoints match the accepted fixture run with QT_HASH_SEED=0, normalizing only
random preset IDs. The new fixture is registered for Mac and Windows; Windows
execution remains pending. PanelControls is untouched; PanelMenus remains one
responsibility at 185 lines. No panel visibility, readiness or persistence policy
was changed. Candidate compilation and any live trial remain separate gates.

Do not change the presentation sequence merely to claim progress. The next
missing evidence is the optimized operation's real share of restoration time in
Resolve; existing logs do not isolate that cost. Use the narrow timing span in a
reviewed candidate before choosing a broader restoration/presentation change.

## Menu optimization live review — October 26

Installed `[checkpoint]`: the project maintainer reports reopening is quicker, approves merge, and
still sees individual panels loading. Merged to local main as `[checkpoint]`;
no push/publication. Installation verified 54 payload files and 24 unchanged
protected settings files. All 54 hidden suites passed before installation;
this candidate's displayed matrix and Windows runtime remain unverified.

Two recorded live sessions show eight menu wiring passes each: total 206.9 ms
and 199.6 ms, median 28.4 ms and 26.4 ms per pass. Edit resume to native-plan
assembly request measured 1.862 and 1.856 seconds. Previous captured runs measured
roughly 4–5 seconds, but layouts and conditions were not controlled; do not turn
that comparison into a precise speedup claim. the maintainer’s report supports a
perceived improvement. Raw sessions are in `[local evidence reference omitted]`.

The visible loading sequence remains unresolved. This optimization changes menu
preparation, not the sequence that acquires individual native panels, presents
their temporary hosts and then assembles the saved arrangement. The next bounded
work should target presentation during that existing restore transaction while
preserving native visibility/readiness, exact saved layout and failure recovery.
Do not simply hide native content: prior Source-loading failures make that an
unsafe assumption. Accepted installed `[checkpoint]` remains the rollback checkpoint.

# Remaining repair order

**Next code-organization task:** give workspace capture and saving a focused owner outside `PanelControls`, reusing `WorkspaceSnapshot`, `WorkspaceSettings`, and `PageWorkspaceState`. Replace the coordinator's existing implementation rather than adding a parallel save path. Preserve file schemas, incomplete-restoration guards, native-return checkpoints and temporary-expansion snapshots. Verify production startup, named/unsaved restoration and restart before accepting the extraction. The local extraction is implemented; acceptance evidence is still being reviewed.

**Panel and container implementation:** use the product definitions at the top of this document. Consolidate existing panel identity, native ownership and supported operations; keep placement, visibility and group membership explicit. Any consolidated definition must govern production operations and replace scattered rules; the test-only `PanelPresentation` structure alone is insufficient. The agreed container policies above must be included in future implementation and acceptance checks; the earlier Close repair does not establish all of them.

1. **Startup and Timeline sizing.** Compare saved divider sizes, applied sizes, and final native content bounds. The reported screenshot has a visible Timeline header but an unusably compressed track area. Check first launch separately from restored layouts; preserve checkpoints rather than resetting arrangements.

2. **Visibility and recovery.** Identify the actual cause of floating-window disappearance and blinking. Separate explicit user visibility requests, native-content recovery, tab selection, and window ordering. Remove a conflicting decision only when demonstrated; do not add another keep-showing timer.

3. **Native loading and command reliability.** Preserve successful Source evidence and resolve the experimental launch-flag dependency. Verify double-click A then B, Match Frame, and Find in Media Pool separately, with correct pictures or clip selection and no layout changes.

4. **Dragging and docking targets.** Distinguish docking beside one panel from docking beside a whole stack. Check preview bounds and release results. Investigate deactivation cancellation separately.

5. **Labels and control consistency.** Fix clipped names and make Mixer’s drag affordance understandable with the same Panel rules as its Timeline neighbor.

These tracks may share findings, but their fixes and acceptance results must remain distinguishable.

**Separate docking finding:** in the assembled full layout, detach Inspector and use Dock Panel without first closing it: the session reports "The original panel slot changed." Close is not involved in this reproduction. Direct docking after native return passes. Do not widen the Close repair to change native slot restoration silently.

# Code organization rules

- Reuse the existing layout tree and persistence format. Do not add a competing layout engine or a second independent state model.

- Make each component’s decision responsibility explicit. A new production state representation must replace an existing decision path, not sit beside it.

- Separate behavior repairs from mechanical file splitting. The large coordinator needs narrower responsibilities; line-count reduction alone is not success.

- Preserve native ownership restoration, grouping, tabs, magnetic docking, maximize, hotkeys, Fixed Playhead, Stabilize without Inspector, hideable bars, saved and unsaved arrangements, and the shared Windows foundation.

- Keep implementation, compilation, fixtures, installation, and live acceptance separate. A passing test that asserts the wrong behavior is not acceptance.

- Preserve a reliable native return underneath any future simplification of its user-facing controls.

# What the project maintainer needs to do

No further specification review is needed to start the first repair. Correct requirements only if they do not match the desired experience.

After local work is complete, the project maintainer receives a short explanation, a reviewable change, and a narrowly defined live test. Installation requires explicit authorization. No new layout tour or reconstruction of previously reported failures is required.

**Current status:** first repair implemented and locally checked on an isolated branch; installation, live Resolve acceptance, Windows execution and coordinator extraction remain pending. Installed software is unchanged.

## Rollback after Source regression reports — October 26

This status supersedes the earlier installed/accepted statements above. the project maintainer
reported a possible Media Pool double-click → Source regression on `[checkpoint]`.
That passive initial-presentation experiment was withdrawn and installation
restored to `[checkpoint]`. the project maintainer then reported double-click loading still failing
on that build and requested a further rollback. Neither report establishes the
causal change; prior fixture passes do not establish native clip-picture success.

Installed software is now `[checkpoint]`, before both
reopening optimizations. Resolve was closed for recovery. All 54 payload hashes
match its install plan; all 24 protected settings/preference hashes match the
checkpoint captured immediately before this rollback. Current layouts were not
reset. Rotated logs and the verification receipt are preserved in
`[local evidence reference omitted]` (sibling worktree).

Live acceptance remains pending. The Source experiment still requires the test
launcher with `OPENGATE_EXPERIMENT_VISIBLE_NATIVE_STASH=1`; ordinary reopening
is not equivalent. No source changes were reverted, merged or pushed during
recovery. The menu optimization already on local main is now under regression
review; the passive presentation experiment remains isolated. Suspend further
speed changes until loading is confirmed on the restored checkpoint.

### Installation update: one checkpoint forward — October 26

At the maintainer’s explicit correction, restored exactly `[checkpoint]`, not `[checkpoint]`.
All 54 installed payload files match its plan; all 24 protected files remain
unchanged from the immediately preceding snapshot. Receipt and current logs:
`[local evidence reference omitted]`. the project maintainer reports
Match Frame working and double-click loading failing on the older `[checkpoint]`.
Loading remains unresolved; this restoration is a version preference, not
acceptance of Source behavior or proof of a performance-regression cause.

## Native loading reconciliation — October 26, current takeover

Installed payload remains [checkpoint], reverified 54/54. Its ordinary session lacked
the experiment flags. Same-build launcher session [checkpoint] confirms experiment
activation and the project maintainer reports double-click loading working, but hidden docked
and floating Source fail to reveal, and visible floating Source blinks. Find
remains separate and unresolved. Normal-launch candidate and recall diagnostics
are isolated and uninstalled; no merge/push/publication. See
reconciliation and verification (internal reference retained locally).

## Authorized installation — October 26

the project maintainer authorized this candidate and Resolve was verified closed. Installed the
reviewed helper bundle as [checkpoint]-dirty (fix/native-loading-normal-launch), with
source and binary hashes in build/macos/candidate-receipt.json. All four bundle
files match; all 24 protected settings/preferences remain unchanged. Installation
receipt: build/macos/authorized-install/receipt.json. Recoverable [checkpoint] checkpoint:
[local path omitted]
No merge, push or publication. Ordinary-launch loading is expected to work without
the experiment flag; hidden-Source recall is diagnostic only, and blink/Find are
not claimed fixed. Next live test: ordinary launch, A/B pictures, then one hidden
Source double-click to capture the rejection reason. Earlier uninstalled status
in this document describes preparation and is superseded by this receipt.

## Native command reveal candidate — October 26

Installed ordinary-launch candidate remains unchanged. Its new live diagnostics
prove Source recall rejects Resolve's custom clip view as not-abstract-item-view.
The subsequent uninstalled behavior repair adapts native release-time Source Clip
activation and actual Find/Match Frame QAction triggers to the existing retained
panel doors. Custom-view and standard-view assembled suites pass; universal build
and bootstrap checks pass. See native navigation repair (internal reference retained locally)
for evidence, diff, organization review and exact live gaps. Floating blink is not
claimed fixed. Native Find selection, matching frame/pictures and reveal require
live acceptance; candidate-specific installation permission is still required.
The rest of the product checklist remains open and speed optimization stays paused.

## Authorized installation of navigation repair — October 26

the project maintainer authorized and Resolve was verified closed. Installed [checkpoint]-dirty; all
four helper bundle files match and all 24 protected settings/preferences are
unchanged. Receipt: build/macos/navigation-install/receipt.json. Recovery checkpoint:
[local path omitted]
This supersedes the candidate-pending statements above. Live acceptance of native
double-click/Match Frame/Find output and retained reveal remains pending; floating
blink is not claimed fixed. No merge, push or publication.

## Live review after installation — October 26

the project maintainer reports: "find in media pool doesn't work. everything else does" in the
requested three-command test. Treat this as positive live evidence for the tested
double-click and Match Frame output/reveal, not blanket acceptance of all product
requirements or an explicit withdrawal of every historical blink report.
Session [test identifier omitted] / test process / [checkpoint]-dirty records
clean shutdown. Evidence: build/macos/navigation-live-review/session.jsonl and
user-results.json.

Match Frame: native action 856 -> queued reveal 857 -> complete 872; second test
1435 -> 1436 -> 1454. Custom Media Pool double-click: armed 1282 -> native source
activation queues 1289 -> existing Source door completes 1307. These traces
accompany the maintainer’s result, rather than substituting for correct native pictures.
Find has no navigation.native-action observation. workspaceShowPanelMediapool at
797/941/987/1046 is Show Media Pool, not proof Find executed or failed to select a
clip. Do not blame the maintainer’s invocation or change mappings from this observation.
The exact Find input/dispatch path remains to be identified; the QAction-based
adapter is incomplete for the reported sequence.

Architecture assessment: automatically preserving the parked native tree addresses
the observed full-layout visibility dependency introduced by OpenGate hosting.
It is an empirically supported compatibility technique, not proof of Resolve's
internal implementation. Source reveal replaces a demonstrably invalid standard
Qt-item-view assumption with a gesture-scoped native Source Clip activation
adapter. That adapter depends on Resolve control identity and release-time
behavior; both are evidenced for this runtime and remain compatibility risks for
other Resolve versions. NativePanelNavigation identifies explicit intent and uses
the existing single showPanel/save door; it neither loads clips nor owns a second
visibility model. The large coordinator only wires state and callbacks. Existing
standard-item fallback, Native return and ordinary-launch behavior are preserved.

Contract 9 is partially met: tested Source navigation works, Find remains open,
and Reverse Match Frame is a separate outstanding contract item. No contract
completion or main merge is claimed. Keep this checkpoint recoverable on the
feature branch; recommend identifying/fixing native Find before integrating the
whole navigation change. A future explicitly authorized partial merge must state
its exact accepted scope and leave Find open. No merge/push/publication performed.

### Find native shortcut path identified — October 26

The latest three-command test accepts tested double-click and Match Frame, but
Find fails. Native R dispatch maps to clipFindClipInMediaPool in the current saved
preset and does not emit the QAction trigger our observer expected. A bounded
uninstalled repair now observes the existing QShortcut activation and reuses the
sole preset reader and panel door; it creates no custom Find command or mapping.
Assembled remapped-key/ambiguity/reassignment checks and native-preset regression
pass; universal compilation passes. See opengate-native-navigation-reveal-[checkpoint].md.
Keep checkpoint [checkpoint] recoverable; no main merge or contract completion claimed.

## Authorized Find correction installation — October 26

the project maintainer authorized [checkpoint]-dirty and Resolve was verified closed. All four installed
helper bundle files match; all 24 protected settings/preferences are unchanged.
Receipt: build/macos/find-shortcut-install/receipt.json. Recovery checkpoint:
[local path omitted]
Earlier uninstalled statements describe preparation and are superseded here.
Live native Find selection and retained reveal remain pending. No main merge,
push or publication. Next test: ordinary launch, hidden Media Pool, the maintainer’s
existing R Find shortcut, correct selected clip and retained panel revealed.

## Accepted navigation checkpoint; blink remains open — October 26

the project maintainer reports the three tested native navigation paths working and authorizes
merging/pushing their fixes, then investigating floating Source blinking. This
supersedes the earlier pending Find result. No full contract acceptance is claimed.
See checkpoint scope and evidence (internal reference retained locally).
Installed payload and user settings remain unchanged by this source checkpoint.

## Mixer panel candidate — October 26

Native navigation checkpoint remains merged in main (`[checkpoint]`). the project maintainer deferred
the floating Source blink trial after reporting no improvement; it remains
isolated and unmerged. Installed `[checkpoint]` was reverified read-only against all
four staged bundle files. Mixer development is on `fix/mixer-panel-repair` from
accepted main, excluding the blink trial. No Mixer installation or publication.

See Mixer repair and evidence (internal reference retained locally) for the
independent default leaf, reversible header/expansion constraints, reproduced
retired-wrapper discovery fix, organization review and live gaps. The native GPU
crash is not yet claimed fixed. Startup Timeline compression and the rest of this
checklist remain open; changing the default recipe does not rewrite saved layouts.

### Default must present the complete layout correctly

the project maintainer clarified that Resolve Layout must look correct as a complete preset,
including usable Timeline height, viewers above, independent fitted Mixer, intact
native toolbars/tabs and readable headers. Correct membership alone is insufficient.
The 3:2 center split failed an assembled proportion assertion because small recipe
weights were passed directly to Qt as pixel sizes. Shared proportional sizing now
passes that assertion, saved-divider and native-minimum checks. Live rendering
acceptance remains open; the new default never overwrites a retained last view.

## Authorized installation — October 26

the project maintainer authorized installation of the Mixer/default-presentation candidate.
Installed `[checkpoint]-dirty` from `fix/mixer-panel-repair`, helper SHA-256
`314fd16443c594bc1b296215b407bd33ca21f0d9de6bcb7918e2745ce10dd8f8`. All four installed bundle files
match the staged candidate; Current metadata matches; all 24 protected settings
and preference files are unchanged. Resolve was confirmed closed before install
and checked again by the installer. Rollback checkpoint:
`[local path omitted]`.
Receipt: `[local evidence reference omitted]`.
This supersedes earlier uninstalled statements. No merge, push or publication.
Native live acceptance remains pending. Launch normally; preserve the current
view with Save as New before selecting Resolve Layout to inspect the updated
default. Check complete panel presentation, then Mixer maximize/restore and
close/recall after Timeline expansion. No launch flags are required.

## Authorized header-repair installation — October 26

Installed approved nested-section header repair at [test time omitted] UTC with Resolve closed.
Helper SHA-256 `e260d08b8992b844b3957f29c752f3bed42fac8e64340d0e4621505d7b408d99`; all four installed payload files and
Current metadata match. Rollback checkpoint: `[local path omitted]`.
Receipt: `[local evidence reference omitted]`.

Resolve reopened at [test time omitted] UTC (session `[test identifier omitted]`)
during verification. Twenty-three protected files remain identical; Workspace.json
changed with recorded successful runtime saves beginning [test time omitted] UTC, after
installation completed. Its pre-install copy is preserved; the live workspace
was not overwritten to force a checksum match. Earlier uninstalled statements
are superseded. Live header/tab acceptance and docking-target investigation
remain open. No merge, push or publication.
