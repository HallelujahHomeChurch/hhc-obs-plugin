"""Verify and freeze the version-negotiated D1 rc2 delivery."""
from pathlib import Path
import hashlib, json, sys, yaml
root=Path(__file__).resolve().parents[1]
source=Path(sys.argv[1])
for name,digest in {
 'contract.md':'e499e4b4c7c668b16a373c2c1b5676a83472bf1bb148c5ccef6e645e4428c57d',
 'openapi.snapshot.yaml':'4e090d1ea961a448112c9ffdf596b0ab837ccb7f367fc05c102d5c0e24f5021d',
 'fixtures.json':'8d677562a90dfdf6731a1666d2ce9254a79a681f685a871d2bb8740ff0d6d65b',
}.items():
 assert hashlib.sha256((source/name).read_bytes()).hexdigest()==digest,name
schemas=yaml.safe_load((source/'openapi.snapshot.yaml').read_text(encoding='utf-8'))['components']['schemas']
selected={}
def visit(v):
 if isinstance(v,dict):
  if '$ref' in v: add(v['$ref'].split('/')[-1])
  for c in v.values(): visit(c)
 elif isinstance(v,list):
  for c in v: visit(c)
def add(name):
 if name not in selected:
  selected[name]=schemas[name]; visit(selected[name])
add('BroadcastControlD1Envelope')
s=json.dumps({'components':{'schemas':selected}},separators=(',',':'))
(root/'src/d1-schema.inc').write_text('\n'.join('R"D1('+s[i:i+12000]+')D1"' for i in range(0,len(s),12000))+'\n',encoding='utf-8')
(root/'tests/fixtures/d1.json').write_bytes((source/'fixtures.json').read_bytes())
print('PASS: fixed D1 rc2 hashes')
