"""Developer-only: regenerate embedded schemas from the immutable Mac delivery (PyYAML)."""
from pathlib import Path
import hashlib
import json
import sys
import yaml

source = Path(sys.argv[1])
hashes = {
    "implementation-manifest.json": "40685fe66911ba85ec159a8b99fb7a3a1551d0cf3512e8f9329408cb6d80b511",
    "openapi.snapshot.yaml": "845f7864578754bf80009f659075d4e3b53f20cc049f7a63b5bf138df70395cf",
    "asset.openapi.snapshot.yaml": "089fe35b8495cca1db229f0594380b33e3616ca7fa1c4678d154848adfc54508",
    "integration-handoff.md": "f49541269165975d981e299a69c7773fc639b199023e4b9bdaa4d0b0b42cbb11",
    "wire-fixtures.json": "bbd056e6e3bb7546caadd4a2ff913ca760341ca61cb6bdfb4e94425be9c82bcc",
}
for name, expected in hashes.items():
    if hashlib.sha256((source / name).read_bytes()).hexdigest() != expected:
        raise SystemExit(f"Immutable B1 hash mismatch: {name}")
schemas = yaml.safe_load((source / "openapi.snapshot.yaml").read_text(encoding="utf-8"))["components"]["schemas"]


def references(value):
    if isinstance(value, dict):
        for key, item in value.items():
            if key == "$ref":
                yield item.rsplit("/", 1)[-1]
            else:
                yield from references(item)
    elif isinstance(value, list):
        for item in value:
            yield from references(item)


names = {name for name in schemas if name.startswith("Broadcast")}
while True:
    expanded = names | {ref for name in names for ref in references(schemas[name])}
    if expanded == names:
        break
    names = expanded
encoded = json.dumps({"components": {"schemas": {name: schemas[name] for name in sorted(names)}}},
                     separators=(",", ":"), ensure_ascii=True)
root = Path(__file__).resolve().parent.parent
# MSVC has a per-literal size limit. Adjacent literals preserve the exact JSON.
(root / "src/b1-schema.inc").write_text(
    "\n".join('R"B1(' + encoded[i:i + 12000] + ')B1"' for i in range(0, len(encoded), 12000)) + "\n",
    encoding="utf-8", newline="\n")
(root / "tests/fixtures/b1.json").write_bytes((source / "wire-fixtures.json").read_bytes())
print("PASS: immutable B1 delivery frozen; C1 untouched")
