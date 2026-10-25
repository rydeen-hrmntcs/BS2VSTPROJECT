# Global-control discovery — 18 October 2026

## Local Control

User opened Function/Exit + Local once and selected Off then On, explicitly
confirming both labels on the hardware display. Passive capture
`midi-lab-results/20261018-170122-record-3819f5/` recorded zero MIDI messages.
Hardware was left On. This is a single no-transmission observation, not proof
that Local Control never emits MIDI in any configuration or that reception is
unsupported. Timing overlap was not independently measured.

The cached official manual's Global: Local section describes the on-key control
and On/Off behavior but provides no receive route in that section. No BSII-specific
CC/NRPN receive route has been verified. Do not claim a mapping or automatically
send a generic channel-mode message based only on its standard MIDI name.
Global readback, ordinary patch inclusion and hosted UI remain unverified.
No Local Control parameter has been added to the plugin.

## Global Tune — emitted MIDI verified

User opened Function/Exit + Tune once. Starting display was 0, endpoints -50/+50,
and user explicitly confirmed restoring display 0 after the sweep.
Capture `midi-lab-results/20261018-170313-record-d5b16b/events.jsonl` contains
NRPN **64:6** raw values 14–114, all 101 unique values. Final raw value was 64.
Display conversion is raw minus 64, matching the manual's cents range -50…+50.
This is NRPN MSB 64, not 0, and only Data Entry MSB was observed.

Remote reception, independent value-query support, ordinary patch inclusion and
plugin integration remain unverified. Do not assume the patch-based sampler can
restore this global setting. Hardware ended at its original Tune 0.

### Tune receive test failed; hardware restored manually

The scoped sender transmitted NRPN 64:6 with Data Entry CC6=65 (channel 1),
expecting +1 from the emitted raw-minus-64 model. User reopened Tune without
touching Patch/Value and observed **50**, not 1. This invalidates the assumed
simple symmetric receive encoding. The correct encoding and mismatch cause
remain unknown. No global-value query is verified yet.

User manually restored Tune to 0 and confirmed completion. No further tuning
test was sent. Keep Tune receive support disabled until independently verified;
do not use the emitted offset as a receive implementation specification. The
temporary Tune option was removed from the sender, retaining verified Input Gain.

## Global MIDI Channel — emitted changes captured

User performed the requested hardware channel 2 then channel 1 selection.
Capture `midi-lab-results/20261018-171002-record-f7e410/events.jsonl`
contains two matching groups:

- Channel 2: SysEx `F0 00 20 29 00 33 00 58 02 F7`, then NRPN 64:4, CC6=2 on channel 2.
- Channel 1: SysEx `F0 00 20 29 00 33 00 58 01 F7`, then NRPN 64:4, CC6=1 on channel 1.

Channel numbering here is human-facing 1–16. The NRPN notification uses the
newly selected channel. Final emitted channel was 1. Initial channel was not
reported separately by the user. Only values 1 and 2 were tested; remote
reception, other channel values, query support and persistence remain unverified.
Neither observed message format is yet a verified command to send to hardware.
No plugin control was added.

## MicroTune table selection — emitted values 0 and 1 verified

User opened the second Tune/MicroTune on-key function and reported starting
display `t 0`. Requested sequence was `t 0` to `t 1` then back to `t 0`,
followed by Function/Exit; user confirmed completion. Capture
`midi-lab-results/20261018-171137-record-600ae3/events.jsonl` contains
NRPN **0:0**, CC6=1 then CC6=0 on channel 1. No other messages were recorded.
Final emitted selection was 0. This verifies outgoing table selection only for
tables 0 and 1. Supported table range, incoming commands, tuning-table contents,
patch inclusion, persistence and query support remain unverified. No tuning
table contents were edited and no plugin control was added.

### MicroTune full hardware selection range

User confirmed maximum `t 8` and return to `t 0`. Capture
`midi-lab-results/20261018-171238-record-51d87d/events.jsonl` contains
NRPN 0:0 on channel 1 with CC6 sequence 1,2,3,4,5,6,7,8,7,6,5,4,3,2,1,0.
All nine selection values 0–8 are therefore observed, with final selection 0.
This resolves the hardware selection range left open above. Incoming command
support, patch inclusion, persistence and table contents remain unverified.

### MicroTune receive spot-check — documented RPN command

Official source: https://userguides.novationmusic.com/hc/en-gb/articles/25494271270930-Bass-Station-II-appendix
(Table selection section). The receive route is MIDI tuning program change
**RPN 0:3**, distinct from the emitted NRPN 0:0. Documented channel-1
sequence: `B0 64 03 65 00 06 tt 64 7F 65 7F` (hex; running status).
The guide says tt 0–9, whereas the observed hardware selector offers 0–8;
value 9 is untested and the scoped sender restricts values to 0–8.

`swift tools/MicroTuneSender.swift 1` sent CC100=3, CC101=0, CC6=1,
then CC100=127 and CC101=127 to clear RPN selection. User reopened
MicroTune without changing Patch/Value and explicitly confirmed display t 1.
This is a successful receive spot-check for table 1, not a full-range test.
Then `swift tools/MicroTuneSender.swift 0` completed to restore the starting
selection; user subsequently confirmed display t 0 after reopening MicroTune
without changing Patch/Value. Restoration is therefore confirmed.
No tuning-table contents or saved presets were written. The official guide
also states the active tuning table can be saved with a patch; local patch
encoding has not yet been established. No plugin control has been added.

### MicroTune ordinary patch storage — table 0/1 comparison

Read-only edit-buffer requests returned 154 bytes at table 0, after RPN selection
of table 1, and after restoring table 0. Retained under
`tests/microtune-fixtures/` as table-0-before.syx, table-1.syx and
table-0-restored.syx. The only 0-to-1 difference is zero-based byte 116,
0 to 16 (0x00 to 0x10). The restored dump is byte-for-byte identical to
the baseline. Thus table selection affects ordinary patch data; bit 0x10
at offset 116 is implicated by this pair. The complete field encoding for
values 2–8 is not yet established. No patch was sent back or saved to a preset
and no tuning-table contents were edited.

### MicroTune full selection field verified for 0–8

The scoped RPN sender selected tables 2–8; each was followed by an ordinary
154-byte edit-buffer request. Relative to table 0, table 2 changed byte 116
to 32, table 3 to 48, table 4 to 64, table 5 to 80, table 6 to 96, and
table 7 to 112. Table 8 instead changed byte 115 from 0 to 1. No other
bytes differed. Combined with the earlier table 1 fixture (byte 116 = 16),
all nine selections decode as:

`table = ((b115 & 0x01) << 3) | ((b116 & 0x70) >> 4)`

Offsets are zero-based. Fixtures table-2.syx through table-8.syx,
range-before.syx and range-restored.syx are retained in
`tests/microtune-fixtures/`. Final RPN selection of table 0 produced a dump
byte-for-byte identical to the range baseline. This verifies RPN reception
and ordinary patch readback for all nine supported selections. Receiving a
whole patch containing this field, host automation and UI integration remain
untested. No tuning-table contents or stored presets were written.

### Plugin integration and local validation

MicroTune Table 0–8 is now implemented in FX + Perf. New parameter index 88
follows all previously published controls. The receive registry retains NRPN
0:0; the output helper explicitly sends RPN 0:3 and nulls the RPN selection.
Processor input clears stale NRPN state on RPN selection. Patch decoding,
editor refresh, parameter state recall and normal automation paths include
the new field. Earlier statements that no control exists describe the
discovery stage and are superseded by this integration.

Validation report: `validation-results/20261018-171953-19614/`. All seven
offline CTest groups passed, including the nine retained hardware fixtures.
Native processor integration passed output bytes, host index preservation,
state recall, emitted NRPN reception and RPN/NRPN isolation. Direct SysEx
integration passed. AU/VST3 built and installed after checking Live was closed,
with previous bundles backed up. Both installed signatures passed strict
verification; Apple auval reported AU VALIDATION SUCCEEDED. Manual acceptance
in Ableton for this control remains pending. Public prerelease assets were
not replaced by this local installation.

### Manual Ableton acceptance passed

User verified the installed plugin in Ableton: selecting table 1 in the plugin
was confirmed as t 1 on the hardware display after reopening MicroTune without
editing Patch/Value. Changing hardware to t 2 updated the plugin automatically.
With Live RX paused, hardware selection t 8 left the plugin at 2; Get Patch
then updated the plugin to 8. User completed the requested plugin selection
back to 0 and resumed Live RX. No separate hardware-display confirmation was
requested for that final selection. These results complete basic hosted
send, receive, pause and SysEx readback acceptance; MicroTune automation
recording/playback and whole-patch transmission remain separately untested.

## Global Tune repeat receive probes

After user-confirmed hardware Tune 0, the scoped diagnostic sender repeated
NRPN 64:6 with CC6=65. User reopened Tune without touching Patch/Value and
again observed +50. User manually restored 0 and confirmed completion.

The next probe sent CC6=1, producing user-confirmed +1. A subsequent CC6=127
probe produced +50, not -1. User then manually returned Tune to 0, exited
the function, and confirmed completion. User also reconfirmed the hardware
range -50 through +50.

These are individual command/display observations, not a complete receive
mapping. They suggest positive direct values with an upper limit but do not
establish a universal clamp rule, negative encoding, 14-bit behavior, a
different RPN route, or a firmware defect. In particular, do not send raw 64
as a presumed neutral value or add a partially working signed Tune control.
No ordinary patch can verify or restore this global setting in our current
tools; restoration was manual and user-confirmed.

### Negative endpoint and exact-message round trip

At the user’s suggestion, a fresh Tune-zero edit-buffer dump was captured:
`midi-lab-results/20261018-225133-backup-df343a/baseline.syx`.
User set hardware Tune to -50 and exited the function. Passive capture
`20261018-225137-record-bd8895/events.jsonl` contains exactly 50 NRPN
64:6 values, CC6 descending 63 through 14, using only CC99, CC98 and CC6.
No Data Entry LSB or SysEx messages were captured.

The -50 dump `20261018-225214-backup-548844/baseline.syx` is byte-for-byte
identical to the zero dump (both 154 bytes). No ordinary patch field for
Global Tune was found in this comparison; this does not rule out other
global SysEx formats. These dumps and the emitted trace are retained in
`tests/global-tune-fixtures/`.

After user-confirmed manual restoration to zero, the diagnostic sender
replayed NRPN 64:6 CC6=14. User reopened Tune without editing and observed
+14, rather than the -50 represented by the hardware’s outgoing raw 14.
User manually returned Tune to 0 and exited, then confirmed completion.

Thus outgoing and incoming representations demonstrably differ for this
exact message. Negative receive encoding remains unresolved. No Global
Tune plugin control or speculative restore encoding has been added. The
temporary diagnostic sender was removed after the experiments. See
global-tune-protocol-research.md for the checked primary references.

## Local Control repeat and standard-message probes

User confirmed starting Local On, then performed the requested hardware
On -> Off -> On sequence and exited. Passive capture
`midi-lab-results/20261018-225625-record-7bfcdd/` recorded zero messages,
matching the earlier no-transmission observation.

A subsequent positive-control capture
`midi-lab-results/20261018-225750-record-55136b/events.jsonl` received three
note-on/off pairs on channel 1 after the user pressed/released one key
(notes 88,100,112). This confirms input reception in the subsequent run;
it does not independently timestamp overlap during the zero-message run.
The note trace is retained as tests/local-control-fixtures/key-positive-control.jsonl.

The MIDI Association defines CC122=0 as Local Off and CC122=127 as Local On:
https://midi.org/midi-1-0-control-change-messages .
That standard alone does not establish BSII implementation support.

A scoped diagnostic sender sent channel-1 CC122=0 from the previously
confirmed On state. User reopened the Local menu without editing and reported
Off. The sender then sent CC122=127; the user answered no when asked whether
Local showed On. An alternate compatibility probe CC122=1 also did not
produce a user-confirmed On display. Exact display text after the latter
two probes was not separately supplied. No audible verification was performed.

User then manually selected Local On, exited the function, and confirmed
completion. The temporary sender was removed. These observations do not
establish a reliable bidirectional receive mapping or a firmware fault.
In particular, do not implement an On/Off control from the apparently
successful Off observation alone. Local remains unimplemented pending
better receive verification; no patch or global-query field was invented.

## Hardware MIDI Channel receive and acknowledgement verified

User confirmed starting channel 1. Scoped sender NRPN 64:4 target 2 on
channel 1 produced user-confirmed hardware display 2. Sending target 1 on
channel 2 restored user-confirmed display 1.

A repeat round trip was passively captured in
`midi-lab-results/20261018-230533-record-56b3cd/events.jsonl`.
The synth emitted exactly two SysEx notifications:
`F0 00 20 29 00 33 00 58 02 F7`, then the same with channel byte 01.
No NRPN echo was recorded in this remote-command run. Notifications provide
a tested confirmation path distinct from ordinary patch replies. The final
notification reports restoration to channel 1. The trace is retained as
tests/hardware-channel-fixtures/remote-roundtrip.jsonl.

The official guide documents a 1–16 hardware channel range:
https://userguides.novationmusic.com/hc/en-gb/articles/25494313827986-Bass-Station-II-in-detail
(Global: MIDI Channel). Remote receive/confirmation is empirically verified
for 1 and 2 only; other channel values remain hardware-untested.

### Explicit Global-page hardware channel command

Added a target selector and Set Synth Channel button, separate from sound
automation parameters. The command sends NRPN 64:4 on the current plugin
communication channel, waits for the validated 0x58 notification, then follows
the reported channel. MIDI output is held while awaiting confirmation.
A 1.5-second timeout reports no confirmation and does not assume success.
Normal Live RX follows hardware channel notifications without needing the
editor open; paused RX ignores unsolicited notifications, while an explicit
pending command still accepts its confirmation.

Notifications are consumed separately from patch data and cannot satisfy
Get Patch or project-sound capture. Existing patch/snapshot parameter counts
and automation indices are unchanged. DAW note routing is separate: changing
the synth/plugin channel does not change External Instrument or track routing.
Basic hosted acceptance remains pending.

### Hardware-channel local validation and installation completed

Report: `validation-results/20261018-231112-29650/`. All seven offline
groups passed. Native processor integration replayed both captured channel
confirmations and passed command routing on the old channel, follow only
after confirmation, one request at a time, timeout without guessed success,
paused RX rules, editor-independent following and separation from patch
serials. The decoder accepts only the exact notification header and range
1–16. Existing parameter indices and all prior integration checks passed.
Direct SysEx regression passed. Standalone, AU and VST3 builds passed.
Live was confirmed closed; installed plugins were backed up and replaced,
strict signatures verified and Apple AU validation passed. Hosted acceptance
remains pending. Public preview assets were not replaced. The scoped
`tools/MidiChannelProbe.swift SOURCE_CHANNEL TARGET_CHANNEL` sender used
in discovery is retained; its transport output alone is not confirmation.

### Hardware-channel hosted acceptance passed

In Ableton, the user selected target 2 on the plugin Global page and clicked
Set Synth Channel. They confirmed channel 2 in the acknowledgement status and
the top communication-channel selector. Moving the hardware Cutoff knob then
updated the plugin on channel 2. Selecting target 1 and issuing the command
again confirmed channel 1 and restored the top selector to 1. These results
complete the basic hosted 1-to-2-to-1 command and incoming-control check.
Other channel values, unsolicited hardware channel changes, power-cycle
persistence and DAW note-routing changes were not tested in this pass.
Public preview assets were not replaced.

### Hardware-origin channel following and power-cycle caveat

Recorded 19 October 2026 from the preceding session's user observations.
Following the remote 1-to-2-to-1 acceptance check, a USB power cycle exposed
hardware channel 2 while the plugin remained on channel 1. Get Patch succeeded,
but Program Change did not affect the hardware until the plugin channel was
matched to 2. The reason the prior remote restoration to 1 did not persist is
unresolved; remote channel changes must not be advertised as power-cycle
persistent on this evidence.

The user later manually selected hardware channel 1 with Live RX running and
confirmed that both the plugin top Channel selector and Global selector followed
hardware changes. External Instrument was separately confirmed on channel 1.
This adds manual acceptance of unsolicited hardware-origin channel following.
See project-sound-recall.md for the matched-channel USB reconnect result and
its limits. No new synth measurements were made on 19 October.

## Paired-data Global Tune probe and recovery — 20 October 2026

Do not repeat this probe as routine testing. The user reported unexpected manual
negative-step behavior after it; the cause is unresolved, and no negative Global
Tune receive support was established. No plugin control was added.

Starting conditions: user reported Tune 0, exited the function, confirmed channel
1 for hardware/plugin, and Ableton playback was requested stopped. The temporary
CoreMIDI sender required exactly one Bass Station destination and dispatched:

`B0 63 40; B0 62 06; B0 06 7F; B0 26 7F; B0 63 7F; B0 62 7F` (hex).

This selected NRPN 64:6, sent Data Entry MSB/LSB 127/127, then nulled NRPN.
A possible signed 14-bit -1 was the hypothesis, not a supported BSII encoding.
The user reopened Tune without editing and observed +50. On attempting manual
restoration they reported the lowest reachable value was 49.

The sender then dispatched the same sequence with both data bytes zero:
`B0 63 40; B0 62 06; B0 06 00; B0 26 00; B0 63 7F; B0 62 7F`.
After exiting/reopening Tune without editing, the user confirmed display 0.
A requested manual range check then produced this user-reported sequence:
0 -> -1 -> 49, followed by inability to return to zero manually. This was not
captured as an outgoing MIDI trace and is not an established wrap/clamp algorithm.
Further reproduction was stopped. A second zero-pair dispatch and exit/reopen
again produced user-confirmed 0; no further Patch/Value edits were requested.

Final state: display 0, manually confirmed. Normal negative adjustment range,
actual frequency, internal state and persistence across reboot are NOT verified.
No audio was connected. Zero-pair dispatch restored the displayed value but did
not establish a complete functional repair. Do not claim firmware corruption,
a specific arithmetic bug, or proof that the LSB is ignored from these reports.
No tuning-table data, preset write or persistent-save command was sent.

The temporary probe source/executable were removed after recording their exact
messages here. Do not reintroduce the negative candidate without a separate
recovery/diagnosis plan. Further investigation should establish normal manual
behavior in an isolated fresh session first, and seek Novation guidance rather
than expanding speculative sends. A synth firmware regression test is unavailable
in this repository; local unit tests cannot reproduce this hardware symptom.

## Ableton input-route isolation — 20 October 2026

The later observations supersede treating the manual-adjustment symptom as an
established standalone hardware fault. User reported that Tune operated normally
with Ableton closed and restored Tune 0 manually. After disconnecting USB, opening
an empty Ableton Set without plugins/External Instrument, and reconnecting USB,
the requested manual 0 -> +1 -> 0 check worked normally. In the original Set,
changing the BS2 track to MIDI From = No Input also made the requested manual Tune
check work at the correct values. No additional diagnostic MIDI was sent.

These controlled differences strongly implicate a MIDI echo through that track's
input/device/output route. This is consistent with the independently measured
asymmetry: a hardware Tune +1 emits raw 65, while remotely receiving 65 displayed
+50 in earlier tests. Returning the outgoing message can therefore change Tune
unexpectedly. The exact outbound echo was not captured during this isolation;
its specific device/path remains an inference, not a traced packet-level result.
Neither firmware corruption nor a permanent change to the manual range is
established. The paired-data probe did not establish a negative receive encoding.

Working configuration: MIDI From = No Input on the original BS2 track. This
blocks keyboard/CC input through that host track; MIDI clips and parameter
output can still be tested separately. The plugin's existing direct hardware RX
is a separate path; no code was changed. If hardware MIDI recording is needed,
use a separate input track without a return route to the synth, and verify that
configuration before recommending it as accepted. Do not re-enable the previous
input/output loop or send more speculative Tune probes to diagnose this symptom.
The final requested resting state is Tune 0 and Function/Exit; a complete range
sweep and acoustic frequency measurement were not performed in this isolation.

## Local Control with host input blocked — 20 October 2026

After the Tune routing isolation, playback was requested stopped and the BS2
track remained MIDI From = No Input. The user opened Function + Local once and
reported On, then exited and confirmed readiness. A scoped CoreMIDI probe sent
exactly `B0 7A 00` (channel 1 CC122=0), with no Tune, patch or save commands.
The user reopened Local without editing and reported On, not Off.

The probe then sent `B0 7A 7F` (CC122=127, intended Local On). After exiting and
reopening Local, the user again reported On. This matches the initial display but
is not positive evidence of On reception because no state transition was seen.
Off reception was not confirmed in this run. The earlier isolated Off report
from 18 October remains historical evidence; it has not been reproduced under
this blocked-input configuration. Do not silently promote either observation
into a reliable implementation or an assertion that all CC122 reception is absent.

No audio was connected; internal key-to-sound behavior was not checked. The menu
may not be sufficient evidence of the functional receive behavior. No plugin
Local Control parameter was added. Further acceptance requires a direct audible
check with host echo blocked, or a documented reliable query/receive path.
The final displayed setting was On; the temporary sender was removed.


## Audible Local Control check — 25 October 2026

The current Ableton screenshot showed BSII track MIDI From = No Input and
track 2 MIDI To = No Output. Playback was requested stopped and Arp Off. The
user heard a hardware key through headphones, then manually selected Local Off
and confirmed that key produced no sound. A temporary scoped CoreMIDI sender
sent exactly channel-1 `B0 7A 7F` (CC122=127) to the single Bass Station
II destination. The user reported no sound returned. Manually selecting Local
On restored audible keyboard sound. Final Local On recovery is user-confirmed.

This adds a functional negative result for the standard Local On message,
beyond the earlier display-only observations. It does not prove absence of all
remote Local Control routes; actual synth channel was not freshly confirmed in
this check, although the displayed External Instrument route was channel 1.
No Tune, patch, save or tuning-table command was sent. The temporary probe was
removed automatically. Keep Local Control unimplemented pending reliable
bidirectional receive evidence or manufacturer clarification.
