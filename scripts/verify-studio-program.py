"""Verify actual OBS Program pixels while Preview changes and a transition runs."""
import argparse
import importlib.util
import json
import pathlib
import subprocess
import tempfile

spec = importlib.util.spec_from_file_location('media_qa', pathlib.Path(__file__).with_name('verify-media.py'))
media_qa = importlib.util.module_from_spec(spec)
spec.loader.exec_module(media_qa)


def verify(directory):
    root = media_qa.local_path(directory)
    receipt = json.loads((root / 'studio-program.json').read_text(encoding='utf-8'))
    observations = {item['stage']: item for item in receipt['observations']}
    for stage, program, preview in [('preview-b', 'HHC studio A', 'HHC studio B'),
                                     ('transition-complete', 'HHC studio B', 'HHC studio A'),
                                     ('preview-b-after-transition', 'HHC studio B', 'HHC studio B'),
                                     ('preview-a-after-transition', 'HHC studio B', 'HHC studio A')]:
        item = observations[stage]
        assert item['studioMode'] and (item['program'], item['preview']) == (program, preview), item
    inventories = list(root.rglob('inventory.json'))
    assert len(inventories) == 1, 'one completed recording required'
    media = inventories[0].parent
    media_qa.verify(media, 12)  # Full hashes, frame rate, all three timelines and decode.
    samples = []
    for rung in ('1080p', '720p', '480p'):
        for second, green in [(2, False), (4, False), (7, True), (10, True)]:
            decoded = subprocess.run(['ffmpeg', '-v', 'error', '-i',
                                      media_qa.ffmpeg_path(media / rung / 'index.m3u8'),
                                      '-ss', str(second), '-frames:v', '1',
                                      '-vf', 'crop=32:32:16:16,scale=1:1',
                                      '-pix_fmt', 'rgb24', '-f', 'rawvideo', 'pipe:1'],
                                     cwd=tempfile.gettempdir(), capture_output=True, check=True)
            assert not decoded.stderr and len(decoded.stdout) == 3, 'one decoded RGB pixel required'
            red, g, blue = decoded.stdout
            if green:
                assert g > 200 and red < 30 and blue < 30, (rung, second, list(decoded.stdout))
            else:
                assert red > 70 and g < 60 and blue < 60, (rung, second, list(decoded.stdout))
            samples.append({'rung': rung, 'second': second, 'rgb': list(decoded.stdout)})
    return {'evidence': 'real local OBS Studio Mode/Program/Preview/transition; not E2E',
            'observations': receipt['observations'], 'pixels': samples}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('directory', type=pathlib.Path)
    args = parser.parse_args()
    result = verify(args.directory.resolve())
    (args.directory / 'studio-program-validation.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps({'passed': True, 'decodedPixels': len(result['pixels'])}))
