# OpenGate continuation handoff — October 27, 2026

Continue OpenGate for Jordan Tetewsky. This is a sustained implementation and
maintenance task, not just adding two hotkeys. Read and update the authoritative
checklist throughout; maintain its progress and unresolved-issue ledger, refactor
at proper responsibility boundaries, and integrate good work in reviewable
checkpoints while isolating experiments.

## Start here

Worktree: `/Users/jordantetewsky/Developer/OpenGate-features/main-release`.
Expected main HEAD before these uncommitted doc updates: `518fcd0`.
Read applicable parent/repository AGENTS.md, then:

- `doc/platforms/opengate-repair-checklist-20261024.md`, especially the October 27
  current priorities/progress section at the top (authoritative additions).
- `doc/platforms/opengate-native-entry-flow-acceptance.md`.
- `doc/platforms/opengate-task-audit-20261020.md`.
- `doc/platforms/prototype003-v1-checkpoint.md` and the navigation/Mixer evidence
  linked from the checklist.

Inspect actual Git status and preserve these uncommitted documentation updates
and all other agents' work. Reconcile historical claims with receipts and dates.
Check installation, process and profile state read-only; Jordan was preparing a
fresh-user tutorial and requested uninstall/profile backup while preserving native
hotkeys. Never launch/terminate Resolve or reinstall/restore settings implicitly.

## Immediate implementation priorities

1. Exact assignable command names: **Previous Audio Channel** and **Next Audio
   Channel**. For a four-channel polywav, Previous is 4→3→2→1→4; Next is the reverse.
   Jordan means changing the selected audio clip's source-channel mapping, not
   moving selection between clips/tracks. Investigate native mapping and selection
   scope first; establish whether the operation changes only that instance or shared
   source attributes. Do not assume a mono/stereo/adaptive mapping policy or silently
   affect other uses of the media. Ask only precise unresolved behavioral questions
   after independent investigation. Follow the detailed acceptance in the checklist.
2. One assignable **Reveal in Finder / File Explorer** operation for a single
   Timeline or Media Pool audio/video clip, without requiring Find in Media Pool.
   Reuse native selection/path mechanisms; keep panel placement and visibility intact.
3. Then address Wolfgang's Mac Free startup failure, visible workspace enumeration,
   costly divider repaint and Panel Manager clarity; plan attribution/repository
   work separately. His Windows Studio success is real external live evidence,
   not proof that every contract item is complete.

## Architecture, progress and integration

Before production edits state module responsibilities. Inspect the shared workflow
command catalog/bridge, native edit commands, native shortcut integration and
platform wrappers. Keep commands separate in responsibility and testing, using
existing registration/dispatch. Extract touched independent responsibilities from
large coordinators when necessary; remove replaced paths and move real callers.
No duplicate listeners/controllers or speculative timers. Shared semantics in core;
OS behavior in adapters. Verify both callers and report untested platform behavior.

Update the checklist with evidence and exact status after each meaningful result.
Flag quirks rather than burying them or declaring the whole product done. Keep
implementation, build, fixture, installation, user acceptance, merge and publication
separate. Prepare/merge good reviewed accepted work in coherent checkpoints within
Jordan's authorization, keeping uncertain experiments isolated; do not merge a
failed trial just to empty a branch. No new install or release without appropriate
candidate authorization. Preserve public 0.0.3 assets and release-page images.

Progress reports are not stopping points. Continue independent authorized work;
only ask Jordan to test when the test answers a precise remaining question that
existing evidence cannot answer. Do not repeat completed layout tours. Do not claim
background work after ending the turn. The rest of the checklist remains in scope.
