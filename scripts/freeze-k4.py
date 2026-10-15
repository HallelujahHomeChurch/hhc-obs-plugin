"""Verify the immutable K4 delivery and freeze additive schemas/fixtures."""
from pathlib import Path
import hashlib
import json
import sys
import yaml

source = Path(sys.argv[1])
manifest = source / "docs/obs-capture/studio-v2/manifest.json"
assert hashlib.sha256(manifest.read_bytes()).hexdigest() == "e984a5a54b80c817ba701f6923d73c02b57bf9c5b7d238f15442543c5ae18464"
delivery = json.loads(manifest.read_bytes())
for path, expected in (delivery["files"] | delivery["frozenContracts"]).items():
    if hashlib.sha256((source / path).read_bytes()).hexdigest() != expected:
        raise SystemExit(f"Immutable contract hash mismatch: {path}")
schemas = yaml.safe_load((manifest.parent / "openapi.snapshot.yaml").read_text(encoding="utf-8"))["components"]["schemas"]
encoded = json.dumps({"components": {"schemas": schemas}}, separators=(",", ":"), ensure_ascii=True)
root = Path(__file__).resolve().parent.parent
(root / "src/k4-schema.inc").write_text(
    "\n".join('R"K4(' + encoded[i:i + 12000] + ')K4"' for i in range(0, len(encoded), 12000)) + "\n",
    encoding="utf-8", newline="\n")
for name, target in [("wire-fixtures.json", "k4.json"), ("identity-fixtures.json", "k4-identity.json")]:
    (root / "tests/fixtures" / target).write_bytes((manifest.parent / name).read_bytes())
print("PASS: K4 manifest, schemas, fixtures and all frozen K3/B1/C1 hashes")
