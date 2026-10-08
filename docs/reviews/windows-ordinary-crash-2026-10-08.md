# Ordinary-mode OBS interruption: partial acceptance

At20:36:16 Taipei on 2026-10-08, the isolated developer portable OBS32.2.2 started with `--portable --disable-updater --disable-missing-files-check`, without `--multi`. It encoded the synthetic Program with the existing three-rendition NVENC capture implementation. After six closed objects passed SHA-256 checks, the exact owned process5912 was forcibly interrupted at20:36:52; its retained process handle reported exitCode=-1. The real nonzero application UUID sentinel remained. No sentinel was fabricated, removed or renamed.

Local session `5dae7dca-5190-44d2-91c3-6cd8b5cf9e7c` is account-bound to offline-validation-only and has six retained objects. A separate actual SessionStore recovery process returned0 and recovered those objects. The journal remained byte-identical to its pre-interruption copy. normalEnd, stopIntent, sealAcknowledged and confirmedReady all remained false, and no final inventory was invented. SHA-256, size, H.264/AAC formats, three dimensions,48kHz stereo,30000/1001 and full retained-fragment decoding passed.

The same portable installation restarted in ordinary mode at20:37:39. Its actual log reported `Crash or unclean shutdown detected` and process19928 remained live at the hidden crash prompt. The current f009725 normal plugin/helper/TLS files had been installed from the previously verified portable-short candidate without copying credentials. Native modules and the HHC dock have not yet loaded while this prompt is pending. Choosing normal/safe mode and proving candidate dock availability remain unqualified. Displaying the isolated interactive test window has been requested; elapsed time is not approval. There was no production HTTP or fresh recording intent in this test.

## Corrected isolation assumption

OBS32.2.2 `frontend/utility/platform-windows.cpp`, CheckIfAlreadyRunning, uses OBSStudioCore for ordinary installed OBS, and OBSStudioPortable plus the absolute configuration path for portable OBS. Consequently separate portable roots can run simultaneously without sharing this single-instance mutex. The actual ordinary-mode run above coexisted with the original-recording/HHC long process in portable-concurrent. Waiting for that long process to finish is not a prerequisite for this separate-root interruption test. Crash sentinel handling is also per configuration root.

Earlier `--multi` interruption tests do not cover ordinary crash detection: that mode constructs CrashHandler without the unique application launch UUID. The new ordinary-mode interruption and detection evidence closes that gap only as far as explicitly described above.

## Native UI launch limitations observed

A Computer Use launch with the plain forward-slash executable path resolved to installed OBS, process26564, rather than the intended portable instance. The visible controls showed streaming and recording inactive. That newly launched process was closed through its returned native window; the original basic.ini, recordEncoder.json and streamEncoder.json hashes remained unchanged.

An explicit process identifier with the backslash executable path targeted the intended portable-short executable, but the launcher did not provide its required bin/64bit working directory. OBS reported missing locale/en-US.ini and then failed to load locale, before native plugin startup. Both error dialogs were acknowledged and that process exited. The subsequent scoped developer launch with an explicit working directory is the applicable ordinary-mode evidence; these UI launch failures are not evidence of a plugin initialization defect.

The hidden crash prompt is absent from Computer Use's targetable window inventory. It must be exposed and observed before any dialog input. Do not replace this with forged window handles, injected Qt choices, sentinel manipulation or `--multi` and claim operator acceptance.

Safe evidence is in the adjacent JSON and `artifacts/normal-single-instance-crash-28bcb5d`. The original-recording/HHC9000-second process13768 remains independent and active. Full ordinary restart UI, physical keyboard, production recovery and physical power failure remain open.
