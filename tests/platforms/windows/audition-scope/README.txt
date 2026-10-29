Fairlight scope restoration experiment with automatic captures
=============================================================

Build from the repository root:
  .\tests\platforms\windows\audition-scope\build.cmd
Output: build/windows/audition-scope/Release/audition-scope.exe and
AuditionScope.dll. Uses the normal Windows toolchain/Qt 5.15.2 package and the
private headers needed by the existing observer's manager discovery. No icons
are regenerated. Production source and published releases are unchanged.

Run in the existing normal .001 Resolve session
----------------------------------------------
No restart or reinjection of Unbound is needed. Previous intervention tests must
be stopped. Open Fairlight, dock Media Pool, and select the same audition clip.

1. Start once from the repository root:
     .\tests\platforms\windows\audition-scope\start.cmd
2. Return to Resolve and click audition Play while Media Pool is docked.
3. Undock Media Pool and click audition Play in the floating window.
4. Dock Media Pool again and click audition Play.
5. Stop once:
     .\tests\platforms\windows\audition-scope\stop.cmd

Stop then Play is equally valid in EVERY playback step; no continuous playback
across a transfer is required. No capture commands or precise pauses between
steps are required. The observer records and labels playback actions itself.
Check whether the floating step has audio, smooth waveform and Control Room
metering. Note any unexpected change of keyboard focus/window activation.
Report whether floating audition was audible and whether docking remained normal.

Automatic captures
------------------
Each Start creates a timestamped JSONL capture under:
  build/windows/audition-scope/Release/logs/
Includes one full registration inventory at startup and lightweight native scope,
context and current-widget reads for subsequent logged events. Each relevant
click/focus action gets before/after records with an action number and phase:
docked-audition, floating-audition, floating-media-pool or main-timeline. One
deferred record after the native input handler returns captures the settled state.
No extra user action is needed for any of these snapshots. The usual context,
active/mute/focus/transport signals are recorded too; custom enum payloads remain
undecoded. Play clicks are marked; FocusIn and other Media Pool clicks can create
additional action records without implying another Play command.

The capture/intervention stops automatically after three minutes or 2500 records;
the short sequence above should take much less. Explicit Stop disconnects the
observers and intervention and flushes buffered records. It does not force a new
native context. Normal subsequent main-window interaction supplies that reset.
The DLL stays loaded until Resolve exits. --inventory produces a read-only
inventory and checks compatibility without enabling the intervention. --mark is
an optional flush/checkpoint, not needed for this sequence.

Intervention
------------
The previous snapshots showed that activating the independent Media Pool host
switches native lookup scope from Fairlight 0x05000000 to separate-window scope
0x07000000, while audition registrations remain under 0x05010000. The player
itself remains visible and registered. The failed hidden proxy also violated
Resolve's visibility prerequisite and could not correct the scope mismatch.

This experiment intervenes only on MouseButtonPress/FocusIn from the visible,
foreground, specifically named Unbound Media Pool host while Fairlight is visible
and the manager's scope is exactly 0x07000000. It sends a synthetic WindowActivate
notification to UiMainWindow THROUGH QApplication::sendEvent. Resolve's native
application event handling is allowed to select its normal page scope. If a
readback confirms 0x05000000, it sends one FocusIn notification to the original,
visible input target, again through QApplication event handling.

Only those exact synthetic event instances are consumed by our application event
filter after native application processing, before ordinary QWidget delivery.
This avoids broadcasting a fake activation through the main window's children.
Original user events continue unchanged. Reentrancy is guarded. Even if Stop or
the record cap occurs during nested native processing, the exact synthetic event
is still consumed before the observer is removed.

No direct scope/context writes, signal emission, private method calls, patches,
widget reparenting, activateWindow or setFocus calls. Native handlers may themselves
change focus or other state: before/after focus/active-window checks record this.
Do not assume those native side effects are absent based on fixture behavior.
Docked content, other panes, the Dock control, hidden pages and unknown scopes
are excluded from correction. The scope is read back rather than assumed restored.

Compatibility and validation
----------------------------
The full read-only map diagnostic validates manager identity and exact Resolve
21.1.1.10 context-code fingerprints before any native offsets are read for event
tracing. Two additional fingerprints cover window-event dispatch and scope setup.
An unsupported image or missing objects prevents the intervention from starting.
Only fixed-size 128-byte manager reads occur on events; allWidgets/map discovery
runs once at explicit Start. No polling, periodic scans or full map reads on clicks.

Build dependency audit and fixtures passed. The fixture models application notify
ordering, successful/failed scope selection, physical Qt focus/window preservation,
original input delivery, exact synthetic-event consumption, automatic captures,
excluded targets/scopes, stop during native notification, and docked phase labels.
It cannot establish Resolve's audio behavior or native focus side effects.
Live read-only readiness check on PID 253048: all 168 registrations decoded and
scope-gate allowed:true. No intervention was enabled during that check.

This is a controlled diagnostic, not a production fix. Success would establish
that restoring native scope/context through this event path recovers audition;
production design still needs lifecycle and interaction validation. Failure logs
distinguish rejection of scope correction from successful context activation that
still does not produce audio.

Live result, 2026-10-13
----------------------
User confirmed audible floating audition. Complete capture
focus-253048-20261013-151608-801.jsonl shows one correction on timecodeEditor FocusIn
shortly after undock: scope 0x07000000 -> 0x05000000, context -1 -> 0x05010000, then
previewPlayer IsActiveChanged(true). Actual Qt focus and active window stayed
unchanged; content remained floating. Later floating Play needed no correction.
Docked Play before and after followed the native path without intervention.
Explicit stop after 41.001 seconds, no cap hit, all signal connections valid.
Waveform, meters and subjective docking behavior were not checked by the user;
the capture establishes the docking context sequence, not full visual/audio QA.
Remaining lifecycle, handoff, keyboard and version-compatibility validation is
needed before adopting a production implementation.

Follow-up: the user repeated the test and explicitly confirmed smooth waveform
updates and Control Room meter signal from undocked audition. Those two items
are now validated by user observation on this configuration. Broader handoff,
keyboard, lifecycle and compatibility validation remains outstanding.
