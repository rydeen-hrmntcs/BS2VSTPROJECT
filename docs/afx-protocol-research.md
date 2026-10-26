# AFX overlay protocol research

27 October 2026. Research only: no MIDI sent, application changes, commits, bank selection, overlay edits or saves. Extended modulation remains deferred to v2 after v1 and its final GUI.

## Recommendation: preserve the active buffer before selecting a bank

The first implementation increment should be a separate, explicitly invoked **Read Active AFX Buffer** action: request each of the 25 key records, request each name where supported, and save the complete original response messages plus a manifest. It must not select a bank, change patches, change protection, send notes, clear, import or save. Capture the ordinary current patch separately using the existing proven reader, because overlay records do not contain the underlying patch or global state.

This is a recommendation inferred from the protocol and documented loss behavior, not a hardware-validated AFX implementation. Novation says unsaved edits disappear on bank change, and a patch change can also change banks. Eight independent banks each hold 25 keys. Bank 0 disables overlays; banks 1–8 select them. The selected bank belongs to the patch; the overlay bank is independent storage. [Novation, In detail: AFX mode / Saving overlays][detail]

Components' **Get AFX Bank** is not an active-buffer-safe reader: its `Ase.main` explicitly calls `bX(e)` (bank select), waits 10 ms, then loops over the 25 keys. Even reselecting the current bank has not been proved to preserve unsaved edits. Do not copy that selection step into the first reader. [Official Components source: `Ase`, `this.sendMessage(bX(e)),yield oA(10)`][components]

## Wire shapes established by primary sources

All numbers below are hexadecimal unless stated otherwise. `H = F0 00 20 29 00 33 00`; `k` is a key index 00–18 (decimal 0–24), starting at the bottom home-octave C. Framing bytes are included in lengths.

| Operation | Request/message | Response or effect | Evidence |
|---|---|---|---|
| Read selected bank's key | `H 4F k F7` | `H 4E k <96 seven-bit data bytes> F7`, 106 bytes total | Appendix Export/Import; Components `vX(79,i)`, `UX.fromSysex` |
| Read key name | `H 54 k F7` | `H 53 k <16 name bytes> F7`, 26 bytes total | Components `vX(84,i)`, `UX.loadSysex`, `IX` |
| Select overlay bank | `H 50 b F7` | Changes active bank; `b=0` off, 1–8 bank | Components `bX`, `pX=80`, Get Bank flow; manual bank numbering |
| Select bank through NRPN | NRPN MSB 0, LSB 112 | State-changing selection | Appendix; Components `overlayBankIndex` NRPN mapping |
| Set protection | NRPN MSB 0, LSB 116 | 0 permits edits; 1 enables overlay protection | Appendix; In detail Protecting overlays |

The read commands have no bank-address argument: `k` is the key only. A full eight-bank collection therefore requires bank selection in the inspected Components path. No direct addressed stored-bank read was established. Calling a sequence of selections and reads “read-only” would conceal loss of unsaved active edits. [Appendix Export][appendix]; [Components `Ase`, `vX`, `bX`][components]

Names are gated in Components by `x1(appVersion, {commit:676})`, meaning app-version comparison greater than or equal to commit 676. This is distinct from the separate AFX editor gate at firmware 696. Do not invent a public version-number mapping. On unsupported or unconfirmed firmware, report names as unavailable; do not synthesize a successful empty response. [Components `Ese={commit:676}`, `x1`, `afxEditorNotSupported`][components]

## Preserve raw bytes; keep decoding separate

For a data response: indices 0–6 are H, 7 is 4E, 8 is key, 9–104 are the 96 packed bytes, and 105 is F7. Components `OX` / `RX` unpack every eight seven-bit bytes into seven eight-bit bytes, giving 84 decoded bytes. This is contiguous bit-stream packing, **not** the alternate high-bit-mask codec also present in the same bundle. For one input block p[0..7], decoded byte i (0..6) is `((p[i] << (i+1)) & 255) + (p[i+1] >> (6-i))`.

`UX.fromSysex` sets `hasOverlay = decoded[0] == 0`; do not infer presence from a nonempty name or nonzero payload. It maps fields through `kD` using a dedicated `overlayIndex`, whereas ordinary patch conversion uses `patchIndex`. The overlay is not a truncated or ordinary 154-byte patch. The writer constructs 84 decoded bytes ending `0,0,2`, but a backup must preserve whatever bytes the hardware returns rather than regenerating reserved fields. [Components `UX.fromSysex`, `UX.toSysex`, `OX`, `RX`, `kD`][components]

For a name response: indices 0–6 H, 7 = 53, 8 = key, 9–24 = 16 name bytes, 25 = F7. Components' display helper accepts character values 32–125 and trims whitespace; this is display behavior, not authority to alter backup bytes. Its exported bank contains data/clear and name messages per key, but archival capture should retain the actual inbound messages. [Components `IX`, `UX.loadSysex`, `qX.makeDownload`][components]

The offline bank importer recognizes a 10-byte `H 4C k F7` clear marker for an empty key. However, the live fetch predicate `TX` rejects messages shorter than 11 bytes. Whether real hardware ever answers a read with this short marker is **unverified**; do not silently discard such a capture or assume it is a standard response. [Components `WX.loadOverlaySysex`, `TX`][components]

Suggested capture manifest: schema version, UTC capture time, exact device/firmware identity and MIDI endpoints, declared scope `active-buffer` or specific stored bank, bank/protection values with their evidence and an explicit unknown state, per-request bytes, response bytes, message order/time, key, command, length, SHA-256, validation outcome, timeout/error and completeness. Keep raw unexpected messages too. An active buffer must not be labelled a verified saved bank without evidence. Require all 25 data responses and supported name responses before declaring complete; retain partial capture on failure. Validate manufacturer/model/protocol, terminator, 7-bit bytes, exact response type/length and key against the outstanding request. Components' `Ase.onMidiMessage` only checks `TX`; that weak matching is not sufficient for our reader. [Components `Ase.promiseSendRequest`, `Ase.onMidiMessage`, `TX`][components]

## State/protection queries: still unknown

The appendix establishes setters, not getters, for bank and protection. Inspected Components AFX helpers and device service do not establish a protection getter or an independently confirmed current-bank query. Components receives NRPN 0:112 as `overlayBankIndex`; receiving a change notification is not a state query. Normal patch backup may expose the bank associated with that patch, but it must not be assumed to prove unsaved active-bank or protection state without verification. Protection status, unsaved/dirty state and the safety of same-bank reselection remain unknown. Do not guess unused command IDs.

The device service's `H 58 F7` request and 10-byte `H 58 channel F7` response refer to **MIDI channel**, not AFX protection/bank state (`Yre`, `handleMidiChannelSysex`). [Official Components source][components]

## Bank backup and editing gates

After the active buffer is captured and offline-validated, design a separate eight-bank operation. It necessarily changes selection in the evidenced path; obtain authorization for that state change and a reviewed plan for preserving/restoring unsaved buffer contents before doing it. Read each bank with 50 serial requests where names are supported (400 responses total across eight banks), associate each record with its selected bank, and verify complete raw captures. Returning to the original bank alone does not restore unsaved edits. This research does not authorize restoration writes.

Mutation messages to exclude from the first reader: `H 49 F7` clear bank, `H 4A F7` save bank, `H 4B dst src F7` copy, `H 4C k F7` clear key, `H 4D dst src F7` move, `H 4E k data F7` import; Components additionally provides `H 52 k F7` clear name, `H 53 k name F7` set name, `H 57 note F7` live-edit selection. Saving commits the active bank; clearing does not automatically save. [Appendix mutation sections][appendix]; [Components `MX`, `xX`, `_X`, `sendSelectOverlayMessage`][components]

Open validation questions: current buffer behavior while overlays are off; empty-key response details; exact firmware support for name reads; preservation of unknown/reserved bytes; reliable current bank/protection/dirty-state observation; safe same-bank reselection; restore-and-readback procedure and power-cycle persistence. Do not implement parameter editing or bank writes on the strength of source inspection alone.

## Source provenance

Only Novation-owned documentation and shipped Novation Components source were used as protocol authority. Local source: `validation-results/stored-patch-protocol/main-D3lNZfPT.js`, supplied Components app 1.72.5; SHA-256 independently verified as `2322dc9fd409e5910e69f83cf5a6d4127c71322ded94e9bbc99cbba4be5a673a`. The minified symbols/anchors above refer to that exact asset; future builds may rename them.

[detail]: https://userguides.novationmusic.com/hc/en-gb/articles/25494313827986-Bass-Station-II-in-detail
[appendix]: https://userguides.novationmusic.com/hc/en-gb/articles/25494271270930-Bass-Station-II-appendix
[components]: https://components.novationmusic.com/assets/main-D3lNZfPT.js


## Initial hardware read-only evidence — 27 October 2026

User viewed the hardware overlay-bank menu without changing it and reported o0
(overlays off). The current ordinary patch was saved at
`midi-lab-results/20261027-002635-backup-887598/baseline.syx`. A first key0 data
request returned exactly106 bytes with command4E and key0.

The new explicit `lab.py read-afx-active --reported-bank 0` reader requested25
key data records successfully (106 bytes each). The first name request54/key0
received no matching reply within3s; capture stopped rather than inventing names.
No other SysEx reply was logged for that request. This is an observed timeout,
not proof of missing firmware support. Raw packet/message logs and a partial
transport manifest are retained in `midi-lab-results/20261027-003338-afx-active-07ad01/`.

An explicit data-only repeat (`--data-only`) returned all25 data records again,
byte-identical to the first collection:2650 bytes, SHA256
`7da8a11abff114dc7b30c6995d473054053a6869d3f84689c6f0bca53797777e`.
Evidence: `midi-lab-results/20261027-003956-afx-active-cd4a67/`, including
repeat-comparison.json. All25 decoded first bytes indicated overlay-present by
Components' interpretation, despite reported o0. The provenance/meaning of this
disabled-bank buffer is not established; it is not labelled saved bank0 or a
backup of all8 banks. Names remain unavailable in this capture, and archive
`complete` stays false while dataComplete is true.

The later ordinary patch capture
`midi-lab-results/20261027-004018-backup-98e8c4/baseline.syx` matched the starting
154 bytes exactly. No selection, protection, imports, clear or save commands
were sent. Current-buffer restoration remains unverified. Private overlay data
stays ignored; no user data is committed as fixtures.

## Read-only tool increment

`tools/midi-lab/AfxRead.swift` only constructs4F/54 read requests, serially with
100ms spacing and3s timeout. It preserves raw packet logs, reconstructed SysEx
messages (including unexpected messages), matched raw responses and request
provenance. Unexpected lengths/commands/keys cannot satisfy a request. There is
no bank-select or mutation path. `--data-only` intentionally skips names;
timeouts retain partial files and return failure through lab.py.

`lab.py inspect-afx FILE [--json]` validates a local archive without MIDI.
`afx.py` separates strict data/name records, presence decoding, duplicate/missing
keys and unsupported clear markers. Seven-bit packing is tested against an
independent integer-bitstream implementation. It never regenerates overlays or
marks archives transmit-eligible. A complete named archive requires all25 data
and25 name records with no issues; data-only coverage is reported separately.

Validation: reader compiles; six synthetic AfxTest checks pass. Focused CTest
AfxOffline, MidiLabTools and ResearchTools passed3/3 (3.97s). No plugin runtime,
installed bundles or host parameter indices changed. Full-suite stamp predates
this tool increment.

## Components layout reference supplied by user

User screenshots show a new local bank, Live Edit off, eight-bank destination
selector,25-key keyboard and selected-key side panel. Empty keys disable editing;
a factory overlay dragged into the local first key enables controls. Visible
sections include fixed note/octave tuning, output level, oscillator sync/Sub
tuning, Amp/Mod envelope retrigger count/retrigger/velocity/fixed duration and
LFO slew/sync/key-sync. Per-key Copy/Paste, Download/Upload and Remove are separate
from top-level Save and Send to Bass Station II. Screenshots are UI references,
not hardware transfer or parameter-map evidence. No Live Edit/Send was requested.

Recommended next increment: a local AFX document/browser preserving opaque raw
records and showing25 key slots, names/presence and provenance before enabling
parameter editing. Distinguish missing names from empty names, and local document
changes from device writes. Full8-bank backup still requires a reviewed selection
and unsaved-buffer restoration plan.
