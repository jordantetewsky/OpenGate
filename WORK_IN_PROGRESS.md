# OpenGate development checkpoint — not a release

**The audio-channel commands are not working end to end. The unified native UI
has not been implemented. Local fixture passes do not establish live acceptance.**

Start with the **[current work, requirements, findings and bug index](doc/platforms/opengate-current-work.md)**.

October 30: [Timeline content, width, toolbar and focused-command plan](doc/platforms/opengate-timeline-content-plan-20261030.md)
records the agreed acceptance contract and local planning/feature branches. Setup
only; these branches contain no new production implementation at creation.
The **[full task list and evidence ledger](doc/platforms/opengate-repair-checklist-20261024.md)**
retains the detailed history and remaining acceptance gates.

| Branch | Scope | Current status |
| --- | --- | --- |
| [wip/command-status-20261029](https://github.com/jordantetewsky/OpenGate/tree/wip/command-status-20261029) | Combined task list, findings and bug index | Documentation checkpoint; no runtime change |
| [experiment/audio-direct-action-test](https://github.com/jordantetewsky/OpenGate/tree/experiment/audio-direct-action-test) | One manually prepared native audio action | Installed test 684bc2f failed context capture before native invocation; permit disabled |
| [fix/audio-context-receiver](https://github.com/jordantetewsky/OpenGate/tree/fix/audio-context-receiver) | Local correction for receiver capture when Timeline lacks focus | Regression fails before/passes after; not installed or live accepted |
| [investigate/native-workspace-parity](https://github.com/jordantetewsky/OpenGate/tree/investigate/native-workspace-parity) | One native UI direction and Relay/performance investigation | Requirements and source/log findings only; no implemented redesign or demonstrated Relay speedup |

These branches preserve development work for review. They do not update `main`,
create release tags, publish packages or make incomplete features available as a
supported build. Raw runtime logs, active permits, user settings, project data,
local build products and binaries are not part of this checkpoint.

The local receiver correction has hidden Mac audio-action and native-edit-command
fixture coverage and arm64/x86_64 production syntax checks. Windows callers were
reviewed but not compiled or live tested. Actual clip binding, audio, repaint,
Undo, silent preparation and the complete command workflow remain unverified.
