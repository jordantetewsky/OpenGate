# OpenGate 0.0.3 prototype.1

October 26, 2026. Jordan accepted the tested behavior and authorized merging to
main and building a package. This is a local package checkpoint, not publication
authorization or completion of every product requirement.

## Later release status — reconciled October 27

This package checkpoint was subsequently merged and published as
[v0.0.3-prototype.1](https://github.com/jordantetewsky/OpenGate/releases/tag/v0.0.3-prototype.1).
Runtime source is `903859173218cd0d037af555febb9d956382e4f3`; main README checkpoint
is `518fcd0`. Both Mac universal PKG (signed, notarized, stapled) and Windows x64
EXE (unsigned) are published. Reported release receipts: 54 Mac hidden suites,
9 Mac displayed suites, 50 Windows fixture runs. Wolfgang reports Windows Studio
startup, docking and workspace recall working, with intermediate recall presentation
and divider repaint concerns. Mac Free startup fails in his report; cause unproven.
These later results supersede the historical pending publication/platform statements
below without establishing blanket live acceptance. See the authoritative repair
checklist for current installation observations and remaining requirements.

## Changes

- Retains the accepted ordinary-launch native navigation fixes: Media Pool
  double-click loads/reveals Source, Match Frame reveals Source, and native Find
  locates the clip and reveals Media Pool without replaying Resolve's command.
- Mixer is an independent neighbor of Timeline in Resolve Layout, with shared
  placement, header and drag behavior. Expansion releases the native container's
  width cap temporarily and restores it afterward.
- Default layout proportions are applied as proportions rather than tiny pixel
  requests, preserving usable Timeline height and respecting native minima.
- Empty transferred-content wrappers retire their discovery name immediately
  after native restoration, preventing subsequent restores selecting a stale Mixer.
- Source and Media Pool keep their individual headers when they are neighbors
  inside a tabbed section. Embedded and floating groups now use the same rule.
  Long section-tab labels elide on the right. Docking transition logs include
  pointer/candidate geometry for the remaining target-discovery investigation.
- The optional Jordan Tetewsky Preset uses Jordan's current saved layout: Media
  Pool above Source on the left, Timeline in the center, Program on the right;
  explicit hidden neighbors are retained. Machine display metadata is omitted
  from the shipped copy. The user's local files are untouched.

## Installation behavior

A fresh profile starts directly in OpenGate's immutable Resolve Layout, with
native controls, Timeline, independent Mixer and header/footer bands. Jordan's
preset is optional. Existing profiles keep their last working arrangement and
their saved presets; an upgrade is not a layout reset. Installer seed/no-overwrite
checks and the assembled first-open/restart flow cover these distinct cases.

## Acceptance and limits

Jordan reports Mixer mostly working, the default's initial appearance mostly
correct, and the header follow-up working, then requests this checkpoint. These
are source-behavior results, not exhaustive acceptance of a newly packaged binary.
Docking target discovery remains under investigation. Floating Source blink is
deferred polish; its ineffective trial remains isolated and is excluded here.
The earlier native GPU crash is not claimed conclusively eliminated. Windows
runtime parity and the rest of the repair checklist remain open. This release includes macOS and Windows installers using the shared managed
workspace. Windows runner results and live acceptance are reported separately.

Shared responsibility boundaries and detailed evidence are recorded in
[the Mixer repair note](opengate-mixer-panel-repair-20261026.md). Release build and
fixture receipts identify the exact clean source commit. No settings, hotkeys,
layouts or currently installed binaries are changed by building this package.
