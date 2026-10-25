# Patch-bank browser

25 October 2026. The Banks tab opens a complete bank and explicitly loads one
selected entry into the BSII temporary edit buffer. Stored-preset writing and
full-bank restoration are not implemented.

## Use

1. Open Banks → Open Bank... and choose a complete BSII .syx export.
2. Browse the 128 numbered entries and patch names. Opening, selecting, scrolling
   and double-clicking do not send MIDI or change editor parameters.
3. Preserve any unsaved hardware sound with Capture Project Sound or a separate
   edit-buffer backup. Stop playback and avoid hardware MIDI echo routing.
4. Choose an entry and click Load Selected to BSII. This replaces the temporary
   sound using the existing validated edit-buffer sender. It does not save into
   the displayed stored slot. The editor's patch controls update silently from
   the selected data; host automation gestures and extra CC sends are suppressed.
5. The send status reports dispatch, not hardware acknowledgement. Get Patch can
   retrieve the actual hardware sound for comparison. Existing automation can
   subsequently change it; there is no sample-accurate playback scheduling here.

The bank and selection survive editor close/reopen for the current plugin
instance. They are not embedded in the Set or automatically replayed on Set
loading, transport start or reconnect. Keep the original bank file. The loaded
bank is an immutable in-memory snapshot, so changing the file externally does
not silently change the selected patch; deliberately reopen it to read a new file.
Loading from a bank does not silently replace Capture Project Sound or the
selected automatic recall policy. Capture/configure recall explicitly if desired.

## Accepted format and boundaries

The strict parser accepts exactly 128 complete 154-byte messages with the BSII
prefix, 7-bit interior bytes, zero header bytes 6 and 9–12, byte 7 equal to 1,
and unique export indices 0–127 in byte 8. Order can vary. The file read is
bounded to 19,713 bytes so trailing data is detected without loading a huge file.
Malformed/partial/unknown-format files leave the previous bank and selection
untouched. Other export variants are not guessed into compatibility.

Names use the existing sanitized printable-name decoder. Selection is by export
index, not source order. Loading preserves every payload byte and changes only
the observed header into the validated temporary format. The strict sender checks
the final result again before clearing known preset identity or transmitting.
No bank/stored-slot command is exposed. No new automation parameters are added.
See [observed capture evidence](patch-bank-research.md),
[extraction](bank-entry-extraction.md) and [sender limits](edit-buffer-file-send.md).

## Validation

AU/VST3 and native editor test builds passed. Focused MidiLabTools, ResearchTools
and the new PatchBankOffline CTest group passed (4.20 s). The model test was then
strengthened to distinguish wrong-but-7-bit-valid commands from framing errors;
its focused rebuild and rerun passed (0.37 s). The overall offline suite now has
ten registered groups, but a full suite stamp was not refreshed in this increment.
`tools/validate.sh` includes the new model target for future full validations.

The native editor test passed with zero failures: empty/open browser, all 128
rows/names, reordered export indices, silent open/selection, retained previous
bank on invalid input, missing-output rejection, exactly one byte-identical
converted patch on explicit Load, no CC echo or host automation gestures,
preset identity clearing, transient workspace exclusion from host state and
editor-reopen persistence without retransmission. Existing MicroTune import,
Undo/Redo, state/file checks and file-send restrictions passed in the same test.
Virtual MIDI only; no private user bank was used as a committed fixture.

Local evidence: `validation-results/bank-browser/native-test.log` and
`bank-browser.png`. The rendered page was inspected: entry selection, names and
actions are readable; a shorter editor uses the outer scrollbar for the footer.
No fresh manual hosted acceptance or physical hardware load is claimed by these
native results. Installation evidence follows when completed.

## Installation completed — 25 October 2026

After the user saved and quit Live, a process check confirmed it closed. Both
AU/VST3 bundles were installed with prior versions backed up under
`validation-results/bank-browser/installed-before/`. Both MIT notices matched
their source files, strict local ad-hoc signatures passed and Apple AU validation
succeeded. Logs: auval.log and installation.txt in that report directory.
Public preview downloads were not replaced. No physical hardware send was
performed for installation.


## Ableton hardware acceptance — 25 October 2026

The user opened the private Components bank and confirmed all 128 entries/names.
After restarting Live, reopening the bank was necessary (intentional transient
workspace). Entry 11 and the bank remained selected/visible across closing and
reopening only the plugin editor. The user then explicitly clicked Load Selected
to BSII with playback stopped. Fresh hardware readback matched the expected
entry-11 temporary patch byte for byte. The pre-load temporary sound was restored
and its readback verified. Stored presets and tuning-table contents were not
written; no acoustic verification was performed.

Private evidence (ignored):
- Before load: `midi-lab-results/20261025-132413-backup-a4d338/baseline.syx`
- Loaded readback: `midi-lab-results/20261025-132459-backup-b32a2f/baseline.syx`
- Expected: `validation-results/bank-extraction-check/entry-11-temporary.syx`
- Verified restoration: `midi-lab-results/20261025-132504-restore-2fae03/`

Selection-only hardware silence was covered by native virtual-MIDI tests; this
manual session did not take paired hardware captures around selection.
