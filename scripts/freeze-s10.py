"""Freeze only S10 additive schemas from the hash-pinned CMS snapshot."""
from pathlib import Path
import hashlib, json, sys, yaml

source = Path(sys.argv[1])
expected = {
    'contract.md': '05427d60479d10ece164bbd19d7f055003af5d194475c158aa04bc0f3ae8e953',
    'cms.openapi.snapshot.yaml': 'b9ae8320535133f0d3fc92b1e3f0898e3218b45710d6d7cbfc97e2a0cd48458f',
    'asset.openapi.snapshot.yaml': '1011b00785af8da4051f529f2cb2bff04f11f12352471b20c971a3bd9f906199',
    'fixtures.json': '8b48000e84ac2ffafb8b73bb27deece1337e0f577de9ab6d223d1b73db2bb9b9',
    'manifest.json': '74d7b3499bfe9bd94510872be0f0a0ec6f5cd6147faacd23070405645c5462a4',
}
for name, digest in expected.items():
    assert hashlib.sha256((source/name).read_bytes()).hexdigest() == digest, name
schemas = yaml.safe_load((source/'cms.openapi.snapshot.yaml').read_text(encoding='utf-8'))['components']['schemas']
selected = {}
def visit(value):
    if isinstance(value, dict):
        if '$ref' in value:
            add(value['$ref'].split('/')[-1])
        for child in value.values(): visit(child)
    elif isinstance(value, list):
        for child in value: visit(child)
def add(name):
    if name not in selected:
        selected[name] = schemas[name]
        visit(selected[name])
for name in ['BroadcastOutputOpenInput', 'BroadcastOutputOpenedEnvelope', 'RecordingPackageInventory', 'RecordingCaptureEnvelope', 'RecordingCaptureResultEnvelope', 'RecordingLiveProgress']:
    add(name)
encoded = json.dumps({'components': {'schemas': selected}}, separators=(',', ':'))
target = Path(__file__).resolve().parents[1]/'src/s10-schema.inc'
target.write_text('\n'.join('R"S10('+encoded[i:i+12000]+')S10"' for i in range(0,len(encoded),12000))+'\n',encoding='utf-8')
(target.parents[1]/'tests/fixtures/s10.json').write_bytes((source/'fixtures.json').read_bytes())
print('PASS S10 rc2 fixed hashes; frozen', len(selected), 'schemas')
