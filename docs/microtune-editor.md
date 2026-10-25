# MicroTune editor page

Added 19 October 2026, using the existing offline table model and file format.
The **MicroTune** tab edits a local working table. It does not read, send or save
hardware tuning tables. The existing FX + Perf MicroTune Table parameter still
selects the synth's active table separately.

## Use

- Scroll through all 128 MIDI keys. The table shows key number, note name,
  offset in cents and calculated target frequency (A4 = 440 Hz).
- Double-click an **Offset (cents)** cell and enter a signed number. Each offset
  is relative to that key's equal-tempered pitch, not cumulative. For example,
  +50 on key 69 gives A4 +50 cents. Invalid text or out-of-range pitches keep
  the previous value and show an error.
- Select notes by clicking the Key or Note columns. Shift-click selects a range;
  Command-click adds/removes individual notes. **Select All** selects all 128.
- Enter signed cents beside **Select All**, then use **Set Offset** to replace
  selected offsets relative to equal temperament, or **Add Cents** to transpose
  their current pitches. **Reset Notes** returns only selected notes to equal
  temperament. Each action is atomic: an invalid value or any out-of-range target
  rejects the entire selection without clipping or partial edits.
- **Undo / Redo** revert or reapply whole document changes, including bulk actions,
  individual offsets, names, file targets, New and successful Load. A new edit after
  undo discards the redo branch; unchanged values and rejected edits add no history.
  The latest 64 changes are retained while the processor instance exists. History
  survives closing the editor but is not saved in the DAW Set; restoring host state
  begins a fresh history. Saving a file does not clear history. File-save status
  compares the current content with the last successful save/load.
- **Name** labels this working document; **File target** records table slot 0–8
  in the document only. Changing it does not select a table on the synth.
- **Import Tuning...** opens a separate local `.scl`/optional `.kbm` or numeric `.tun` preview, with
  explicit reference pitch and unavailable-note handling. Applying imports is
  undoable. See [Scala import](scala-import.md) for the full workflow and limits.
- **Save Table...** writes a local JSON table file. **Load Table...** reads the
  same versioned format, with all 128 notes required. It does not load `.syx`,
  `.bsiistate` or Scala files. Invalid files leave the current table intact.
- **New Table** creates equal temperament while retaining the chosen file target.
  New/Load asks before replacing edits not yet saved to a local table file.

No synth is required for these operations. The existing offline file tool can
export this JSON format to SysEx; see [offline tool instructions](microtune-offline.md).
There is no hardware Send or Save Tables action on this page.

## Working state and DAW projects

The processor owns the working document, so closing and reopening the editor
does not discard it. Saving a DAW Set also embeds the document separately from
APVTS parameters. Reopening the Set restores it silently and marks it as needing
a separate local file copy; it does not load the table into the synth. File-save
status and a working document in a DAW Set are distinct.

Old Sets without the new metadata start with equal temperament. Invalid/oversized
session table metadata also resets this working document to that default. The
current file loader rejects invalid contents instead of replacing valid work.
The UI synchronises if a host restores state while the editor remains open.

Get Patch, live MIDI, ordinary snapshots, Send All and project-sound recall do not
replace or transmit this working table. A captured hardware patch records the
selected table ID, not its complete tuning contents. Merely keeping a local table
inside the Set does not change that hardware-recall limitation.

Document edits notify the host that non-parameter state changed; they are not
95 new automation lanes. The existing **95 parameters and indices** are unchanged.
No MIDI transmission or persistent-save command is connected to these actions.

## Bounds and presentation

Absolute target pitches are currently restricted to MIDI-note coordinates 0–127.
Thus negative offsets on key 0 and positive offsets on key 127 are rejected.
This conservative application bound is not a measured BSII hardware frequency
limit. Calculated target Hz is not an audio measurement and does not include
other oscillator offsets or Global Tune.

The note list scrolls. At small plugin window sizes, the whole page can scroll
vertically as well; resize the editor larger to show more rows and status.
This is a functional development UI; the final visual redesign remains deferred.

## Verification

The standalone build and all eight offline groups passed. Native editor tests
exercise actual editable cells, positive/negative and invalid input, local file
round trips, failed loads/saves, editor recreation, existing/new/old/malformed
DAW state and stable host parameter indices. A private virtual MIDI sink asserts
zero output across these operations. Native test renders cover normal size and
minimum size, including scrolling to the note list. No physical synth is used.

Report: `validation-results/20261019-114143-34549/`. Final standalone/AU/VST3
builds passed; all eight offline groups passed again. The native MicroTune editor
test passed, including a fresh processor restore and normal/minimum-size renders.
Existing native processor automation/connection and direct SysEx tests passed.
Renders were visually inspected; page scrolling exposes the note list at minimum
size and the status fits at the normal size. Native checks sent no physical MIDI.

Ableton was confirmed closed before installation. Previous AU/VST3 bundles were
backed up under the report's `installed-before/`; new copies were locally signed,
installed, strict signatures verified and Apple AU validation passed. Public
preview assets were not replaced. File chooser dialog interaction and actual DAW
acceptance of this new page remain manual checks; tested file methods are the same
ones invoked by those callbacks.

## Undo and bulk-edit increment — 19 October 2026

Added local document history and atomic note selection operations. No tuning-table
transmission, Scala import or automation parameters were added. Hardware acceptance
is not required for these local operations; DAW acceptance of the updated page is
still a manual check.

Validation for this increment: standalone/AU/VST3 builds passed; all nine offline
test groups passed in `validation-results/20261019-165539-5836/`. Native editor
checks passed with zero failures, covering UI bulk actions, atomic rejection,
undo/redo, saved-content status, history capacity and branch handling, metadata
history, editor recreation, host-state history reset and zero MIDI output.
Existing native processor automation checks also passed. Final page renders in
`validation-results/tuning-history-editor-final/` were visually inspected; the
minimum-size viewport reaches notes and status by scrolling. Ableton was checked
closed before installation; prior bundles were backed up under `installed-before/`,
and installed AU/VST3 signatures and Apple AU validation passed. Actual DAW use of
these new controls remains a manual acceptance check. Public preview assets remain
unchanged.

Numeric TUN details and supported subset: [TUN import](tun-import.md).


## Tuning visualization — 25 October 2026

The local MicroTune page now includes an all-128-key cents-offset graph and a
keyboard showing the focused octave. Graph values are offsets from each MIDI
key's equal-tempered pitch, with an automatic symmetric scale of at least
±50 cents. These are calculated document values, not measured synth pitches.
The focused note shows its name, MIDI key number, offset and calculated Hz.

Click the graph to focus a note/octave; click a white or black keyboard key to
select it in the existing table. Left/right arrows move one MIDI key; Shift
extends a range and Command/Control-click adds to the selection. Table selection
also updates the keyboard. Selections never audition notes or alter pitches.
Edits, import application, Undo/Redo and document replacement refresh the graph
through the existing document revision path. The outer page scrolls at smaller
window sizes so the visualization, existing editing controls and table remain
reachable. No host parameters or hardware-transfer behavior were added.


Validation: AU/VST3 Release builds passed. The expanded native MicroTune editor
program passed with zero failures, including graph endpoint selection, black-key
hit precedence, arrow navigation, table-to-visual focus, last partial octave and
selection leaving the document revision unchanged. Existing edit/import/history,
file/state, MIDI-isolation, file-send gate and bank-browser checks passed in the
same run. Graph/keyboard and default/minimum-size renders were inspected; the
outer scrollbar exposes the keyboard and note list at smaller heights. Evidence:
`validation-results/tuning-visual/native-test.log`, `tuning-visual.png` and
`microtune-*.png`. Staged AU/VST3 licence notices and strict local signatures
passed. AU/VST3 installation completed after confirming Live had quit. Prior
bundles are backed up under `validation-results/tuning-visual/installed-before/`;
installed licence notices and strict signatures passed, as did Apple AU
validation (`auval.log`). Finder metadata added to the original staged bundle
was detected before replacement; temporary staging with that metadata removed
passed verification. Manual hosted visual acceptance is next. Public release
assets are unchanged.


Manual Ableton visual acceptance — 25 October 2026: on the installed build,
the user confirmed that changing a note's cents offset updated the graph, and
that Undo and Redo both updated it correctly. Manual graph/keyboard selection,
import-driven visual refresh and hosted reopen checks were not newly verified;
related native coverage is recorded above. No hardware tuning transfer was
performed.


### Octave navigation increment — 25 October 2026

The user also confirmed clicking a visual keyboard key selects the matching
note in Ableton. Added Previous octave / Next octave buttons and a visible
note-name/MIDI-key range under the keyboard. Navigation selects the same note
class 12 keys away, clamps to key 127 in the final partial octave and disables
movement beyond either end. Existing table/graph selection still follows the
focused note. This is local selection only, with no tuning edits or audition.
AU/VST3 builds passed. The native editor test passed with zero failures,
including octave moves preserving note class, both disabled boundaries, final
partial-octave clamping and unchanged document revision. The updated render was
inspected. Evidence: `validation-results/tuning-octaves/native-test.log` and
`tuning-visual.png`. Staged licence notices and strict local signatures passed.
AU/VST3 installation completed after confirming Live had quit. Previous bundles
are preserved under `validation-results/tuning-octaves/installed-before/`.
Installed strict signatures, licence notices and Apple AU validation passed;
see `installation.txt` and `auval.log` in that report directory. Hosted
octave-button acceptance passed: the user confirmed Next octave and Previous
octave update both keyboard labels and selected table note in Ableton
(25 October 2026).

Hardware tuning transfer was deferred at the user's request until an external
audio interface is available for tuner measurement. No tuning data was sent.
