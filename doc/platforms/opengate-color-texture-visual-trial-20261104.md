# Authorized color/texture visual trial — November 4

Jordan requested a temporary install to try textures and colors, while the full
native persistence/Undo work remains open. The numbered native-entry contract
was reread, the task audit updated, and existing assembled entry paths retained.
This explicitly authorized visual trial does not close the full feature gates.

## Installed and loaded

- Initial source `6634ee90133b05112f94261fd5480b47f58bbf0b` was installed and
  launched with process-local flags. A later ordinary launch omitted those flags,
  so the manager and custom menus were absent. Build loading alone was insufficient
  readiness evidence; the trial activation must also be verified.
- Corrected local-trial source `035838e4e4ce2fd75f9280c6a2853dd0eb227727`
  activates its own configuration before controller construction. Normal builds
  do not compile this activation path. It writes only the current process's
  environment, with no global launchctl environment or user-profile mutation.
- The ordinary launch, without command-line trial flags, records
  `color-trial.activation.ready`: manager and paint enabled, correct sample palette.
  One helper starts in process 3069/session
  `5486ac9a-b5b7-42a0-8a96-ca263eb95e2c` at 20:27:42 UTC.
- Both Mac architectures, strict bundle signing, 25 installed managed hashes,
  exact rollback rehearsal and 197 unchanged protected paths verify before launch.
  New runtime SHA256:
  `0a1aa5e10bad2e0f77a76ec0e36a5c01fd95822b59b1029c89c4c4110b67f39f`.
- Latest transaction `DF31A110-3AAE-4D8A-B485-87B2A503176F` restores 6634ee9.
  Transaction `BB6FF56C-3755-4F08-83A8-A698ECB5B0CD` restores the best-known
  660cdfd baseline. Older 7b191f8/fractured checkpoints remain intact.

Receipts/stages: `build/color-texture-trial-startup-20261104/` and the initial
`build/color-texture-trial-20261104/`. The latter owns the separate copied layout,
sample palette, pending drafts, texture preference and diagnostics. Builder edits
and the ordinary OpenGate profile were not replaced by this installer.

## Trial actions and limits

Open a test project/timeline, select a clip, then choose a sample under the native
Clip Color submenu. OpenGate UI has Clips & Tracks Color Manager above Original
UI; flat Panels access and the color-menu manager entry use the same managed
panel. Edit a preset and use Update Preset; the toolbar texture toggle shares the
manager's preference. Native stock definitions remain locked.

Assignments are display bindings and clear on process exit. Original UI parity,
canonical checkmarks, native Undo/Redo for custom assignments, saved-project/DRP
association, track-lane tinting, active native timeline ID/tab agreement and
matched playback performance are not accepted. Native alpha is unsupported by the
paint trial; the editor retains it. Media Pool custom coloring remains disabled.
User testing must establish actual clip-background/foreground/playhead behavior;
fixture passes and the activation log do not establish native paint acceptance.

## Organization and validation

`ColorVisualTrial` owns one-time process configuration; the entry point wires it
before creating the existing controller, and the Mac builder explicitly opts in
with a local directory. No second palette, controller, docking engine, event
filter or project writer was added. Missing-profile and empty-launch-environment
startup checks pass, including loading the intended palette. Shared manager,
rename, scale/menu and color renderer fixtures are indexed in the implementation
checkpoint. The startup adapter is Mac-only and does not alter Windows callers.

No main merge or installer/package publication. This is an installed local
preview, pending live acceptance, not the completed persistent color feature.
