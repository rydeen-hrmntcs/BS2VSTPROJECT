"""Synthetic AFX capture inspection tests. No MIDI ports or hardware."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools/midi-lab'))
import afx


def pack(decoded):
    value = int.from_bytes(decoded, 'big')
    return bytes((value >> (7*i)) & 127 for i in range(95, -1, -1))


def message(command, key, body=b''):
    return afx.PREFIX+bytes([command, key])+body+b'\xf7'


def capture():
    data = []
    for key in range(25):
        decoded = bytes([0 if key == 3 else 1])+bytes((i*7+key) % 256 for i in range(83))
        data.append(message(0x4e, key, pack(decoded)))
        data.append(message(0x53, key, f'Key {key}'.encode().ljust(16, b' ')))
    return b''.join(data)


class AfxTests(unittest.TestCase):
    def test_independent_bitstream_vectors(self):
        self.assertEqual(afx.unpack_data(bytes([64])+bytes(95)), bytes([128])+bytes(83))
        for shift in (0, 1, 37, 128, 255):
            decoded = bytes((i+shift) % 256 for i in range(84))
            self.assertEqual(afx.unpack_data(pack(decoded)), decoded)
        with self.assertRaises(ValueError): afx.unpack_data(bytes(95))
        with self.assertRaises(ValueError): afx.unpack_data(bytes([128])+bytes(95))

    def test_complete_opaque_archive(self):
        report = afx.inspect_bytes(capture())
        self.assertTrue(report['complete'])
        self.assertEqual((report['bytes'], report['dataKeys'], report['nameKeys'], report['overlaysPresent']), (3300, 25, 25, 1))
        self.assertFalse(report['savedBankVerified'])
        self.assertFalse(report['transmitEligible'])
        self.assertFalse(report['restorationVerified'])
        self.assertEqual(report['records'][7]['displayName'], 'Key 3')
        self.assertEqual(report['records'][6]['hasOverlay'], True)

    def test_names_do_not_infer_presence(self):
        report = afx.inspect_bytes(message(0x4e,0,pack(bytes([1])+bytes(83)))+message(0x53,0,b'Named but blank '.ljust(16,b' ')))
        self.assertFalse(report['records'][0]['hasOverlay'])
        self.assertFalse(report['complete'])

    def test_unsupported_shapes_and_corruption(self):
        for raw in (message(0x4c,0), message(0x52,0), message(0x4f,0), message(0x4e,25,bytes(96)),
                    message(0x4e,0,bytes(95)), message(0x53,0,bytes(15)),
                    message(0x4e,0,bytes([128])+bytes(95)),
                    bytes.fromhex('F0 00 20 29 00 34 00 4E 00')+bytes(96)+b'\xf7'):
            self.assertFalse(afx.inspect_bytes(raw)['complete'])
        self.assertEqual(afx.inspect_bytes(message(0x4c,0))['records'][0]['kind'], 'unverified-clear-marker')
        self.assertFalse(afx.inspect_bytes(capture()[:-1])['complete'])
        self.assertFalse(afx.inspect_bytes(capture()+message(0x4e,0,bytes(96)))['complete'])
        self.assertFalse(afx.inspect_bytes(capture()+b'\x90\x3c\x40')['complete'])

    def test_data_only_is_not_a_complete_named_archive(self):
        data = b''.join(message(0x4e,k,bytes(96)) for k in range(25))
        report = afx.inspect_bytes(data)
        self.assertTrue(report['dataComplete'])
        self.assertFalse(report['namesComplete'])
        self.assertFalse(report['complete'])
        self.assertEqual(report['missingNameKeys'], list(range(25)))

    def test_read_only_cli_and_source_preservation(self):
        with tempfile.TemporaryDirectory() as folder:
            file = Path(folder)/'active.syx'; raw = capture(); file.write_bytes(raw)
            before = set(Path(folder).iterdir())
            result = subprocess.run([sys.executable,'-B',str(ROOT/'tools/midi-lab/lab.py'),'inspect-afx',str(file),'--json'],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            self.assertTrue(json.loads(result.stdout)['complete'])
            self.assertEqual(file.read_bytes(),raw)
            self.assertEqual(before,set(Path(folder).iterdir()))


if __name__ == '__main__': unittest.main()
