# B1 publication display correction, 2026-10-12

During the ongoing c2166da OBS long field, Console correctly requests automatic publication, but the OBS checkbox displays false. The bound C1 bridge intentionally reports publication cancelled because Console owns publication. The controller previously adopted Console policy and then overwrote it with this C1 status.

The controller now applies C1 publication cancellation only to standalone C1 sessions. Both Console policy values and later C1-only synchronization retain the bound publication choice. Recovering another retained session also clears an unrelated earlier cancellation warning; durable control intents are unchanged. HTTP, wire schema, encoding, Stop/seal and publication requests are unchanged.

The real Qt controller/future/Dock regression first reproduced two checkbox failures. A separate real recover() transition test then reproduced the stale warning identified by independent review. After the two production corrections, the full native CTest suite passed 14/14, exit0, at 14:45 Asia/Taipei UTC+8. The complete B1 fixture is read with an explicit nonempty assertion and source working directory. No GPU or real HTTP is invoked by these tests.

Independent follow-up review found no actionable findings. That reviewer inspected code and the saved results, without independently running tests or accessing OBS/platform.

Raw RED/GREEN logs are retained under ignored artifacts/b1-current-long-20261012. The ongoing live run continues with the original c2166da binary/ZIP; it does not test this correction. A freshly built candidate and hosted CI are required before installation after the long field ends normally. No running DLL replacement, merge or deployment is performed.
