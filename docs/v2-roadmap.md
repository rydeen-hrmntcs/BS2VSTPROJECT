# BSII Control v2 milestones

Agreed direction, 27 October 2026: finish the remaining v1 editor/controller
work and final GUI before starting extended software modulation. This is a
planning document, not authorization to implement v2 now or a delivery schedule.
The current development version is still 0.8.x; this document does not rename a
release or replace public binaries.

Based on the user's `bs2_extended_modulation_ideas-4 (1).md` proposal. Keep the
musical goal: richer modulation with an outgoing stream matched to measured
hardware behavior. Numerical rates/error percentages in the proposal are
illustrations, not established limits.

## Gate 0 — finish and freeze v1

Finish or explicitly resolve the scope of the remaining supported hardware
work, including AFX and the deferred audio/tuner-based MicroTune transfer checks.
Global Tune, Local Control and external sequence recording remain protocol gaps;
closing v1 requires an explicit decision about their release status, not guessed
commands or an assumption they must all be solved. Stored-preset writing has
passed the slot1 workflow; other destinations, persistence and recovery coverage
remain separate. Full-bank writing is not implied by the single-slot feature.

Complete the final GUI against the user's reference, retain tested editor,
automation, recall, bank and tuning behavior, and run the relevant host/session
release checks. Document supported platforms, signing status and remaining
limitations. Preserve a stable v1 source/release baseline before v2 work begins.
Detailed v1 checklist: [remaining work](remaining-work.md).

## Milestone 1 — observe the existing control path

Add lightweight, optional counters and repeatable capture/export of requested
parameter updates, coalesced values, outgoing MIDI messages/bytes, bursts and
scheduling delay. Distinguish one parameter update from its wire cost: paired
CC and NRPN need multiple messages. Separate parameter automation from notes,
clock and administrative patch/bank transfers.

Current baseline: parameter changes are rounded to integer wire-domain values,
latest values are coalesced per parameter, and a nominal 100Hz message-thread
timer dispatches them. This is neither a global 100-message budget nor sample-
accurate delivery. The current JUCE VST3 wrapper takes the final ordinary
parameter point from each host queue; later layers cannot recover discarded
points. Confirm these facts again against the implementation when work starts.

Exit: reproducible virtual-port traffic reports, negligible measured overhead,
and unchanged MIDI/automation behavior with monitoring disabled. A UI meter
must describe measured outgoing traffic, not the synth's CPU load.

## Milestone 2 — measure hardware capabilities

Run bounded single/multiple-parameter experiments, then combine external changes
with internal LFO modulation. Compare message families and USB/DIN separately
when those connections are available. Use audio capture/tuner measurements for
pitch, stepping and audible response; final readback alone cannot prove transient
accuracy or timing. Preserve the starting sound and avoid unknown global commands
or stored-memory operations in stress tests.

Record device/firmware, transport, controls, measured rate, byte cost, delay,
missed/final values and audio observations. Internal AFX/LFO speed is not evidence
of external MIDI capacity. Begin with one control family; expand only as results
justify it, rather than repeatedly sweeping every parameter.

Exit: versioned profiles with tested conditions, evidence and unknowns. No
universal safe frequency, perceptual threshold or fixed percentage claim without
measurement. Choose explicit error/delay budgets from this evidence.

## Milestone 3 — introduce a bounded scheduler

Add causal, per-device scheduling with hardware-value deduplication, latest-value
coalescing, bounded control delay, measured rate/byte budgets, final-value flush
and starvation prevention. Treat paired CC/NRPN groups as indivisible control
operations. Define shared output ownership/budget across cooperating plugin
instances, or reject conflicting ownership; separate per-instance budgets alone
cannot protect one shared physical port. External apps remain outside that
ownership guarantee.

Invalidate cached assumptions on preset changes, hardware edits, output changes
and reconnects. Keep notes/clock ownership explicit. Never send patch saves,
stored writes, bank traffic or unrelated SysEx through a modulation route.
Stepped controls require deliberate semantics; do not smooth mode selectors.

Exit: deterministic virtual-port tests plus repeatable hardware/audio checks
show bounded delays, correct endpoints and no message-group interleaving,
starvation, host automation regressions or automatic stored writes. Retain
v1-compatible defaults. Any quality modes describe their actual reduction;
avoid an Exact label when the full incoming curve cannot be preserved.

## Milestone 4 — add macros and route ownership

Start musical features with a small fixed set of macros and clear destination,
amount, polarity and range controls. Define base parameter value versus effective
modulated value, combination/clamping rules and hardware-knob behavior while a
route owns a parameter. Specify hardware feedback/echo handling so outgoing
modulation cannot feed back into the base value and accumulate drift.

Keep route documents separate from sound patches and administrative hardware
state. Make snapshot/scene capture explicit about base versus effective values.
Preserve the 95 existing host parameter IDs/indices; append any agreed new host
parameters through a stable layout. Runtime modulation must not create streams
of host edit gestures or undo entries.

Exit: predictable manual automation plus macros, safe bypass/unrouting and saved
Set reopen behavior, with legacy v1 Sets unchanged and new documents versioned.

## Milestone 5 — add software LFOs and step/random sources

Add a minimal useful set of plugin-side sources: free/tempo-synced LFOs, a step
modulator and sample-and-hold/random. Route through the tested scheduler. Prefer
modulating the BSII's internal modulation rates/depths when that keeps fast
movement inside the synth. Define start/stop, phase, retrigger and repeatability
rather than inferring them from existing hardware clock-following tests.

Exit: musically useful routes remain responsive within measured budgets; bypass,
transport changes and Set restore behave predictably. No added look-ahead buffer
by default. Zero reported audio latency must not be described as zero MIDI/control
delay, and a smooth internal curve does not imply interpolation by the hardware.

## Milestone 6 — add note-triggered envelopes and deeper routing

Only after defining a reliable note-input path, add MIDI-triggered envelopes,
velocity/aftertouch remapping and limited modulator-to-modulator routes. The
current External Instrument/device-chain arrangement does not establish that the
plugin receives clip notes. Define channel/duplicate/echo behavior, held notes,
sustain, note-off, disconnect and panic handling before enabling these features.
Bound or reject routing cycles; do not introduce unbounded feedback graphs.

Exit: note-triggered behavior works in the documented DAW routing without doubled
notes, stuck notes, uncontrolled feedback or dependence on the editor being open.

## Later candidates, after the above gates

Patch morphing, scene interpolation, probability/generative evolution, modulation
presets/recording and optional look-ahead need separate scope decisions. Morphing
must respect discrete controls and exclude administrative/persistent operations.
A software sequencer would be a separate feature, not proof of access to the
BSII's four built-in sequence memories. Look-ahead remains optional and requires
an explicit host-latency policy; it is not part of the initial causal scheduler.

Before each milestone, agree its smallest useful increment and acceptance checks.
Do not turn this roadmap into a wholesale refactor, promise hardware bandwidth
from offline tests, or add speculative features to v1 while finishing its GUI.
