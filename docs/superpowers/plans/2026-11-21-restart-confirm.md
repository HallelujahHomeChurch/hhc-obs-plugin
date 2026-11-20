# Restart confirmation implementation plan

Goal: A released HHC output can start its next session after confirmation without losing installed recovery behavior.

Scope: src/key-controller.cpp and tests/key-dock-test.cpp; retain the deployed automatic-recovery commit before the focused fix. No credentials, journals, media, preferences or production profile migration.

- [x] Start from fetched origin/main e12a06e in fix/restart-confirm-20261121; integrate existing 538e1ed recovery behavior and retain main journal/close fixes.
- [x] Reuse the pinned SDK, configure an isolated build, and run baseline checks.
- [x] Add a regression for a visible confirmation after normal stop/release, cancellation, and unreleased/stale state; observe failure before changing the post-confirmation guard.
- [x] Apply the smallest shared-controller fix; run the focused tests, full CTest, media/package checks, and independent code review.
- [ ] Open PR, require green CI, merge under the user's explicit delivery authorization, then build/package from the merged commit; verify source and SHA-256 and publish release.
- [ ] Verify formal OBS outputs are idle before installation; preserve binaries/configuration/journals and retain rollback evidence. Do not interrupt an existing OBS process.
- [ ] Prepare isolated portable OBS, synthetic scene/media, separate output/helper/module config and credential storage, and a non-announcing test source. Local loopback first; wait for main chat's platform-ready signal before final A-stop-confirm-B live acceptance.

Decision: user requested direct execution, so no extra plan approval. Native worktree creation did not respond; use the repository's ignored .worktrees directory. Installed recovery behavior is part of compatibility, not a new feature.

Validation: 35 CTest cases passed across the low-privilege suite and two separately elevated credential tests; the final native UI and stop regressions passed after the isolated credential assertion was completed. The 90-second outage check passed. Media grid and Windows watcher path checks passed. Fresh read-only review found no actionable issues. No formal OBS was started or interrupted.
