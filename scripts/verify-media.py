"""Local media QA only. Does not replace Mac V1 Asset validator."""
import argparse, hashlib, json, pathlib, re, subprocess, sys

def verify(root, expected):
    inventory=json.loads((root/'inventory.json').read_text(encoding='utf-8'))
    assert inventory['normalEnd'], 'not a normal end'
    assert inventory['fpsNum']==30000 and inventory['fpsDen']==1001
    objects={o['path']:o for o in inventory['objects']}
    assert len(objects)==len(inventory['objects']), 'duplicate inventory object'
    for key, meta in objects.items():
        assert re.fullmatch(r'master\.m3u8|(?:1080p|720p|480p)/(?:init\.mp4|index\.m3u8|(?:segment-\d{5}|seg-\d{6})\.m4s)',key), 'unsafe media path'
        path=root/key
        assert path.stat().st_size==meta['size'], key
        with path.open('rb') as stream:
            assert hashlib.file_digest(stream,'sha256').hexdigest()==meta['sha256'], key
    if (root/'master.m3u8').exists():
        assert 'master.m3u8' in objects, 'untracked master playlist'
        master=(root/'master.m3u8').read_text()
        assert re.findall(r'^(\d+p/index\.m3u8)$',master,re.M)==['1080p/index.m3u8','720p/index.m3u8','480p/index.m3u8']
    results=[]
    timeline=None
    for height,width in [(1080,1920),(720,1280),(480,854)]:
        base=root/f'{height}p'
        playlist=(base/'index.m3u8').read_text()
        assert '#EXT-X-ENDLIST' in playlist
        segments=re.findall(r'^(?:segment-\d{5}|seg-\d{6})\.m4s$',playlist,re.M)
        times=[float(x) for x in re.findall(r'#EXTINF:([0-9.]+)',playlist)]
        assert len(times)==len(segments)
        assert len(times)==3 if expected==61 else len(times)>0
        assert abs(sum(times)-expected)<0.2, (height,'duration',sum(times))
        if timeline is None: timeline=times
        else: assert times==timeline,(height,'unaligned rendition',times,timeline)
        for filename in ['init.mp4','index.m3u8',*segments]:
            assert f'{height}p/{filename}' in objects, 'untracked media object'
        probe=json.loads(subprocess.check_output(['ffprobe','-v','error','-show_streams','-of','json',str(base/'index.m3u8')]))
        video=next(s for s in probe['streams'] if s['codec_type']=='video')
        audio=next(s for s in probe['streams'] if s['codec_type']=='audio')
        assert (video['codec_name'],video['width'],video['height'],video['r_frame_rate'])==('h264',width,height,'30000/1001')
        assert (audio['codec_name'],audio['sample_rate'],audio['channels'])==('aac','48000',2)
        # Full decode catches corrupted/timestamp-invalid packets; not server validation.
        decoded=subprocess.run(['ffmpeg','-v','error','-copyts','-i',str(base/'index.m3u8'),'-f','null','-'],capture_output=True,check=True)
        assert not decoded.stderr, decoded.stderr.decode(errors='replace')
        results.append({'height':height,'segments':len(times),'durations':times,'total':sum(times),'video':video,'audio':audio})
    return {'evidence':'local ffprobe/full decode/hash QA; not Mac V1','renditions':results}

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('directory',type=pathlib.Path);p.add_argument('--seconds',type=int,required=True);a=p.parse_args()
    result=verify(a.directory,a.seconds)
    (a.directory/'local-validation.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    print(json.dumps({'passed':True,'renditions':[{'height':r['height'],'segments':r['segments'],'duration':r['total']} for r in result['renditions']]}))


