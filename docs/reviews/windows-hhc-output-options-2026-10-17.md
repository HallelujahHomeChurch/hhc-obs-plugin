# HHC output preferences — isolated preview

Matches the four requested OBS 32.2.2 General / Output options, in an Output group with the native QFormLayout, 170 px label spacer and 2 px top margin. Keep-recording depends on auto-record; disabling auto-record preserves the stored keep-recording choice. Defaults are all false, preserving previous behavior.

Preferences are independent HHC output.ini values under the module configuration, not changes to OBS BasicWindow settings. Apply / OK and close / Escape include checkbox edits; Cancel restores saved values. Keys remain in Credential Manager.

OBS frontend recording_start / recording_stop reuse the existing recorder and output profile. Like OBSBasic_Streaming.cpp, successful output start requests recording when enabled; a user stop requests recording stop if auto-record is enabled and keep-recording is off, including an already running local recording. There is no recording ownership or YouTube-specific policy. Transfer failures and Console End do not invoke the user-stop recorder linkage; existing remote End isolation is preserved. A confirmation cannot start a replacement output when the original output finishes during the modal prompt.

Validation: 23 local CTests passed; expanded native-settings and key-controller tests additionally cover saved preferences, dependency, cancel/reopen, native validation rejection, recording callback choices and rejected start/stop confirmation. Isolated UI inspection follows packaging. These are local / mock tests, not a new live capture or native recording media acceptance. No platform / encoding / wire changes and no formal OBS update.
