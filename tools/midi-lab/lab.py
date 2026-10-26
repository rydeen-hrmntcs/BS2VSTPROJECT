#!/usr/bin/env python3
"""Local BSII evidence recorder. No MIDI output unless backup/restore/sweep is chosen."""
from __future__ import annotations
import argparse
import importlib.util
import csv
from collections import Counter
from datetime import datetime
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import time
import uuid

ROOT = Path(__file__).resolve().parents[2]
CACHE = ROOT / 'build' / 'midi-lab'
RESULTS = ROOT / 'midi-lab-results'
SIGNED = {'velocityAmp', 'velocityMod', 'modWhLfo1Pitch', 'modWhOsc2Pitch',
          'aftertouchFilter', 'aftertouchLfo1', 'aftertouchLfo2Spd', 'modWhFilter', 'modWhLfo2Filter'}
# Documented discovery routes, not yet implemented/verified app controls.
DISCOVERY = {}


def registry():
    source = (ROOT / 'Source/BSIIMidi.h').read_text()
    numeric = re.findall(r'\{\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)",\s*Protocol::(\w+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\s*\}', source)
    choices = re.findall(r'\{\s*"([^"]+)",\s*"([^"]+)",\s*"([^"]+)",\s*Protocol::(\w+),\s*(-?\d+),\s*(-?\d+),\s*\{(.*?)\},\s*(\d+),\s*(true|false)\s*\}', source, re.S)
    result = []
    for ident, name, section, protocol, msb, lsb, lo, hi, default in numeric:
        result.append(dict(id=ident, name=name, section=section, protocol=protocol,
                           msb=int(msb), lsb=int(lsb), minimum=int(lo), maximum=int(hi)))
    for ident, name, section, protocol, msb, lsb, values, default, verified in choices:
        result.append(dict(id=ident, name=name, section=section, protocol=protocol,
                           msb=int(msb), lsb=int(lsb), choices={int(v): label for v, label in re.findall(r'\{\s*(-?\d+),\s*"([^"]+)"\s*\}', values)}))
    declared = len(re.findall(r'\{\s*"[^"]+",\s*"[^"]+",\s*"[^"]+",\s*Protocol::', source))
    if not result or len(result) != declared or len({r['id'] for r in result}) != len(result):
        raise ValueError('Control registry extraction failed; source format changed')
    mapped = set(re.findall(r'\{"([^"]+)"', (ROOT / 'Source/BSIIPatchFields.h').read_text()))
    for control in result:
        control['patchMapped'] = control['id'] in mapped
    return result


class Decoder:
    """MIDI1 stream parser, preserving running status across CoreMIDI packets."""
    def __init__(self, controls=None):
        self.controls = registry() if controls is None else controls
        self.running = None
        self.data = []
        self.sysex = None
        self.msb = {}
        self.nrpn = {}
        self.nrpn_data = {}
        self.warnings = Counter()

    def identify(self, channel, controller, value):
        self.msb[channel, controller] = value
        if controller in (99, 98, 101, 100): self.nrpn_data.pop(channel, None)
        if controller == 6: self.nrpn_data[channel] = value
        if controller in (101, 100):
            self.nrpn.pop(channel, None)  # RPN selection cancels NRPN selection.
        state = self.nrpn.setdefault(channel, [None, None])
        if controller == 99: state[0] = value
        if controller == 98: state[1] = value
        selected = tuple(state)
        matches = []
        for spec in self.controls:
            raw = None
            protocol = spec['protocol']
            if protocol == 'cc7' and controller == spec['msb']:
                raw = value - 64 if spec['id'] == 'bendAmount' else value
            elif protocol == 'pairedCC8' and controller == spec['lsb']:
                high = self.msb.get((channel, spec['msb']))
                if high is None:
                    self.warnings['paired LSB without preceding MSB'] += 1
                else: raw = (high << 1) | (value >> 6)
            elif protocol == 'nrpn7' and controller == 6 and selected == (spec['msb'], spec['lsb']):
                raw = value
            elif protocol == 'nrpn8' and controller == 38 and selected == (spec['msb'], spec['lsb']):
                high = self.nrpn_data.get(channel)
                if high is not None: raw = (high << 1) | (value >> 6)
            if raw is None: continue
            item = dict(id=spec['id'], value=raw, name=spec['name'])
            if 'choices' in spec:
                item['choice'] = spec['choices'].get(raw)
                item['inRange'] = raw in spec['choices']
            else:
                item['inRange'] = spec['minimum'] <= raw <= spec['maximum']
            if spec['id'] in SIGNED: item['display'] = raw - 64
            matches.append(item)
        return matches, selected if controller == 6 and None not in selected and selected != (127, 127) else None

    def feed(self, values, seconds=0):
        out = []
        for byte in values:
            if not isinstance(byte, int) or not 0 <= byte <= 255:
                raise ValueError('Invalid MIDI byte')
            if byte >= 0xF8:
                # Clock/active-sensing must not interrupt any in-flight message.
                continue
            if byte == 0xF0:
                if self.sysex is not None: self.warnings['abandoned SysEx'] += 1
                self.sysex = [byte]; self.running = None; self.data = []
                continue
            if self.sysex is not None:
                if byte == 0xF7:
                    self.sysex.append(byte)
                    out.append(dict(seconds=seconds, type='sysex', bytes=self.sysex))
                    self.sysex = None
                    continue
                if byte & 0x80:
                    self.warnings['interrupted SysEx'] += 1
                    self.sysex = None
                else:
                    self.sysex.append(byte)
                    if len(self.sysex) > 1024 * 1024:
                        self.warnings['oversized SysEx'] += 1; self.sysex = None
                    continue
            if byte & 0x80:
                if self.data: self.warnings['incomplete MIDI message'] += 1
                self.data = []
                self.running = byte if byte < 0xF0 else None
                continue
            if self.running is None:
                self.warnings['data without channel status'] += 1
                continue
            self.data.append(byte)
            kind = self.running & 0xF0
            need = 1 if kind in (0xC0, 0xD0) else 2
            if len(self.data) < need: continue
            channel = (self.running & 15) + 1
            data = self.data; self.data = []
            event = dict(seconds=seconds, channel=channel, bytes=[self.running] + data)
            if kind == 0xB0:
                event.update(type='cc', controller=data[0], value=data[1])
                matches, nrpn = self.identify(channel, *data)
                if matches: event['controls'] = matches
                if nrpn: event['nrpn'] = list(nrpn)
                if data[0] in DISCOVERY:
                    event['discovery'] = DISCOVERY[data[0]]
                if not matches and nrpn: event['unmapped'] = f'NRPN {nrpn[0]}:{nrpn[1]}'
                elif not matches and data[0] not in (99,98,6,38,100,101) and not any(s['protocol'] == 'pairedCC8' and s['msb'] == data[0] for s in self.controls):
                    event['unmapped'] = f'CC {data[0]}'
            elif kind == 0xC0:
                # Record the raw program. BSII display numbering must be measured.
                event.update(type='program', program=data[0])
            elif kind == 0x90:
                event['type'] = 'noteOn' if data[1] else 'noteOff'
            elif kind == 0x80: event['type'] = 'noteOff'
            elif kind == 0xE0: event.update(type='pitchWheel', value=(data[1] << 7 | data[0]) - 8192)
            else: event['type'] = 'other'
            out.append(event)
        return out

    def finish(self):
        if self.sysex is not None: self.warnings['unfinished SysEx'] += 1
        if self.data: self.warnings['incomplete MIDI message'] += 1


def run_dir(label):
    stamp = datetime.now().astimezone().strftime('%Y%m%d-%H%M%S')
    path = RESULTS / f'{stamp}-{label}-{uuid.uuid4().hex[:6]}'
    path.mkdir(parents=True, exist_ok=False)
    return path


def execute(args, log=None):
    if log:
        with open(log, 'w') as out:
            proc = subprocess.run(list(map(str, args)), cwd=ROOT, stdout=out, stderr=subprocess.STDOUT)
        if proc.returncode: raise RuntimeError(f'Command failed; see {log}')
    else:
        subprocess.run(list(map(str, args)), cwd=ROOT, check=True)


def offline_tools():
    spec = importlib.util.spec_from_file_location('bsii_offline', Path(__file__).with_name('offline.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def bridge():
    binary = CACHE / 'Bridge'
    source = Path(__file__).with_name('Bridge.swift')
    CACHE.mkdir(parents=True, exist_ok=True)
    if not binary.exists() or binary.stat().st_mtime < source.stat().st_mtime:
        execute(['swiftc', source, '-o', binary], CACHE / 'bridge-build.log')
    return binary


def inspector():
    binary = ROOT / 'build' / 'Release' / 'BSIIPatchInspect'
    # Only actual local include dependencies affect this binary, not unrelated UI headers.
    inputs = set()
    def dependencies(path):
        if path in inputs: return
        inputs.add(path)
        for name in re.findall(r'^#include \"([^\"]+)\"',path.read_text(),re.M):
            candidates = [path.parent/name, ROOT/'Source'/name]
            for candidate in candidates:
                if candidate.is_file(): dependencies(candidate); break
    dependencies(Path(__file__).with_name('PatchInspect.cpp'))
    if not binary.exists() or any(p.stat().st_mtime > binary.stat().st_mtime for p in inputs):
        raise RuntimeError('Run ./tools/validate.sh or lab.py prepare to build/update the actual patch decoder')
    return binary


def inspect(path):
    proc = subprocess.run([str(inspector()), str(Path(path).resolve())], capture_output=True, text=True)
    if proc.returncode: raise ValueError(f'Patch decode failed: {proc.stdout or proc.stderr}')
    result = json.loads(proc.stdout)
    if not result['valid']: raise ValueError(result.get('error'))
    return result['fields']


def analyse(path, destination, expected=None, channel=None):
    decoder = Decoder()
    controls, unknown, kinds, programs = {}, Counter(), Counter(), []
    events_file = destination / 'events.jsonl'
    with open(path) as stream, events_file.open('w') as output:
        for line in stream:
            line = line.strip()
            if not line: continue
            if line.startswith('{'):
                row = json.loads(line)
                values, seconds = row['bytes'], row.get('seconds', 0)
            else:
                try: values = list(bytes.fromhex(line)); seconds = 0
                except ValueError:
                    if line.startswith('Connected:'): continue
                    raise ValueError(f'Unrecognised capture line: {line[:80]}')
            for event in decoder.feed(values, seconds):
                output.write(json.dumps(event) + '\n')
                if channel is not None and event.get('channel') != channel:
                    continue
                kinds[event['type']] += 1
                if event['type'] == 'program': programs.append(dict(channel=event['channel'], raw=event['program']))
                if 'unmapped' in event: unknown[event['unmapped']] += 1
                if 'discovery' in event: unknown[event['discovery'] + ' (documented; app pending)'] += 1
                for control in event.get('controls', []):
                    state = controls.setdefault(control['id'], dict(count=0, minimum=control['value'], maximum=control['value'], last=control['value'], outOfRange=0))
                    state['count'] += 1
                    state['minimum'] = min(state['minimum'], control['value'])
                    state['maximum'] = max(state['maximum'], control['value'])
                    state['last'] = control['value']
                    state['outOfRange'] += not control['inRange']
        decoder.finish()
    result = dict(kinds=dict(kinds), controls=controls, unmapped=dict(unknown), programs=programs,
                  warnings=dict(decoder.warnings), expected=expected, channel=channel)
    result['verdict'] = 'OBSERVED' if expected and expected in controls else 'NOT OBSERVED' if expected else 'RECORDED' if kinds else 'NO MESSAGES'
    (destination / 'summary.json').write_text(json.dumps(result, indent=2) + '\n')
    lines = [f"# MIDI Lab: {result['verdict']}", '', 'This records MIDI evidence; it does not certify UI movement, sound or stored patch values.', '']
    for ident, state in sorted(controls.items()):
        lines.append(f"- {ident}: {state['count']} updates, {state['minimum']}…{state['maximum']}, last {state['last']}; out of range {state['outOfRange']}")
    if programs: lines += ['', 'Program Change raw numbers (display numbering unverified):', json.dumps(programs)]
    if unknown: lines += ['', 'Unmapped / discovery routes:', json.dumps(dict(unknown), sort_keys=True)]
    if decoder.warnings: lines += ['', 'Parser warnings:', json.dumps(dict(decoder.warnings))]
    (destination / 'summary.md').write_text('\n'.join(lines) + '\n')
    print(f"{result['verdict']}: {sum(kinds.values())} messages, {len(controls)} identified controls, {len(programs)} preset changes")
    print(f'Report: {destination / "summary.md"}')
    return result


def compare(before, after, destination):
    left, right = inspect(before), inspect(after)
    changes = {k: dict(before=left[k], after=right[k]) for k in left if left[k] != right[k]}
    a, b = Path(before).read_bytes(), Path(after).read_bytes()
    result = dict(byteIdentical=a == b, changedControls=changes,
                  changedBytes=[i for i, (x,y) in enumerate(zip(a,b)) if x != y])
    (destination / 'comparison.json').write_text(json.dumps(result, indent=2) + '\n')
    print(f"Byte-identical: {a == b}; {len(changes)} decoded controls changed. Report: {destination / 'comparison.json'}")
    return result


def check_sweep(control, directory):
    def normal(text):
        return text.lower().replace('-', '').replace('raw', '').replace('modwheel', 'modwh').replace('speed', 'spd')
    matches = [c for c in registry() if normal(c['id']) == normal(control)]
    if len(matches) != 1: raise ValueError('Cannot match sampler control to actual app registry')
    spec = matches[0]
    with (directory / f'{control}.csv').open() as source:
        rows = list(csv.DictReader(source))
    if not rows: raise ValueError('No sweep samples')
    paths = [directory / f"raw-{int(r['sent_raw']):03d}.syx" for r in rows]
    proc = subprocess.run([str(inspector())] + [str(p.resolve()) for p in paths], capture_output=True, text=True)
    if proc.returncode: raise ValueError('Actual decoder rejected a sweep patch: ' + proc.stderr[:200])
    decoded = [json.loads(line) for line in proc.stdout.splitlines()]
    if len(decoded) != len(rows): raise ValueError('Decoder sample count mismatch')
    failures = []
    for row, patch in zip(rows, decoded):
        raw = int(row['sent_raw'])
        if spec['id'] == 'oscSyncRaw': expected = 0 if raw == 0 else 127
        elif spec['id'] == 'subOctaveRaw': expected = 0 if raw < 63 else 127
        elif 'choices' in spec: expected = list(spec['choices']).index(raw)
        else: expected = max(spec['minimum'], min(spec['maximum'], raw))
        actual = patch['fields'][spec['id']]
        if actual != expected:
            failures.append(dict(sent=raw, expected=expected, actual=actual))
    report = dict(control=spec['id'], samples=len(rows), failures=failures,
                  scope='MIDI-setting / stored-patch decoder comparison, not audio or UI')
    (directory/'decoder-comparison.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f"Actual decoder: {len(rows)} samples, {len(failures)} mismatches")
    if failures: raise RuntimeError(f"Decoder mismatches: {directory/'decoder-comparison.json'}")
    return report


def sweep(control, directory):
    # Existing sampler has measured clamp rules and sends no preset-save command.
    proc = None
    problem = None
    try:
        with (directory / 'sweep.log').open('w') as out:
            proc = subprocess.Popen(['swift', str(ROOT / 'tools/MIDISampler.swift'), str(directory), control], cwd=ROOT, stdout=out, stderr=subprocess.STDOUT, start_new_session=True)
            if proc.wait() != 0: problem = 'Sampler failed; inspect sweep.log'
    except KeyboardInterrupt:
        problem = 'Interrupted'
        if proc and proc.poll() is None:
            os.killpg(proc.pid, signal.SIGTERM)
            try: proc.wait(timeout=5)
            except subprocess.TimeoutExpired: os.killpg(proc.pid, signal.SIGKILL); proc.wait()
    finally:
        before, after = directory / 'before.syx', directory / 'after.syx'
        if before.exists():
            if problem or not after.exists() or before.read_bytes() != after.read_bytes():
                # Restore only the backup this run created, never an older checkpoint.
                execute([bridge(), 'restore', before], directory / 'recovery.log')
                execute([bridge(), 'request', directory / 'recovered.syx'], directory / 'recovery-readback.log')
                after = directory / 'recovered.syx'
            compare(before, after, directory)
        else:
            problem = problem or 'No baseline backup captured; no verified test'
    if problem: raise RuntimeError(f'{problem}. Recovery evidence retained in {directory}')
    check_sweep(control, directory)
    print(f'Sweep complete. Full patch restored. Evidence: {directory}')


def write_reference(directory):
    controls = registry()
    (directory / 'controls.json').write_text(json.dumps(controls, indent=2) + '\n')
    lines = ['# Implemented BSII control reference', '', 'Generated from Source/BSIIMidi.h. Values are storage values, not necessarily display units.', '', '| ID | Group | Protocol | Route | Range / choices | Patch readback |', '|---|---|---|---|---|---|']
    for c in controls:
        route = f"{c['msb']}:{c['lsb']}" if c['lsb'] >= 0 else str(c['msb'])
        values = str(c.get('choices', f"{c.get('minimum')}…{c.get('maximum')}"))
        lines.append(f"| {c['id']} | {c['section']} | {c['protocol']} | {route} | {values} | {c['patchMapped']} |")
    (directory / 'controls.md').write_text('\n'.join(lines) + '\n')
    print(f'{len(controls)} controls. Reference: {directory / "controls.md"}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='command')
    record = commands.add_parser('record', help='passive: sends nothing')
    record.add_argument('--seconds', type=int, default=30)
    record.add_argument('--expect', choices=[c['id'] for c in registry()])
    replay = commands.add_parser('replay', help='offline: analyse saved JSONL or hex capture')
    replay.add_argument('capture', type=Path); replay.add_argument('--expect')
    record.add_argument('--channel', type=int, choices=range(1,17))
    replay.add_argument('--channel', type=int, choices=range(1,17))
    check = commands.add_parser('check-sweep', help='offline: compare existing sweep captures with actual decoder')
    check.add_argument('control'); check.add_argument('directory', type=Path)
    commands.add_parser('prepare', help='build cached transport and real decoder')
    commands.add_parser('reference', help='offline: generate control reference from current source')
    commands.add_parser('backup', help='request current edit buffer without changing it')
    restore = commands.add_parser('restore', help='explicit: replaces current edit buffer and verifies readback')
    restore.add_argument('backup', type=Path)
    inspect_cmd = commands.add_parser('inspect', help='offline: run the app actual SysEx decoder')
    inspect_cmd.add_argument('patch', type=Path)
    diff = commands.add_parser('compare', help='offline: compare complete bytes and decoded controls')
    diff.add_argument('before', type=Path); diff.add_argument('after', type=Path)
    active = commands.add_parser('sweep', help='explicit: sends test values, backs up/restores current sound')
    active.add_argument('control', help='existing sampler name, e.g. bend-amount')
    status_cmd = commands.add_parser('status', help='offline: concise current-source coverage and validation report')
    status_cmd.add_argument('--json', action='store_true')
    analysis = commands.add_parser('analyse-sweep', help='offline: analyse labelled edit-buffer captures; never sends MIDI')
    analysis.add_argument('directory',type=Path)
    stream = commands.add_parser('inspect-sysex', help='offline: inspect binary SysEx framing; never sends MIDI')
    stream.add_argument('capture', type=Path)
    stream.add_argument('--json', action='store_true')
    bank = commands.add_parser('review-bank', help='offline: review/compare the observed BSII bank export profile; no MIDI')
    bank.add_argument('capture', type=Path)
    bank.add_argument('--against', type=Path)
    bank.add_argument('--json', action='store_true')
    extract = commands.add_parser('extract-bank-entry', help='offline: create one temporary-patch .syx from a bank; never sends MIDI')
    extract.add_argument('capture', type=Path)
    extract.add_argument('export_index', type=int)
    extract.add_argument('output', type=Path)
    extract.add_argument('--json', action='store_true')
    afx_inspect = commands.add_parser('inspect-afx', help='offline: inspect active AFX replies; never sends MIDI')
    afx_inspect.add_argument('capture', type=Path)
    afx_inspect.add_argument('--json', action='store_true')
    afx_read = commands.add_parser('read-afx-active', help='read active AFX buffer only; no bank selection or writes')
    afx_read.add_argument('--reported-bank', type=int, choices=range(9))
    afx_read.add_argument('--data-only', action='store_true', help='explicitly skip optional name requests; names remain unavailable')
    args = parser.parse_args()
    if args.command is None: args = parser.parse_args(['record'])
    if args.command in ('inspect-afx', 'read-afx-active'):
        import afx
        if args.command == 'inspect-afx':
            report = afx.inspect_file(args.capture)
            print(json.dumps(report, indent=2) if args.json else f"AFX data: {report['dataKeys']}/25; names: {report['nameKeys']}/25; complete: {report['complete']}. Read-only inspection; no write eligibility.")
            if not report['complete']: raise SystemExit(1)
            return
        directory = run_dir('afx-active')
        source = Path(__file__).with_name('AfxRead.swift')
        binary = CACHE / 'AfxRead'
        CACHE.mkdir(parents=True, exist_ok=True)
        if not binary.exists() or binary.stat().st_mtime < source.stat().st_mtime:
            execute(['swiftc', source, '-o', binary], CACHE / 'afx-read-build.log')
        execute([binary, directory] + (['--data-only'] if args.data_only else []), directory / 'capture.log')
        path = directory / 'active-buffer-replies.syx'
        if path.stat().st_size:
            report = afx.inspect_file(path)
        else:
            report = dict(complete=False, dataKeys=0, nameKeys=0, error='No matching AFX replies')
        report.update(reportedBank=args.reported_bank, bankEvidence='user-reported, not queried', firmware=None)
        (directory / 'inspection.json').write_text(json.dumps(report, indent=2)+'\n')
        print(f"AFX data: {report['dataKeys']}/25; names: {report['nameKeys']}/25; complete: {report['complete']}. Capture: {directory}")
        print('No bank selection, protection change, overlay import, clear or save sent.')
        transport = json.loads((directory / 'transport.json').read_text())
        if transport['stoppedEarly'] or report.get('issues') or not report.get('dataComplete', False): raise SystemExit(1)
        return
    if args.command in ('inspect-sysex', 'review-bank', 'extract-bank-entry'):
        spec = importlib.util.spec_from_file_location('bsii_sysex_stream', Path(__file__).with_name('sysex_stream.py'))
        stream_tools = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(stream_tools)
        if args.command == 'extract-bank-entry':
            report = stream_tools.extract_bank_entry(args.capture, args.export_index, args.output)
            print(json.dumps(report, indent=2) if args.json else f"Wrote {report['bytes']}-byte temporary patch: {report['output']}\nSHA256: {report['sha256']}\nNo MIDI sent; stored presets unchanged.")
            return
        if args.command == 'review-bank':
            report = stream_tools.review_bank(args.capture, args.against)
            print(json.dumps(report, indent=2) if args.json else stream_tools.bank_summary(report))
            return
        report = stream_tools.inspect_file(args.capture)
        print(json.dumps(report, indent=2) if args.json else stream_tools.summary(report))
        if not report['framingClean']: raise SystemExit(1)
        return
    if args.command == 'status':
        offline = offline_tools()
        report = offline.status(ROOT,registry())
        print(json.dumps(report,indent=2) if args.json else offline.status_text(report)); return
    if args.command == 'analyse-sweep':
        offline = offline_tools()
        report = offline.analyse_sweep(args.directory,inspector())
        directory = run_dir('analyse-sweep')
        (directory/'report.json').write_text(json.dumps(report,indent=2)+'\n')
        (directory/'report.md').write_text(offline.sweep_text(report))
        print(offline.sweep_text(report)); print(f'Report: {directory / "report.md"}'); return
    if args.command == 'prepare':
        CACHE.mkdir(parents=True, exist_ok=True)
        bridge()
        configure = ['cmake','-S',ROOT,'-B',ROOT/'build','-DBSII_BUILD_TESTS=ON']
        if not (ROOT/'build/CMakeCache.txt').exists(): configure += ['-G','Xcode']
        execute(configure, CACHE/'configure.log')
        execute(['cmake','--build',ROOT/'build','--config','Release','--target','BSIIPatchInspect','-j','4','--','CODE_SIGNING_ALLOWED=NO'], CACHE/'inspector-build.log')
        print('MIDI Lab ready'); return
    if args.command == 'check-sweep':
        check_sweep(args.control, args.directory.resolve()); return
    if args.command == 'inspect':
        print(json.dumps(inspect(args.patch), indent=2)); return
    directory = run_dir(args.command)
    (directory / 'run.json').write_text(json.dumps(dict(command=vars(args), started=datetime.now().astimezone().isoformat()), default=str, indent=2) + '\n')
    if args.command == 'reference': write_reference(directory)
    elif args.command == 'replay': analyse(args.capture, directory, args.expect, args.channel)
    elif args.command == 'record':
        if not 1 <= args.seconds <= 3600: raise ValueError('Seconds must be 1…3600')
        binary = bridge()
        print(f'Passive capture: {args.seconds}s; move hardware controls now. No MIDI output.', flush=True)
        raw = directory / 'packets.jsonl'
        with raw.open('w') as output, (directory/'transport.log').open('w') as errors:
            proc = subprocess.Popen([str(binary), 'listen', str(args.seconds)], stdout=output, stderr=errors, start_new_session=True)
            try: code = proc.wait()
            except KeyboardInterrupt:
                proc.send_signal(signal.SIGINT); code = proc.wait(timeout=5)
        if code: raise RuntimeError(f'MIDI input failed; see {directory / "transport.log"}')
        analyse(raw, directory, args.expect, args.channel)
    elif args.command == 'backup':
        execute([bridge(), 'request', directory/'baseline.syx'], directory/'transport.log')
        print(f'Backup: {directory / "baseline.syx"}')
    elif args.command == 'restore':
        execute([bridge(), 'restore', args.backup.resolve()], directory/'transport.log')
        print(f'Restore verified. Report: {directory}')
    elif args.command == 'compare': compare(args.before, args.after, directory)
    elif args.command == 'sweep':
        inspector()  # Fail before touching hardware if real decoder is unavailable.
        bridge()     # Compile recovery transport before any mutation.
        sweep(args.control, directory)


if __name__ == '__main__':
    def interrupted(signum, frame):
        raise KeyboardInterrupt
    signal.signal(signal.SIGTERM, interrupted)
    try: main()
    except (ValueError, RuntimeError, OSError, subprocess.SubprocessError) as error:
        print(f'ERROR: {error}', file=sys.stderr); sys.exit(1)
