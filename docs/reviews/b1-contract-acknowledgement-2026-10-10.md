# B1 Windows contract acknowledgement

Fixed release handoff: [CMS 0ba40cce](https://github.com/HallelujahHomeChurch/hhc-web-api/blob/0ba40cce16f40c187c7fb019d3ca2a0a746379af/docs/obs-capture/live-control-v1/mac-release-2026-10-10.md).
Referenced rc2 files fetched at immutable CMS `eef3427c1d5931eccefb0083dbc87c57b31c9e8d`.
Document package `b1-mac-2026-10-10.rc2`; wire `b1-2026-10-10.rc1`.
C1 `c1-2026-10-08.2`, inventory schema 1 unchanged.

All actual downloaded bytes match:

| File | SHA-256 |
| --- | --- |
| implementation-manifest.json | 40685fe66911ba85ec159a8b99fb7a3a1551d0cf3512e8f9329408cb6d80b511 |
| openapi.snapshot.yaml | 845f7864578754bf80009f659075d4e3b53f20cc049f7a63b5bf138df70395cf |
| asset.openapi.snapshot.yaml | 089fe35b8495cca1db229f0594380b33e3616ca7fa1c4678d154848adfc54508 |
| integration-handoff.md | f49541269165975d981e299a69c7773fc639b199023e4b9bdaa4d0b0b42cbb11 |
| wire-fixtures.json | bbd056e6e3bb7546caadd4a2ff913ca760341ca61cb6bdfb4e94425be9c82bcc |

Embedded public B1 schemas include their complete referenced schemas; C1 uses its own unchanged schema root. Fixtures retain exact source bytes and their fake hashes are never uploaded. `scripts/freeze-b1.py` verifies all five hashes before regeneration. Operator flows do not require this developer tool or a CLI.

Admin #219 (`e9f2c122893de7fe4d0065cc3414f6c885035b62`) and Website #197 (`95e329ae4ea12638e45a7cb8dc0aa0b70a7054e6`) are user-reported CI-passing, unmerged and undeployed. The historical handoff's deployment/feature flags do not establish current production readiness. No production B1 request or OBS/GPU test is authorized by this acknowledgement.
