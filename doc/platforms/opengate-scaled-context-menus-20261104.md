# Context menus in scaled content — November 4

Jordan reports Media Pool right-click menus failing above 100% GUI scale and
requires menus to remain usable at every supported scale. Runtime logs confirm
the Media Pool canvas was embedded at 168% on installed source `660cdfd`.

## Reproduced boundary and repair

Qt automatically embeds descendant popup windows of a graphics-proxy widget as
child graphics items. The original fixture reproduced this for menus parented
inside the scaled canvas: they were not native popup windows and did not appear
in `QApplication::activePopupWidget()`. That changes native menu tracking and
confines presentation to the outer graphics viewport. Menus parented outside
the canvas did retain native tracking. This is a reproduced component defect,
not proof of the exact failing handler in Resolve.

`ScalableContentViewport` remains the sole owner of reversible canvas embedding.
It explicitly embeds the canvas with `Qt::BypassGraphicsProxyWidget` in its
ancestry so Qt leaves descendant popup windows native. The prior flag is restored
on release. There is no new global event filter, popup controller, command replay,
layout engine, timer, scene repaint pass or persistent setting. Native menu
handlers, actions and selection remain owned by Resolve. Stock menus remain
native size while the canvas scales, consistent with the unscaled native toolbar.

## Evidence and limits

The existing panel-scale harness now includes a focused popup matrix for bin
widgets and nested graphics views, popup ownership both inside/outside the
canvas, other-editor focus, mouse context input, tracked native submenus,
screen bounds, original action execution once, and popup-policy restoration.
The pre-repair tracking assertion failed six times. The repair passes the full
panel-scale fixture under optimization and AddressSanitizer at 70%, 100%, 125%,
147%, 200% and 300%. Existing custom-painted glyph scaling, selection, inline
rename, scroll offsets and layout restoration checks also pass.

A temporary diagnostic QGraphicsScene subclass crashed when Resolve's Qt invoked
its context-menu override during painting. It was removed; the fixture now uses
an ordinary scene plus a QObject event observer. No production scene subclass or
native virtual-table assumptions were introduced. The original 300% failure in
the item-context diagnostic was not evidence of a production hit-test defect;
the supported scene event route is checked instead.

The shared viewport is used by Media Pool. Timeline uses its existing separate
input router, and other panels use metric scaling rather than canvas embedding;
this patch does not silently replace their paths. Windows shares the viewport
implementation but has not been runtime-tested here. Live Resolve acceptance
must still cover bins, thumbnails, lists, empty space and submenus in drawer,
docked and floating Media Pool layouts, across supported GUI/display scales.
No installation, user project mutation or claim of universal menu acceptance.
