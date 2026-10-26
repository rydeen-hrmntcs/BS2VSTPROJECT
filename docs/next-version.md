# Next version notes

For current evidence and unfinished work, see [the 19 October checklist](remaining-work.md).
Earlier entries below preserve implementation chronology.

## Explicit stored-preset writer — 26 October 2026

Banks now has a separate destination, Prepare and confirmed Write action. It
backs up the current sound and all stored patches, rechecks files/destination,
sends one preserved stored packet and verifies every slot independently. No
automatic retry/replay/rollback or full-bank writer. Model/native virtual-synth
and processor regressions passed; AU/VST3 are installed with strict signatures,
licence checks and AU validation. Manual hosted acceptance is next. Prior scoped
physical slot1 write/restoration passed, with the entire bank restored exactly.
See [workflow and limits](stored-preset-writer.md).

## Bank browser — 25 October 2026

Banks now opens complete exports, lists entry numbers/names and explicitly loads
one temporary patch through the validated sender. Browsing sends nothing. Local
bank/selection survive editor closure but are not embedded or replayed with DAW
state. Stored-slot/bank restoration remains unimplemented. See
[workflow and validation](bank-browser.md).

## Edit-buffer file-send restriction — 25 October 2026

Send .syx now accepts only the validated temporary-patch format; bank/stored
commands, other SysEx and malformed/trailing data are rejected without output
or preset-identity changes. Native virtual-MIDI tests and AU/VST3 build/install
checks passed. See [behavior and evidence](edit-buffer-file-send.md). No bank
restoration feature or public binary release was added.

## Deferred parameter gaps — overlay audit, 18 October 2026

User requested retaining this list for later while completing manual LFO sync
acceptance first. Compared the photographed AFX overlay with the current control
registry and cached official firmware 4.14 manual. These functions are not yet
exposed as plugin controls; do not treat their MIDI routes or SysEx fields as
hardware-verified merely because the manual mentions them.

- [x] Mod Wheel → Filter Frequency: implemented following the full hardware sweep;
      manual hosted acceptance still pending.
- [x] Mod Wheel → LFO2 → Filter Frequency: implemented following the full hardware
      sweep; manual hosted acceptance still pending.
- [x] Oscillator tuning error (`E:OscErr`): implemented; basic Ableton MIDI and Get Patch checks passed.
- [x] Paraphonic mode (`P:Para`): implemented on verified NRPN 0:107; basic Ableton send/receive, paused RX and Get Patch checks passed. Earlier conflicting CC110 observations retained in research notes.
- [x] Glide divergence (`g:GlideDiv`): implemented; basic Ableton MIDI and Get Patch checks passed.
- [x] Independent Sub Oscillator coarse and fine tuning: implemented; basic Ableton send/receive, paused RX and Get Patch checks passed; intermediate display rounding approximate.
- [ ] Hardware Local Control.
- [x] Hardware Global MIDI Channel: explicit Global-page command with confirmation; basic hosted command and hardware-origin following acceptance passed. Remote-change persistence across reboot remains unresolved. The top Channel selector alone does not set the synth channel.
- [x] MicroTune table selection 0–8: MIDI send/receive and SysEx decoding implemented;
      basic hosted send/receive, paused RX and Get Patch acceptance passed. No tuning-table contents are edited or saved.
- [ ] Global Tune receive encoding and tuning-table editing/transfer remain unresolved.
- [x] External Input Gain: separate Global control; basic Ableton send/receive, paused RX and Get Patch isolation checks passed; excluded from patch snapshots and Send All.
- [ ] AFX overlay selection, editing and read protection; keep AFX work last.
- [ ] Full-bank dump/management workflow; single-patch request/capture exists.

Envelope retrigger, retrigger count and fixed duration already have app controls;
their presence on the overlay does not imply a missing implementation. LFO
Speed/Sync and all 35 divisions are now implemented; basic hosted mode/division,
paused-RX Get Patch and one LFO1 division automation check passed. Audible clock
accuracy remains untested. Sequence-step editing and verified sequence-slot behavior remain separate
unfinished work. Final GUI redesign follows feature implementation.

## Live RX status wording

Requested on 16 October 2026 during hardware validation.

Show the current state using these exact labels:

- **Live RX is paused** when live CC/NRPN reception is paused.
- **Live RX is running** when live CC/NRPN reception is enabled.

The current checked control labelled “Resume Live RX” was confusing because it described an action while its tick represented the paused state. Use the requested status wording in the next version. SysEx reception remains active while Live RX is paused.

Implemented: the control now shows these status labels. A tick still means reception is paused; clicking toggles the state.

## Sub Octave switch and control visibility — 17 October 2026

Replaced the Sub Octave rotary presentation with connected −1/−2 buttons and an explicit selected-state highlight. The underlying `subOctaveRaw` parameter ID, 0–127 storage, CC81 route, and snapshot layout remain compatible. Raw values below 63 select −2; 63 and above select −1. The user identified hardware −2 while a fresh patch showed byte 37 bit 3 clear, establishing the label association. Clicking −1 sends 127; clicking −2 sends 0. Both selections were verified by fresh hardware patch readback; hardware ended at its starting −2 position. Clicking an already selected button keeps one state selected.

The standalone editor now starts at 900×760 and allows resizing (760×600 through 1600×1200). Control grids adapt to width; pages scroll vertically when their content is taller than the viewport. Patch Volume shares the Oscillator 1 section, placing its twelve controls in two visible rows at the default size. Visual checks confirmed both rows and their values are visible, and scrolling reveals the bottom FX control/value. MIDI, decoding, and snapshot data formats are unchanged.

## Linked Trigger selectors

Editing Trigger on either Amp Env or Mod Env now sets both selectors and sends
both trigger NRPNs to the hardware. Both pages label it 'Shared: Amp + Mod'.
This follows the requested front-panel-style workflow and observed switch behavior;
it does not claim every mixed-mode sound-engine interaction is established.
Distinct parameter IDs and incoming patch/snapshot values remain preserved.
Retrigger, Retrigger Count, Fixed Sustain and ADSR remain independently editable.

## Full signed Bend Amount

Bend Amount now supports -24 through +24 semitones, displays a plus sign for
positive values and accepts signed numeric entry. CC107 and stored byte16 use
semitones+64. Negative values no longer collide with missing-update sentinels in
live reception, SysEx application or duplicate-send tracking. Parameter ID remains
bendAmount; snapshot numeric values remain semitones. Expanding the parameter
range changes normalized host automation scaling; AU/VST3 migration is not verified.

## Signed modulation and clearer panel controls

Velocity-to-Amp/Mod displays -63…+63; Modwheel-to-LFO1/Osc2 and Aftertouch-to-
Filter/LFO1/LFO2 Speed display -64…+63. Numeric input converts back to existing
raw0…127 storage (velocity retains raw minimum1). Confirmed hardware Aftertouch
LFO2 Speed endpoints -64/+63 correspond to NRPN0:76 values0/127. No route or
snapshot storage change. Osc Sync now uses highlighted Off/On buttons with
canonical raw0/127; all nonzero received values show On. Sub Octave remains -1/-2.
Portamento label now reads Portamento / Glide Time. Checklist hardware instructions
now use overlay labels and the confirmed first-C Filter Tracking shortcut.

### Preset selection and refresh

- Preset selector 0–127, Previous/Next and reload. Selecting sends Program Change
  on the selected MIDI channel; does not write any stored preset.
- Hardware Program Change follows the selected channel and Live RX setting.
- Waits 200 ms after the latest selection before requesting the edit-buffer patch.
  Requests are serialized; a reply belonging to a superseded selection is discarded.
- Starts with unknown preset identity. Snapshot load, SysEx import/send and MIDI
  output/channel changes clear it. An edit-buffer dump alone cannot identify a
  stored preset slot.
- Hardware display preset 1 was captured sending raw Program Change 1 on channel 1.
  Remaining slot numbering follows the documented 0–127 range.
- Automated checks cover settling, rapid changes, stale response rejection,
  unknown state, bounds and millisecond counter wrap.

## Correct Unicode label encoding

Labels containing arrows, ellipses and the title dash now explicitly use UTF-8
conversion. JUCE's ASCII string constructor previously interpreted individual
UTF-8 bytes as characters, producing garbled symbols. This affects presentation
only; MIDI routes, parameter state and patch data are unchanged.

## Plugin patch reception independent of host SysEx filtering

VST3/AU instances now open a direct SysEx-only hardware input matched to the
selected output name. Host-routed notes/CC/NRPN/Program Change retain their
existing path; duplicate host dumps are skipped while direct input is active.
The input belongs to the processor and closes before its receive state is torn
down. Standalone input configuration remains unchanged. Connection and timeout
messages distinguish plugin and standalone. Verified with an isolated virtual-port
test and successful154-byte Get Patch in Ableton Live12.4.6, including paused RX.
See plugin-smoke-test.md for evidence and remaining host integration work.


### Processor-owned parameter MIDI output

Host parameter edits continue sending after closing the editor. The processor owns
the native output and coalesces APVTS changes on a 100 Hz message-thread timer;
GUI controls no longer send their own duplicate messages. Scoped suppression keeps
hardware reception, patch imports and state recall silent. Shared Trigger edits
work in the processor too, while imported mixed trigger fields stay lossless.
Channel and Live RX state survive editor recreation within the same instance.
Verified with isolated native MIDI regression and actual Ableton Resonance readback
with the editor closed. Full session connection recall and recorded automation
playback remain pending; this does not establish broad DAW compatibility.


### Saved MIDI connection

Host sessions now include selected MIDI output identifier/name, channel and Live
RX. State loading schedules native connection from the processor timer, independent
of editor creation, and retries unavailable saved ports. A changed identifier may
use one exact name match; ambiguous/missing matches never select another synth.
Connection restore sends no initial patch. Portable snapshots remain parameter-only.
Existing Sets need a save with the updated build after selecting the output.
Local regression covers a real processor save/new-instance restore without editor,
channel3 MIDI dispatch, obsolete identifiers and missing-device handling. Manual
Ableton restart verification is still pending.


### Optional project sound recall

Added On reconnect: Off / Stored preset / Captured sound and Capture Project Sound.
Host state stores the explicit recall choice, preset target and raw edit-buffer
capture. Capture is fresh and explicit; routine DAW saves, automation and Get Patch
do not replace it. Default Off preserves existing Sets. Processor recall runs once
on session restore or detected saved USB endpoint reconnection, then waits300ms
before flushing queued parameter changes. It never runs on each Play. The accepted
patch format only addresses the temporary edit buffer. No stored preset save command
is introduced. Local native/offline checks, all format builds and AU validation
pass; manual hardware acceptance remains pending. See project-sound-recall.md.


### Hardware controls independent of the host device chain

Plugin direct MIDI input now accepts CC/NRPN and Program Change alongside SysEx.
This follows a user-isolated receive failure after External Instrument, while
reception on a MIDI-only track and native hardware capture both worked. The host
control decoder is skipped while direct input is open, avoiding duplicate updates;
missing/ambiguous direct inputs retain host fallback. Live RX pause, selected-channel
rules and standalone Active MIDI Inputs are retained. Notes, pitch wheel and clock
are excluded from direct input and keep their existing host route. Expanded native
regression, original SysEx-only test, offline suite, format builds and AU validation
pass. Original-chain hardware acceptance remains pending.

## MicroTune selection

Added MicroTune Table 0–8 in FX + Perf as appended host parameter index 88.
Hardware NRPN 0:0 updates the control; output uses the distinct documented
RPN 0:3 followed by RPN null selection. Patch decoding uses verified byte
115 bit 0 and byte 116 bits 4–6. All nine hardware fixtures are retained.
Connection recall, snapshots and automation use the existing parameter paths.
The control selects a table only; tuning-table contents and stored preset slots
are untouched. Basic Ableton send/receive, paused RX and Get Patch acceptance passed.
Automation playback and incoming whole-patch verification remain pending.
See global-control-research.md for measured evidence.

## Offline MicroTune table tool — 19 October 2026

Added a reusable 128-note model, versioned local document codec, validated
note-change SysEx encoder and BSIITuningTableTool create/set/inspect/export
commands. No MIDI port access, persistent save or plugin UI integration is added.
The existing 95 host parameters and current plugin behavior are unchanged.
All eight offline groups and the actual CLI file workflow pass. See
[microtune-offline.md](microtune-offline.md) for application pitch bounds and
hardware acceptance still required.

## Local MicroTune editor page — 19 October 2026

Added a MicroTune tab with all 128 notes, editable cents offsets, calculated target
Hz, document name/target metadata and New/Load/Save local table actions. The
working document belongs to the processor and restores silently with DAW state;
existing 95 automatable parameter indices are unchanged. Patch snapshots, Get
Patch, Send All and sound recall do not consume it. No hardware table send or
persistent save is enabled. See [page usage and checks](microtune-editor.md).

Local-page validation/install: final standalone/AU/VST3 builds, all eight offline
groups, native MicroTune UI/state/file/zero-MIDI checks, existing processor and
direct SysEx regressions passed. Normal/minimum-size renders inspected. With Live
closed, prior plugins were backed up and updated AU/VST3 installed; strict
signatures and Apple AU validation passed. Report:
`validation-results/20261019-114143-34549/`. Real DAW UI acceptance remains pending;
no public prerelease assets were replaced.

### Local tuning undo and bulk edits — 19 October 2026

Added Undo/Redo for the last 64 local document changes, plus selected-note Set
Offset, Add Cents and Reset Notes. A bulk action validates the entire selection
before applying and undoes in one step. Save/load content determines the local
file dirty indicator. Closing the editor keeps history; host state restore clears
history while restoring the working document. No automation parameter indices or
hardware tuning-transfer behavior changed.

### Local Scala import — 19 October 2026

Added .scl import with optional .kbm, an explicit reference/mapping preview and
per-key unavailable reasons. Applying is one undoable document replacement; an
explicit keep-current policy is required for incomplete mappings. Destination
slot metadata, MIDI routing and automation parameter indices remain unchanged.
A pinned MIT-licensed Surge tuning-library dependency is vendored with notices
included in macOS bundles. TUN and hardware tuning transfer remain pending.

### Numeric TUN import — 19 October 2026

The shared Import Tuning dialog now accepts numeric TUN v0/v1 data, including the
current u-he generator layout. The file controls its base reference; KBM/manual
reference fields are disabled. Imported pitches share preview, explicit preservation,
one-step undo and native JSON saving. Formula/mapping sections cannot silently
fall back to numeric compatibility data. No new hardware action or host parameter.

### TUN v2 formula and keyboard mapping import — 19 October 2026

Added a bounded single-dataset TUN v2 adapter using the pinned MIT AnaMark formula
engine. Supports the documented equation tokens, sequential references, loops,
whole-scale retuning and keyboard mapping. Muted keys are explicit preview states;
keeping them preserves current pitches rather than muting MIDI. Formula errors
cannot fall back to numeric compatibility sections. JSON, undo and host parameter
indices remain unchanged. Both third-party notices ship in the macOS bundles.
