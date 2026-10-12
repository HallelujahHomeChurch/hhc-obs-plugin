# Admin auto-cover original-page follow-up

Please investigate the existing Admin Console image state without changing B1/C1 or media validation. This does not block the running Windows long-source test.

Loaded Admin34b5073f6d821fde400a4a1a1de583861be58f0f, CMS0187, Asset0090, Website0203. Recording1a336a82-3540-4037-8a49-6e1ace08c314/capture0b4a109f78c45cdc0f06ab1b2c4d2541, installed pluginc2166da44da0d2fccbd4f84d82aa54a2fa03ad55. Times are2026-10-12 Asia/Taipei UTC+8.

Reproduction: keep the original Console page open; initial auto cover342408181862cc686c230be3e32e5ae5 is ready. Upload/save a per-field fixed cover7606c37969518902d6d2ae4356b58da1 (revision2), then switch/save back to auto at about13:40 (revision3). The API returns selection.mode=auto/coverState=ready with the original auto cover ID, but the original page has zero images labeled currently selected cover, still absent at14:09. A fresh read-only Console page shows the image, and the member list also shows it. No original-page reload is used to hide the failure. This scopes the symptom to retained Console UI state; no root cause is asserted yet.

Auto JPEG is64331 bytes, SHA-2562baa5ad611f92c84a92486926667c9e1058d84466fcc079e2f32769a6e33f358. Fresh Console/member-list decoded pixels match. Raw sanitized evidence remains in Windows artifacts/b1-current-long-20261012/auto-reuse-console-display-safe.json and auto-cover-safe.json. Do not delete or modify this field while Windows acceptance continues.

Please reproduce with the real cover component, identify the state/request/URL lifecycle cause, and add a test for custom→auto cached-image reuse on the same open page. Also retain custom selection when late auto work finishes. Return PR/CI/deployment status plus immutable commit and a Windows retest prompt; do not treat tests or a fresh-page image as same-page acceptance. No wire-schema or OBS change is requested.
