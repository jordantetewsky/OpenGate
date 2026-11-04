# Rename command context — November 4

Jordan reported that typing a letter or Shift-letter during bin/clip renaming
could open an OpenGate panel. This repair remains isolated from palette work.
The installed `660cdfd` runtime is unchanged.

## Responsibilities

- `PanelCommands` continues to own command input routing; it claims text input
  before Qt's global shortcut activation and suppresses recognized OpenGate
  workflow/command shortcut targets while a text editor has focus.
- `CommandTextContext` owns bounded editing-interval evidence for delayed
  workflow requests. It observes focus and existing input delivery, adds no
  application event filter or polling, and never reads/stores editor contents.
- `WorkflowCommandBridge` still owns single-use file requests. It rejects a
  request issued during editing even if the editor has since lost focus, before
  popup deferral, without replaying or altering native commands.
- `PanelCommandCatalog` provides read-only shortcut ownership from the existing
  native preset snapshot, independently of command availability/focus.
- `workflowcommandidentity.h` contains the native action ownership policy shared
  by the observer and input router; the observer's duplicate local policy was
  removed. No second binding store or command controller was introduced.

Unrelated QAction shortcut targets retain delivery even when their key collides
with an OpenGate assignment. Claiming ShortcutOverride lets the text editor
receive its own key handling; it does not execute the OpenGate command.

## Verification

The extended existing workflow fixture reproduced native workflow activation
during bin and graphics-scene clip rename and delayed activation after focus
left the editor before the repair. The repaired optimized AddressSanitizer run
passes, including Shift-letter insertion, modified-key ownership, native Undo
target delivery despite a key collision, late workflow rejection and fresh
post-rename dispatch exactly once. It also covers graphics-item editability
changing without QWidget focus changing.

The existing optimized AddressSanitizer panel-command and native-preset suites
pass. The native-unmodified workflow observer fixture passes. An initialization
ordering failure found by those broader suites was corrected by constructing the
shared context owner before installing the routing filter.

These are hidden macOS Qt fixtures, not live Resolve acceptance. Shared core
callers on both macOS and Windows use the same router/bridge; no Windows runtime
test was available. Native AppKit delivery that bypasses Qt and publishes only
after an editor closes still needs live inspection; issue timestamps alone
cannot establish an earlier keypress time. No private native history/Undo API,
project state, installation or user keyboard preset was changed.
