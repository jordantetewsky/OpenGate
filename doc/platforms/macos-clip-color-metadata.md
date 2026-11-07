# Custom hue metadata probe

## Current per-instance storage gate — November 6, 2026

### November 7 native compound checkpoint

Actual key lookup now passes absent/present controls and exact payload hashes on
one owned audio instance. The key-specific compare/write path preserves a second
unrelated native parameter. A real native compound submits Orange, updates paint
A to B, then returns to the same Blue fallback. Both native color calls return
true, commit/destructor complete, and post-commit readback contains exact paint B
with the unrelated parameter intact. Receipt:
`build/native-color-canonical-history-20261106/phases-20261107-001232-e64396/receipt.json`,
SHA256 `605c55031aaf3d6ca1a9343854c4e1430a228ad2b80be683c61b904021bc3e45`.

The run stops **before any Undo trigger**: its menu lookup returns no candidate.
Custom Undo/Redo and abort are therefore still untested, not disproved. The owned
project closes successfully. A separate abort attempt then refuses an editing
project before any import/write; it leaves that project untouched. Normal UI
assignments remain temporary and do not use the saved carrier.

The next isolated stage uses the previously observed exact native action IDs
`editUndo`/`editRedo`, independent of contextual display labels and menu ordering.
Its optimized menu fixture covers 600 menus, alias deduplication, changing labels,
stale expected text, disabled and ambiguous actions. Six external-coordinator
checks and five codec tests pass offline, including raw/Zstd semantic equality,
map-order variation, every truncation, duplicate pairs and the actual saved
canonical receipt. These are test-helper results, not custom history acceptance.

### Evening native result: canonical backing carrier passes

The live `ImportExportMetadataBA` Qt write route below is disproved as the editing
path: it reads back in memory but is absent from the native saved export. Its
serialization shadow is distinct from the canonical editable record.

An owned generated-audio test now passes actual insertion, save/close/reopen and
native DRP export/import through `Sm2TiItemLockableBlob`'s `BtMediaMetadata`
container. The full JSON paint payload, an unrelated canonical test key, legacy
metadata fixture entries and native Blue fallback survive. Native clip UUIDs
change on DRP import; the payload remains on the correct remapped instance via
its native backing-record owner. No marker carrier or archive patching is used
for the live writes. Receipt:
`build/native-color-canonical-write-20261106/phases-20261106-211520-9094b3/receipt.json`,
SHA256 `5a7a7ba4fa894d69a805a365c2708ba6ba7ccf434f99d543a1d69f07dbd13cef`.

Actual native-menu `editUndo` and `editRedo` also pass the separate stock-color
Blue/Purple control. Receipt:
`build/native-color-canonical-write-20261106/phases-20261106-205353-6276d1/receipt.json`.
This is **not** custom-metadata Undo/Redo acceptance. Normal palette commands do
not yet use the canonical writer; automatic paint restoration, custom history,
video instances, native recipient edits, plugin-absent handoff and platform/
version parity remain open. The carrier is an exact-image experimental native
API path, not an established public extension contract.

Native `FieldsBlob` contains a QDataStream map with `BlobData`, then a version
10001 envelope. Its `0x80` raw / `0x81` Zstd marker is outside protobuf data.
Therefore the observed raw color tag is field **5**, not the earlier tentative
field640 interpretation that included the marker. Canonical metadata is outer
field3 containing a version1000001 envelope; named pairs use metadata field3,
with key/value fields1/2. Multiple named-pair ordering can vary between native
serializations, changing compression bytes without changing state. Compare exact
decoded keys/values and all other observed native fields, rather than raw archive
hash equality. The bounded fixture decoder preserves unsupported-field bytes and
rejects malformed/truncated input; it is not yet a production decoder.

Native key lookup `10012649c` has been identified statically: it returns a borrowed
BtString pointer, with one static miss object at `slide+11d8a4af0`. The first canonical
write fixture used freshly generated keys with external decoded-baseline checks;
its raw `before.contains(key)` check cannot prove absence under compression and
must not be promoted as a production no-overwrite contract. Raw serialized hashes
also cannot serve as stable revisions for multi-key records. The next native
gate is atomic A/B metadata and fallback restoration through real Undo/Redo,
including same-fallback changes and abort behavior.

The remaining paragraphs record the earlier investigations and their limits.

Jordan clarified that marker custom-data was an analogy for another hidden field:
**do not create hidden or visible marker carriers**. The saved record should hold
the current assignment and exact paint/fallback information; native history must
handle changes. No custom append-only color-history log is a substitute.

An earlier generated-audio test did verify a different carrier: timeline
MediaPoolItem metadata survived reopen and native DRP export/import, but project,
timeline, source and instance IDs changed on import. Receipt:
`timeline-content-candidate/build/color-timeline-metadata-native-20261104-absolute/receipt.json`.
That result is useful for a timeline-level palette catalog. It did not prove
portable per-instance association, rendering or native Undo. Do not describe it
as a successful custom-clip-color handoff, or describe all metadata storage as
untested.

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

For the next native import check, an offline copy of that same generated-audio
DRP now contains a 604-byte, Qt-encoded per-instance fixture payload and an
unrelated preservation field in `ImportExportMetadataBA`. No markers were added;
the original DRP and every unrelated archive member are unchanged. The verified
fixture is `build/native-color-disposable-render-20261105/seeded-d-r-p/generated-audio-hidden-data-v2.drp`,
SHA256 `03ac4c2d5fc8b595edb1ef2511bc674eaafd609e89888a2ed48256c5430eb867`.
The unversioned earlier build attempt is unverified and must not be used. This
is a diagnostic input, not a production project writer or native acceptance.

## Earlier source-media probe

The installed scripting SDK declares `MediaPoolItem.GetThirdPartyMetadata(key)` and `SetThirdPartyMetadata(dict)`. This is a storage candidate, not an arbitrary-color rendering API.

`probe-red-clip-color.py --metadata --run --output <scratch-output>` uses the existing guarded disposable-project path and generated silence. It refuses to switch, save or mutate an already-open project. `clipcolormetadataprobe.py` owns the namespaced payload/key and readback. The caller owns project creation, save/close/reopen, clip-count/identity/start/end/duration checks, supported native control color and DRP export. A 30-second worker deadline and receipts retain failures. Do not run against an editing project.

The probe writes one additive `com.cinewaltz.opengate.clip-color-probe.v1` metadata key with Bright Red/#FF0000 and an Orange native control fallback. Orange is a test control, not a claimed nearest-color production mapping. Reopen verifies exact payload and unchanged clip structure. It never deletes or replaces clips.

Open-project refusal and Python compilation pass. Live storage execution has not run. DRP import/metadata portability, per-timeline-instance identity (metadata is on source media), rendering, undo and nearest native fallback remain unverified. No Red/custom-color UI is exposed by this branch. Users without OpenGate would still need ordinary supported native clip values; metadata alone cannot render the hue.
