# Bass Station II Arp / Seq protocol research

Researched 2026-10-18 without sending MIDI or touching hardware. Read `docs/local-reference.md` and the cached official 4.14 manual first. Documentation establishes routes and musical meanings, but does **not** establish exact wire value encodings for the six new controls.

## Documented routes versus unknown encodings

| Control | Documented message | Musical values | Exact raw values |
|---|---|---|---|
| Arp On | CC 108 | Off / On | Unspecified; do not assume 0/127, 0/1, or a 64 threshold |
| Arp Latch | CC 109 | Off / On | Unspecified |
| Arp Octaves | CC 111 | 1–4 octaves; same physical selector selects SEQ slots in Play/Record | Unspecified; mode-dependent receive/storage behavior unverified |
| Arp Note Mode | CC 118 | Up, Down, UpDn, UpDn2, Played, Random, Record, Play | Unspecified; panel order does not prove 0–7 encoding |
| Arp Rhythm | CC 119 | Patterns 1–32 | Unspecified; do not assume wire equals display or display minus one |
| Seq Retrig | NRPN 106, represented in this project as 0:106 | Off / On | Unspecified; numeric NRPN 106 implies MSB 0, LSB 106, but data-entry encoding remains unverified |

The MIDI table prints `106` for Seq Retrig rather than `0:106`. These addresses, and the empty range cells, appear in the [official 4.14 PDF, page 24](https://fael-downloads-prod.focusrite.com/customer/prod/s3fs-public/downloads/Bass%20Station%20II%20User%20Guide%204.14%20English.pdf#page=24). The [current official online appendix](https://userguides.novationmusic.com/hc/en-gb/articles/25494271270930-Bass-Station-II-appendix) retains the same six routes and blank ranges. No exact encodings were found in these primary sources.

## Tempo and clock

The documented internal tempo range is 40–240 BPM. Incoming external MIDI clock takes precedence automatically; moving the Tempo control then displays external BPM. After clock disappears, playback continues at the last clock rate until moving the knob returns control to the internal clock. These are documented musical behaviors, not newly observed firmware results. No Tempo CC or NRPN appears in the MIDI parameter list. This is absence of a documented route, not proof that an undocumented route cannot exist. See [official 4.14 PDF, page 18](https://fael-downloads-prod.focusrite.com/customer/prod/s3fs-public/downloads/Bass%20Station%20II%20User%20Guide%204.14%20English.pdf#page=18).

Host clock output would be a separate transport feature requiring timing/start/stop behavior verification. It must not be presented as a confirmed direct parameter mapping.

## Sequence slots and steps

The instrument documents four persistent sequence memories, each holding up to 32 notes/rests; the selected sequence is also saved with a patch. The physical SEQ selector chooses memory 1–4 in Record/Play. Recording advances step position as notes/rests are entered; durations are supplied by Rhythm during playback. Legato and ties are panel recording operations. Seq Retrig On restarts every two bars; Off allows the sequence to finish across rhythm boundaries. See [official 4.14 PDF, page 19](https://fael-downloads-prod.focusrite.com/customer/prod/s3fs-public/downloads/Bass%20Station%20II%20User%20Guide%204.14%20English.pdf#page=19).

No dedicated sequence-slot CC/NRPN, random-access step read/write command, step position message, rest/tie MIDI encoding, or sequence SysEx layout is specified in the reviewed table/manual. The shared physical Octaves/SEQ selector suggests CC 111 as a discovery candidate, but **does not prove** it selects sequence slots remotely. Patch inclusion does not establish sequence byte offsets. Do not implement guessed bit positions.

## External note routing caveat

Novation documents Function + Latch as the toggle between MIDI-to-Arp input and Arp-to-MIDI output. Input mode arpeggiates incoming notes; output mode sends generated notes. No remote address for this routing toggle is given in that article. See [Novation support: external arp control](https://support.novationmusic.com/hc/en-gb/articles/206861739-Control-Bass-Station-II-s-Arp-from-External-Controller).

## Evidence boundaries and next verification

No hardware observations were made during this research. Existing project notes explicitly classify these new addresses as discovery routes, not validated application controls (`docs/midi-lab.md`). Preserve captured raw messages separately from human labels.

The next useful evidence is an incoming-only capture of every physical state: On/Latch, all eight Mode positions, Octaves 1–4, all 32 Rhythms, both Seq Retrig states, and SEQ slots in Play versus Record. Establish emitted encodings first, then separately validate receive behavior and patch dump changes. Recording mode/slot changes may alter sequence state, so discovery should preserve the existing sound/sequence data. Tempo and sequence-step editing should remain explicitly unsupported until their routes are demonstrated.

## Hardware capture evidence — 2026-10-18

Incoming-only MIDI Lab captures on the user's firmware 4.14 unit, channel 1:

- `midi-lab-results/20261018-000928-record-9603f5/events.jsonl`: following the requested Direction sequence Up → Down → UpDn → UpDn2 → Played → Random, CC118 emitted 0, 1, 2, 3, 4, 5 respectively. An initial transient 3 preceded Up. Record and Play were deliberately not tested.
- `midi-lab-results/20261018-001103-record-ecf7a3/events.jsonl`: two Arp On presses emitted CC108 values 1 then 0; two Latch presses emitted CC109 values 1 then 0. This supports the emitted On=1 / Off=0 encoding; no separate LED-state confirmation was collected.

These captures establish emitted messages only. Remote receive behavior and SysEx storage remain unverified. Raw captures are local ignored artifacts; this note preserves the observed values.

### Arp octave capture

With Direction on Random, the requested octave sequence 1 → 2 → 3 → 4 emitted CC111 values 2, 3, 4 (`midi-lab-results/20261018-001301-record-984d0b/events.jsonl`). The initial 1 produced no message. A separate move from 4 back to 1 emitted 3, 2, 1 (`midi-lab-results/20261018-001441-record-97c767/events.jsonl`), confirming octave labels 1–4 match emitted raw values 1–4. This does not establish receive behavior or sequence-slot selection in Play/Record mode.

### Arp rhythm capture

Incoming-only capture `midi-lab-results/20261018-002823-record-c20f2f/events.jsonl`: hardware Rhythm moved fully anticlockwise to labelled pattern 1, then swept fully clockwise to labelled pattern 32. All CC119 values 0–31 were observed, with no gaps. This establishes emitted encoding raw = displayed pattern minus one. Remote receive and SysEx storage remain unverified.

### Seq Retrig capture

Initial capture `midi-lab-results/20261018-003026-record-99c112/events.jsonl` recorded NRPN 0:106 with Data Entry 0 only, so could not establish both states. Repeat capture `midi-lab-results/20261018-003218-record-d88fa8/events.jsonl` recorded Data Entry 1 then 0 for NRPN 0:106; the user explicitly confirmed seeing both On and Off while performing Off → On → Off. Emitted encoding is therefore Off=0 / On=1. Incidental CC1 messages were unrelated. Remote receive and SysEx storage remain unverified.

### Record / Play capture

Capture `midi-lab-results/20261018-003407-record-e5f199/events.jsonl` recorded CC118 values 6, 7, 6, 5 during Random → Record → Play → Random. Intermediate Record was crossed while returning. The raw values are retained, but these initial instructions used the wrong physical Play/Record order; this capture alone did not establish their labels. See corrected capture below. No notes or Rest/Legato actions were requested.

## Initial live-only implementation (superseded below)

Six live-only choices appended after the existing 76 host parameters, preserving their indices, IDs and ranges. New Arp / Seq page includes existing Swing (unchanged ID/range), On, Latch, Octaves, Direction / Seq Mode, Rhythm and Seq Retrig. All six new controls support the existing automation, MIDI reception and saved-session parameter paths. Hardware receive checks are recorded below; these do not establish SysEx storage. Get Patch does not populate these six controls: no SysEx offsets have been guessed. Send All deliberately skips them to avoid replaying stale mode/latch values or arming Record during bulk patch recall. Their values remain in DAW state and parameter snapshots; captured raw sound recall remains lossless, subject to the instrument's actual patch contents. Tempo, step editing and remote sequence-slot selection remain unimplemented/unverified.

### Corrected Direction labels

The user corrected the physical clockwise order to Up, Dn, UpDn1, UpDn2, Played, Random, Play, Record. Fresh capture `midi-lab-results/20261018-003727-record-59276e/events.jsonl` requested Random → Play → Record and recorded CC118 6 then 7, with explicit user confirmation of the printed labels. Thus Play=6 and Record=7; the provisional reversed mapping was corrected before building/installing. The JSONL choice strings in this capture were generated from that provisional registry and are wrong; the raw bytes and user-labelled action sequence are the evidence. Earlier UpDn was relabelled UpDn1 to match the hardware.

## Local validation and installed build

2026-10-18: standalone/AU/VST3 Release builds PASS. Seven registered offline CTest groups PASS, including all 13 MIDI Lab checks. Native processor integration PASS with added Arp/Seq CC/NRPN output, incoming editor/APVTS updates without MIDI echo, saved values and stable pre-existing host indices. Native SysEx input regression PASS. Installed AU/VST3 signatures verified and AU validator PASS; previous bundles retained locally in `validation-results/20261018-003852-4230/before/`. No hardware values were sent by these virtual-port checks. User closed Ableton before installation. Subsequent manual Ableton/hardware checks are recorded below.

## Manual Ableton / hardware checks — 2026-10-18

User-reported checks on the installed build from commit 680c7f9:

| Control | Hardware → app | App → hardware |
|---|---|---|
| Rhythm | Full 1–32 sweep followed correctly after startup hiccup below | Changing 1 to 32 audibly changed the running arp |
| Octaves | Hardware 1 reflected as app 1 | App 1 to 4 audibly expanded the arpeggio span |
| Direction | Hardware Random reflected as app Random | Up to Down audibly changed ascending to descending |
| Arp On | Hardware On reflected as app On | App Off stopped the repeating arp |
| Latch | Hardware Off reflected as app Off, arp stopped | App On sustained the arp after releasing keys |
| Seq Retrig | Hardware Off and On both reflected correctly | App Off and On both confirmed by reopening the hardware Function setting without changing its value |

Do not describe the untested reverse transitions or all eight Direction modes as manually verified. Play/Record emitted encodings are captured; subsequent basic mode checks are recorded below, with sequence-content tests still outstanding.

Initial Rhythm check failed: hardware at 32, app at 1, CC RX reportedly waiting. Hardware Cutoff then worked; user reported Rhythm began following after moving Cutoff. No root cause established. The subsequent cold-start retest passed (see below); the cause remains unknown and no fix is claimed.

Feedback correction: incoming MIDI does not automatically make the hardware LCD display the edited parameter. Rhythm, octave span and direction sends were checked audibly. Seq Retrig was checked by explicitly reopening its on-key settings page after each app edit.

### Follow-up startup and sequence-mode checks

- User saved the Set, quit Ableton completely and reopened it. Hardware Rhythm followed immediately before touching Cutoff. The earlier reception hiccup did not reproduce; no code fix or root cause is claimed.
- App Direction Play plus Arp On played the existing stored sequence when the user held one hardware key. App-to-hardware Play behavior confirmed for that slot.
- Hardware Direction Up changed the app from Play to Up.
- App Direction Record made the hardware display show `rec`. No notes were requested or reported entered. This confirms entering Record mode only, not sequence recording or preservation of sequence contents.
- Hardware Direction Play changed the app from Record to Play.

Feedback correction: Play/Record have no dedicated LED indicators. The Play test used audible sequence playback; Record used the hardware's explicit `rec` mode display. Sequence-slot selection, note/rest/tie recording, random-access step editing and sequence-content persistence remain untested.

## Controlled SysEx discovery — in progress

Hardware changes followed by requested 154-byte edit-buffer dumps, with Ableton playback stopped. Retained fixtures and the ordered change log are in `tests/arp-seq-fixtures/manifest.json`; expected values are APVTS choice indices, not necessarily MIDI raw values.

- Rhythm: byte 80 (zero-based) changed 31 → 0 → 16 for displayed 32 → 1 → 17. Candidate field low five bits; other Rhythm values and shared upper bits still need checking.
- Arp On: byte 77 toggled 40 → 32 → 40 for On → Off → On, XOR 8 only.
- Latch: byte 77 toggled 40 → 56 → 40 for Off → On → Off, XOR 16 only.
- Octaves/SEQ selector in Play: byte 78 changed 4 → 16 when selecting 4. Changing Direction Play → Up altered only byte 79, 12 → 0, leaving byte 78 at 16. In Up mode Octaves 4 → 1 → 2 → 3 stored 16 → 4 → 8 → 12 in byte 78, supporting a three-bit field shifted left two. Octave choice index is decoded raw minus one. This does not independently validate audible sequence-slot contents or their persistence.
- Direction: Play → Up changed byte 79 from 12 → 0. Other modes still need controlled SysEx samples.

These findings have not yet been added to the decoder; no guessed offsets or sequence-note payload edits have been used.

### Complete Direction SysEx captures

All eight settings now have labelled hardware captures: Up, Down, UpDn1, UpDn2, Played, Random, Play, Record store byte 79 values 0, 2, 4, 6, 8, 10, 12, 14 respectively. Each successive change altered only that byte. The candidate field is `(byte[79] >> 1) & 7`. Added seven fixtures to the existing manifest; Up was already captured. No keys or sequence-entry buttons were requested during Record. The subsequent controlled Seq Retrig comparison is recorded below.

## SysEx readback implementation

All six fields now feed Get Patch / imported-patch decoding and the shared inspector registry. The controlled fixtures are 22 original 154-byte hardware dumps, with labels and before/after differences preserved in the manifest. Seq Retrig Off changed byte 77 from 40 to 8; On restored 40 (mask 0x20, no other bytes changed). The final field map (zero-based bytes) is:

| Field | Stored bits | Editor value |
|---|---|---|
| Arp On | byte 77 mask 0x08 | 0/1 |
| Latch | byte 77 mask 0x10 | 0/1 |
| Seq Retrig | byte 77 mask 0x20 | 0/1 |
| Octaves/SEQ | byte 78 mask 0x1C | compacted raw 1–4 minus one; invalid values skipped |
| Direction | byte 79 mask 0x0E | index 0–7 in corrected panel order |
| Rhythm | byte 80 mask 0x1F | index 0–31, displayed 1–32 |

Send All now includes these controls, but skips Direction when its value is Record so bulk recall does not arm sequence recording. Explicit mode edits remain available. Existing host IDs, indices and ranges are unchanged. Old 76-control snapshot fixtures continue to validate their original entries; they have not been retroactively populated with guessed values. New fixture tests check all four octaves, all eight modes, both binary states and Rhythm 1/17/32, as well as neighbouring-bit isolation and invalid stored octave rejection. Full 32-pattern SysEx sweeping and sequence-note payload interpretation are not claimed.

### SysEx implementation validation / installation

2026-10-18: Release standalone/AU/VST3 builds PASS. All seven CTest groups PASS after correcting the legacy fixture checks to distinguish their 76 mapped controls from obsolete raw aliases retained in their XML. All 15 MIDI Lab tests PASS, including all 22 labelled patch fixtures, neighbour-bit isolation and invalid-octave rejection. Native processor integration PASS, including host-silent application of the six Arp/Seq decoded values; direct native SysEx regression PASS. Installed AU/VST3 signatures and AU validator PASS. Prior plugin bundles retained in ignored `validation-results/20261018-011828-6393/before/`; failed initial fixture expectations and passing retests are retained in the same report directory. User saved and closed Ableton before installation. Manual hosted Get Patch validation with Live RX paused is the next acceptance check.

### Manual hosted Get Patch acceptance — PASS

On the installed 83f7679 build, the user paused Live RX and confirmed that Get Patch updated each of the six new controls after hardware edits:

| Control | Confirmed displayed result after Get Patch |
|---|---|
| Rhythm | 32 |
| Octaves | 4 |
| Direction | Random |
| Latch | On |
| Arp On | On (hardware was initially Off, so the requested change was reversed) |
| Seq Retrig | Off |

The initial reported octave inconsistency was clarified by the user: the selector was correct; the diagnostic at the bottom displayed the internal zero-based choice index 0–3. Index 3 corresponds to displayed Octaves 4. This was not a confirmed octave-readback defect.

These are specific manual SysEx readback checks in Ableton with live CC/NRPN application paused. They do not add sequence-note editing, verify every possible combination, or establish full sequence-slot content recall. Resume Live RX after testing for normal hardware following.


## Ableton clock following — 26 October 2026

User-confirmed headphone test with Ableton Output: Bass Station II Track and
Sync enabled, Input Sync disabled, Clock Type Song and Sync Delay 0.00 ms.
The BSII track MIDI From remained No Input. Playback was requested running,
with hardware Direction Up, Latch Off and Arp On, holding a hardware key.

Initial apparent clock failure (hardware Tempo freely adjustable to 240) was
resolved when the user enabled Ableton's audio engine. They then reported the
hardware tempo followed Ableton and confirmed the audible arp slowed/sped up
with 120 → 60 → 120 BPM. The source of clock is Ableton's MIDI output; no plugin
clock generator or direct Tempo mapping was added. This is a basic tempo-follow
check, not a timing-jitter, phase, transport restart, LFO or DIN-port measurement.
Start/Stop/Continue and clock-loss behavior remain to be tested separately.


### Clock stop/restart and synced LFO 2

Keeping the hardware key held, the user reported the arp continued at roughly
the same tempo after Ableton Stop. Setting Ableton to 90 BPM and restarting
playback changed the arp to that tempo. This establishes tempo following on
restart for this setup, not exact phase/reset or MIDI Continue semantics.

For LFO 2, requested setup was Sync / 1/4 / Triangle with LFO 2 filter depth
raised, Cutoff near the middle and a sustained hardware note, Arp Off. The user
heard repeated filter movement and confirmed it slowed to roughly half speed
and returned as Ableton changed 120 → 60 → 120 BPM. This is an audible relative
rate check, not a measured 1/4-period/phase/jitter test. Other divisions,
DIN, reconnection and independent clock-loss/internal-knob recovery remain
separate checks. No plugin clock generator or firmware control was added.


### Synced LFO 1 acceptance — 26 October 2026

Requested setup: LFO 1 Sync / 1/4 / Triangle, Osc 1 LFO 1 pitch depth slightly
above its neutral editor value 128, Arp Off and a sustained hardware key.
The user heard pitch modulation at Ableton 120 BPM and confirmed it slowed when
changing to 60 BPM during playback. After Stop, the modulation remained at the
last received rate even when Ableton's tempo was changed while stopped.
This is a user-reported audible relative-rate check, not a measured frequency,
phase or jitter result. Basic audible clock following is now confirmed for arp,
LFO 1 and LFO 2 in this Ableton setup. Other divisions, DIN and disconnect/recovery
remain unverified. Final instruction: release the key and stop playback.
