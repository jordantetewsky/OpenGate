Fairlight audition context-notification experiment (Windows diagnostic)
=====================================================================

Question
--------
Can Resolve's ordinary context-update path activate audition audio while Media
Pool stays in its independent window? Paired passive traces established that
Qt/player focus arrives but logical audition context and IsActiveChanged(true)
are missing only in the floating host. This experiment tests that missing step.

Build from the repository root:
  .\tests\platforms\windows\audition-context\build.cmd
Requires the standard Windows toolchain and Qt 5.15.2 including private headers
for the existing recorder's manager discovery. Output:
  build/windows/audition-context/Release/audition-context.exe
  build/windows/audition-context/Release/AuditionContext.dll

Test using the current normal .001 session
-----------------------------------------
No Resolve restart is needed if normal .001 is already loaded. Do not launch the
child-host build. If starting fresh: Resolve -> project -> Fairlight -> launch
release/win/2026.10.13.001/Unbound.exe. Stop the earlier passive recorder if it is
still running. This experiment includes its own recorder.

1. Keep Media Pool floating, select the same audition clip, and stop playback.
2. Run from a terminal at the repository root:
     .\tests\platforms\windows\audition-context\start.cmd
3. Return to Resolve and click audition Play in the floating Media Pool. Check
   audible audio, waveform smoothness and Control Room metering.
4. Click the main timeline and start timeline playback. Click floating audition
   Play again. Check whether audition takes over immediately and audibly.
5. Click inside the floating Media Pool and press Space twice, with a pause
   between presses. Note whether audition or timeline responds.
6. Stop the experiment:
     .\tests\platforms\windows\audition-context\stop.cmd
7. Return to Resolve, click the main timeline to reset native context, then
   click floating audition Play again. Note whether the previous silent behavior
   returns. If audition remains running, stop it before the timeline/reset step.
8. Dock Media Pool and click audition Play to return to the normal docked state.

Keep steps 2-6 under three minutes. Report sound, waveform, meters, handoff and
Space behavior, plus what happened after Stop. The log distinguishes a failure
to activate context from activation that still fails to produce audition audio.
No further passive tracing commands are necessary.

Logs:
  build/windows/audition-context/Release/logs/focus-PID-UTC.jsonl
The launcher queues commands on Resolve's GUI thread. A new file should appear
after Start. bridge-ready must report enabled:true. --inventory is metadata-only
and does not enable the intervention. --mark flushes an optional checkpoint.
The same UNBOUND_AUDITION_TRACE_DIR override as the passive recorder is supported.

Mechanism and scope
-------------------
On explicit Start, discover the known Fairlight page, original Media Pool slot,
preview player and focus manager once. Create an invisible plain QWidget child
of the original slot with StrongFocus policy, outside every layout. On an actual
mouse press or FocusIn event inside the independent Media Pool host, deliver one
synthetic QFocusEvent(FocusIn) to that proxy through QApplication::sendEvent.
Resolve's application-level event processing can then recognize the original
branch and update its own context. The proxy event is consumed by our application
event filter after that application-level processing; it never reaches a native
pane handler. The user's original input continues to its actual recipient.

This is an active intervention, not just a passive trace. It does not call
setFocus, activateWindow, native playback methods, or private Resolve functions;
does not emit Resolve signals or write context fields; and does not reparent
player content. It leaves normal Qt focus selection to native input handling.
Native context handlers can themselves have side effects, which is part of what
the live test measures. bridge-after records whether actual Qt focus and active
window stayed unchanged during the synthetic notification.

Scope is limited to visible Fairlight and its visible preview player in the
specifically named Unbound Media Pool top-level host. Docked/child-host input,
other panes, the Dock button and inactive pages bypass the intervention.
Reentrancy is guarded; no polling or recurring discovery. Stop, capture cap or
the three-minute timeout disables forwarding and schedules proxy cleanup.
Stop does not force a replacement Resolve context; the next real timeline click
provides the native reset before the off-again comparison. The DLL remains loaded
until process exit. A rebuild cannot replace code already loaded in Resolve.

The passive recorder is reused through a checked build overlay. This variant
omits Show/Hide logs and propagated child WindowActivate/Deactivate noise to
avoid the premature record cap seen in the previous floating capture. It retains
context/player signals, focus, clicks, parent changes and top-level activation,
plus bridge-before/after snapshots. Limit remains 2500 records / 180 seconds.
Events buffer until start flush, explicit mark, stop, cap, timeout or normal exit.
No production source, published release, settings or icons are changed.

Validation and interpretation
-----------------------------
Build audits launcher/DLL dependencies. The fixture models application notify
ordering and verifies proxy routing, original click delivery, preserved actual
Qt focus/window, content remaining floating, exclusions, trace records, explicit
stop/restart, proxy cleanup and disabling at the record cap. It cannot model
Resolve's private context logic or verify audio output.

If native context and active signals recover with audio, this supports context
activation as causal and motivates a narrower production design. If signals
recover but audio does not, another condition remains. If signals do not recover,
the proxy path was insufficient and cannot answer whether activation fixes audio.
No outcome by itself makes this synthetic-notification experiment release-ready.

Live result, 2026-10-13
----------------------
The user confirmed audition remained silent both with the intervention enabled
and after stopping it. Capture focus-253048-20261013-130914-632.jsonl contains eight
completed notification attempts and all nine successful observer connections,
but no context or player-active changes. Qt focus/active window stayed unchanged.
Explicit stop after 42.567 seconds; no cap hit. This proxy approach did not activate
native audition context. It is retained as a failed diagnostic, not a workaround.
It does not answer whether successful context restoration would restore audio.
