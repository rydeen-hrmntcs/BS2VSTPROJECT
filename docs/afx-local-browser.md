# AFX local archive browser

27 October 2026. First plugin AFX increment: local inspection only. No hardware
reads/writes or parameter editing from this page. [Protocol/capture evidence](afx-protocol-research.md)
is separate from this UI.

## Workflow

Open the AFX tab and choose Open AFX archive. The supported modern archive is a
binary .syx containing exactly one data reply for each wire key0–24, with optional
name replies. The read-only MIDI Lab capture produces this file as
active-buffer-replies.syx. The page displays25 hardware keys C2–C4, labelled in
tooltips as human key1–25 and zero-based wire index0–24. Click a key for presence,
name coverage and preserved record sizes. Selecting sends no note or MIDI.

All25 data records are required, but names can be missing individually. Missing
names show Name unavailable; a returned blank name shows (empty name). The page
reports name coverage separately and never invents names. Presence follows the
first byte of Components' dedicated AFX packing, not the name or patch decoder.
This is a view of captured records, not a claim that this bank is saved or active
on the connected synth. Bank identity, protection and dirty state are unknown.

Save exact copy writes the input bytes unchanged, retaining record order and
unknown/reserved bytes. This is a local archive copy, not a verified hardware
restore file. Files containing partial data, duplicate keys, ordinary154-byte
patches, unsupported commands/clear markers, malformed/trailing bytes or over3300
bytes are rejected, keeping the previous archive. Components exports that encode
empty keys with clear markers are not supported by this first strict reader.

The source filename is displayed; its tooltip contains the file SHA256. The
archive and selected key survive editor recreation within a processor instance,
but are not embedded in a DAW Set. Reopen the file after reopening the Set. This
transient workspace adds no automation parameters, sends no MIDI and does not
change ordinary patch parameters, known preset identity or stored-write state.
At smaller editor heights, scroll the outer page to reach every control.

## Architecture

BSIIAfxArchive.h owns immutable raw input and indexed opaque key/name records.
It validates exact modern106-byte4E data and26-byte53 name messages with bounded
file reads. It constructs no MIDI commands. AfxPage (Source/BSIIAfxPage.cpp)
uses only the processor's transient local archive workspace. File choosers use
SafePointer callbacks; invalid opens and failed saves preserve the workspace.
Full8-bank hardware backup, live read integration, parameter decoding/editing,
copy/paste, protection, selection and hardware restoration are later increments.

## Validation

AU/VST3 Release builds passed. Focused AfxOffline and PatchBankOffline CTest
groups passed2/2. Native editor checks passed with zero failures, covering all25
key selections, absent-name display, exact local file copying, failed opens/saves,
editor recreation, no host-state embedding/replay, unchanged APVTS/current preset
identity and zero MIDI from AFX operations. Existing MicroTune/file-send/bank and
virtual stored-writer checks passed in the same run. Synthetic records only;
private captured user data is not committed as a test fixture.

Evidence: `validation-results/afx-local-browser-final/native-test.log`,
`afx-browser.png`, `afx-small.png` and `afx-small-detail.png`. Keyboard/default
and minimum-size scrolled details were visually inspected. The page requires
scrolling at smaller heights, including to reach selected details.

Live was process-confirmed closed immediately before installation. Prior AU/VST3
are under that report's `installed-before/`; installed strict local signatures,
both bundled MIT notices and Apple AU validation passed. See installation.txt and
auval.log. Public release assets are unchanged. Manual hosted acceptance is next.

A private exact copy of the data-only hardware capture is available for that
check at `~/Downloads/BSII-AFX-active-buffer-2026-10-27.syx`. It has25 records,
2650 bytes and no names. Captured with o0 reported, it is not labelled a verified
saved bank and is not a restoration claim. No physical MIDI was sent by the
browser or native tests.
