# HHC stream status fix

Uses the same class=state-active and stylesheet refresh as OBS32.2.2 native controls. Active color follows running HHC capture, not public broadcast status. Opening intent alone stays neutral, and finish removes active color.

Footer GPU percentage uses NVIDIA driver NVML nvmlDeviceGetEncoderUtilization (https://docs.nvidia.com/deploy/nvml-api/latest/api/group__nvmlDeviceQueries.html), sampled off the UI thread every two seconds. System32 DLL only; no redistributed driver or subprocess. Measures the single NVIDIA GPU encoder engine including other applications, not 3D utilization or HHC-only load. Unsupported/missing/multiple devices return unavailable (—). Multi-GPU adapter identity mapping is deferred explicitly. No CPU fallback or encoding change.

23 local CTests passed, including active/idle style, GPU percentage/unavailable text. Real local read-only NVML sample returned 0% while idle. Tests are local/mock; no new public live capture or under-load telemetry acceptance. Existing formal install is unchanged.
Mac handoff: mac-studio-start-ui-followup-2026-10-17.md.
