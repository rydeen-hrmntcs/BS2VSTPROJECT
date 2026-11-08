# MicroTune contents: implementation research

19 October 2026. Documentation research only; no hardware available, MIDI sent,
runtime code changed or tuning data saved. Existing table selection 0–8 is
already implemented and hardware-tested; this note concerns table contents.

## Documented BSII support

Novation describes nine editable tables, indexed 0–8; only 1–8 can be saved.
Table 0 resets to standard tuning at startup. Each table maps 128 MIDI notes.
The documented real-time note-change message uses universal SysEx, device ID 0,
sub-IDs 08/02, table number, entry count and four bytes per entry: key, base
semitone and two fractional bytes. Selection uses RPN 0:3, not a table-data dump.
The documented precision floor is one semitone/256; finer wire precision should
not be promised acoustically. These are documented capabilities, not new tests.
[Novation appendix: Micro-tuning](https://userguides.novationmusic.com/hc/en-gb/articles/25494271270930-Bass-Station-II-appendix#micro-tuning).

Novation's example raises key A4 by 50 cents in table 1:

```text
F0 7F 00 08 02 01 01 45 45 40 00 F7
```

A separate proprietary command saves tuning tables:

```text
F0 00 20 29 00 33 00 48 F7
```

Treat that command as an explicit persistent operation, not an individual-table
save: no table index is supplied.
[Novation appendix: Table save](https://userguides.novationmusic.com/hc/en-gb/articles/25494271270930-Bass-Station-II-appendix).

Components supports uploading a tuning file, setting its reference pitch and
sending it to a tuning slot. Its guide does not publish the file parser or a
tuning-table retrieval protocol.
[Novation Components guide: Tuning Tables](https://support.novationmusic.com/hc/en-gb/articles/4411786558098-Bass-Station-II-Components-guide).

## Standards boundary and missing evidence

The MIDI Association lists bulk dump/request and real-time single-note changes
as distinct MIDI Tuning Standard messages. That does not establish that BSII
implements all of them. The linked full specification opened as a Google Drive
sign-in/loading page in this research; reserved encodings and bulk-dump details
were not verified from that document.
[MIDI Association specification entry](https://midi.org/midi-tuning-updated-specification).

No BSII tuning-table request/reply or persistent-save acknowledgement was found
in the checked Novation appendix and Components guide. This is a bounded search,
not proof that none exists. Ordinary patch readback currently exposes only the
selected table ID; it is not a backup of 128 note frequencies. Source evidence:
BSIIPatchDecoder.h and the existing microTuneTable field. Reopening a captured
sound therefore cannot currently restore edited external tuning-table contents.

The appendix's selection subsection says [0:9], conflicting with its nine-table
explanation and our verified 0–8 fixtures. Preserve 0–8; do not expose a tenth
table based on that inconsistent line. Global Tune remains a separate unresolved
parameter; changing a table is not a Global Tune fix.

## Proposed first implementation, not an implemented feature

Prefer an offline table editor and explicit transmit workflow before automatic
recall. Keep a host-owned table separate from the last known hardware selection.
Do not label locally generated or imported values as fetched from the synth.

Suggested scope:

1. Represent 128 absolute target pitches, source/provenance, target table and
   dirty state. Use a versioned local format first; Scala scale and keyboard-map
   import should be a separately specified parser, not arbitrary SysEx replay.
2. Build pure conversion and message-generation helpers. From the documented
   fractional weights, the mathematical target is
   `p = base + high/128 + low/16384`; negative offsets need a lower base note,
   not negative MIDI bytes. Validate finite inputs and representable boundaries.
   Test carry/rounding and documented examples, including identity and ±50 cents.
   Verify reserved sentinel values against the full specification before shipping.
3. Generate only the documented note-change family with an exact target and
   validated entries. Define/test chunking explicitly rather than assuming one
   message can contain all 128 notes. Separate “Send table” from “Save tables”.
4. Keep tuning data out of ordinary patch snapshots and Send All. Decide later
   whether the DAW Set should embed and explicitly restore table contents; that
   would be new behavior, including ordering before captured-sound recall.
5. Add UI integration only after the codec is covered offline. Label dispatch
   as sent, not verified, unless a hardware readback path is established.

These are design recommendations inferred from the evidence and our current
patch/global-state separation, not requirements asserted by the manual. No new
host parameters or automation indices are needed for a first table-file tool.

## Short later hardware plan

- Obtain a known complete tuning backup/restoration method before editing an
  existing custom slot. If no readback exists, use a user-approved slot whose
  complete contents are known; do not overwrite an unknown custom table.
- Select the target, change one note without issuing persistent save, and measure
  the resulting pitch with a tuner or recorded audio. The LCD and 154-byte patch
  are insufficient evidence for the changed note frequency.
- Restore that note and measure again. Check an unchanged neighbouring note.
- Test inactive-table writes, chunk boundaries, range limits and selection order.
- Only after restoration is established, test explicit save/power-cycle behavior
  and its effect on other tables. Table 0's startup behavior is a separate case.

## Recommendation

MicroTune content editing is the stronger documented candidate for implementation
without the synth: a pure table model and codec can be built and tested locally.
Keep the first hardware acceptance small. Bulk readback and automatic tuning
recall remain investigation items, not promised capabilities.

## Offline implementation follow-up — 19 October 2026

The pure table model, strict local JSON document, note-change codec and file tool
are now implemented; see [offline tool usage and limits](microtune-offline.md).
All eight offline groups and the CLI workflow checks pass. The application range
is deliberately restricted to absolute MIDI pitches 0–127 while reserved/wider
MTS bounds remain unverified. No plugin UI, transport, save or hardware-readback
integration has been added. The proposed hardware plan above remains pending.

The subsequent [local editor page](microtune-editor.md) adds note editing and
file load/save inside the plugin, plus silent DAW working-document persistence.
It still has no hardware tuning-data send, readback or persistent-save action.

## Single-note hardware check — 8 November 2026

With the Audient iD14 audio return and Ableton Tuner at a 440 Hz reference, a
held MIDI note 69 read A at +0.3 cents. Edit-buffer readback confirmed table 0.
A direct CoreMIDI message addressed only note 69 in table 0:

- +25 cents: `F0 7F 00 08 02 00 01 45 45 20 00 F7`. Tuner read A at +24.6 cents.
- Standard pitch: `F0 7F 00 08 02 00 01 45 45 00 00 F7`. The user confirmed
  the reading returned near baseline. An exact final reading was not recorded.

The negative-offset check used the same audio setup. Baselines were A at +0.3
cents and A♯ at 0 cents. With table 0 confirmed again, the message
`F0 7F 00 08 02 00 01 45 44 60 00 F7` targeted MIDI note 69 at −25 cents.
A measured −25.4 cents and A♯ stayed at 0 cents. After sending the same
standard-pitch message above, A measured +0.3 cents again.

All test messages addressed note 69 in temporary table 0. The positive and
negative single-note changes and restoration passed acoustically, with MIDI
note 70 serving as the neighboring-note control for the negative test. Batch
transfer, table readback, persistent save and plugin transfer remain open.
