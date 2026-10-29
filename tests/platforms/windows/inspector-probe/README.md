# Read-only Inspector sizing probe

Build from the repository root:

```powershell
cmake -S tests/platforms/windows/inspector-probe -B build/windows/inspector-probe -G "Visual Studio 17 2022" -A x64 "-DCMAKE_PREFIX_PATH=$env:USERPROFILE/Qt/5.15.2/msvc2019_64"
cmake --build build/windows/inspector-probe --config Release
.\build\windows\inspector-probe\Release\inspector-probe.exe
```

The launcher loads `InspectorProbe.dll` alongside any existing Unbound helper.
It queues a read-only Inspector snapshot on Resolve's Qt GUI thread. It does not
change selection, text, layout, or size policies. The DLL stays loaded until
Resolve exits; another run takes another snapshot. Restart Resolve before using
a rebuilt probe. Keep the probe separate from normal release binaries.

JSON output appears beside the probe executable as `inspector-PID-TIMESTAMP.json`.
It includes the Inspector slot, floating Inspector, ancestor sizes, widget names,
classes, size hints, layout constraints and Qt properties. Captures can include
the selected clip's filename. The 4,000-widget cap is reported by `truncated`;
do not treat a truncated capture as a complete Inspector tree.

## Findings on Resolve 21.1.1.10, 2026-10-12

The live floating Inspector's complete title-bar branch was captured, although
the full Inspector tree exceeded the cap. `frameEffectsWidgetsTitle/labelTitle`
is a `BmdDavUI::UiElidedLabel`, with `elideMode = 2` (`Qt::ElideMiddle`) and an
Expanding horizontal policy. With the selected long filename its minimum-size
hint was 619 pixels. This propagated to a 765-pixel header minimum-size hint and
an actual 765-pixel minimum width on the floating host.

The native middle-elision mechanism is already enabled. The fix is to
relax only this title label's horizontal size-hint contribution while floating,
letting it fill available space and restoring the original policy on docking.
No filename rewriting or replacement label is needed. `SidebarFloatSession`
now applies the targeted policy change, with a regression fixture for shrinking,
text updates and policy restoration. Fresh-session Resolve validation is still
required for the resulting release.
