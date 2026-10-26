"""Read-only AFX active-buffer archive inspection. Never constructs write packets."""
from pathlib import Path
from sysex_stream import inspect_bytes as inspect_stream, digest, MAX_BYTES

PREFIX = bytes.fromhex('F0 00 20 29 00 33 00')


def unpack_data(payload):
    if len(payload) != 96 or any(b >= 128 for b in payload):
        raise ValueError('AFX data needs exactly 96 seven-bit packed bytes.')
    # Novation Components OX/RX contiguous bit stream, not MIDI mask-byte packing.
    return bytes(((payload[start+i] << (i+1)) & 255) | (payload[start+i+1] >> (6-i))
                 for start in range(0, 96, 8) for i in range(7))


def inspect_bytes(data):
    stream = inspect_stream(data)
    records, issues = [], list(stream['issues'])
    data_keys, name_keys = set(), set()
    for number, segment in enumerate(stream['segments']):
        if segment['kind'] != 'sysex' or not segment['complete']:
            continue
        raw = bytes.fromhex(segment['normalizedHex'])
        row = dict(messageIndex=number, bytes=len(raw), sha256=digest(raw), kind='unexpected', transmitEligible=False)
        if (len(raw) >= 10 and raw.startswith(PREFIX) and raw[-1] == 0xf7
                and all(b < 128 for b in raw[1:-1]) and raw[8] < 25):
            key, command = raw[8], raw[7]
            row.update(key=key, commandHex=f'{command:02X}')
            if command == 0x4e and len(raw) == 106:
                decoded = unpack_data(raw[9:-1])
                row.update(kind='data', hasOverlay=decoded[0] == 0, decodedBytes=84,
                           decodedSha256=digest(decoded))
                seen = data_keys
            elif command == 0x53 and len(raw) == 26:
                row.update(kind='name', displayName=''.join(chr(b) for b in raw[9:-1] if 32 <= b <= 125).strip())
                seen = name_keys
            else:
                seen = None
                if command in (0x4c, 0x52):
                    row['kind'] = 'unverified-clear-marker'
            if seen is not None:
                if key in seen:
                    issues.append(dict(code='duplicate-key-record', key=key, kind=row['kind']))
                seen.add(key)
        if row['kind'] not in ('data', 'name'):
            issues.append(dict(code='unsupported-afx-response', messageIndex=number))
        records.append(row)
    keys = set(range(25))
    data_complete = data_keys == keys
    names_complete = name_keys == keys
    return dict(schemaVersion=1, scope='active-buffer-replies', readOnly=True,
                bytes=len(data), sha256=digest(data), records=records, issues=issues,
                dataKeys=len(data_keys), nameKeys=len(name_keys),
                missingDataKeys=sorted(keys-data_keys), missingNameKeys=sorted(keys-name_keys),
                dataComplete=data_complete, namesComplete=names_complete,
                complete=data_complete and names_complete and not issues,
                overlaysPresent=sum(r.get('hasOverlay', False) for r in records if r['kind'] == 'data'),
                savedBankVerified=False, transmitEligible=False, restorationVerified=False,
                limitation='Current-buffer records only. Bank/protection/dirty state are not queried; no saved-bank or restore claim.')


def inspect_file(path):
    path = Path(path)
    with path.open('rb') as source:
        raw = source.read(MAX_BYTES+1)
    return dict(source=str(path.resolve()), **inspect_bytes(raw))
