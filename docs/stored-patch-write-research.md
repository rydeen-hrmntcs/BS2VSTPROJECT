# Stored-patch write protocol research

26 October 2026. Official Components client source inspected read-only. No MIDI was sent, no synth memory was changed, and no runtime sender was added.

## Primary source and reproducibility

The official [Components page](https://components.novationmusic.com/) currently loads [main-D3lNZfPT.js](https://components.novationmusic.com/assets/main-D3lNZfPT.js). Its page identifies app version `1.72.5+1798590e`. The fetched UTF-8 JavaScript was 4,102,899 bytes, SHA256 `2322dc9fd409e5910e69f83cf5a6d4127c71322ded94e9bbc99cbba4be5a673a`. Minified names below refer to this exact bundle. These are source observations, not a published firmware specification or a completed hardware write test.

## Exact message and destination

The bundle's `TD` constants explicitly name `REPLACE_CURRENT_PATCH:0`, `REPLACE_STORED_PATCH:1`, `REQUEST_CURRENT_PATCH:64`, and `REQUEST_STORED_PATCH:65`. Its `PD` offsets are protocol version 6, message type 7, patch index 8, payload 9; version is 0, payload size 144, full SysEx size 154.

The actual push helper `rJ.startPush(payload,index)` chooses stored replacement when index is defined, invokes `device.buildMessage([0,1,index,...payload])`, and sends the result to `mainOutput`. Omitting index chooses current-patch replacement type 0 and index 0. The device's `buildMessage` wraps arguments with `[240,0,32,41,productId[0],productId[1]]` and trailing 247; the Bass Station II class `Xre` supplies `Kre=[0,51]`. Thus the official single stored-patch write route is:

```text
F0 00 20 29 00 33 00 01 INDEX [144 payload bytes] F7
```

All bytes shown are hexadecimal. INDEX is zero-based 00–7F. Slot 1 uses 01. The single-send controller passes the selected destination directly into `patch.pushTo(device,index)`; its dialog template sets `entityCount=128` and `zeroIndex=true`. The bank sender independently iterates indices 0 through 127 and passes each directly to the same push method. This establishes receive intent and destination semantics from the manufacturer's transmitting implementation, rather than inferring them from an outgoing bank dump.

Important correction to earlier shorthand: offsets 9–12 are **payload**, not extra addressing-header bytes. For this project's captured profile those bytes happen to be zero. Preserve all 144 bytes at original message offsets 9–152 unchanged; do not reconstruct them from decoded controls. The slot model `uJ.makeSysEx(device,index)` independently builds the same stored message for integer destinations and masks the exported index with 127. An implementation should reject out-of-range destinations rather than imitate that mask.

All findings above are from the [official client bundle](https://components.novationmusic.com/assets/main-D3lNZfPT.js): search anchors `TD={REPLACE_CURRENT_PATCH`, `rJ=Gc.extend`, `makeSysEx(e,t)`, `Kre=[0,51]`, `buildMessage(e){const t=this.productId`, and `sendPatchToDevice=iA`.

## Readback, acknowledgement and pacing

The stored fetch helper `tJ.startFetch(index)` sends `device.buildMessage([0,65,index])`, so the request for slot 1 is:

```text
F0 00 20 29 00 33 00 41 01 F7
```

It listens before sending, accepts a 154-byte Novation/BSII response with matching byte-8 index, and extracts offsets 9 through 152. It also supports legacy 122-byte dumps by extending them, which this project's strict modern workflow need not accept. Fetch timeout is 1,000 ms. Its validator is relatively permissive; a project readback should additionally check exact response command/version, framing and all interior bytes.

The push helper calls `output.send(message)` then immediately resolves. It registers no acknowledgement listener or write timeout. Therefore successful dispatch is not evidence of storage completion. The bank-send loop waits 100 ms after each push in production (`aAe=100`), but no single-write minimum settle time or persistence guarantee is established. These observations come from `tJ=Gc.extend`, `rJ=Gc.extend`, `aAe=`, and `sendPack=iA` in the [same official bundle](https://components.novationmusic.com/assets/main-D3lNZfPT.js).

## Concrete bounded validation and remaining unknowns

The source establishes a concrete command for an explicitly approved slot-1 replacement/restoration test. Before sending, retain fresh current-buffer and full-bank backups, display the source hash/destination, construct exactly one 154-byte packet with INDEX=01, preserve payload offsets9–152, and validate offline. Read the original slot independently first using the documented stored request and compare with the fresh bank entry. After the approved replacement, independently request stored slot1 and obtain a complete bank readback to verify every other slot stayed unchanged. Restore the original slot using the same route and repeat both comparisons. Do not label any write successful on MIDI dispatch alone.

Still unverified on this hardware: whether its firmware accepts this stored-write route, persistence beyond the edit buffer/reselection or restart, precise settle time, any undocumented write failures/acknowledgements, and interruption behavior. Full-bank restoration, tuning stores, AFX overlays and separate sequence-memory coverage remain outside this finding. No bulk-transfer authorization follows from this research.


## Exact source excerpts retained locally

The fetched official asset and selected source excerpts are retained under ignored `validation-results/stored-patch-protocol/`, with URL/hash/byte count in `source-evidence.json`. These files contain public application source only; no user patches.

The following excerpts are copied from the [official bundle](https://components.novationmusic.com/assets/main-D3lNZfPT.js), with unrelated code omitted between excerpts:

```javascript
PD={MESSAGE_PROTOCOL_VERSION:6,MESSAGE_TYPE:7,PATCH_INDEX:8,PAYLOAD:9}
TD={REPLACE_CURRENT_PATCH:0,REPLACE_STORED_PATCH:1,REQUEST_CURRENT_PATCH:64,REQUEST_STORED_PATCH:65}
Kre=[0,51]
// Xre (Bass Station II):
get productId(){return Kre}

// Shared device builder:
buildMessage(e){const t=this.productId;if(!t||t.length<2)throw new Error("product id is not valid");return new Uint8Array([240,0,32,41,t[0],t[1]].concat(e,[247]))}

// rJ.startPush body:
let s=sJ;void 0===t&&(s=iJ,t=0),this.sendMessage([s,t,...e])
// rJ.sendMessage body after obtaining output n and device t:
const i=t.buildMessage([0,...e]);n.send(i),this.resolve()

// uJ.makeSysEx:
makeSysEx(e,t){const n=new Uint8Array(this.binary),i=Number.isInteger(t)?lJ:aJ;return e.buildMessage([0,i,127&t,...n])}
```

Here `sJ` and `lJ` are destructured aliases of `REPLACE_STORED_PATCH`; `iJ` and `aJ` alias `REPLACE_CURRENT_PATCH`. The single-send controller calls `e.pushTo(this.bassStationII,t)`, where `t` is the resolved destination selector result. Its compiled template pairs `["@entityCount","@entityName","@zeroIndex"]` with `[128,"Patch",true]` among the dialog arguments. The bank loop calls `t[e].pushTo(this.bassStationII,e)` for every zero-based array index. These independent source paths establish that the stored destination is passed as the zero-based index without an increment.


## Scoped hardware acceptance — 26 October 2026

The user authorized temporarily replacing/restoring stored preset **1**. Current
edit-buffer sound was preserved before the Components session. The user used
Get Bank, saved the bank to their Components account, then separately downloaded
`Downloads/BSII-bank-before.syx`. Save in Components is an account operation;
Download creates the local SysEx. The strict reviewer accepted all128 entries
and 19,712 bytes. A separate private copy was retained. Bank SHA256:
`675ee6c6bced2facd0b880a58b1bbe8bdfd2587237d774d78d5c61dc64a8a232`.

After the user physically selected preset1, fresh edit-buffer data matched its
extracted temporary patch exactly. The above stored request for slot1 then
returned a strict 154-byte stored-shaped reply matching its original bank entry.
A scoped reader requested slots0–127 individually, validating exact version/type,
returned index, length, framing and 7-bit interior. The complete result matched
the independent Components download byte for byte before writing. Reads were
paced 100ms apart with a 3s per-reply timeout (application policy, not a proven
firmware timing limit).

The test packet preserved entry11's complete 144-byte payload and changed only
its stored destination to1. Original slot1 and replacement payloads were distinct.
A narrowly scoped sender accepted only a single 154-byte `00 01 01` stored
message, with hashes rechecked before dispatch. Exactly one replacement was sent;
after a 500ms settle, all128 slots were requested again. Slot1 matched the
replacement packet exactly; the other127 entries were unchanged. The original
slot1 packet was then sent once through the same route. Another complete stored
readback matched the starting bank in all19,712 bytes. No power-cycle persistence
or interrupted-write recovery test was performed; dispatch alone was never used
as confirmation.

Finally the pre-session temporary sound was restored and its full readback
verified. No full-bank write, other stored-slot write, tuning-table data/save,
AFX or sequence-recording command was sent. No plugin runtime source or installed
bundle changed. This establishes a scoped stored-write/readback/restoration path
for tested slot1; it does not implement the plugin's stored-write UI, verify all
write destinations or authorize full-bank restore. Existing read-only tool
eligibility flags remain unchanged.

Private, ignored evidence:
- `midi-lab-results/20261026-bank-before-write-004030/`: bank-before.syx,
  bank-before-stored-requests.syx, slot-1-stored-readback.syx,
  slot-1-test-plan.json, exact source/original packets, scoped Swift probes,
  bank-after-test-write.syx, replacement-verification.json,
  bank-after-slot-restoration.syx and restoration-verification.json.
- Replacement packet SHA256:
  `e87850fd3114e28468a490df5adb8119df3d9f1180b19691393b74843e010cd3`.
- Original slot1 packet SHA256:
  `2d540df75caf2c2120e654f89196055f487badf99b476d02c7b032fd25dba148`.
- Temporary sound before session:
  `midi-lab-results/20261026-003303-backup-878bb8/baseline.syx`.
- Verified temporary-sound restoration:
  `midi-lab-results/20261026-004736-restore-ab8562/`.

Next implementation: a separate explicit single-slot operation with source and
destination preview, required current destination backup, strict command gate,
readback verification and recovery reporting. Keep current-patch file sending,
local bank selection, host automation and automatic recall separate. No automatic
retry, whole-bank overwrite or undocumented acknowledgement should be added.
