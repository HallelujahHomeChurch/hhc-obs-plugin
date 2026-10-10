"""Developer regression: accelerated stream-copy of actual closed NVENC media; no encoding."""
import argparse
import pathlib
import re
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("media", type=pathlib.Path)
parser.add_argument("--ffmpeg", required=True)
args = parser.parse_args()
ffmpeg = str(pathlib.Path(args.ffmpeg).resolve())
source = pathlib.Path(__file__).resolve().parents[1] / "src/hls-muxer.cpp"
segment_time = re.search(r'"hls_time", "([0-9.]+)"', source.read_text())[1]
media = pathlib.Path("\\\\?\\" + str(args.media.resolve()))

def run(*arguments):
    subprocess.run([ffmpeg, "-nostdin", "-v", "error", "-xerror", *arguments], cwd=root, check=True)

with tempfile.TemporaryDirectory(prefix="hhc-grid-") as scratch:
    root = pathlib.Path(scratch)
    joined = root / "native.mp4"
    joined.write_bytes((media / "480p/init.mp4").read_bytes() + (media / "480p/seg-000000.m4s").read_bytes())
    # Normalize to video-only input so loop duration is the video frame grid, not AAC padding.
    run("-i", str(joined), "-map", "0:v:0", "-c", "copy", str(root / "video.mp4"))
    run("-stream_loop", "68", "-i", str(root / "video.mp4"), "-c", "copy",
        "-hls_time", segment_time, "-hls_segment_type", "fmp4", "-hls_playlist_type", "vod",
        "-hls_flags", "temp_file+independent_segments", str(root / "index.m3u8"))
    durations = [float(v) for v in re.findall(r"#EXTINF:([0-9.]+)", (root / "index.m3u8").read_text())]
    assert len(durations) >= 68, durations
    bad = [(i, v) for i, v in enumerate(durations[:-1]) if abs(v - 900 * 1001 / 30000) > 0.000002]
    assert not bad, f"short interior fragment on cumulative grid: {bad}; hls_time={segment_time}"
    print(f"PASS: {len(durations)} accelerated NVENC-copy segments; hls_time={segment_time}; no short interior fragment")
