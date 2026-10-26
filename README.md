# BSII Control

A MIDI editor for the **Novation Bass Station II**, built with JUCE and C++20.
Edit the connected synth from your Mac, follow hardware knob movements in the app,
fetch its current patch, and save editor snapshots or raw SysEx files.
The sound comes from the Bass Station II; this project does not emulate its audio engine.

## Download the test build

[Download the VST3 and AU prerelease](https://github.com/rydeen-hrmntcs/BS2VSTPROJECT/releases/tag/v0.8.2-preview.20261018).

**Apple Silicon Macs running macOS 27.0 or newer only.** Bass Station II hardware
is required. These are early test builds, locally signed and not notarized;
macOS may block downloaded copies on another Mac. The release includes installation
instructions, known limitations and SHA-256 checksums.

**Current status:** working standalone editor under active development. The project
also builds AU and VST3. An early VST3 check in Ableton Live 12.4.6 on Apple
Silicon passed loading, hardware patch reception, and host parameter edits with
the editor closed; AU passed Apple validation. User testing also confirmed Cutoff
and Resonance automation playback, project sound recall on Set reopen, and stored-preset
USB reconnect with the editor closed and matched channels. Captured-sound USB reconnect
and wider compatibility still need testing.
Hardware checks have used a Bass Station II reported as running firmware **4.14**.

The `v0.8.2` tag preserves the original project baseline. `main` includes the later
fixes and features described below. The download above is a prerelease snapshot,
not a stable release. Some app/build
version labels still show earlier numbers.

## Available features

- **95 automatable controls** across Osc 1, Osc 2, Mixer, Filter, LFO/Glide,
  Amp Env, Mod Env, FX/Performance, Arp/Seq and Global pages.
- **Local MicroTune editor:** edit all 128 notes, undo/redo changes, edit selections together,
  preview/import Scala `.scl`/`.kbm` and `.tun` files (including v2 formulas/mapping), load/save local table files and retain
  a working copy in the DAW Set. No hardware table transmission or save yet.
- **Hardware MIDI channel setup:** the Global page can request a synth channel
  change and follow its confirmation. DAW note routing remains separate.
- **Live MIDI in both directions:** app edits send CC/NRPN messages, and supported
  hardware changes update the editor while Live RX is running.
- **Preset selection, 0–127:** select a number, step with Previous/Next, or reload
  with Load Preset. Hardware preset changes are followed automatically, with a
  short settling delay before the app fetches the current patch.
- **Get Patch from BSII:** request the current temporary patch and apply its
  supported fields to the editor, without manually performing a hardware dump.
- **Snapshots and SysEx:** save/load editor state, capture and save a raw patch,
  import a patch into the editor, or send a patch to the synth.
- **Clearer controls:** Sub Octave −1/−2 and Osc Sync Off/On switches, signed bend
  and modulation values, resizable pages and scrolling for smaller windows.

The Arp/Seq page adds On, Latch, Octaves, Direction/Play/Record, Rhythm and Seq Retrig alongside Swing. Get Patch now reads all six controls from the captured SysEx fields. Send All includes them, except that it will not arm Record mode; select Record explicitly when needed. Their emitted encodings were captured from firmware 4.14, and selected live changes have been checked both ways in Ableton. Basic Play/Record mode changes have also been checked; sequence-content editing remains unverified. An initial reception hiccup did not reproduce after restarting Ableton, and its cause is unknown. See [Arp/Seq research and evidence](docs/arp-seq-research.md).

The LFO page includes independent Speed/Sync selectors and all 35 clock divisions for both LFOs. Live MIDI routes and SysEx storage were verified against hardware captures for all values. These controls support automation, saved sessions and Get Patch; basic hosted mode/division, paused-RX Get Patch and one LFO1 division automation check passed. Audible clock accuracy remains untested. See [LFO sync evidence](docs/lfo-sync-research.md).

This is a partial instrument editor. Sequence-step editing, verified sequence-slot contents, Global Tune, Local Control, tuning-table editing and bank management remain unfinished. **AFX mode is planned last.** See the [current coverage and remaining-work checklist](docs/remaining-work.md).

## Build on macOS

Requirements:

- Xcode with command-line tools.
- CMake **3.22 or newer**.
- Internet access for the first configure, which downloads **JUCE 9.0.4**.
- Python **3.10 or newer** for the regression suite and MIDI Lab only.

From the repository folder:

```sh
cmake -S . -B build -G Xcode
cmake --build build --config Release --target BSIIControl_Standalone -- CODE_SIGNING_ALLOWED=NO
```

The standalone app is created at:

```text
build/BSIIControl_artefacts/Release/Standalone/BSII Control.app
```

Open it in Finder after the build. This is a local development build; the build
command does not provide a signed, notarized installer. A fresh AU/VST3 host test
is still required before treating the plugin formats as ready for use.

Build outputs and downloaded JUCE sources are excluded from Git. To work through
GitHub Desktop, clone this repository, or use **File → Add Local Repository** for
an existing checkout. Building does not automatically replace another app copy
you may already have open.

## Connect and start editing

1. Connect the Bass Station II to the Mac over USB and turn it on. Listen through
   the synth's audio output or your usual audio interface.
2. Open the standalone app. In **Settings… → Active MIDI inputs**, enable the
   Bass Station II input.
3. Select **Bass Station II** as the app's MIDI output. Set **Channel** to match
   the synth's MIDI channel; the app starts on channel 1.
4. Click **Get Patch from BSII** to bring the current sound into the editor before
   editing. At startup, the displayed controls may reflect an earlier app state.
5. Keep **Live RX is running** enabled to follow supported hardware changes.
   App control edits are sent to the selected MIDI output immediately.

The preset selector initially shows **Unknown / edit buffer**. A current-patch
reply does not identify a stored preset number. Selecting a preset in the app or
receiving a hardware preset change establishes the number.

**Selecting a preset replaces the synth's temporary sound.** Save any edits you
want to keep first. The preset selector does not overwrite a stored preset slot;
use the hardware's save procedure when you intend to save into its preset memory.

### Early Ableton VST3 setup

Enable VST3 system folders in Live's Plug-Ins settings, then load
**GutzDev → BSII Control** on a MIDI track. Keep the plugin activated in Live.
For MIDI notes, use External Instrument before BSII Control and configure the
track input/monitoring as needed. Hardware controls now use the plugin's direct
input, independently of instrument placement; host input is a fallback when no
unique direct input is available.
Select the hardware output/channel in the plugin, then save the DAW Set. Host
state now includes that connection, channel and Live RX setting so the processor
can reconnect without opening the editor. Older Sets need one save with the new
build after choosing the output. With recall set to Off, reconnection does not automatically send a patch;
the user confirmed this reconnects successfully after an Ableton restart. Patch
replies, live CC/NRPN controls and hardware preset changes use a direct MIDI input.
Input/output names must match uniquely. When direct input is active, duplicate
DAW-routed controls are not decoded again. Live RX pause and selected-channel
filtering still apply; notes, clock and other performance traffic keep their DAW route. These are early tests, not complete DAW
support; see [plugin smoke-test results](docs/plugin-smoke-test.md).

### Optional project sound recall

Use **On reconnect** to choose **Off**, **Stored preset**, or **Captured sound**.
Stored preset remembers the selected preset number. For an exact backup of the
hardware sound, click **Capture Project Sound**, wait for success, choose Captured
sound, then save your DAW Set. That capture stays fixed until you capture again.
Recall runs once on session load/USB reconnection, including with the editor closed;
it does not run on every Play or overwrite stored hardware presets. Wait for recall
to finish before playback. Existing Sets default to Off. Stored-preset and captured-sound recall have passed manual Ableton Set-reopen checks. Stored-preset USB reconnect also passed with the editor closed and matching hardware/plugin channels; captured-sound USB reconnect remains pending; see [recall behavior and checks](docs/project-sound-recall.md).

## Which save, load or transfer control should I use?

| Control | What it does |
| --- | --- |
| **Get Patch from BSII** | Reads the synth's current edit buffer and updates the editor's supported controls. |
| **Save Snapshot…** | Saves the editor's parameter state as a `.bsiistate` file. Fetch the hardware patch first if you want its current values. |
| **Load Snapshot…** | Loads a `.bsiistate` into the editor. It does not send that state to the synth automatically. |
| **Send All → BSII** | Sends exposed editor controls through live MIDI, except that it never arms Record mode. This is not a full raw-patch transfer. |
| **Import .syx → Editor** | Decodes a raw patch file into the editor without sending it to the synth. |
| **Send .syx → BSII** | Sends one validated 154-byte temporary edit-buffer patch. Rejects banks, stored-patch commands, other SysEx and malformed/trailing data without sending. See [file-send limits](docs/edit-buffer-file-send.md). |
| **Banks → Open Bank / Load Selected** | Browse a complete 128-patch bank locally; explicitly load one entry into the temporary buffer. Those actions do not write stored presets. See [bank browser](docs/bank-browser.md). |
| **Banks → Prepare / Write stored preset…** | Saves a fresh current-sound/full-bank backup, previews one destination, asks for explicit confirmation and verifies all stored slots afterward. See [stored-preset writer](docs/stored-preset-writer.md). |
| **AFX → Open archive / Save exact copy** | Inspect a local 25-key AFX data archive and preserve its bytes. Names may be unavailable. No hardware edits or sends. See [AFX browser](docs/afx-local-browser.md). |
| **Arm Patch Capture** | Arms receipt of the next manual dump. Perform the dump on the hardware, then save the received message. |
| **Save Last .syx…** | Saves the last received SysEx message unchanged. Use Get Patch or a manual capture first to obtain a fresh one. |

A snapshot represents editor state; a raw `.syx` preserves the received patch
message, including data for fields the editor may not expose. Importing a file,
loading a snapshot or sending SysEx clears the displayed preset identity.

Clicking the Live RX control toggles between **Live RX is running** and
**Live RX is paused**. Pausing stops live CC/NRPN and hardware preset following;
SysEx reception and Get Patch remain available. It does not disable app-to-synth
editing. A tick on the current control means reception is paused; read the status
text to see the state.

## What has been tested?

- The registry has **95 automatable controls**: **94 patch-mapped controls** and
  the separate global Input Gain control. Retained fixtures and regression checks
  cover the mapped fields; this does not establish every UI or audible behavior.
  The original 76-control fixture set contains **96 files and 9,251 patch messages**;
  newer feature fixtures live in separate directories. See the [current audit](docs/remaining-work.md).
- Manual checks have exercised supported controls in both directions, with fixes
  and retests recorded in the [manual test review](docs/manual-test-review.md).
- Snapshot round trips and combined Send All recalls have been checked against
  saved hardware patches: [state validation](docs/state-integration-validation.md).
- Preset changes have been checked from the app and hardware. After app preset
  selection, all 76 editor values matched the saved hardware patch:
  [preset validation](docs/preset-validation.md).
- The current offline suite has **nine registered test groups**, including
  15 Python checks for the MIDI Lab tools.

These checks establish specific MIDI, patch storage and editor behaviors. They do
not measure acoustic accuracy or establish complete instrument/DAW coverage.
See the [coverage audit](docs/validation-coverage.md) for the detailed control
inventory and [current handover](docs/project-status.md) for newer results and limits.

Known limits include approximate Coarse display rounding, broader MIDI-channel
coverage, device reconnect behavior and AU/VST3 host testing. Preset following with
paused RX, mismatched channels and restored channel matching has been checked.
Trigger edits in either envelope page currently apply to both envelopes, matching
the chosen front-panel workflow; independent mixed-trigger sound-engine behavior
has not been established. See [trigger findings](docs/trigger-behavior-research.md).
The hardware has occasionally shown `---` after SysEx transfer while remaining
playable; its meaning is unresolved.

## Local validation and MIDI tools

Run the build and offline checks with:

```sh
./tools/validate.sh
```

No synth connection is needed and no MIDI is sent. The routine builds the
standalone app and regression tools, prints a short result and saves full logs
under the ignored `validation-results/` folder. App signing and live hardware
checks are separate.

The [MicroTune tab](docs/microtune-editor.md) edits and saves local tables without
a synth. The [offline file tool](docs/microtune-offline.md) also exports SysEx.
Hardware table transfer/save integration remains pending.

For passive hardware MIDI recording and short reports:

```sh
python3 tools/midi-lab/lab.py
```

The default mode listens for 30 seconds without creating a MIDI output port.
Recordings stay in the ignored `midi-lab-results/` folder. The tool also offers
explicit patch backup/restore, controlled sampling, offline replay and comparison
using the app's actual patch decoder. See [MIDI Lab instructions](docs/midi-lab.md)
before using modes that change the current sound.

## Project reference

- [Start here / development handoff](docs/START-HERE.md)
- [Current coverage and remaining work](docs/remaining-work.md)
- [Chronological project handover](docs/project-status.md)
- [Changes since the baseline](docs/next-version.md)
- [Hardware validation evidence](docs/hardware-validation.md)
- [Manual test checklist](docs/manual-test-checklist.md)
- [Official manual reference index](docs/local-reference.md)
- [Implemented MIDI control definitions](Source/BSIIMidi.h)

For compact validation status and offline capture inference, see the
[research toolkit](docs/research-toolkit.md).

Scala import workflow and supported limits: [local import guide](docs/scala-import.md).

TUN import, including u-he export layout: [supported formats](docs/tun-import.md).

TUN v2 formula and keyboard mapping support: [workflow and limits](docs/tun-v2-import.md).
