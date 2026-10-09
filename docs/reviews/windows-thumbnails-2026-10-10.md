# Windows native thumbnail acceptance — 2026-10-10

Actual downloaded CI plugin: `d59f1eb8035168cdc205eebcd1b42f82ecd76c0e`
for shared35 and thumbnail36–40; isolated Auto28 used `7bc114f`.
The subsequent new long41 uses `3462fe5`; its evidence is identified separately.
Fixed platform handoff: `hhc-web-api@40bffe8fc222eaa95d192a3d28f939f101e3cff2`,
SHA-256 `66ae0c1c6b79ab41656fd2e04b51d709a3c00e4bea25e8e77cdebd2ca558430e`.
Owner-reported Asset 88 / CMS 169 / Gateway 199 / Website 185 / client 1.0.53.
C1 `c1-2026-10-08.2`, schema 1, immutable manifest SHA-256
`f702477236ba184853c119ffa9c4da0cc9f6d44cf7911785f85f3aa6d94e8be9`.
All eleven immutable contract references were checked. OBS 32.2.2, RTX 4060,
30000/1001 fps, three NVENC renditions, 900 frames / 30.03 seconds.
Times below use Asia/Taipei (+08:00). No platform source or original OBS
profile changes. The fixed handoff specifies one Auto image per live capture;
periodic image replacement is not an acceptance requirement.

## Auto

Recording `b384ab52-76fc-4dd8-bd41-7637a6f70ded`, capture
`b95611e8baf1cbce48c7e38a8f4458c8`: first ready live image observed
02:25:05.721, 45270 JPEG bytes, SHA-256
`4b5a0833ee90be5df4cb762f938a1e733e523993bd1fd6b61d3ed94123ada0ca`.
ID and bytes remained unchanged through five-minute upload interruption.
Member list, search and recommendations loaded actual 1280x720 images;
poster presence was observed. After asynchronous VOD inheritance settled at
03:02:07.329, exactly three ready Auto candidates remained and the first was
selected with the same actual JPEG hash. Native auto-publication, exit 0 and
published VOD natural end passed. Browser blob-byte fetch failed; it is not
presented as a browser poster hash match.

Shared-load recording `cd07076b-082d-4361-bf29-0e5185de3409`, capture
`85a97e39455d279eb9cc23deb4b44fc3`, independently passed stop/seal/ready/
automatic publication, all187 hashes and full three-rendition decode.
VOD decoded the outage interval and naturally ended at 04:12:30.251:
1800.170666 seconds, 1080p, 1x,32372 frames, zero dropped frames/no error.
Its live/selected-first-Auto JPEG hash was
`14081e8dcce80b686dd28cd7da24189804f7c73a1dd9b2e7d5d0542fa7601deb`.
Its ten-minute live recovery gate **failed**; thumbnail and VOD passes do not
qualify that gate. See [separate recovery handoff](windows-shared-recovery-2026-10-10.md).

## Fixed defaults and shared-reference cleanup

Admin upload/crop/save set global custom revision2, upload ID
`d62098e6db4d3fe337d5e622397381de`. Server JPEG:33711 bytes, SHA-256
`01f0ad28a8c57239b7fbd0c9a2baa6b6789516c18c45cbeb7f93ee0431bbaced`.
Input PNG hashes are retained separately; they are not server JPEG hashes.

| Capture | Recording | Initial inherited revision | Local media |
| --- | --- | --- | --- |
| `f840a977917588328044311f4f79d4ed` | `89d2139b-5894-416b-95d6-261eea204549` | 2 | 5 segments per rendition,121.054267 seconds |
| `a2c98c955cf9c6aff040c58edfd9b07a` | `3a1b6556-bae7-4e3e-9aed-cd873d93a658` | 2 | 20 segments per rendition,600.132867 seconds |

Both actual new captures inherited exactly the same upload ID and JPEG.
The121-second capture accepted stop03:53:16.450628275, published automatically,
exited0 at03:59:49.8321870, and retained the selected custom JPEG bytes in VOD.
Full local hash/grid/timeline/three-rendition decode passed. Published VOD
replayed from0 to natural end, observed04:19:50.847:121.087999 seconds,
1080p/1x,3628 frames, zero dropped frames/no error.

Global default B was saved in the Admin UI, read back04:17:25.276 as revision3,
upload ID `15b3c12b7a7cca4cef990b23427353cd`. Existing600-second capture still
had inherited revision2, revision1 and the original fixed A ID/hash.
The old Auto long capture likewise retained its original Auto snapshot.

After preserving original media and published-play evidence, only the owned
121-second synthetic recording was deleted:204, then404. Global default had
already changed to B, so it no longer retained A on behalf of the captures.
At04:20:01.591 the other capture still served the exact original33711-byte
JPEG. Its selected VOD custom cover at04:22:40.719 also matched that hash.
The600-second local original-media check exited0 at04:20:40.8889096.
All original operator profile hashes were unchanged on completed native exits.

The600-second capture published automatically at04:24:50.802 and exited0
at04:24:51.3230544. After publication, a fresh member page loaded the1280x720
poster and played the VOD from0 to natural end, observed04:35:58.889:
600.170666 seconds,1080p/1x,17990 frames, zero dropped frames/no error.
Selected custom VOD JPEG bytes still matched A after peer cleanup.

New capture `ae629481f356c4af926485049b09fcce`, recording
`2110de72-c986-44bf-84ed-d6509e39b318`, read back inherited revision3/B at
04:26:43.133:34322 JPEG bytes, SHA-256
`7bd94154a93f3648e340adc0f0390c84d5c1fe58ddeebad8bc0ba75d986a3a90`.
Admin UI saved Auto revision2 (pending04:27:19.820), then uploaded and saved
fixed revision3. Auto had already completed before fixed was saved; this
sequence does **not** prove late-work fencing. It does prove cache reuse:
switching back to Auto revision4 immediately returned ready at04:30:29.058,
with the original Auto ID `44adfb50e1652f5e93e9ee232913a2a4` and exact44626-byte
JPEG hash `52653f71f1d6ec32b50bef287da681032475e2dedfcfa8b5b21ef93ee368d046`.
Automatic-publication intent remained pending throughout these scene settings.

This capture later paused native synchronization at recording clock811962ms
with the generic internal-sync message. Encoding continued to a normal local
end at900.232667 seconds:30 segments per rendition,97 original objects.
The initial native run timed out with `complete=false`, although its controlled
window close exited0 at04:55:14.5040810 and original profiles were unchanged.
The first exception/canonical error code was not retained; root cause remains
unknown. It is not attributed to the platform, OAuth or file sharing.

The nominal900-second local check failed its0.2-second timer tolerance. A
separate check against the original900.232667-second media passed all97
hashes, strict grid/aligned timelines and full three-rendition decode at
04:46:05.774732. Inventory SHA-256:
`0fb84c35f68a7c3f7330e8693baa40dbf6350d05a34923febc4d5c513753a865`.
No duration, object or operation key was changed to pass validation.

An explicitly scoped developer step accepted the original stop at
04:46:07.399266143. The actual downloaded CI helper then resumed the same
capture and accepted seal at04:52:52.858416 with original key
`74b2da75-7dfd-4644-850c-70ae83e6a9d1.seal`. All97 objects were confirmed;
first sampled ready was04:55:34.300908. The helper exited0 at04:59:56.3096613.
A direct read at05:04:18.460 confirmed published, exactly three ready Auto
candidates, selected first candidate `a5b01b8775e2074d11c8528f9a8c93c6-1`,
and the same44626-byte JPEG hash as the earlier live Auto image. Helper
recovery is separate from an uninterrupted native synchronization pass.

## Remaining observations

All times in this report use Asia/Taipei (+08:00). Global defaults were
restored to Auto revision6 at05:13:35.198; retention remains30 days.

Capture38 was also resumed through the actual OBS recovery dropdown and
resume button, using the downloaded `d59f1eb` CI runtime and its original
local identity. No new capture or encoder was created. It showed published
with `complete=true`, no warning, and OBS exit0 at05:36:25.619644 with the
original profile hashes unchanged. Published VOD reached natural end at
05:24:31.767:900.266666 seconds,1080p/1x,26984 decoded frames, zero dropped
frames or sampled media errors. This successful recovery does not erase the
initial native synchronization timeout.

## New default and pending Auto-to-fixed case

Recording `89312550-8d7c-4fbd-ba2a-6093fd489365`, capture
`ba6b26b80c59cd6aee1f86a5e5aba996`, inherited fixed B revision3. Its snapshot
retained B through subsequent global Auto4/B5/Auto6 changes. Stop was
accepted05:04:12.096915619; ready was first sampled05:06:23; native automatic
publication completed and OBS exited0 at05:09:50.6846861 with original
profiles unchanged. A per-scene B upload was saved after stop, so this is
not a live late-work race. Published selected custom cover
`b6f8f6c09f28aa454deaa6bdc2c400f5-0` matched the34322-byte B JPEG hash above.
All49 objects/full three-rendition decode passed against the unchanged
420.2198-second inventory; nominal420±0.2-second timer tolerance failed.
Published VOD reached natural end05:19:17.175:420.245333 seconds,1080p/1x,
12598 decoded frames, no dropped frames or media error. First observed
published VOD playback was480p/frame count8; this is separate from live
Auto startup.

Recording `49df7af4-64b9-4e02-afd2-3d88f2389a63`, capture
`a194554a868ba81badfd943abfb01325`, inherited fixed B revision5. Its
per-scene fixed upload `6a8ac076411b99660923a0b262f75b34` was prepared before
requesting Auto. Auto revision3 was waiting at05:14:03.855 and pending at
05:14:38.333 while still live. Saving fixed revision4 returned ready at
05:15:08.934, and05:17:28.872 still showed fixed revision4, the same upload,
and identical34322-byte B JPEG bytes. Pending Auto work did not overwrite
the observed live fixed selection; exact worker execution time is unavailable.

Stop was accepted05:21:09.977930716; ended05:22:39.039871, sequence19.
All67 objects and full three-rendition decode passed at600.032767 seconds,
including the nominal timer tolerance. Inventory SHA-256:
`0524187493c14d654f2910994593fad3b83ac29c25ad4951e1e22ad55e3e480b`.
At05:40:36.223 capture was ready, recording draft, selected VOD cover null,
with all four VOD candidates still pending, including custom
`72ab928ca53324759f519ab165f9ffed-0`. The live setting remained fixed4/ready.
Automatic publication remained pending; no manual publication or cover
selection was used to force success. The original native fixture timed out
with `complete=false`, then closed normally with exit0 at05:35:39.327305,
original profiles unchanged. VOD inheritance, publication and playback of
this case were still unpassed at that observation.

Subsequent automatic publication was first sampled05:50:11.611807. Direct
readback05:56:06.876 confirmed published, all four covers ready, and custom
`72ab928ca53324759f519ab165f9ffed-0` selected. Its34322-byte JPEG hash
exactly matched the earlier live fixed image. The late Auto candidates did
not replace it. No manual publication or forced cover selection occurred.
The actual downloaded CI3462fe5 helper had resumed the original capture
after controlled test-runtime maintenance; its original inventory remained
unchanged. The original native timeout is retained separately. Published
member VOD played from zero at1x to natural end06:10:00.027,600.063999
seconds,1080p,17987 decoded frames,31 dropped frames, no media error.
The first callback after ordinary play observed480p/mediaTime0 at
05:59:59.825, presentedFrames2/decodedFrames8. A paused frame existed before
instrumentation, so this is not first-presented-frame evidence. There were
no manual quality, rate or seek changes during this VOD watch. Its final
fixed selection and JPEG inheritance remain correct.

A fresh member-list navigation at the stop/ready-to-publication transition
displayed "這部影片目前無法觀看。" and no video/decoded frames. It does not
qualify live cold startup, and it preserves a second observation of the
publication gap described below.

An old Admin tab returned401 for live-cover-settings and retention-policy;
the dialog's reload also returned401 despite displaying IT HHC. A new tab
loaded the existing account session and successfully uploaded/saved B.
Token internals were not read. Root cause is not determined; this is a
platform/browser-refresh observation, not a native OAuth failure claim.

At04:21:38.684, reloading the600-second member page while capture state was
ready, recording still draft and auto-publication pending displayed
"這部影片目前無法觀看。" The earlier live replay had naturally ended without
error, and direct shared-image/VOD bytes survived cleanup. This readiness-to-
publication window is retained even though the subsequent published-play
check passed. It is not attributed to shared-image deletion. Retired live-content GET subsequently
returned404 `capture_not_found`; diagnostics retain that response and read
actual VOD cover content separately.

Auto480p startup remains unqualified. On an ordinary member-list navigation
to the new capture, a25ms passive observer was installed before any video
existed and survived client navigation. First observed decoded frames at
04:33:14.727 were frame count2,1920x1080,1x/unpaused; subsequent samples
advanced without an error. The quality menu confirmed Auto selected, manual
1080p unselected. This resolves the earlier instrumentation gap but cannot
identify individual frame1 between samples or claim480p playback from playlist
requests. Existing measured ABR down/up passes remain separate.

The subsequent new CI3462fe5 capture `632bfda5df0932c7698cd4713cb5fff3`
provided stronger evidence: a passive video-frame callback registered on
mount before member SPA navigation observed presentedFrames1 at05:51:48.153,
1920x1080,decodedFrames4,mediaTime90.09,1x/unpaused/no error. The quality
menu confirmed Auto selected and all manual choices unselected. No reload,
return-to-live or quality change preceded it. Live480p startup fails this
case; published VOD480p startup and ABR are separate results.

Browser screenshot commands timed out; native fixture PNGs and allowlisted
media/network observations are retained. No successful browser screenshot,
physical network/power failure, Safari test, signed release or production-ready
qualification is claimed.

## Completed-test cleanup

After preserving actual hashes, original media, native outcomes and published
VOD natural-end evidence, completed synthetic28/37/39 were deleted under the
standing test cleanup authorization at05:55:32.9183247,05:55:34.9464885 and
05:55:36.9731580 respectively. Each typed API DELETE returned204 and
subsequent recording GET404. Synthetic36 had already been cleaned after its
shared-image test. Diagnostic35/38 and long34/thumbnail40/current41 were
excluded, as were all formal recordings. Local evidence remains retained.
