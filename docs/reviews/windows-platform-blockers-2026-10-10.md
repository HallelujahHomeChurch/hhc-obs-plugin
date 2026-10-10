# Fixed-version Windows acceptance: platform follow-up

Owner-reported deployment: Asset88/CMS169/Gateway199/Web185/client1.0.53,
handoff `hhc-web-api@40bffe8fc222eaa95d192a3d28f939f101e3cff2`.
C1 remains `c1-2026-10-08.2`, schema1; manifest and all11 immutable
references match their hashes. Actual test product is downloaded CI
`d59f1eb8035168cdc205eebcd1b42f82ecd76c0e`. The subsequent Windows-only
metadata reader fix `3462fe5` is separately qualified. All following times
are Asia/Taipei (+08:00). Preserve both original packages and operation keys.

1. Shared-load recovery failed the actual-player ten-minute gate. Capture
   `85a97e39455d279eb9cc23deb4b44fc3`, recording
   `cd07076b-082d-4361-bf29-0e5185de3409`: restore03:24:10.077;
   local upload/confirm zero03:33:03.692728; gate03:34:10.077.
   At the gate queued9/verified141, sequence45/end1381.38; original player
   1080p/1x/unpaused, actual currentTime1112.485148, no media error,
   lag387.880852 seconds. No reload or manual return-to-live before gate.
   First automatic jump03:35:13.247; lag remained152.844575 at03:37:14.251.
   Stop/seal/ready/automatic publication and outage VOD passed independently.
   Correlate admission/upload/verification/decode/publication stage logs and
   player availability/follower behavior; do not attribute all delay to one
   component without evidence. See the separate shared-recovery report.

2. Resolved pending state; retain latency correlation for Long34 capture
   `8057a3c4c2a27d699057f5e5434f9909`, recording
   `08c9f38a-397e-4037-9661-26bab09a2277`: stop accepted05:19:58.754341759,
   ended05:22:26.978623, sequence299/end9000.224567. At05:38:56.797536:
  907 declared,queued4/verified903,validating,automatic publication pending,
   terminalReason null. Stop and seal were accepted with original keys.
   All907 local hashes, aligned900-frame/30.03-second interior segments and
   full three-rendition decode passed; inventory SHA-256
   `2d5dfc3cf463c839b0cf42d48ed08999944cd6d62263da8fe158de165f1c831d`.
   Original live replay ended at9000.255999 seconds without a media error;
   native OBS exited0 with unchanged profiles but `complete=false` at its
   bounded platform wait. Subsequent first samples were ready/all907 verified
   05:44:56.718350 and automatically published05:50:04.731566. Its selected
   first Auto VOD JPEG matched the44929-byte live image/hash. Actual native
   recovery returned published/complete=true/exit0 at06:01:04.8791409 without
   a new capture, encoder or changed keys. This is no longer a pending
   ready/publication blocker. Retain job/stage timing for latency correlation;
   do not alter duration or add segments.

3. Resolved pending VOD/publication state; Thumbnail40 capture
   `a194554a868ba81badfd943abfb01325`, recording
   `49df7af4-64b9-4e02-afd2-3d88f2389a63`: pending Auto was switched to
   prepared fixed revision4 while live. Fixed upload
   `6a8ac076411b99660923a0b262f75b34` stayed ready and unchanged through
   stop05:21:09.977930716; ended05:22:39.039871. At05:40:36.223:
   capture ready/all67 objects verified, recording draft, automatic
   publication pending,selectedCoverId null, all four VOD candidates pending.
   Custom candidate `72ab928ca53324759f519ab165f9ffed-0`; Auto candidate
   prefix `c43b6e679f3e114aa9319374d6288efe-`. Fixed live JPEG34322 bytes,
   SHA-256 `7bd94154a93f3648e340adc0f0390c84d5c1fe58ddeebad8bc0ba75d986a3a90`.
   Original600.032767-second inventory/full decode passed; hash
   `0524187493c14d654f2910994593fad3b83ac29c25ad4951e1e22ad55e3e480b`.
   Native timeout `complete=false`, controlled OBS close exit0, no issue/code
   returned. First sampled automatic publication05:50:11.611807; direct
   readback05:56:06.876 showed all four VOD covers ready, custom selected,
   identical34322-byte fixed JPEG hash. The original capture was resumed by
   the verified new CI helper with unchanged inventory/keys. This is no
   longer a pending-cover/publication blocker. Published member VOD played
   from zero at1x to natural end06:10:00.027:600.063999 seconds,1080p,
  17987 decoded frames,31 dropped frames, no sampled media errors.
   Retain cover-generation/copy/selection/publication stage timing as a
   performance observation, without inferring a worker failure.

4. Live Auto480p cold startup failed a stronger first-presented-frame check
   on new capture `632bfda5df0932c7698cd4713cb5fff3`, recording
   `8a2a3ce3-5dff-4da6-9350-32932e926642`, actual CI product3462fe5.
   A passive MutationObserver was installed on the member list before any
   video existed and registered `requestVideoFrameCallback` when it mounted
   after ordinary SPA navigation. At05:51:48.153, presentedFrames1,
   decodedFrames4, width1920/height1080,mediaTime90.09,1x/unpaused/no error.
   The quality menu confirmed Auto selected and all manual choices unselected.
   No reload, return-to-live or quality action preceded this frame. Startup
   did not present480p. Earlier25ms sampling of capture
   `ae629481f356c4af926485049b09fcce` first observed2 decoded frames at
  1080p/1x04:33:14.727 despite Auto checked, with a25ms passive observer
   installed before SPA navigation. Published VOD38 and39 separately started
   observed480p/frame8 and ended normally. ABR downgrade/upgrade passes are
   separate. Please confirm the live startup policy and deployed player path.
   Fresh member navigation/reload during ready/draft/publication-pending
   transitions in37 and40 displayed "這部影片目前無法觀看。"; subsequent
   published37 playback passed. A stale Admin tab401 persisted through its
   reload, while a fresh tab used the existing account successfully. These
   browser observations have no native OAuth root-cause attribution.

5. Thumbnail acceptance clarification remains open. The forwarded A list
   requests later occasional automatic updates. Fixed handoff line87 says
   subsequent live/reconnection must not periodically replace the image.
   Stable first-generated images, explicit mode switches and identical-byte
   VOD inheritance pass; no later automatic image replacement was observed
   or qualified. These statements do not define a non-periodic trigger.
   Please provide that trigger or a corrected immutable handoff. Do not
   silently count the narrower initial-generation pass as completion of
   the forwarded requirement. C1/wire/digest and the plugin remain unchanged.

6. Current3462fe5 Long41 normal-live observation exceeded the120-second
   p95 estimate. Same recording/capture as item4, no outage injected,
   no reload/manual ReturnLive/quality change.8828 playing decoded1x samples
   before actual stop08:18:56.063: p50=116.276907,p95=128.849099,
   p99=133.113367,max134.879626,3380 samples above120 seconds;
   all1080p and no sampled fatal HTMLMedia error. Encoder-wall-origin minus
   actual currentTime is an estimate, not visible-timecode OCR. Post-stop
   samples are excluded; earlier partial115.277484-second p95 is not a
   whole-run pass. Other renderer/browser/hash-scan loads are documented;
   this is not strict isolated-GPU qualification. Retain waterline/buffer
   and stage-log correlation without attributing cause to one component.
   Local907 hashes/stream formats/playlist grid/three full decodes and original live natural
   end passed independently. See [current-CI long evidence](windows-long-current-ci-2026-10-10.md).

Global live-cover defaults were restored to Auto revision6 at05:13:35.198,
retention30 days. No manual publish, arbitrary re-upload, schema relaxation,
duration change, credentials or signed URLs were used to force acceptance.
