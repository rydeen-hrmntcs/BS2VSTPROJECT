# Patch-bank review and restoration plan

25 October 2026. Read-only bank review is implemented; restoration is not.
No bank messages were transmitted during this increment. Existing installed
AU/VST3 sender restrictions remain unchanged.

## Offline review now available

```sh
python3 -B tools/midi-lab/lab.py review-bank source.syx
python3 -B tools/midi-lab/lab.py review-bank source.syx --against backup.syx
python3 -B tools/midi-lab/lab.py review-bank source.syx --against backup.syx --json
```

Files must match the export shape independently observed in the manual/Components
comparison: exactly 19,712 bytes, 128 complete 154-byte BSII messages, header bytes
6–7 equal 00 01, bytes 9–12 zero, and every byte-8 export index 0–127 present
exactly once. General framing and 7-bit payload checks come from the stream
inspector. This strict profile rejects missing/duplicate indices, unknown header
variants, extra channel/real-time bytes and malformed/trailing data. Other export
formats are not automatically converted. Unknown variants can still be examined
with inspect-sysex. [Capture evidence](patch-bank-research.md).

Preview lists export indices, original message positions and message/payload
SHA256 hashes in JSON. Optional comparison matches entries by export index,
not stream position; reordered equal entries produce zero changes while the
whole-file byteIdentical flag remains false. Without a reference file it makes
no claim about differences from current hardware. With a saved reference, it
compares those two files only; a saved file is not a live hardware query.

Output is stdout only, source files remain unchanged and no MIDI transport or
write commands are created. Export indices are not advertised as verified
hardware write destinations. Every review keeps transmitEligible=false,
writeAddressingVerified=false and restorationVerified=false. Preview success
is not authorization to send a bank. JSON contains hashes, not a replay script.

## Next hardware evidence, before stored writes

1. Preserve the current temporary sound separately. Obtain a fresh complete
   stored-bank backup before any write session; the existing dated backup may no
   longer represent the current synth. Verify completeness, hashes and a second
   retained local copy. Keep tuning tables, AFX and sequence contents separate.
2. With user agreement to change the active sound, physically select a known
   stored preset and request its edit buffer. Compare it with the corresponding
   exported entry, retaining header and payload differences. Confirm numbering
   using more than one labelled preset, rather than assuming export order is a
   verified receive address. No memory overwrite is needed for this comparison.
3. Only after matching evidence, design/validate a separate selected-entry
   audition conversion to the known edit-buffer format. Never strip/change a
   header and transmit it merely because the field decoder accepts its body.
   Preserve every unknown payload byte; do not regenerate a patch from the 94
   exposed fields and call it a full restore.
4. Document the stored-write route, acknowledgement/timeout behavior and the
   exact bytes to send before asking for authorization for a specific disposable
   destination. Do not brute-force opcodes or use unrelated AFX commands.

## Proposed stored-slot operation, not implemented

A future UI should show source file/hash, source entry, verified destination
number, and the current destination backup. The user approves that concrete
replacement. Require explicit action per write; do not send on file selection,
transport start, Set loading or USB reconnect. Bank state stays separate from
ordinary patch recall and the 95 published automation parameters.

Test one approved disposable slot first. Read back the stored result independently
of the edit buffer, and compare the full preserved payload. Obtain a fresh bank
readback to check neighbouring entries remained unchanged. Restore the original
slot through the same verified path and compare it again. A dispatched message,
unchanged LCD or successful MIDI API call is not a confirmed write.

If a transfer fails or readback differs, stop further writes, retain the exact
source/backup/progress logs and report the actual state; do not retry or promise
rollback based on unverified commands. Full-bank restore is a separate later
operation with a fresh review/authorization and whole-bank post-readback. Streaming
all 128 messages cannot be assumed atomic; partial writes need an explicit
recovery plan before that operation is exposed.

The existing Send .syx action stays restricted to validated temporary patches.
Do not use it for bank restoration. [Sender limits](edit-buffer-file-send.md).

## Verification — 25 October 2026

Focused CTest MidiLabTools and ResearchTools both passed (3.05 seconds). New tests
cover per-index comparison independent of file order, one-entry differences,
complete/unique index requirements, wrong commands/headers, malformed bytes,
truncation/trailing data, invalid reference banks, false write-eligibility flags,
and CLI source-file preservation. Only synthetic banks use retained single-patch
test data; private user banks were not committed as fixtures.

The tool reviewed the private manual and Components exports successfully: 128
unchanged entries, byte-identical at the previously recorded SHA256. No runtime
plugin code, installed bundles or hardware data changed in this increment.
The full offline validation stamp predates these tool changes; no broad rebuild
was needed for this bounded Python-tool change.

## Read-only hardware evidence follow-up — 25 October 2026

The user selected 0, 1, 11 and 127 and each fresh edit-buffer payload matched its
exported entry exactly. Header byte 7 differed for 0; bytes 7/8 differed for the
others. The 0/1 pair is ambiguous because export entries 0–9 share payloads, but
11 and 127 each matched only their own export index among all 128 entries.
See [capture details and limits](patch-bank-research.md#labelled-presetreadback-comparisons--25-october-2026).

This completes initial labelled readback evidence. Next is selected-entry
conversion/export and offline byte-exact/validation tests; hardware audition
and any stored-slot write remain separate. The tool's write-addressing and
transmission flags must remain false. No automatic memory-write workflow was
established by comparing outgoing exports with edit-buffer responses.

## Selected-entry extraction and temporary round trip — 25 October 2026

[Offline extraction](bank-entry-extraction.md) is now implemented with exclusive
output creation and unchanged payloads. Focused tests include the actual production
send gate; four private real outputs exactly match saved hardware readbacks. A
subsequent entry-11 edit-buffer transfer matched its full readback, then the fresh
starting temporary patch was restored and matched independently. No stored slot
was written. This advances temporary loading only; the stored-write authorization,
addressing and fresh-bank verification prerequisites above remain in force.


## First stored-slot round trip — 26 October 2026

[Official Components source and scoped hardware evidence](stored-patch-write-research.md)
now establish stored write type1 and stored request type65, both with zero-based
destination. The user authorized slot1. Fresh Components and independent all128
stored requests matched before writing. One entry11-to-slot1 write changed only
slot1; one restoration returned the complete bank byte-identically. The original
temporary sound was also restored with verified readback. This supersedes the
unresolved receive-route status for that scoped test, not the remaining UI,
other-destination, power-cycle or full-bank restoration work. Existing review
flags remain read-only. No general stored-write plugin action exists yet.
