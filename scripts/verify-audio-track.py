"""Measure real OBS fixture AAC output; wire mocks and silent media cannot prove routing."""
import argparse
import array
import importlib.util
import json
import math
import pathlib
import subprocess
import tempfile

spec = importlib.util.spec_from_file_location('media_qa', pathlib.Path(__file__).with_name('verify-media.py'))
media_qa = importlib.util.module_from_spec(spec)
spec.loader.exec_module(media_qa)


def verify(directory, track, tone):
    root = media_qa.local_path(directory)
    inventories = list(root.rglob('inventory.json'))
    assert len(inventories) == 1, 'one completed local recording required'
    media = inventories[0].parent
    metadata = json.loads((media / 'local-session.json').read_text(encoding='utf-8'))
    assert metadata['audioTrack'] == track, 'selected dock audio track changed'
    results = []
    for rung in ('1080p', '720p', '480p'):
        source = media_qa.ffmpeg_path(media / rung / 'index.m3u8')
        decoded = subprocess.run(['ffmpeg', '-v', 'error', '-i', source, '-map', '0:a:0',
                                  '-af', 'pan=mono|c0=c0', '-ar', '48000', '-f', 'f32le', 'pipe:1'],
                                 cwd=tempfile.gettempdir(), capture_output=True, check=True)
        assert not decoded.stderr, decoded.stderr.decode(errors='replace')
        samples = array.array('f')
        samples.frombytes(decoded.stdout)
        samples = samples[48000:96000]  # Interior second avoids AAC priming and final padding.
        assert len(samples) == 48000, 'audio too short'
        rms = math.sqrt(sum(x * x for x in samples) / len(samples))
        amplitudes = {}
        for hz in (440, 880):
            omega = 2 * math.pi * hz / 48000
            real = sum(x * math.cos(omega * i) for i, x in enumerate(samples))
            imaginary = sum(x * math.sin(omega * i) for i, x in enumerate(samples))
            amplitudes[hz] = 2 * math.hypot(real, imaginary) / len(samples)
        if tone:
            assert rms > 0.10 and 0.15 < amplitudes[tone] < 0.25, (rung, 'selected tone missing', rms, amplitudes)
            other = 880 if tone == 440 else 440
            assert amplitudes[other] < 0.02, (rung, 'unexpected tone', amplitudes)
        else:
            assert rms < 0.00001, (rung, 'unselected or muted audio leaked', rms)
        results.append({'rung': rung, 'rms': rms, 'amplitudes': amplitudes})
    return {'evidence': 'local real OBS selected/muted audio routing; not E2E',
            'selectedTrack': track, 'expectedToneHz': tone, 'renditions': results}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('directory', type=pathlib.Path)
    parser.add_argument('--track', type=int, choices=range(1, 7), required=True)
    parser.add_argument('--tone-hz', type=int, choices=(0, 440, 880), required=True)
    args = parser.parse_args()
    result = verify(args.directory.resolve(), args.track, args.tone_hz)
    (args.directory / 'audio-routing-validation.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps({'passed': True, 'track': args.track, 'toneHz': args.tone_hz,
                      'rms': [r['rms'] for r in result['renditions']]}))
