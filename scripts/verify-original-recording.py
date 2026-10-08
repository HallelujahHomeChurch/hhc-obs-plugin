"""QA for the isolated real OBS original-recording + HHC fixture; not E2E."""
import argparse
import hashlib
import json
import pathlib
import subprocess
import tempfile


def verify(directory, seconds):
    receipt_path = directory / 'original-recording.json'
    assert receipt_path.exists(), 'original recording receipt missing'
    receipt = json.loads(receipt_path.read_text(encoding='utf-8'))
    assert receipt['started'] and receipt['stopped'], 'original recording did not finish'
    assert receipt['stopRequested'], 'original recording stopped unexpectedly'
    assert receipt['hhcComplete'], 'HHC did not finish normally'
    assert receipt['overlapMs'] >= seconds * 1000, 'insufficient simultaneous recording time'
    path = pathlib.Path(receipt['file'])
    assert path.is_file() and path.stat().st_size > 0, 'original recording file missing'
    probe = json.loads(subprocess.check_output([
        'ffprobe', '-v', 'error', '-show_streams', '-show_format', '-of', 'json', str(path)
    ], cwd=tempfile.gettempdir()))
    video = next(s for s in probe['streams'] if s['codec_type'] == 'video')
    audio = next(s for s in probe['streams'] if s['codec_type'] == 'audio')
    assert (video['codec_name'], video['width'], video['height'], video['r_frame_rate']) == (
        'hevc', 1920, 1080, '30000/1001'), 'original video settings changed'
    assert (audio['codec_name'], audio['sample_rate'], audio['channels']) == (
        'flac', '48000', 2), 'original audio settings changed'
    assert seconds <= float(probe['format']['duration']) <= seconds + 30, 'unexpected recording duration'
    decoded = subprocess.run([
        'ffmpeg', '-v', 'error', '-i', str(path), '-f', 'null', '-'
    ], cwd=tempfile.gettempdir(), capture_output=True, check=True)
    assert not decoded.stderr, decoded.stderr.decode(errors='replace')
    with path.open('rb') as stream:
        digest = hashlib.file_digest(stream, 'sha256').hexdigest()
    return {'evidence': 'local real OBS original recording hash/full decode; not E2E',
            'file': str(path), 'size': path.stat().st_size, 'sha256': digest,
            'duration': float(probe['format']['duration']), 'overlapMs': receipt['overlapMs'],
            'stats': receipt['stats'], 'video': video, 'audio': audio}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('directory', type=pathlib.Path)
    parser.add_argument('--seconds', type=int, required=True)
    args = parser.parse_args()
    result = verify(args.directory.resolve(), args.seconds)
    (args.directory / 'original-recording-validation.json').write_text(
        json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps({'passed': True, 'sha256': result['sha256'],
                      'duration': result['duration'], 'overlapMs': result['overlapMs']}))
