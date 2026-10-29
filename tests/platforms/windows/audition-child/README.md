# Fairlight audition: same-window child-host experiment

Build from the repository root:

```powershell
.\tests\platforms\windows\audition-child\build.cmd
```

Run `build/windows/audition-child/Release/Unbound.exe` in a fresh Resolve session
without another Unbound helper. This is a diagnostic build, not a normal release.
The build uses the existing icon, audits dependencies and tests manual transfer
and saved restoration through the real helper, including all nine Fairlight panes.
No production source files are changed. CMake generates a source overlay in the
build directory and fails if its replacement anchors no longer match.

Only Fairlight Media Pool changes its host type. Its native layout and contents
are still transferred, but the host is a `Qt::Widget` child of the Fairlight page.
It appears at (24,48) within that page, above the existing UI, without a Windows
caption or taskbar entry. Its existing Dock button restores the native layout.
Other panes retain normal floating behavior. Native toggle reclamation remains.
Child-host window geometry restoration, opacity fading, taskbar/caption setup
and geometry persistence are skipped because those operations are for windows.
The host is parented to the visible page rather than the slot, which the native
toggle hides. This preserves page ancestry, but not the original slot ancestry.

Settings use `%LOCALAPPDATA%/Unbound/audition-child.json`, separate from the normal
workspace (`UNBOUND_SETTINGS_FILE` still overrides this for fixtures).

## Live comparison

1. Start Resolve fresh, open the project and Fairlight page, then run this build.
2. With Media Pool docked, confirm normal audition audio and smooth waveform.
3. Start docked audition, then click Undock. Note whether playback stops.
4. In the child overlay, click audition Play. Check audible audio, waveform
   smoothness and Control Room meters. Click in Media Pool and test Space.
5. With timeline playback running, start audition from the overlay. Check whether
   timeline playback stops and audible audition takes over.
6. Click the existing Dock button to restore the pane.

The fixture verifies that the transferred content's `window()` remains the main
window and `page->isAncestorOf(content)` stays true, plus reversible docking,
native actions, page visibility and settings. It cannot verify Resolve audio.
Success would implicate the top-level window boundary or its associated native
window operations. Failure would keep transfer/layout lifecycle explanations in
play; neither outcome identifies a private Resolve API by itself.
