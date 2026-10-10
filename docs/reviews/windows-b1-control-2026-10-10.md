# Windows B1 control implementation evidence

Candidate implementation only; no new production capture, recording, live stream or GPU run in this phase. Actual deployment versions and capture/recording IDs therefore remain **not observed** for B1. All controller HTTP fixtures use loopback and synthetic identities. Times in subsequent E2E evidence must include Asia/Taipei (+08:00) and UTC.

Pinned contract acknowledgement: [B1/C1](b1-contract-acknowledgement-2026-10-10.md).

Implemented:

- Capability negotiation and selectable Console broadcasts. Bind intent is committed before POST; accepted 202/null waits by GET, preserving the original operation key. The uploader adopts that original capture and never creates another C1 capture.
- Independent active-only two-second control worker, bounded transient backoff, account/session/epoch fencing, atomic command journal and ACK replay. HTTP 409/412 first reconcile by GET without changing keys.
- Markers use current encoder video PTS across all three renditions and the strictly next 900-frame boundary. The clock becomes unavailable after stop/failure/destruction. No verified watermark or scheduledAt gate is used. Public End ACK is exclusive and never stops HHC, YouTube or global OBS outputs.
- B1 UI uses Console title/policy, preserves standalone C1 options and only exposes the HHC stop action while bound. A control retry keeps the original journal. Uncertain saved ACKs are reconciled before stop/seal recovery; inactive completed journals perform no control polling.
- Existing C1 normal-stop/tail/upload/seal/ready/autopublish and abnormal abort flow, Credential Manager and OAuth retained unchanged. Broadcast journals store IDs, wire version, bind/ACK bodies and receipts; no credentials or signed URLs.
- Established uploads no longer depend on B1 GET. Saved uncertain ACKs fence only seal, after C1 stop/upload; rejected stale commands are reconciled and retired without poisoning newer commands. See the [final review and RED/GREEN fixes](windows-b1-final-review-2026-10-10.md).

Local evidence:

- Contract fixtures, marker calculation and original ACK replay observed RED before implementation, then GREEN.
- Additional accepted/null test reproduced duplicate bind POST; fixed to GET-only after acceptance. Cancelled uncertain command test covers fencing and later command handling. Nullable journal entry introduced by a mutable JSON read was identified and fixed at that read.
- CTest 14/14 passed: fixed B1/C1 wire, synthetic HTTP/recovery/finalization, mock dock, auth/account isolation, metadata sharing, controller lifetime and marker policy. Media grid and Windows watcher exit-code tests passed separately.
- Tests include future scheduledAt without a local start gate, end boundary 20 after start 8, lost ACK replay choosing original 8 rather than later 99, no invented ACK revision, old epoch rejection, cancelled ACK, 409/412 GET reconciliation and bound C1 adoption.
- Qt lifecycle checks hold a completed control result before GUI consumption: logout remains fenced, End keeps capture phase unchanged, inactive sessions launch no control request, expired clocks yield no marker, corrupt B1 recovery never falls back to C1 create.

Pending independent qualification:

1. Final source review and both Important fixes are complete; hosted CI and unsigned candidate package evidence are linked from the draft PR and package manifest for the actual commit.
2. Actual deployed Admin #219/Website #197 versions, B1 writer enablement and user notice before real OBS/API tests. Historical handoff deployment/flags are insufficient.
3. Real OBS bind/preview/start/end boundaries, actual range access, normal Stop, stop/seal/ready/autopublish, crash recovery, and current-source 2.5-hour run.
4. Deployed member-player quality switching/last-frame preservation, complete duration/seek/pause/rate/DVR, Auto ABR and regular meeting-load follower recovery to stable approximately 2–3 minutes.
5. Five-minute outage/ten-minute catch-up remains separate stress evidence. Existing approximately 379/388-second recovery failures are retained and are not changed to pass by this implementation or successful VOD publication.

Known limits: local tests cannot prove actual encoder/muxer timing or platform exposure. Terminal command/binding conflicts preserve the journal and require reconciliation; no new key, new epoch, new capture or CPU fallback is invented. This unsigned preview is not production-ready acceptance.
