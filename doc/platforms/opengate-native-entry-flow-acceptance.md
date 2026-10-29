> Historical assembled-flow contract: reconcile its mode terminology with the current unified-flow requirements before implementation acceptance.

# OpenGate workspace entry: current acceptance contract

This supersedes the earlier native-slot-first interpretation. Implementation and
fixture results are not live Resolve acceptance. Detailed evidence and gaps live
in opengate-workspace-flow-v2.md and opengate-task-audit-[checkpoint].md.

1. First launch enters OpenGate in Resolve Layout, reusing the existing emulator
   panel tree and real native controls. No emulator selection, background switch
   or Panel Manager setup. Later launches restore the last working state for each
   supported page, including unsaved modifications while Resolve Layout is selected.
2. OpenGate Workspaces lists the immutable Resolve Layout first, then saved user
   workspaces. Update Current Workspace is disabled for that built-in and unnamed
   views, enforced below the UI. Save as New creates a writable user workspace.
3. Use Original Resolve UI restores the real native layout/owners, preserving the
   last complete OpenGate checkpoint. Selecting an OpenGate workspace re-enters it.
4. Hotkeys and temporary backtick maximize/restore remain usable in Native mode;
   temporary expansion does not switch modes or load a default arrangement.
5. A persistent title drag, detach or grouping action in Native enters OpenGate
   and completes the gesture without loading Resolve Layout or resetting unrelated
   panel positions. Native-origin changes form an unnamed working arrangement.
6. In OpenGate mode, the permanent main-window top strip holds window controls,
   project title and Panels even when Workspace Toolbar is hidden. In Original
   Resolve UI, hide the OpenGate Panels title accessory, including during temporary
   Native maximize; restore it on OpenGate re-entry. This is the maintainer’s explicit
   2026-10-23 clarification following its overlap with a native Inspector button.
   Floating panel titles stay panel
   titles; neither Panels nor the project label migrates to Inspector. One primary
   workspace owns the existing native-style Workspace Toolbar above its panel
   area and complete native Page Navigation below it, including Home and Settings.
   Floating groups never take ownership based on size, focus or Timeline location.
7. Each band is independently pinned or hidden. Hiding releases its height;
   showing reserves it. Docked content and maximize use the actual content viewport
   with both bands, either one or neither. Ordinary floating windows may overlap.
8. Drag visible panel titles as well as detach buttons. Ordinary native clicks
   retain their behavior. Remaining docked content reclaims vacated space; native
   visibility updates cannot reopen empty leased slots. Detach does not shrink
   the user's primary window. Native return restores exact owners and constraints.
   In OpenGate mode, X on a floating panel or group hides its retained placement;
   Panels or its shortcut recalls the same host, geometry and group. Closing must
   not dock into a native slot. Docked hide/show keeps its existing behavior;
   dragging or an explicit placement control changes docking. This is the maintainer’s
   agreed 2026-10-23 clarification and supersedes earlier close-to-dock policy for
   OpenGate mode. Original Resolve UI retains its existing close behavior.
9. Panels is a flat current-page inventory including Workspace Toolbar and Page
   Navigation. Optional parked panels must be reachable. No All Panels, Edit,
   Workspace or Floating Windows inventory subtrees. Header controls cannot overlap
   title/Inspector or become unreachable. Main title-bar Panels remains available
   with the toolbar hidden, in addition to menubar access. Timeline and its toolbar
   remain one arranged element; use overflow when narrow. Mixer sits next to Timeline and keeps its own
   placement control/title drag. It follows the same independent float, neighbor
   and tab rules as other panels. Each panel
   has one readable title; no internal widget identifiers or duplicate title rows.
   Explicit navigation commands reveal their destination using this same panel
   visibility model: Find in Media Pool reveals Media Pool, Match Frame reveals
   Source, and Reverse Match Frame reveals Timeline. Retain floating geometry and
   group membership; show a hidden group and select every nested tab on the path
   to the destination. Resolve executes the native command once; OpenGate never
   replays it. Ordinary background updates do not reveal panels. Stabilize is an
   operation, not navigation: keep Inspector hidden and the selected tab unchanged.
10. Maximized panel contents fill the available content area without clipping;
    maximizing only the outer host is insufficient, including for Mixer.
    Native painting must remain correct throughout resize, stacked timelines,
    docking and maximize. A blank timeline until a width threshold is a live bug;
    dummy-widget fixtures cannot prove this fixed. Preserve native minima or
    provide reachable overflow instead of breaking painting.
11. Supported pages retain their independent working state on departure/return.
    Unsupported pages keep native behavior. Fusion adapters follow separately,
    sharing these contracts; Windows parity needs its own integration and tests.
12. Exercise the assembled first-open -> title drag -> vacancy reclaim -> band
    combinations -> maximize/restore -> Save/Update protection -> restart -> Native
    -> temporary maximize -> persistent Native gesture -> OpenGate selection flow.
    Then live-test actual pointer gestures, native GPU rendering, page switching,
    remapped keys and Relay/Deliver before release acceptance. Keep build, fixture,
    installation and live acceptance evidence separate. No overstatement.
