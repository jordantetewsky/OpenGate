# Installed audio-channel hotkey Undo test — October 27

Candidate runtime: `361d3df720a49b196c15fc3205d4d5360271d973`.
Branch/worktree: integration/audio-undo-test / audio-undo-test.
This is a bounded experiment, not an Undo fix or release candidate. The external
API invocation previously failed native Undo. This test checks whether the actual
installed Workflow Integration hotkey path behaves differently. Do not assume it does.

## Candidate contents and recovery

- Four exact assignable commands: Previous/Next Audio Channel — Skip Excluded
  Channels; Previous/Next Audio Channel — All Source Channels.
- Existing native command/preset/menu path, shared direction/range operation and
  workflow dispatcher; no second input listener or Undo history.
- Retains c12980f audio-menu and Timeline diagnostics. No toolbar/stacked-timeline
  repair is claimed. No Source blink experiment included.
- Staged at audio-undo-test/build/undo-test: install.sh, payload, candidate.json,
  source-hashes.tsv, installed-hashes.tsv and transaction-test.json.
- Installer refuses while Resolve runs, verifies staged hashes, makes a fresh
  checkpoint, installs helper/metadata/four scripts, and verifies installed hashes.
  Inventory includes all four new scripts so rollback removes them if previously
  absent. Current uninstall inventory includes them too. No profile seed/reset.
- Rollback restores the installation present immediately before the test install
  (currently c12980f), including metadata and any existing same-named scripts.
  Published 0.0.3 assets remain untouched. User settings are not rollback targets.
- Before actual install: obtain approval for this exact candidate; user saves and
  quits Resolve. Hash OpenGate settings/native Resolve preferences before and after.
  Never terminate Resolve. No installation approval is inferred from preparation.

## First live gate: one hotkey and native Undo

1. Launch normally and confirm loaded build 361d3df in diagnostics. Save the working
   project and switch to a **new disposable project**, not a second timeline inside
   the working project. Record original project identity for later user return.
2. Import the prepared five-channel test source (audio-undo-test/build/undo-test/
   five-channel-tones.wav: channels 1–5 are 330/440/550/660/880 Hz) or a known polywav.
   Configure the disposable source as mono tracks using channels 3,4,5. Make a test
   instance plus an unselected same-source instance and a second timeline instance.
   Configuration writes belong only to fixture setup in this disposable project.
3. Save a reusable test baseline. Before running a command, snapshot project ID,
   active timeline ID, selected IDs, all test-instance mappings, Media Pool mapping
   and native Undo availability. Confirm exactly one intended mono audio instance
   selected, currently channel 3. Do not infer selection from playhead position.
4. Preserve the native keyboard preset before changing assignments; use a copied
   test preset. Assign **Next Audio Channel — All Source Channels** through existing
   native Keyboard Customization, using a conflict-free key chosen with Jordan.
   Return to Edit and confirm intended selection. Do not test while a dialog/menu
   is tracking. Invoke the assigned key once, not the external connector setter.
5. Read back mapping and selection: selected instance must become channel 4, every
   other instance and Media Pool mapping unchanged. Listen for the actual channel
   change separately; logs/readback alone do not establish audible output.
6. Invoke native Undo **once**. Verify selected instance returns to channel 3 and
   unrelated fixture items/mappings remain unchanged. If Undo instead affects a
   previous setup edit, is unavailable, or fails to restore the mapping, STOP.
   Do not repeatedly Undo, manufacture a second history, or try it in the real edit.
7. Only if Undo passes, invoke Redo once and verify channel 4 and isolation again.
   Record route/build, snapshots, user audible report and Undo/Redo separately.

## Expanded acceptance only after the first gate passes

- Assign/test all four entries. All Source cycles 1–5; Skip Excluded cycles 3–5.
  Verify Previous and Next wrapping, repeated fresh state and both menu/hotkey entry.
- From excluded channel 1 or 2, Skip Excluded moves to the next/previous eligible
  source number with wrapping. Source mapping itself remains unchanged.
- Multiple selected mono clips advance independently from their current channels;
  selected video stays unchanged. Establish native Undo grouping explicitly.
- Switch active timeline between invocations; affect only its selected instances.
- No selection, single-channel/sole eligible channel, offline media, unsupported
  stereo/adaptive/linked-source configuration, dialogs and native shortcut conflicts.
- Preserve native keyboard behavior, Stabilize and Fixed Playhead. Restore the user's
  original preset after testing if a temporary preset was used; never clear bindings.
- Existing numbered native-entry contract remains authoritative. Targeted fixtures
  do not reaccept first launch, dragging, bands, native return, page restoration,
  GPU painting, Mixer or Timeline toolbar. Their existing open defects remain open.
- Windows wrapper generation tested locally; Windows native build/live test absent.

## Verification before proposed installation

Clean universal Mac helper and bootstrap built. Nine operation tests, generated Mac
and Windows wrappers, native Edit routes, all four preset assignments/remapping/
conflicts, all four command-menu labels/dispatch and workflow result fixture pass.
Both runtime signatures verify. Local mocked install/rollback restored all six
managed paths exactly and removed the four newly installed scripts; a settings
sentinel was unchanged. No live command, live Undo or audible acceptance yet.
