# OpenGate

> **Development branch — incomplete features, not a release.** Start with the
> [current task list, findings and bug reports](doc/platforms/opengate-current-work.md)
> and [work-in-progress status](WORK_IN_PROGRESS.md). Audio-channel commands are
> not working end to end; the unified native UI is not implemented.

OpenGate by CineWaltz gives DaVinci Resolve movable panels, split/tab groups and
saved workspaces while preserving Resolve's real native content.

**[OpenGate 0.0.3 prototype.1 — macOS and Windows tester release](https://github.com/jordantetewsky/OpenGate/releases/tag/v0.0.3-prototype.1)**

This tester checkpoint improves native clip navigation, independent Mixer placement,
default layout proportions and panel headers, and includes the updated optional
Jordan Tetewsky preset. See the release page for downloads, verification and open issues.

## See OpenGate in action

On Open you should see something that looks like this:
<img width="1723" height="1088" alt="Screenshot 2026-10-26 at 3 34 57 AM" src="https://github.com/user-attachments/assets/c3a68115-2fb4-4f2c-a76e-a26b168026f8" />

It matches close to what you would natively see in Resolve.... You may notice this menu at the top- it includes two workspace presets, and one that is pinned to match something close to the native Resolve preset that allows you to always return to something familiar:

<img width="433" height="284" alt="Screenshot 2026-10-26 at 3 36 58 AM" src="https://github.com/user-attachments/assets/e6fe1587-47f1-440b-9bab-15b5b306c89f" />

You can grab and float windows, dock them magnetically etc.... You'll find that you get a freedom you never have experienced with the Resolve UI

<img width="1725" height="1091" alt="Screenshot 2026-10-26 at 3 38 37 AM" src="https://github.com/user-attachments/assets/28bfda09-60e0-49c2-b76e-b170db8dd0bf" />

You'll also find additional commands, such as toggle maximum restore which allows you to temporarily maximize and restore any window, stabilize clip, and fixed playhead.

<img width="426" height="125" alt="Screenshot 2026-10-26 at 3 40 04 AM" src="https://github.com/user-attachments/assets/243cacdd-7897-4204-9433-5a7b9d13d3e7" />
<img width="431" height="47" alt="Screenshot 2026-10-26 at 3 40 16 AM" src="https://github.com/user-attachments/assets/8bf976b0-4ab7-456d-9f6a-496fea3554c6" />

It's great being able to temporarily maximize windows and go back to your workspace with a hotkey.

<img width="1728" height="1083" alt="Screenshot 2026-10-26 at 3 41 13 AM" src="https://github.com/user-attachments/assets/c9a12a96-3638-426a-b4e8-663e699307f4" />

<img width="1728" height="1085" alt="Screenshot 2026-10-26 at 3 41 20 AM" src="https://github.com/user-attachments/assets/1c488919-b998-4620-a223-7c27366cec04" />

<img width="1728" height="1095" alt="Screenshot 2026-10-26 at 3 41 27 AM" src="https://github.com/user-attachments/assets/f43668e9-51fc-4e21-a3ac-0b246253bb93" />

## Install and first launch

1. Download your platform's installer from the release's **Assets** section.
2. Save your work and quit Resolve, then double-click the `.pkg` (Mac) or `.exe` (Windows).
3. Open Resolve normally. OpenGate should load automatically. A first OpenGate
   profile opens **Resolve Layout**; existing profiles retain their last working
   arrangement, including unsaved changes and user presets.

Mac: universal Apple Silicon/Intel, macOS 15+, Resolve 21 at its standard location;
Developer ID signed and Apple notarized. Windows: x64, Resolve Studio 21.1 with
Qt 5.15.2; installer unsigned. Windows free-edition startup is unverified. Automatic
startup is implemented and fixture-tested; actual Windows Resolve plugin discovery
still needs tester confirmation. The Start-menu OpenGate launcher is for recovery.
Please report failures to load automatically.

Drag panel titles to rearrange, float or group them. Use **Panels** in the permanent
main title strip to hide and recall panels. **Workspace Toolbar** and **Page
Navigation** hide independently. **Use Original Resolve UI** restores native
ownership while preserving the OpenGate arrangement. The built-in Resolve Layout
is immutable; use **Save as New** for a writable preset.

Assign commands in Resolve's **Keyboard Customization → Workspace → Workflow
Integrations**. The hovered-panel command is **OpenGate - Maximize-Restore Hovered
Panel**. Physical/remapped keys and GPU rendering require live testing on Windows.

## Send support logs

Reproduce the issue, note its time, then double-click the export entry:

| Platform | Export entry | Raw log folder |
| --- | --- | --- |
| macOS | Applications → OpenGate Tools → Export OpenGate Support | `~/Library/Logs/OpenGate` |
| Windows | Start → OpenGate → Export OpenGate Support Logs | `%LOCALAPPDATA%\OpenGate\Logs` |

Send Jordan the **OpenGate support ZIP created on your Desktop**, plus your local
time/time zone, Resolve version and edition, OS version, page/panel, reproduction
steps, expected result and actual result. Include a screenshot for painting or
geometry issues; on Windows also include display scaling and monitor arrangement.
Nothing uploads automatically. Review the ZIP before sharing it.

Events identify OpenGate, module, build, session and sequence, along with observed
input/signals and operation outcomes. A recorded click is not proof that Resolve
completed an operation. Mac export filters private paths and project titles from
its event summary; Windows includes bounded logs and native widget inventories.
Neither exporter copies projects, media or databases. Mac prototype.3 logs stay
in `~/Library/Logs/Unbound`; prototype.4 export reads both folders.

## Uninstall and recovery

Quit Resolve, then use **Applications → OpenGate Tools → Uninstall OpenGate** on
Mac or **Start → OpenGate → Uninstall OpenGate** on Windows. Presets, shortcuts and
arrangements remain. Ordinary uninstall/reinstall does not create a fresh profile.
Keep a verified backup before any separately planned first-user demonstration.
Removing only the launcher does not remove the resident plugin.

## Verification and development

Read the [0.0.3 tester release and expected workflow](https://github.com/jordantetewsky/OpenGate/releases/tag/v0.0.3-prototype.1)
and the [checkpoint notes](doc/platforms/prototype003-v1-checkpoint.md).
Builds and fixtures do not establish installation or live acceptance. Jordan accepted
the tested Mac behavior as a checkpoint; broader native rendering and input testing
remains necessary on both platforms. Floating Source blink remains deferred polish;
docking target discovery remains under investigation. No performance improvement is claimed.
Prior releases remain available for recovery.

New cross-platform features require one behavioral specification, deliberate
shared-code/platform-adapter boundaries, equivalent coverage, Mac regressions,
and explicit platform gaps and live-test status. Two successful builds are not
cross-platform completion.

## Acknowledgments

OpenGate is developed by CineWaltz, based on
[Wolfgang Woehl's Unbound](https://github.com/wolfgangw/unbound). Inherited copyright
and MIT notices remain in [LICENSE](LICENSE). Internal Unbound identifiers remain
where needed for compatibility. See the [historical upstream README](doc/upstream-readme.md).
OpenGate is not affiliated with Blackmagic Design.
