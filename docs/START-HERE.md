# BSII Control — start here

Updated 25 October 2026. This repository is the durable project record. Read this
file and the linked checklist before continuing; do not reconstruct the project
from the old chat. Verify the current checkout and validation status first.

## Location and current state

- Local checkout: `/Users/giuseppeliberale/Documents/Codex/BS2VSTPROJECT`.
- GitHub: https://github.com/rydeen-hrmntcs/BS2VSTPROJECT
- JUCE 9.0.4: `build/_deps/juce-src` (downloaded by CMake).
- [Remaining work and hardware acceptance](remaining-work.md) is the detailed
  checklist. Its dated validation sections are historical, not current evidence.
- Current source includes local MicroTune editing, JSON persistence, bulk editing,
  undo/redo, Scala SCL/KBM, numeric TUN and TUN v2 import. Tuning-table hardware
  transfer/readback/persistent save are still pending.
- [TUN v2 scope and limits](tun-v2-import.md): formulas and keyboard mapping are
  flattened to 128 pitches. Muted/unmapped keys require explicit preservation;
  the editor does not implement note muting. JSON remains supported.
- Fresh offline validation passed all nine groups on 20 October 2026. Local report:
  `validation-results/20261020-030552-19210/summary.txt`. This covers the standalone
  build and offline regressions, not hardware or hosted acceptance.
- TUN v2 AU/VST3 builds and installation completed on 20 October 2026.
  Both native test programs passed: MicroTune preview/apply/undo/JSON/host state
  and processor automation/routing. Local evidence: `validation-results/tun-v2-final/`.
  The preview screenshot was inspected. Both MIT notices match the vendored
  licenses. Installed bundles passed strict local ad-hoc signature verification;
  Apple AU validation passed. Prior bundles are in that report's `installed-before/`.
  The user subsequently confirmed TUN v2 preview, Apply, Undo and Redo in Ableton
  with the A4-plus-50-cents fixture. DAW reopen persistence and hardware tuning
  remain separate checks. Public downloads were not replaced.
- The recall wording fix reports dispatch rather than verified restoration. It
  is now installed with the 25 October sender update. The full nine-group stamp
  above predates later source/tool edits; do not treat it as current.
- Hardware reconnect checks on 20 October: captured sound returned byte-identical
  with the editor closed; Off did not restore that capture; hardware Cutoff changed
  during automation after Off reconnect. Audio was not connected. Recall was last
  saved Off. See `docs/project-sound-recall.md` for evidence and limits.
- Tuning transfer testing was deferred: the user needs tables 1–8 preserved and
  had no audio routing for pitch measurement. Table 0 was authorized for a later
  test and selected after power-cycle, but no tuning data was sent. Resume pitch
  measurements only once audio/tuner is available; hardware backup is unresolved.
- Global Tune isolation (20 October): the paired-data candidate did not establish
  negative receive support. Later manual-adjustment problems disappeared with
  Ableton closed, an empty Set, and the original Set's BS2 track set to MIDI From
  = No Input. This strongly implicates host-track MIDI echo interacting with
  asymmetric Tune send/receive encoding, not a proven persistent firmware fault.
  Keep that host input route disabled; the last requested resting Tune value is 0.
  No new probe commands were sent during isolation; the temporary sender was
  removed. Read `docs/global-control-research.md` before further work. Hardware
  recording needs a separate route that does not return these messages to BS2.
- Local Control retest (20 October) with the host input route blocked: CC122=0
  and 127 both left the menu showing On. No state transition or audible response
  was established. Keep Local Control unimplemented pending a reliable test;
  final display On, no Tune/table/preset commands sent in this retest.
- Public prerelease `v0.8.2-preview.20261018` is an OLDER binary release.
  A source push does not replace its downloads. Publish new binaries only when
  requested and clearly state their compatibility and validation scope.

## Immediate next steps

1. Run `python3 -B tools/midi-lab/lab.py status` for compact, source-fingerprinted
   offline validation status. If stale, inspect the intervening changes and run
   relevant checks; use `./tools/validate.sh` when a full refresh is warranted.
2. The read-only SysEx stream inspector is implemented: `python3 -B
   tools/midi-lab/lab.py inspect-sysex FILE [--json]`. See research-toolkit.md.
   Focused MidiLabTools and ResearchTools CTest groups passed; the full validation
   stamp predates this tool change. A passive manual All dump then yielded 128
   complete 154-byte messages, preserved locally in
   `midi-lab-results/20261020-111111-record-9cb540/patch-bank.syx`. The original
   stream and extraction hashes are recorded in patch-bank-research.md. User
   patch contents stay ignored/private. The user supplied an independent
   Components Get Bank/download export; it matched all 19,712 bytes exactly
   (128 messages). See components-comparison.json beside the capture and the
   research note for hashes. The .syx sender restriction is now implemented,
   native-tested and installed (25 October); see edit-buffer-file-send.md. It
   rejects banks/other commands without MIDI or preset-state changes. AU/VST3
   builds, strict local signatures and AU validation passed. Next:
   the [restoration plan](bank-restoration-plan.md). `lab.py review-bank FILE
   [--against BACKUP] [--json]` now provides read-only strict export review and
   per-index differences; focused tests passed. Labelled hardware readbacks for
   0, 1, 11 and 127 now match their exported payloads; 11 and 127 each matched
   uniquely, whereas entries 0–9 share payloads. Details: patch-bank-research.md.
   Selected-entry extraction is now implemented: `lab.py extract-bank-entry
   BANK INDEX NEW_FILE.syx [--json]`. Focused tests passed, including actual
   production-gate checks. Extracted 0/1/11/127 match saved readbacks exactly.
   A temporary entry-11 hardware send and full readback passed; a fresh starting
   patch was then restored byte-identically. See bank-entry-extraction.md and
   midi-lab-results/20261025-012517-bank-entry-temporary-roundtrip/summary.json.
   The older pre-selection temporary sound remains separately preserved in
   midi-lab-results/20261025-011530-backup-42e60e/baseline.syx (not reloaded).
   The Banks page is now implemented and built: open/browse silently, then
   explicitly load one temporary patch. It retains a local bank/selection across
   editor closure without embedding or replaying the bank in DAW state.
   The new PatchBankOffline model test and focused tool groups passed, as did
   the expanded native editor test. AU/VST3 were installed after confirmed Live
   exit; licence/signature checks and AU validation passed. See bank-browser.md.
   Manual Ableton bank opening, editor-reopen selection retention and explicit
   entry-11 temporary loading passed; hardware readback matched exactly and the
   pre-load sound was restored with verified readback. See bank-browser.md.
   No stored-slot write is authorized/implemented;
   that next stage needs a fresh bank backup and a specified disposable target.
   The user confirmed the installed Ableton
   file chooser rejects the Components bank with the expected Not sent status.
   Stored-slot writing/restoration remain unverified; do not transmit the bank.
3. The MicroTune keyboard/cents visualization is implemented in source; targeted
   native interaction/render checks passed with zero failures. AU/VST3 built;
   installed after Live exit, with prior bundles backed up. Installed licence
   notices/signatures and Apple AU validation passed. The user confirmed graph
   updates after cents edits, Undo and Redo, plus keyboard-to-table selection in
   Ableton. Octave navigation is built and native-tested (zero failures); staged
   and installed after confirmed Live exit. Installed signatures/notices and AU
   validation passed. The user confirmed both octave buttons update keyboard
   labels and table selection in Ableton.
   Hardware tuning transfer is deferred until audio-interface/tuner setup is
   available; no tuning data has been sent. See
   microtune-editor.md. Sequence slot 1 was authorized for replacement; all four
   Play selections worked. External MIDI recording failed to advance steps in
   this setup (Sync Off, IN routing); paused pending stronger evidence. The
   ordinary patch dump stayed byte-identical across an audible note change.
4. The TUN v2 build/install increment and manual Ableton preview/apply/undo/redo
   check are complete. Manual DAW reopen persistence is not newly verified.
   Choose the next unfinished feature from remaining-work; hardware tasks need
   the synth.
5. With the synth available, follow the ordered hardware checks in remaining-work.
   Without it, continue only work that can be meaningfully validated offline.
6. For later installations, confirm Ableton has actually quit; never quit it for
   the user. Ask for save and Cmd+Q if needed, back up the bundles and verify them.

## Remaining feature areas

Global Tune receive encoding; Local Control; sequence slots/steps; tempo/clock
routing; MicroTune hardware transfer, readback and save; patch-bank management;
AFX after the other synth areas; GUI redesign after features settle. Wider host,
multiple-device, Intel/older macOS coverage and Developer ID/notarization remain
separate work. See the detailed checklist for evidence and unresolved questions.

## Decisions and working preferences

- Preserve the 95 published host parameter indices (Paraphonic is index 94).
  Local tuning documents are separate from automation parameters and ordinary
  patch data; local editing/import sends no hardware MIDI.
- Current builds target Apple Silicon/macOS 27+. Do not silently broaden support.
- External Instrument currently provides the Ableton notes/audio workflow.
  Standalone note forwarding/audio integration is optional future scope.
- GUI reference: `/Users/giuseppeliberale/Documents/Bass Station II Synth Editor.png`.
- The synth was unavailable during the current offline work. Do not claim hardware
  acceptance from an offline build, virtual MIDI test or documentation.
- Keep token use low: short updates, compact status output, targeted file reads,
  batch independent checks, and let the user operate Ableton for manual tests.
  Avoid repeated full audits, screenshots and tests without a concrete reason.
- Source changes and routine GitHub updates are authorized in this workstream;
  keep release assets separate. Respect current tool/sandbox approval requirements.
- Synced ChatGPT-project `sources/` files are read-only reference material.

## Suggested first message in a new chat

“Continue BSII Control in /Users/giuseppeliberale/Documents/Codex/BS2VSTPROJECT.
Read docs/START-HERE.md and docs/remaining-work.md, check Git status and compact
validation status, then tell me the next unfinished step. Keep token use low.”
