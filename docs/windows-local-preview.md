# HHC OBS Windows x64 platform preview

Requires OBS 32.2.2 x64 / Qt 6.11.1. This unsigned preview implements C1 c1-2026-10-08.2 against the approved production platform. Local/CI results alone do not establish media readiness, member playback or operational acceptance. Fixed contract acknowledgement: [C1 verification](https://github.com/HallelujahHomeChurch/hhc-obs-plugin/blob/148c8787fc2fef50908dc5b01c5d44716e0638ca/docs/reviews/c1-2026-10-08.2-acknowledgement.json).

## Install and operate through the GUI

1. Exit OBS. Unzip the package using Explorer. Copy both `obs-plugins` and `data` into the OBS installation folder, keeping the directories intact. The package contains the plugin DLL, background upload helper and matching native Windows Schannel TLS backend.
2. Open OBS and enable the **HHC 影音** dock. Choose **登入 HHC**, then complete sign-in/authorization in the system browser. Passwords are never entered into OBS. Tokens and rotated refresh credentials stay in Windows Credential Manager.
3. Set a title and audio track (1–6). Program must be 1920×1080 at 30000/1001 fps, NV12 limited BT.709, 48kHz stereo. NVENC failure is explicit; there is no CPU fallback.
4. Select intent before starting. Both controls off means recording only. **同步開放會員直播** permits member live, independently of **完成後自動發布**. Either exposure requires the issuer's current publish scope. Server ready and server published remain distinct.
5. Stop recording normally. The local stop intent is persisted before notifying the server. Closed backlog/tail uploads continue, then normal inventory is sealed. Wait for server media validation and the actual automatic-publication state. A blocked publication must be investigated in Admin; the plugin never calls manual publish as a fallback.
6. **關閉會員直播，繼續錄影** closes live admission without cancelling publication. **取消會後自動發布** leaves live independent. Accepted controls do not reopen later.
7. On disconnection the account-bound queue remains durable. Temporary network/service failures retry with capped backoff and jitter while the capture remains eligible; repeated failures do not silently discard the queue or create another recording. Permission, authentication, expiry and invalid-response errors require attention. Use **重新檢查本機收錄**, select a session and **繼續同步所選收錄** when intervention is required. Interrupted encoders are explicitly aborted/incomplete; their retained media cannot be fabricated into normalEnd.
8. On an accepted OBS exit, a normal encoder stop is attempted and the helper continues complete sessions without a terminal window. The helper has a fixed24-hour lifetime and does not extend the server's upload deadline. If it needs sign-in or reaches that limit, reopen OBS to inspect/resume the original queue; an expired capture requires Admin handling. Physical crashes require explicit operator recovery; incomplete media remains preserved.

The original YouTube stream and original OBS recording use their existing settings independently. Member playback and full-event DVR belong to the website. Synthetic production test exposure must be deliberately enabled by the authorized operator.

## Remove through Explorer

Exit OBS. Remove only `obs-plugins/64bit/hhc-obs-plugin.dll`, `obs-plugins/64bit/hhc-upload-helper.exe` and `data/obs-plugins/hhc-obs-plugin`. Keep the per-user plugin configuration and queue so pending media is not lost. Credential removal is available in Windows Credential Manager; never delete another application's credentials. Reinstalling uses the same device ID and account-bound queue.

Developer fixtures and fault injection are excluded. Hosted CI has no qualified NVENC GPU. This is a platform preview; see the repository's integration ledger for observed OAuth, recording/live/publication, recovery and current-build long-run results.
