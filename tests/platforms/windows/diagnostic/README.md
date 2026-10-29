# Playback diagnostic build

This directory preserves the diagnostic launcher and controller from build
2026.10.12.010, together with the startup-mode definitions and six smoke tests.
It uses the shared pane headers in `src/core/`, Windows window backend in
`src/platforms/windows/`, and artwork in `src/assets/`; it is not a frozen
copy of every dependency. These sources are deliberately separate from the
normal build. Changes to the production controller are not automatically copied
here; keep the diagnostic copy in sync when investigating future behavior.

From the repository root:

```powershell
.\infrastructure\windows\build-diagnostic.cmd
# Optional Qt location:
.\infrastructure\windows\build-diagnostic.cmd -QtPath C:\Qt\5.15.2\msvc2019_64
```

The command builds the launcher, helper and smoke tests, then tests each mode in
a separate process. Output is `build/windows/diagnostic/Release/`, outside the normal
versioned releases. Product versions end in `.000-diagnostic`. Rebuilds replace
this development output; they never overwrite `release/win/`.

Start a fresh Resolve process for each mode, load the same project, and confirm
normal playback before running, for example:

```powershell
.\build\windows\diagnostic\Release\Unbound.exe --diagnostic=load-only
```

| Mode | Behavior |
| --- | --- |
| `load-only` | Load the DLL without invoking its bootstrap. |
| `bootstrap-only` | Also queue a GUI callback, but create no controller. |
| `no-placement` | Skip main-window placement restoration/tracking. |
| `no-snap` | Skip main-window snapping. |
| `no-decorations` | Skip pane discovery, decorations and restoration. |
| `no-polling` | Alias for normal startup in this optimized code; no recurring discovery timer exists. |

Other components remain enabled. An already injected DLL cannot be replaced by
running another mode. Keep `Unbound.exe` and `Unbound.dll` together. No Resolve
process is launched, stopped or injected by the build command itself.

## Investigation result

On Windows 10 / Xeon E5-2687W v4 / Resolve 21.1.1.10, build 009 reproduced the
playback failure with `no-placement`, while `load-only`, `bootstrap-only` and
`no-polling` were stable. An undocked pane also played normally with polling off.
Build 010 removed recurring discovery, cached widget references and used lifecycle
events instead; the user reported stable playback matching clean Resolve.
This diagnostic source preserves the optimized 010 behavior, not 009's old timer.
