Fairlight focus-context registration snapshots (Windows diagnostic)
=================================================================

Build from the repository root:
  .\tests\platforms\windows\audition-map\build.cmd
Uses the standard Windows toolchain and Qt 5.15.2 including private headers for
the existing recorder's one-shot manager discovery. No icons are regenerated.
Output:
  build/windows/audition-map/Release/audition-map.exe
  build/windows/audition-map/Release/AuditionMap2.dll
The DLL suffix distinguishes it from an inactive first parser prototype already
loaded in the development session. The published Unbound build is unchanged.

Next live test: three snapshots
------------------------------
Use the current normal .001 Unbound session; no restart is needed. The previous
context-notification test must be stopped. Open Fairlight and use the same clip.

1. Dock Media Pool. Click audition Play, then run from the repository root:
     .\tests\platforms\windows\audition-map\capture.cmd
2. Return to Fairlight. Undock Media Pool, click audition Play in the floating
   window, then run the same capture.cmd command again.
3. Return to Fairlight. Dock Media Pool again, click audition Play, then run
   capture.cmd a third time.

Tell the investigator when the three snapshots are done. No Start/Stop commands,
timed playback run, Spacebar exercise or additional trace launcher are required.
Leave each pane in its tested docked/floating state until capture finishes.
This is read-only: audio behavior is expected to remain as before.

Each command queues one snapshot on Resolve's GUI thread and creates a timestamped
JSONL file in build/windows/audition-map/Release/logs/. The normal command-line
message says queued; a capture is valid only if its context-map record contains
valid:true. A rejected image, missing object or malformed map is logged as an
error; it is never treated as a successful registration inventory. Running the
terminal changes foreground focus, so compare manager values with that limitation.
The regular focus/active-window snapshot accompanies every map capture.

What is recorded
----------------
- Every registered context ID and its primary/associated widget references.
  Duplicate IDs are retained: Resolve has multiple registered widgets per ID.
  Entries are sorted by context; equal-ID ordering is not a native priority claim.
- Names, classes, visibility, focus policy, window identity and parent paths for
  references independently matched to Qt's current live-widget inventory.
- Qt same-window ancestry versus the ordinary parent chain to previewPlayer.
- Current manager context (+0x78), lookup scope (+0x74), raw field70 (+0x70), and
  current widget (+0x38/+0x40). No semantic name is assumed for field70.
An associated widget is reported by identity; its role is not inferred from the
pointer alone. Expired/unreadable references are described without dereferencing
their target. Fields are a snapshot, not proof of a particular branch executing.

Version and memory checks
-------------------------
This test deliberately reads private Resolve data, unlike the earlier signal
observer. It is an inspection tool for this investigated build, not production
code or a portable API. It NEVER writes to those fields or calls private methods.
The source executable investigated is Resolve 21.1.1.10, SHA256:
  1a109e719f2d5e20167ecd9e0f1a37407cf62702fcea366e5e7f6dbbdb826f83
Runtime gate checks Qt 5.15.2, x64 PE identity (timestamp/image size), SHA256 of
three exact in-memory context-code regions, and the manager's metaobject address
relative to the loaded executable. This is a code-region compatibility gate,
not a rehash of the entire running executable. Unsupported fingerprints abort.

Verified data layout: manager+0x28 points to the registration map; Qt5 x64 map
data has size at +4 and root at +0x10. Node key is at +0x18, followed by primary
QPointer at +0x20/+0x28 and associated QPointer at +0x30/+0x38. These offsets are
supported by the exact native lookup disassembly and the installed Qt headers.
All raw reads use bounded ReadProcessMemory on the current process, with return
values checked. No raw address is directly dereferenced as a QObject. Tree size
is capped at 2048; alignment, parent links, key order, cycles and node count are
validated. Duplicate keys are allowed, matching native registrations.

No widget changes, synthetic events, activation, playback commands, polling,
background recorder or settings writes. One explicit discovery plus a bounded
map read per command. The DLL remains loaded but installs no ongoing observer.

Validation
----------
Build runs dependency audits and fixtures against actual Qt QMap/QMultiMap and
QPointer layouts, including repeated keys, empty/expired entries, corrupt trees,
unreadable pointers, bounds and incompatible-image rejection.
Live smoke check in PID 253048 succeeded with 168 registrations and resolved
Fairlight audition IDs 0x5010000 (previewPlayer and AudioPageMediaPoolWidget) and
0x5018004 (mediaPoolClipView). The user was on Deliver at the time, so this snapshot
does not substitute for the requested docked/floating Fairlight comparison.

Why this test
-------------
Further static analysis found an explicit isVisible() guard in the focus updater
at preferred VA 0x144f903ec..0x144f903f7, before context lookup. The earlier hidden
proxy could not pass that guard. Additionally, the live map shows previewPlayer
itself is registered, so lost ancestor relationships alone are not a sufficient
explanation. Capturing the current lookup scope alongside actual registrations
will help distinguish wrong-scope lookup from missing/invalid widget registration.

Three-state user result, 2026-10-13
---------------------------------
All three snapshots were valid with identical 168 registration tuples. Docked
and redocked: lookup scope 0x05000000, current context 0x05010000 (audition).
Floating: lookup scope 0x07000000, current context -1. previewPlayer stays visible
and registered under 0x05010000 in every state. Scope 0x07 contains other independent
window registrations, not audition. The mismatch is scope/window classification,
not loss of the registered player. All captures were taken with the terminal in
front; Qt focus/activeWindow null in all three. See investigation notes for limits.
The final run used Stop then Play after docking; this does not invalidate the
resulting-state comparison and does not require a repeat.
