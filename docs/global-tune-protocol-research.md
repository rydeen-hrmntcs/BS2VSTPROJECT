# Global Tune receive protocol — 18 October 2026

## Conclusion

No verified negative Global Tune receive encoding was found in the checked official documentation. Keep transmission disabled until Novation supplies a specification or a controlled hardware test establishes it. This research sent no MIDI.

## Primary documentation checked

- [Novation: Bass Station II in detail](https://userguides.novationmusic.com/hc/en-gb/articles/25494313827986-Bass-Station-II-in-detail), “Global: Tune (upper A#)”: overall tuning spans -50 to +50 cents. The section gives no MIDI receive encoding.
- [Novation: Bass Station II appendix](https://userguides.novationmusic.com/hc/en-gb/articles/25494271270930-Bass-Station-II-appendix): Master Tune survives power-off, but the MIDI parameter list contains no Global Tune/NRPN 64:6 entry. No signed-value format, Global Tune query, master-fine-tuning RPN, or dedicated Global Tune SysEx command is specified on this page.

The appendix does explicitly support **RPN 0:3 for tuning-table selection**, plus **real-time MIDI Tuning Standard note-change SysEx** for editing individual note frequencies in a table. These are separate operations; neither is documented as setting Global Tune or its display. The table-save SysEx saves tuning tables. Do not substitute these operations for Global Tune or infer support for other standard RPNs merely because MIDI defines them. [Source: appendix](https://userguides.novationmusic.com/hc/en-gb/articles/25494271270930-Bass-Station-II-appendix).

## Hardware evidence supplied by the current session

Firmware reported as 4.14. See [global-control-research.md](global-control-research.md) for the capture and receive-test record.

| Direction | Observed behavior |
| --- | --- |
| Hardware to MIDI | Display -50…+50 emitted NRPN 64:6 with CC6 = display + 64; all 101 values 14…114 captured. |
| MIDI to hardware | NRPN 64:6, CC6=65 produced +50; CC6=1 produced +1; CC6=127 produced +50; replaying the hardware negative-endpoint raw 14 produced +14. |

Hardware was manually restored to 0. The emitted offset formula demonstrably fails as a receive formula. The four receive points are compatible with positive-value saturation, but do not establish the decoder or any negative representation. No tested negative receive value is available.

## Remaining gaps

Novation confirmation is needed for the receive route, signed encoding and bit width, Data Entry LSB/sign handling, firmware-specific behavior, and any global query or restoration command. Two checked manual pages cannot prove that no undocumented route exists. Generic RPN fine tuning, universal master tuning, or guessed proprietary SysEx must not be advertised as supported or sent based on this evidence.

## Follow-up negative-endpoint experiment

A fresh user-labelled -50 edit emitted raw 14 (descending from 63 through 14
as the hardware value changed). Replaying that exact NRPN message from zero
produced user-confirmed +14. The 154-byte patch dumps at hardware Tune zero
and -50 were identical. Evidence is retained under
`tests/global-tune-fixtures/`, with the full experiment recorded in
[global-control-research.md](global-control-research.md). User confirmed
manual restoration to zero afterward. No plugin implementation was changed.

## Paired-data follow-up — 20 October 2026

A scoped NRPN 64:6 MSB/LSB 127/127 probe displayed +50 rather than candidate -1.
Subsequent manual negative stepping behaved unexpectedly (user reported
0 -> -1 -> 49). Paired zero restored the displayed 0 twice, but normal manual
range and acoustic tuning were not re-established. Probing stopped; the exact
sequence and limitations are in [global-control research](global-control-research.md).
Do not implement or repeat this candidate as verified signed receive support.

The subsequent Ableton isolation in global-control-research.md found normal
manual adjustment with Live closed, with an empty Set, and in the original Set
when its BS2 track used MIDI From = No Input. The restricted-adjustment symptom
strongly implicates track MIDI echo rather than a proven standalone hardware
fault. Outgoing/incoming Tune encoding asymmetry makes such an echo hazardous;
no negative receive mapping has been established.


## Isolated manual check — 25 October 2026

The user's Ableton screenshot showed the BSII track MIDI From = No Input and
an unrelated MIDI track with MIDI To = No Output. Starting Tune was reported 0.
The user confirmed normal physical 0 → -1 → 0 → +1 → 0 adjustment and return
to zero. No diagnostic MIDI was sent and no frequency measurement was made.
This adds a current normal manual-behavior check, not negative receive support.
A fresh bounded search of Novation documentation and review of the official
appendix found no documented signed Global Tune receive encoding. Further
transmission remains deferred pending reliable protocol evidence or manufacturer
clarification; do not repeat earlier failed candidates as a new test.
