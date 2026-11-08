# AFX decoded field map research

27 October 2026. Offline source inspection only; no MIDI, app source edits, or commits.

## Authority and byte coordinate system

Source: [Novation Components 1.72.5](https://components.novationmusic.com/assets/main-D3lNZfPT.js), SHA-256 `2322dc9fd409e5910e69f83cf5a6d4127c71322ded94e9bbc99cbba4be5a673a`. Local asset: `validation-results/stored-patch-protocol/main-D3lNZfPT.js`. Exact excerpts, character/UTF-8 byte offsets and excerpt hashes are in `validation-results/afx-field-map/source-excerpts.json`; machine-readable map is `validation-results/afx-field-map/fields.json`. Anchors below refer to this exact build.

A 106-byte 4E message contributes bytes9..104 (96 packed bytes). UX.fromSysex calls OX, giving84 unsigned eight-bit decoded bytes. All offsets below are zero-based in those84 decoded bytes. They are overlayIndex values, not patchIndex values. kD has85 field definitions covering1..80, with shared bitfields. [Source anchors: UX.fromSysex, OX/RX, kD]

## Decode and preservation rules

jX uses bool((raw>>rs)&1) for DT_B; raw&min for DT_M; raw-64 for DT_S; raw-128 for DT_S8; and unchanged raw for DT_U/DT_U8. Signed forms use offset binary, not two's complement. Each definition occupies one decoded byte; Boolean and mask fields share specific bits. U8/S8 are eight bits in one decoded byte, not two-byte integers. DT_U passes the entire byte unchanged: do not silently mask or clamp to127. [Source: jX, kD]

The kD min property for DT_M is the AND mask, not a lower bound. Boolean min/max0 is not a zero-only valid domain. Keep original raw bytes and unknown bits; source metadata does not prove a complete hardware validity rule. [Source: jX, kD]

Byte0: hasOverlay=(raw==0). Do not infer presence from names, slot colors or payload nonzero tests. Bytes81,82,83 have no kD fields; UX.toSysex writes0,0,2. Their hardware semantics are unestablished. Preserve original values; do not regenerate a backup via this writer. Additional unmapped high bits in shared bytes should also remain opaque. [Source: UX.fromSysex/toSysex]

## Fixed note and output are special accessor fields

Byte75 is pitch. jX passes the raw unsigned byte to UX.pitch setter, which ignores values below128; otherwise tuningValue=raw&127. Its getter returns128|tuningValue. FixedTuning UI is tuningValue>0. Zero displays normal slot pitch slotIndex+48, while a positive value is the fixed MIDI note. Thus raw128 means normal pitch; raw129..255 encode fixed note1..127. Below128 is an ignored assignment in Components, not established hardware meaning. The constructor starts tuningValue0, so on a newly created model such an ignored value leaves0. There is no extra fixed-note byte in this map. kD pitch min0/max7 is misleading for overlay decoding: preserve it as source metadata, never use it as the overlay validation range. [Source: UX pitch accessor, fixedTuning UI, UX.midiNote, kD]

Byte76 similarly passes to outputLevel setter: only raw>=128 updates outputLevelValue=raw&127. Getter returns128|value; default outputLevelValue is127. Do not label low7 bits of a raw<128 byte as a successfully decoded output value: the source ignores that assignment. This bit7 is an accessor acceptance marker in source, not independently proved a hardware enable/disable flag. [Source: UX outputLevel accessor/default, kD]

## Exact field table

Values below are Components source scalars; physical frequency/time/cents conversion is not claimed. Bn means Boolean bit n, M3 means raw&3, S means raw-64, S8 means raw-128, U/U8 mean raw unchanged. Numeric metadata min/max is retained verbatim in JSON; it is not a validation specification. [Source: kD, jX]

| Decoded offset | Field ID | Format |
|---:|---|---|
| 1 | `voiceOscSync` | B0 |
| 2 | `osc1Wave` | M3 |
| 3 | `osc1PulseWidth` | U |
| 4 | `osc1Octave` | S |
| 5 | `osc1TuneCoarse` | S8 |
| 6 | `osc1TuneFine` | S8 |
| 7 | `osc2Wave` | M3 |
| 8 | `osc2PulseWidth` | U |
| 9 | `osc2Octave` | S |
| 10 | `osc2TuneCoarse` | S8 |
| 11 | `osc2TuneFine` | S8 |
| 12 | `oscSubWave` | M3 |
| 12 | `oscSubMode` | B2 |
| 13 | `oscSubPulseWidth` | U |
| 14 | `oscSubOctave` | S |
| 15 | `oscSubTuneCoarse` | S8 |
| 16 | `oscSubTuneFine` | S8 |
| 17 | `oscSubWaveSubMode` | M3 |
| 18 | `oscSubOctaveSubMode` | S |
| 19 | `mixerOsc1` | U8 |
| 20 | `mixerOsc2` | U8 |
| 21 | `mixerOscSub` | U8 |
| 22 | `mixerNoise` | U8 |
| 23 | `mixerRingMod` | U8 |
| 24 | `mixerExternal` | U8 |
| 25 | `filterFrequency` | U8 |
| 26 | `filterResonance` | U |
| 27 | `filterOverdrive` | U |
| 28 | `filterShape` | M3 |
| 28 | `filterCircuit` | B2 |
| 28 | `filterSlope` | B3 |
| 29 | `envAmpVelocity` | S |
| 30 | `envAmpAttack` | U |
| 31 | `envAmpDecay` | U |
| 32 | `envAmpSustain` | U |
| 33 | `envAmpRelease` | U |
| 34 | `envAmpTrigger` | M3 |
| 35 | `envModVelocity` | S |
| 36 | `envModAttack` | U |
| 37 | `envModDecay` | U |
| 38 | `envModSustain` | U |
| 39 | `envModRelease` | U |
| 40 | `envModTrigger` | M3 |
| 41 | `lfo1Waveform` | U |
| 42 | `lfo1Delay` | U |
| 43 | `lfo1LfoSlew` | U |
| 44 | `lfo1Speed` | U8 |
| 45 | `lfo1SpeedSync` | U |
| 46 | `lfo1SpeedMode` | B0 |
| 46 | `lfo1KeySync` | B1 |
| 47 | `lfo2Waveform` | U |
| 48 | `lfo2Delay` | U |
| 49 | `lfo2LfoSlew` | U |
| 50 | `lfo2Speed` | U8 |
| 51 | `lfo2SpeedSync` | U |
| 52 | `lfo2SpeedMode` | B0 |
| 52 | `lfo2KeySync` | B1 |
| 53 | `modAftertouchFilterFreq` | S |
| 54 | `modAftertouchLfo1OscFreq` | S |
| 55 | `modAftertouchLfo2Speed` | S |
| 56 | `modLfo1Osc1Pitch` | S8 |
| 57 | `modLfo1Osc2Pitch` | S8 |
| 58 | `modLfo1OscSubPitch` | S8 |
| 59 | `modLfo2Osc1PW` | S |
| 60 | `modLfo2Osc2PW` | S |
| 61 | `modLfo2OscSubPW` | S |
| 62 | `modLfo2FilterFreq` | S8 |
| 63 | `modEnv2Osc1Pitch` | S |
| 64 | `modEnv2Osc2Pitch` | S |
| 65 | `modEnv2OscSubPitch` | S |
| 66 | `modEnv2Osc1PW` | S |
| 67 | `modEnv2Osc2PW` | S |
| 68 | `modEnv2OscSubPW` | S |
| 69 | `modEnv2FilterFreq` | S |
| 70 | `modOsc2FilterFreq` | U |
| 71 | `distortAmount` | U |
| 72 | `compressAmount` | U |
| 73 | `envAmpRetrigger` | U |
| 74 | `envModRetrigger` | U |
| 75 | `pitch` | accessor; see above |
| 76 | `outputLevel` | accessor; see above |
| 77 | `envAmpTimedSustain` | B0 |
| 78 | `envModTimedSustain` | B0 |
| 79 | `envAmpRetriggerCount` | U |
| 80 | `envModRetriggerCount` | U |

## Bounded findings

Oscillator1 bytes2–6; oscillator2 bytes7–11; Sub bytes12–18; mixer19–24; filter25–28; Amp envelope29–34 and73/77/79; Mod envelope35–40 and74/78/80; LFO1 bytes41–46; LFO2 bytes47–52; modulation53–70; distortion71; compression72; pitch75; output76. All derive directly from kD. Shared fields: Sub wave bits0..1 and mode bit2 at12; filter shape bits0..1, circuit bit2 and slope bit3 at28; LFO speed mode bit0 and key sync bit1 at46/52.

This mapping establishes source behavior, not hardware round-trip safety. No mutation, bank selection, or transport eligibility follows from it. The existing Source/BSIIAfxArchive.h validates opaque records and computes presence only; its preservation approach remains compatible. No ordinary-patch offset was substituted. UI engineering can display source scalar values conservatively while retaining every raw record.


### Firmware clarification from Components — 27 October 2026

The user supplied a Components Updates screenshot explicitly reporting Version
4.15 installed. Its4.15 note says Components AFX overlay editor support was
added;4.14.19 lists a fix for saving to slot104. Treat4.15 as the currently
observed firmware. Prior4.14 references are historical user reports or the
manual version consulted; do not silently relabel older captures as verified
4.14 or4.15. The exact update timing was not established. This does not resolve
the unanswered54/name request or prove a retail-version/Components commit mapping.
