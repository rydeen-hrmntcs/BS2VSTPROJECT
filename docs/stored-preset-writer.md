# Explicit single stored-preset writer

26 October 2026. Separate from temporary patch loading, ordinary file sending,
Send All, parameter automation and project recall. Protocol evidence and the
scoped physical slot1 round trip are in [stored-patch research](stored-patch-write-research.md).
This increment adds plugin integration; its own installed/hosted acceptance is
separate from that earlier scoped hardware test.

## Workflow

1. Stop DAW playback and leave other MIDI utilities and the hardware controls
   idle. Keep the BSII track MIDI From = No Input to avoid the known host echo.
2. In Banks, open a complete bank and select the source entry. Choose the stored
   destination (0–127, the hardware numbering). Opening/selecting alone sends
   nothing. Load Selected still affects only the temporary buffer.
3. Click Prepare stored write. It first requests the current temporary sound,
   then all 128 stored presets, one at a time. No stored write is sent. Allow
   roughly 15–20 seconds depending on reply/timer scheduling.
4. The complete hardware bank, original destination, current sound, exact planned
   packet and JSON plan are saved and read back from disk before the preview is
   ready. The preview shows the frozen source entry and destination's old name.
5. Click Write stored preset and review the confirmation (source, destination,
   backup folder and exact packet SHA-256). Cancel sends no write. Confirmation
   belongs to that specific prepared transaction, expires after two minutes, and
   rechecks the saved files and the hardware destination before sending.
6. Exactly one validated stored packet is sent, followed by a 500ms settle and
   fresh requests for all 128 stored presets. Success requires the destination
   to match the planned packet and every other slot to match its original backup.
   Dispatch is never reported as verification. No automatic retry or rollback.
7. Get Patch from BSII can refresh the current-sound editor after completion.
   Writing a stored slot does not claim the editor/current buffer now represents
   that slot. Stored operations do not change known preset identity or APVTS.

## Backups and recovery

Show backup opens the operation's unique directory under JUCE's user application
data directory, `BSII Control/StoredPresetBackups/`. Contents are private:

- `current-sound.syx`: validated temporary sound before the operation. Restore
  with the existing temporary Send .syx action if needed; that action cannot
  overwrite a stored slot.
- `before-bank.syx`: all 128 original stored entries, suitable for Open Bank.
- `original-slot.syx`: original destination in stored format (not accepted by
  the ordinary temporary file sender).
- `planned-write.syx`: the exact intended stored packet.
- `write-plan.json`: versioned source/destination/device record and SHA-256
  fingerprints of the bank, original slot, planned packet and current sound.
- `after-bank.syx`: complete verification replies, retained even when contents
  differ from the expected result.

To intentionally restore a successfully changed slot, open that operation's
before-bank.syx, select its original destination entry, choose the same target,
then Prepare and explicitly confirm through this workflow again. The restoration
gets its own fresh backups. Do not replay the entire bank through another sender.
On a timeout/mismatch or connection interruption, keep the backups and inspect
actual stored contents before choosing a recovery action; no rollback is claimed.

## Architecture and boundaries

`BSIIStoredPatchWrite.h` is a message-thread state machine with injected MIDI and
file callbacks. The processor owns the transaction and services it independently
of editor lifetime. Native receive threads only publish SysEx as before. All
current/128-slot requests require matching modern responses after the request's
serial; other commands, indices, stale replies and unsupported profiles cannot
satisfy them. Default spacing is 100ms between reads and a 3s reply timeout.
A fresh destination recheck precedes the sole destructive dispatch point.

The model snapshots the source bytes (all 144 payload bytes), destination and
output identifier. Connection changes invalidate active/ready operations; DAW
state restore cancels them. Nothing is serialized for automatic replay. A
confirmation token cannot authorize a different preparation. Core timer MIDI
parameter/recall dispatch waits while a transaction is busy. Conflicting direct
patch requests, temporary sends, sound capture and channel commands are blocked.
Closing the editor does not abort an already confirmed verification or cause a
new write. No full-bank writer, tuning save, AFX or sequence content commands
are present. Existing 95 host parameter indices are unchanged.

Protocol has no transaction IDs or write acknowledgement; leave Components and
other controllers idle. A matching readback verifies these bytes at the time of
reading, not arbitrary future persistence. Physical validation so far covers
slot1 only; power-cycle behavior, other destinations, multi-device/multi-instance
ownership and interrupted recovery remain distinct acceptance items.

## Validation and installation — 26 October 2026

AU/VST3 and focused/native test builds passed. PatchBankOffline passes synthetic
transactions including read-only preparation, explicit single dispatch, all-slot
verification, failed backup, destination changes, wrong device, expired/stale
replies, missing replies before/after dispatch and no automatic retry/rollback.
The native editor/virtual-synth test passed with zero failures. It checks exact
current-sound/full-bank file backups, a fingerprinted plan, stale confirmation
tokens, real direct-input stored replies, one stored write, all-slot verification
with the editor closed, unchanged APVTS/current preset identity, and no replay on
host-state restore. Earlier assertion failures exposed a reused accumulating
test buffer and overlapping normal editor refresh; file contents were correct,
the test initialization was isolated, and current-patch response/readiness guards
were tightened. Stored replies cannot satisfy Get Patch's current-patch request.
The existing native processor automation/connection/recall test also passed.

Final evidence is under `validation-results/stored-patch-ui-accepted/`:
`native-test.log`, `processor-automation.log`, `stored-write-preview.png` and
`build.log`. The rendered ready preview was inspected. No physical synth was
used in these native tests. The focused PatchBankOffline CTest group passed.

Ableton was process-confirmed closed before installation. Prior AU/VST3 are
preserved under `installed-before/`. Staged and installed strict local ad-hoc
signatures and both bundled MIT licence notices passed; Apple AU validation
passed (`auval.log`, `installation.txt`). Public release assets were not replaced.
Manual hosted preparation/confirmation/verification and restoration passed on
27 October 2026, as detailed below.


## Manual Ableton acceptance — 27 October 2026

The user opened BSII-bank-before.syx, selected entry11 and destination1, prepared
a fresh backup and explicitly confirmed the stored write. The screenshot reported
Stored preset1 verified, other127 unchanged. They then opened the first
operation's before-bank.syx to restore the destination. An intermediate entry0
preview was corrected to entry1 and prepared again before the confirmed
restoration. The user confirmed the final verified status.

The saved complete verification files independently support both outcomes:
entry11-to-slot1 changed exactly slot1 and matched its planned packet; the final
entry1-to-slot1 restoration changed exactly slot1 and returned all19,712 bank
bytes to the pre-test reference. Final SHA256:
`675ee6c6bced2facd0b880a58b1bbe8bdfd2587237d774d78d5c61dc64a8a232`.
This compares the plugin's recorded hardware replies; no additional live query
or power-cycle test was performed. Other write destinations remain untested.

Private evidence: `validation-results/stored-patch-ui-accepted/hosted-acceptance-20261027.json`
records backup-directory paths, source/destination, changed indices and hashes.
Original account/download and patch files remain private. Components Save keeps
the bank in the account; Download creates the local .syx file.

Finder navigation: Library is hidden by default. In Open Bank, Command-Shift-G
with `~/Library/BSII Control/StoredPresetBackups/` exposes the backup directories;
Show backup opens the selected operation's directory directly. Choose the
original write's before-bank.syx, not a later preparation's backup, when restoring
pre-test contents. In particular the newest folder is not necessarily the
original-state backup once another preparation has been made.
