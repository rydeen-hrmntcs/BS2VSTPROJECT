# Current coverage and remaining work

Updated 19 October 2026. Audited source at d20c653, the cached official firmware
4.14 manual, retained fixtures, and the preceding session's user reports.
This is the current task checklist; older status documents preserve chronology.
For the latest handoff and pending installation checks, read [Start here](START-HERE.md).
No synth was connected or measured for this audit. A mapped field is not proof
of every UI path, acoustic result or parameter interaction.

## Implemented inventory

The source registry has **95 automatable parameters: 63 numeric and 32 choices**.
**94** have entries in the shared patch-decoder registry. **Input Gain** is the
one global parameter intentionally absent from ordinary patch decoding,
patch snapshots and Send All. Hardware MIDI Channel setup is a separate,
non-automatable command, not a 96th automation parameter.

| Registry section | Automatable parameters |
| --- | ---: |
| Master | 1 |
| Osc 1, including independent Sub tuning | 13 |
| Osc 2 | 9 |
| Mixer | 6 |
| Filter | 9 |
| LFO / Glide | 16 |
| Amp Env | 8 |
| Mod Env | 8 |
| FX / Perf | 17 |
| Arp / Seq | 7 |
| Global: Input Gain | 1 |

Source: BSIIMidi.h, BSIIPatchFields.h and PluginProcessor.cpp. MIDI Lab's
registry check compares the implemented IDs with all shared decoder fields.
The layout explicitly retains published host indices; do not append a numeric
parameter in a way that shifts the existing Paraphonic parameter at index 94.

## Completed feature groups and evidence limits

| Feature group | Current evidence | Remaining limit |
| --- | --- | --- |
| Original 76 controls | Retained patch captures, decoder regression, manual MIDI tests and targeted fixes | Exact Coarse display rounding, acoustic calibration and exhaustive combinations are not established |
| Arp/Seq six additions plus existing Swing | Live MIDI, 22 labelled patch fixtures, paused-RX Get Patch; basic Play/Record mode checks | No sequence-step editor or verified sequence-slot contents workflow |
| LFO Speed/Sync and both 35-entry division selectors | Full stored-value sampling; manual bidirectional mode/division and paused-RX checks; Ableton LFO1 1/4-to-1/8 automation reached the synth | Audible clock accuracy and broader automation remain untested |
| Two mod-wheel filter routes | All 256 raw sent-value/readback samples and hardware-emitted sweeps | Explicit installed-plugin UI and audible modulation acceptance still pending |
| MicroTune table selection 0–8 | All nine readbacks; basic hosted send/receive, paused-RX Get Patch | Table contents are not edited or transferred |
| Oscillator Error, Glide Divergence, Paraphonic | Retained captures, output/readback tests and basic hosted acceptance | Audible effects and broader automation/session checks remain |
| Independent Sub Fine/Coarse | All 458 sent-value/readback samples; basic hosted checks | Intermediate display rounding is approximate |
| Input Gain | Full emitted raw range; targeted incoming endpoints; hosted checks and patch isolation | No calibrated dB display, global query or power-cycle persistence test |
| Hardware MIDI Channel | Remote 1-to-2-to-1 and incoming Cutoff on 2; hardware-origin selector following with Live RX on | Other channels and remote-change persistence across power-off not hardware-verified |
| Presets, direct live input, automation and saved connection | Targeted Ableton VST3 and native regression checks, including editor-closed output and Set reopen | Other DAWs/AU host use and multiple devices/instances not broadly tested |
| Project sound recall | Set reopen, stored/captured USB reconnect; captured patch byte-identical with editor closed; Off did not restore capture; changing Cutoff readbacks during automation after Off reconnect | Automation immediately after captured-mode reconnect, other channels/devices and acoustic checks remain |

Evidence: [recall](project-sound-recall.md), [global controls](global-control-research.md),
[Arp/Seq](arp-seq-research.md), [LFO sync](lfo-sync-research.md),
[mod-wheel routes](modwheel-filter-research.md), [oscillator extras](osc-extra-research.md),
[Sub tuning](sub-tuning-research.md), [Input Gain](input-gain-research.md),
[host checks](plugin-smoke-test.md).

## Unfinished features

- [ ] **Global Tune:** establish a negative/zero/positive receive encoding.
  Hardware emits NRPN 64:6 with offset values, but replaying raw 14 produced
  +14 rather than -50. Standard RPN 0:1 is only an unverified candidate, not
  established BSII support. Keep a control unimplemented until receive works.
  A 20 October paired-data probe failed. Later unexpected manual stepping was
  isolated to the original Ableton track input route: closed Live, an empty Set,
  and MIDI From = No Input each allowed normal manual adjustment. Track MIDI
  echo is strongly implicated; keep that loop disabled and review the log.
  Negative receive support remains unresolved; do not resume speculative sends.
- [ ] **Local Control:** establish reliable Off and On reception/readback.
  CC122 Off was reported; attempts at On did not confirm success. A manual
  toggle emitted no messages in two captures. No plugin control yet.
  On 20 October with host MIDI From = No Input, CC122=0 and 127 both left
  the Local menu showing On. No audible check was available; reception remains
  unverified, and the unchanged On display is not proof of an On command.
  On 25 October, manual Off silenced the hardware keyboard through headphones;
  channel-1 CC122=127 did not restore sound. Manual On restored it. The synth
  channel was not freshly checked; no reliable remote On mapping is established.
- [ ] **Sequence slots and contents:** verify selection in Play/Record,
  backup/restoration, notes/rests/ties and persistence before attempting step
  editing. Manual headphone checks on 25 October confirmed plugin selection
  of all four existing sequences in Play, matching physical slot selection.
  Slot 1 was authorized for replacement; physical two-note recording worked.
  A controlled audible C–G to C–E change left the full 154-byte edit-buffer dump
  identical, so ordinary patch backup does not preserve those sequence notes.
  External note-entry attempts did not advance recording or change playback,
  including with Function + Latch displaying IN; incoming clip notes were
  separately audible. Remote recording and content backup remain unverified.
  Random-access
  sequence read/write support is not established.
- [ ] **Tempo and clock:** decide scope for DAW clock/transport integration
  and verify routing. Internal tempo has no verified plugin control here.
  On 26 October, Ableton output Sync (Song) with its audio engine enabled
  drove the BSII hardware tempo and audible arp at 120 → 60 → 120 BPM.
  Arp continued at roughly the last tempo on Stop and followed 90 BPM on
  restart. Synced LFO 2 at 1/4 audibly followed 120 → 60 → 120 BPM through
  filter modulation. LFO 1 at 1/4 also slowed at 60 BPM and retained its last
  received rate after Stop despite host tempo changes while stopped.
  Exact timing/phase, other divisions, clock-loss
  recovery and DIN remain unverified.
  Existing LFO sync-division controls do not implement a clock generator.
- [ ] **Arp MIDI routing:** investigate the hardware Function + Latch toggle
  between external MIDI-to-Arp and Arp-to-MIDI; no verified remote command.
- [ ] **MicroTune hardware table workflow:** an [offline table editor and SysEx
  exporter](microtune-offline.md), plus a [local editor page](microtune-editor.md),
  are implemented. Actual hardware transfer, readback/restoration, persistent save
  remain pending; table selection is already implemented and hardware-tested.
- [ ] **Remaining tuning imports:** [Scala `.scl`/`.kbm` import](scala-import.md)
  now has local preview, explicit reference/mapping and unavailable-note handling,
  and one-step undo. [Numeric `.tun` import](tun-import.md), including the current
  u-he export layout, is also implemented. [TUN v2 formula and keyboard mapping
  import](tun-v2-import.md) is now supported. Library browsing, scale generators,
  multi-scale file selection and MTS-file import remain later work. See
  [import research](tuning-import-research.md).
- [ ] **Patch-bank management:** [research and staged plan](patch-bank-research.md)
  recorded; full-bank backup/import and any stored-slot
  writing need explicit design and validation. A [restoration plan and strict
  offline bank review](bank-restoration-plan.md) are now implemented/documented;
  selected-preset/readback comparison passed at 0/1 and uniquely at 11/127.
  [Selected-entry conversion/export](bank-entry-extraction.md) passed offline
  exact-byte/production-gate tests and one temporary hardware load/readback plus
  baseline restoration. Official Components builders now establish the stored
  receive/request commands; a user-approved slot1 write/readback/restoration
  passed with complete all128 before/after verification on 26 October. See
  stored-patch-write-research.md. A [separate explicit writer](stored-preset-writer.md)
  is now implemented, model/native-tested and installed with verified backups,
  confirmation, fresh destination recheck and all-slot readback. Manual Ableton
  slot1 write/restoration passed on 27 October; the final complete verification
  bank matches the original exactly. Other destinations, power-cycle persistence
  and interrupted-transfer recovery remain pending.
  The [Banks page](bank-browser.md) now opens/browses a bank silently and loads
  one temporary patch explicitly. Model/native tests, AU/VST3 installation,
  licence/signature checks and AU validation passed. Manual Ableton browsing,
  editor-reopen selection retention and explicit entry-11 loading passed with
  exact hardware readback and verified baseline restoration; see its guide.
  Load Selected still targets only the temporary sound. The separate stored-write
  action requires Prepare, a saved backup and explicit confirmation; no full-bank
  write workflow exists.
- [ ] **AFX, last synth feature area:** [read-only research/capture](afx-protocol-research.md)
  now preserves25 active data records, repeated byte-identically with o0 reported.
  Name reads timed out; buffer provenance, full8-bank capture and restoration
  remain unverified. No mutations sent. The [local archive browser](afx-local-browser.md)
  now opens strict25-record captures, selects keys and saves byte-identical copies;
  plugin parameter editing and hardware transfer are later increments.
  Remaining scope: overlay-bank selection, protection,
  per-note data editing, transfer and explicit save/clear operations. The manual
  documents dedicated messages; their presence does not verify our hardware
  implementation or justify treating overlay data as a normal 154-byte patch.
- [ ] **Final GUI:** redesign against the user's reference after features settle.

Performance notes, pitch bend, sustain and aftertouch remain part of DAW routing;
modulation-depth controls are implemented separately. Operating without Ableton
External Instrument via dedicated note forwarding/audio integration is an optional
future workflow, not a completed or agreed prerequisite for the editor.
Extended software modulation is deferred to v2 after finishing v1 and its final
GUI. The user agreed this order on 27 October; see [v2 milestones](v2-roadmap.md)
for staged scope and acceptance gates. This is planning only, not an instruction
to start v2 features during remaining v1 work.

## Next hardware session — short, ordered checks

Run with playback stopped initially, an expendable test Set, and the hardware,
plugin and External Instrument channels matched. Check the channel again after
power cycling; a successful SysEx reply does not prove channel matching.

1. [ ] Verify the final channel-1 Set save; use a fresh backup for any unsaved sound.
2. [ ] Finish mod-wheel filter UI/send/receive/Get Patch checks, then listen to
   both assignments on a simple patch. These are implemented, not missing routes.
3. [x] Captured-sound USB reconnect matched all 154 patch bytes with editor closed;
   Off did not restore the capture; Cutoff automation updated hardware after Off
   reconnect (20 October 2026). Listening and automation immediately after
   captured-mode reconnect remain untested; see the recall evidence.
4. [ ] Investigate remote MIDI-channel power-cycle persistence separately from
   immediate channel following; use a restorable starting configuration.
5. [ ] Run focused Global Tune and Local Control probes only after reviewing
   the existing evidence and planning manual restoration.
6. [ ] Investigate sequence slots/contents and clock behavior, then remaining
   tuning/bank workflows. Preserve data before any recording or stored write.
7. [ ] Implement and validate AFX after the other synth features.

Recall UI wording clarified in source on 20 October 2026: stored-preset and
captured-sound recall now report “Sent” and “Hardware not confirmed.” Dispatch
is not a hardware acknowledgement; matching-channel readiness is not confirmed.
This wording change was installed with the 25 October sender update.

Additional release checks: multiple plugin instances/devices; disconnect during
capture/recall/automation; flat automation segments and hardware recording;
AU in a real host, other DAWs and testers' computers. Existing native malformed
input and reconnect tests cover specific paths, not all real-world failures.

## Work possible without hardware

- [x] Record the latest user-confirmed recall/channel results and audit counts.
- [x] Research sequence access and tuning-table protocols against primary
  sources; see [sequence findings](sequence-access-research.md) and
  [MicroTune transfer plan](microtune-transfer-research.md). No documented sequence
  payload was found; tuning note-change/save commands provide a stronger codec
  candidate. Hardware acceptance remains pending.
- [x] Research patch-bank transfer protocols separately; see
  [bank findings and staged plan](patch-bank-research.md). Manual/Components
  workflows are documented; ordinary-bank wire framing/addressing remain
  unverified. The [read-only stream inspector](research-toolkit.md#inspect-a-binary-sysex-stream)
  is implemented and passed focused tests. A manual All capture yielded 128
  complete messages, preserved privately with hashes. An independent Components
  export matched all 19,712 bytes exactly. Sender hardening is now implemented
  and installed with native virtual-MIDI rejection tests, AU/VST3 builds and AU
  validation (25 October). Restoration/slot writes remain unverified.
- [ ] Extend offline fixture checks when a new reproducible case warrants it.
- [ ] Prepare release notes and a newer preview when requested. The public
  v0.8.2-preview.20261018 binaries remain the earlier 4993591/86-control build;
  updating source documentation does not update those binaries.

Existing architecture/OS support is unchanged: current public binaries target
Apple Silicon/macOS 27+. Wider support, Developer ID signing and notarization
remain deferred. This checklist makes no development-percentage estimate.

## Offline validation — 19 October 2026

`./tools/validate.sh` passed configure, the standalone/regression build and all
seven CTest groups (5.51 seconds for the test run). Report:
`validation-results/20261019-112719-32467/summary.txt` (local ignored output).
These checks sent no MIDI and required no synth. Registry extraction found 95
unique controls, 63 numeric/32 choices, and 94 shared patch fields; Input Gain
was the sole unmapped control. No runtime source, installed plugin or public
preview binary was changed. Native virtual-port integration, AU validation and
fresh AU/VST3 builds were not repeated for these documentation-only changes.

### Offline MicroTune increment — 19 October 2026

Added reusable table/document/encoding modules and a local create/edit/inspect/
export tool. Host parameters, MIDI device behavior and installed plugins are
unchanged. `./tools/validate.sh` now builds these targets and passes **eight**
CTest groups. The earlier seven-group report above remains historical evidence.
See [tool usage, limits and validation](microtune-offline.md).

### Local MicroTune page — 19 October 2026

Added note editing and local file load/save in a MicroTune tab. A separate working
document survives editor recreation and DAW-state save/restore, silently. The
hardware-selected table, patch/snapshot paths and 95 automation parameters remain
separate. See [page instructions and limits](microtune-editor.md). Hardware table
transfer/save is still pending, and the public preview is not automatically updated.

### Offline research tools — 19 October 2026

Added a compact `status` command with source-fingerprinted validation and an
`analyse-sweep` command for existing edit-buffer captures. The Glide Divergence
fixture provides a reproducible reference; reports retain evidence hashes and
separate inferred candidates from hardware claims. See [research toolkit](research-toolkit.md).
The suite now has nine registered test groups. This increment changes no plugin
runtime code and adds no tuning-editor or hardware-transfer features.

### Local tuning history and bulk editing — 19 October 2026

MicroTune now has Undo/Redo (64 document changes per processor instance) and
selection-based Set Offset, Add Cents and Reset Notes. Invalid operations reject
the entire selection. History survives editor recreation and resets on host state
restore; the current document remains embedded in the Set. Scala import and
hardware table transfer remain pending. See [editor use](microtune-editor.md).
