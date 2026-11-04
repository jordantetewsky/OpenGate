# Active timeline and displayed tab identity — November 4

Jordan requires the active native timeline ID to match the highlighted/displayed
timeline tab. Caption equality, display index and a passing paint fixture are
not substitutes for that native identity agreement.

## Bounded offline observations

The retained Resolve ARM64 image (SHA256
`801848888e276e40cefcc89011155702ddf67bb6ecfccff4de60ff505685e2a9`)
has a concrete handle-bearing path. `UiTiLayoutContainerImp::OnTabBarClicked`
reads `QTabBar::tabData(index)`, casts it to native `UiTiSequenceTab`, and emits
`TabSelected(SequenceViewId,Sm2SequenceHandle)`. The view ID comes from the stored
`m_TiTimelineView` pointer created by `setupUi`. `UiConformerPanelImp::OnTabSelected`
forwards that handle and view ID into the central switching path.

The inspected entry and receiver do not retrieve the view with a generic
layout-slot/currentWidget cast. Replacing a native layout slot with a presentation
marker alone is therefore not established as the failure. The sequence tab has
no own Qt identity property/invokable getter. Observing its QObject in tabData
does not establish persistent timeline UUID equality.

The scratch DRP corroborates distinct identities: SDK Timeline.GetUniqueId equals
`Sm2Timeline.DbId`; nested `Sm2Sequence.DbId` and `UniqueSequenceId` differ. The
sequence's saved `Parent` points to the timeline DbId. Qt metadata exposes Parent
as `DbObj::HasOne`, but safe handle-to-parent-model-to-DbId access and actual signal
ordering remain unverified. Do not substitute UniqueSequenceId for SDK timeline ID.

Offline findings and a verified manifest remain in the color worktree at
`build/rea-clip-colors/timeline-tab-identity-20261104/`. Manifest SHA256:
`032733a59a8bf65763910537d0f13eef7d96976ff49650cc4c5deb15e994f703`.
This is static shell/artifact analysis, not new REA provider or runtime evidence.
No native offsets were invoked and no production identity adapter was invented.

## Presentation cache check

The focused fixture reuses the existing TimelineViewportFixture and actual
TimelineContentViewport, swaps nested scenes eight times without replacing the
root or resizing, and checks displayed pixels plus damage from later edits.
It passes on the installed baseline source. No production cache repair was
justified by that result. It does not simulate native timeline IDs or establish
semantic tab identity.

Remaining native observation: correlate ordered handle-bearing switch signals,
the selected tab's native object, view/root/nested-scene sets and the active SDK
timeline UUID using a verified read binding. Cover rapid switches, duplicate
names, renaming, reordering, close/reopen and multiple stacked timelines before
closing this acceptance item. No Resolve/SDK/UI access occurred in this pass.
