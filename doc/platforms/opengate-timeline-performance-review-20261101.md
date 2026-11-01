# Timeline rendering and pre-install performance review

Branch: `perf/timeline-render-work`, based on the uninstalled input/header repair
`651b1fd`. The user-requested `6c3ffb4` installation stays in place. This is local
implementation and fixture evidence, not Resolve playback acceptance.

## Production changes and organization review

`TimelineSceneRenderer` owns reusable section raster devices and bounded pixel
damage. It replaces the stateless full-frame renderer; that old path and its
extra composited frame image are removed. Physical pixel clearing and a section
device clip preserve exact glyph phase and prevent native painters that disable
their logical clip from escaping a section. Devices retain alpha; experimental
background flattening/alpha scanning were measured and discarded.

`TimelineSceneUpdates` owns native damage/geometry subscriptions and coalescing,
extracted from `TimelineContentViewport`. Pixel changes no longer rebuild header
geometry. Zero-duration single-shot timers merge pending geometry/repaint requests
at an event boundary; there is no polling loop, playback-rate cap or frame queue.
Damage complexity is bounded, destroyed/rebound nested views are pruned, hidden
presentations release pixel buffers, and reversible native exposure flags restore.

`TimelineContentViewport` wires these components and presents section slices;
`TimelineTrackHeaderLayout` maps native damage through its existing pinned/header/
body/footer geometry. Inline native editors use their own small raster rather
than copying the entire Timeline frame. Native scene/model ownership, handlers,
settings writers, panel command registration and platform adapters are retained.
The shared Qt adapter is used by both platforms; Windows compilation/runtime and
actual Resolve GPU behavior are unverified here.

The helper build now uses `-O2 -g`; debug symbols remain available. The build
manifest records optimization. The existing test harness supports optimized
fixtures, preserving its legacy `UNBOUND_TEST_OPTIMIZE` environment name.

## Matched synthetic comparison

Both sides use the same Qt fixture, native fixture setup, compiler `-O2`, 2,880
synthetic clips, a 1440 x 850 logical presentation and Retina DPR 2. The previous
production headers come from detached, unchanged `651b1fd`; only the benchmark
entry point is supplied externally. Each scale has 20 exposures, 40 playhead
steps, 300 direct mouse moves and 20 pan steps. Numbers below are p95 milliseconds.
The measurements are an isolated local run, not a media decoding, native GPU or
end-to-end pointer-to-screen benchmark.

| Content scale | Prior playhead loop | New playhead loop | Prior full pan | New full pan |
| --- | ---: | ---: | ---: | ---: |
| 70% | 30.956 | 5.334 | 52.312 | 19.609 |
| 100% | 31.071 | 3.167 | 56.962 | 14.375 |
| 147% | 34.965 | 2.946 | 50.884 | 12.477 |
| 213% | 28.568 | 1.192 | 49.480 | 11.487 |
| 300% | 49.714 | 1.173 | 81.609 | 23.907 |

Unchanged exposures paint zero native clips in the candidate. Direct mapped mouse
dispatch p95 is 0.016–0.019 ms with native cursor mirroring, versus 0.007–0.011 ms
in the prior implementation. The playhead loop includes three Qt event-drain
passes plus a forced partial presentation; panning includes native scrolling,
three event-drain passes and a forced full presentation. This deliberately counts
native fixture work as well as presentation work. Raw local outputs:
`build/timeline-performance-review/baseline-651-O2.json` and
`build/timeline-performance-review/final-isolated.json`.

Full panning at 70% and 300% still exceeds a 16.67 ms / 60 Hz frame budget in this
workload. No claim of smooth playback at arbitrary frame rates, media or hardware
is justified. The benchmark's FPS budget table is arithmetic, not observed native
playback. No native decoder, playback setting or project frame rate was changed.

## Verification and native acceptance gate

Optimized Timeline and panel-scale fixtures pass. The Timeline AddressSanitizer
run passes. Exact fresh/incremental pixel comparisons cover backing resolutions
1.4, 2, 2.94, 4.26 and 6, including glyph movement, playhead movement and deletion.
Checks also cover unchanged exposure reuse, hidden buffers, no idle polling,
nested native damage and real geometry updates. The assembled native-entry fixture
passes, preserving ownership, native return and existing persistence boundaries.

Contract clauses 9–12 still require actual Resolve validation: native clip/track
painting, trims and transitions, mapped native commands, page switching with
toolbar/stacked tabs, restart persistence and playback while editing. Fixtures
cannot establish those. A fresh Resolve process loading the candidate is required
for that remaining evidence. No new installation, live acceptance, merge, push
or publication is implied by this report.
