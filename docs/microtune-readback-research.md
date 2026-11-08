# Stored MicroTune table backup research

8 November 2026. No verified way to retrieve the BSII's current stored tuning
tables 1–8 was found in the published protocol or the examined Components client.
This was a read-only source review. No MIDI was sent.

## Published workflow

The Components tuning page documents uploading a tuning file, setting its
reference pitch and sending to a slot. Its patch-bank and AFX sections separately
document retrieval. The tuning section gives no retrieval action.
[Novation Components guide](https://support.novationmusic.com/hc/en-gb/articles/4411786558098-Bass-Station-II-Components-guide).

Novation documents per-note tuning writes, RPN table selection and a save
SysEx with no slot argument. The On-Key Dump section describes patch export.
Our ordinary patch captures establish the tuning table selection field only.
The manual's example note-change messages and its save command do not describe
a tuning-table request or reply. [Bass Station II appendix](https://userguides.novationmusic.com/hc/en-gb/articles/25494271270930-Bass-Station-II-appendix#micro-tuning).
[Patch selection evidence](global-control-research.md#microtune-full-selection-field-verified-for-08).

## Components client

Examined [Novation Components 1.72.5](https://components.novationmusic.com/assets/main-D3lNZfPT.js),
SHA-256 `2322dc9fd409e5910e69f83cf5a6d4127c71322ded94e9bbc99cbba4be5a673a`.
The locally retained bundle is under `validation-results/stored-patch-protocol/`.

- The BSII tuning route `BFe.model()` constructs the local parser `Gie`. The
  editor builds one real-time note-change message per entry from the loaded
  file, sends through `device.mainOutput`, and dispatches the `RFe` save
  command (`F0 00 20 29 00 33 00 48 F7`). There is no slot argument in that
  save message. No synth input or readback was found in this route.
- The firmware updater's `getTuningTables()` obtains the `BSII_TUNING_TABLES`
  asset (`bsii-tuning-tables-605.syx`) by HTTP. Its caller then sends that
  asset to the device. This path supplies Novation's factory data and gives
  no information about the user's existing slot contents.

## Decision

Retain the verified temporary table 0 workflow. A stored-table writer needs
a verified recovery route first. The [MIDI Tuning Standard](https://midi.org/midi-tuning-updated-specification)
describes tuning messages for compatible devices. The published BSII support
for note changes does not establish support for bulk dump requests. This
review does not rule out an undocumented firmware command. Reopen the
investigation if Novation supplies a BSII-specific read/export procedure or
we obtain independent device evidence. Factory files cannot establish the
original values of a user-edited table.
