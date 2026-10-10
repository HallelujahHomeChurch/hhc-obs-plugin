# Windows B1 control integration

Execute inline on `feat/windows-capture`, existing isolated worktree. Scope is Windows plugin only.

Pinned specification: CMS `0ba40cce16f40c187c7fb019d3ca2a0a746379af` release handoff;
schema package `eef3427c1d5931eccefb0083dbc87c57b31c9e8d`, document rc2, wire `b1-2026-10-10.rc1`.
C1 `c1-2026-10-08.2`, schema 1, 30000/1001 fps and 900-frame segments stay unchanged.

1. Freeze and validate B1 schemas/fixtures, including nullable `anyOf`; RED then GREEN contract tests.
2. Add account-bound atomic bind/command journal and HTTP control consumer. Reuse C1 auth, HTTP, queue and upload flow. Test 202/null, replay after lost response, 409/412 reconciliation, cancelled/stale commands and epoch fencing.
3. Integrate selectable broadcast UI, adopt the existing bound C1 capture, independent two-second control polling and next encoder segment markers. Test UI/lifecycle and marker timing; run full local suite, obtain a fresh final review, update draft PR/CI and candidate package.

Inactive sessions never poll control. Console End never stops an OBS output. Persist marker/key before ACK; preserve an existing marker on recovery. Never use verified sequence or scheduled time as a marker or start gate.

Real OBS and production integration stay paused pending deployment/feature confirmation and user notice. Local/mock/CI results do not establish end-to-end acceptance. Preserve historical recovery failures and media evidence.
