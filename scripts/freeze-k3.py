"""Freeze only additive K3 schemas; never replace existing B1/C1 definitions."""
from pathlib import Path
import hashlib
import json
import sys
import yaml

source = Path(sys.argv[1])
for name, expected in {
    "openapi.snapshot.yaml": "f85eb10aee6c88676113454c9f7e61792309a60005eb27736049bc306bd196b6",
    "wire-fixtures.json": "e46f1df639956e54fc0fd165b194f0bef53eb52641b8df1b498a1d8f2940961f",
}.items():
    if hashlib.sha256((source / name).read_bytes()).hexdigest() != expected:
        raise SystemExit(f"Immutable K3 hash mismatch: {name}")
schemas = yaml.safe_load((source / "openapi.snapshot.yaml").read_text(encoding="utf-8"))["components"]["schemas"]
encoded = json.dumps({"components": {"schemas": schemas}}, separators=(",", ":"), ensure_ascii=True)
root = Path(__file__).resolve().parent.parent
(root / "src/k3-schema.inc").write_text(
    "\n".join('R"K3(' + encoded[i:i + 12000] + ')K3"' for i in range(0, len(encoded), 12000)) + "\n",
    encoding="utf-8", newline="\n")
(root / "tests/fixtures/k3.json").write_bytes((source / "wire-fixtures.json").read_bytes())
print("PASS: fixed K3 schemas and fixtures frozen")
