# macOS native tests

Native loading probes and Qt panel/window regression tests live here. The full
shared-core Mac backend and adapter remain unfinished.
See [platform status](../../../doc/platforms/macos.md).

Build and self-test the startup helper without Resolve:

```sh
bash infrastructure/macos/build-loading-probe.sh
```

The helper writes a PID marker when loaded. It has no Qt dependency and does not
change panels. A passing host test does not prove that hardened Resolve will load it.
After quitting Resolve normally, the optional live experiment is:

```sh
bash infrastructure/macos/run-loading-probe.sh
```

This starts the installed executable with a process-local library-loading request.
It does not modify or re-sign the app. Check the printed marker file for evidence;
an empty file is not success. The window backend is still unimplemented.

## OpenFX discovery probe

The installed Resolve OpenFX SDK provides a native plugin entry point. Build a
discovery-only bundle with:

```sh
bash infrastructure/macos/build-ofx-probe.sh
```

The build checks the two exported discovery functions in an isolated test host.
Install `build/macos/ofx-probe/UnboundLoadingProbe.ofx.bundle` under
`/Library/OFX/Plugins/`, then start Resolve normally. The probe advertises zero
effects and writes process identity, main-thread status and Qt singleton presence
to `~/Library/Logs/OpenGate/ofx-loading-probe.log`. No Qt functions are called and
no widgets are changed by discovery. When Qt is available, the probe retains its
library and queues a read-only active-window check on the main dispatch queue.
A scanner process entry alone does not prove access to Resolve's UI. Remove this
bundle after quitting Resolve to uninstall the diagnostic.

## Widget inventory

Prepare matching development headers once (no Qt runtime build or installation):

```sh
git clone --depth 1 --branch v5.15.2 https://github.com/qt/qtbase.git build/macos/qtbase-source
cd build/macos/qtbase-source
./configure -opensource -confirm-license -nomake examples -nomake tests -no-openssl -no-dbus -no-opengl
perl bin/syncqt.pl -version 5.15.2
cd ../../..
bash infrastructure/macos/build-widget-probe.sh
```

Build the native IO helper after the widget probe and install that bundle with Resolve closed;
see the platform status document. The OpenFX bundle is retained for diagnostics. The optional
widget inspector loads only in the Qt host on the main queue and links to the
installed Resolve Qt libraries. It requires runtime Qt 5.15.2. Three snapshots,
after 2, 10 and 30 seconds, record widget names, classes, parent names, visibility
and geometry under `~/Library/Logs/OpenGate/widgets-{1,2,3}.json`. Open a project
during this interval to include its panels. These are local diagnostic files;
review them before sharing. Snapshot capture does not modify widgets.

The experimental control window also offers separate Undock/Dock buttons for
Inspector, Timeline, Source Monitor and Program Monitor. Open the Edit page and
enable dual viewers before detaching Source Monitor. Panels move only when the
corresponding button is clicked; closing a host docks its panel. Playback and
native viewer rendering still need validation in Resolve.

Run native session fixtures after preparing the headers above:

```sh
bash infrastructure/macos/test-panel-sessions.sh
```

These create temporary Qt test windows using the installed runtime. They verify
layout slots, stretch/alignment, grid spans, splitter order/sizes, 30 repeated
cycles, close-to-dock, page visibility, independent three-panel operation,
unsupported/hidden target rejection, source-only stack detachment, source selection
changes, panel-specific Dock button state, restoration on session destruction,
structured diagnostic records and repeated control-window relaunch requests.

The workspace branch also tests seven multi-part sidebars (Media Pool, Index,
Effects, Metadata, Mixer, Sound Library and Keyframes), all eleven targets together, cancelled bulk commands,
native toggle initialization, atomic saved-state reload, hidden-panel recall,
control pinning, disconnected/negative-coordinate display recovery, native
standard/compact/hidden title styles, and mouse-event resizing without a title bar.
Tests keep settings and logs in temporary directories and do not edit the user's
saved workspace.

Hierarchy fixtures verify the actual native front-to-back order and key-window
identity while alternating Source/Program activation and activating Timeline.
They also cover hidden panels, pinned controls, disabling hierarchy and modal
dialogs. Quick Export fixtures verify native action dispatch without rendering.
