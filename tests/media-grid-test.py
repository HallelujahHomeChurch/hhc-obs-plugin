"""Local QA must reject the observed short interior fragment, while allowing a short tail."""
import importlib.util
import pathlib

spec = importlib.util.spec_from_file_location("media_qa", pathlib.Path(__file__).resolve().parents[1] / "scripts/verify-media.py")
qa = importlib.util.module_from_spec(spec)
spec.loader.exec_module(qa)
assert qa.ffmpeg_path(r"\\?\C:\long\index.m3u8") == r"C:\long\index.m3u8"
assert qa.ffmpeg_path(r"\\?\UNC\server\share\index.m3u8") == r"\\server\share\index.m3u8"
qa.check_segment_grid([30.03, 30.03, 0.934267])
qa.check_segment_grid([10.01, 10.01, 0.934267],300)
for times in ([10.00,10.01], [10.01,8.008,10.01], [10.01,30.03]):
    try: qa.check_segment_grid(times,300)
    except AssertionError: pass
    else: raise AssertionError(f'accepted invalid S10 grid: {times}')
for durations in ([], [30.03, 28.028, 30.03], [30.03, 0], [30.03, 32.032]):
    try:
        qa.check_segment_grid(durations)
    except AssertionError:
        continue
    raise AssertionError(f"accepted invalid grid: {durations}")
print("PASS: regular frame grid and short-tail QA")
