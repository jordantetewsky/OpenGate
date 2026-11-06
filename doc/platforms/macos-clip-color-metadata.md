# Custom hue metadata probe

## Current per-instance storage gate — November 6, 2026

The source-media experiment below is historical. A reused source file may have
several differently colored timeline instances; source metadata alone cannot
represent those assignments. Static inspection of Resolve 21.1.1 identifies
`Sm2TiItem.ImportExportMetadataBA` as a candidate per-instance Qt property, with a
versioned QDataStream map. The local disposable service merges only its probe
namespace and attempts exact restoration; it is not a production assignment
writer. Its actual SDK/native ID equality and reversible property behavior still
need live verification.

The first live harness failed during generated-media import, before clip creation
or a metadata write. The corrected external phased runner passes its offline
guards and a zero-write SDK preflight. It currently refuses the open working
project. Neither that preflight nor the static property audit proves project
save/reopen or an OpenGate → stock Resolve save/export → OpenGate DRP round trip.

Public SDK `TimelineItem.SetClipColor` submits a native Undo action. This does not
prove that custom metadata joins that same action. The disconnected custom-history
prototype is excluded from the installed test runtime; its atomic failure behavior
remains unsupported. Native fallback and exact custom state must eventually form
one coherent native history operation, with the actual round-trip and recipient
edit behavior verified separately.

Current receipts, runtime identity and remaining gates are recorded in
[the color update checkpoint](opengate-color-manager-update-flow-20261105.md).

## Earlier source-media probe

The installed scripting SDK declares `MediaPoolItem.GetThirdPartyMetadata(key)` and `SetThirdPartyMetadata(dict)`. This is a storage candidate, not an arbitrary-color rendering API.

`probe-red-clip-color.py --metadata --run --output <scratch-output>` uses the existing guarded disposable-project path and generated silence. It refuses to switch, save or mutate an already-open project. `clipcolormetadataprobe.py` owns the namespaced payload/key and readback. The caller owns project creation, save/close/reopen, clip-count/identity/start/end/duration checks, supported native control color and DRP export. A 30-second worker deadline and receipts retain failures. Do not run against an editing project.

The probe writes one additive `com.cinewaltz.opengate.clip-color-probe.v1` metadata key with Bright Red/#FF0000 and an Orange native control fallback. Orange is a test control, not a claimed nearest-color production mapping. Reopen verifies exact payload and unchanged clip structure. It never deletes or replaces clips.

Open-project refusal and Python compilation pass. Live storage execution has not run. DRP import/metadata portability, per-timeline-instance identity (metadata is on source media), rendering, undo and nearest native fallback remain unverified. No Red/custom-color UI is exposed by this branch. Users without OpenGate would still need ordinary supported native clip values; metadata alone cannot render the hue.
