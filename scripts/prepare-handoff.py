import argparse, hashlib, json, pathlib, shutil, zipfile, importlib.util
spec=importlib.util.spec_from_file_location('qa',pathlib.Path(__file__).with_name('verify-media.py'))
qa=importlib.util.module_from_spec(spec);spec.loader.exec_module(qa)
p=argparse.ArgumentParser();p.add_argument('source',type=pathlib.Path);p.add_argument('destination',type=pathlib.Path);p.add_argument('--seconds',type=int,required=True);p.add_argument('--commit',required=True);args=p.parse_args()
source=args.source.resolve();dest=args.destination.resolve()
assert not dest.exists(), 'destination must be new and immutable'
qa.verify(source,args.seconds)
dest.mkdir(parents=True)
original=json.loads((source/'inventory.json').read_text(encoding='utf-8'))
objects=[]
for r in ['1080p','720p','480p']:
    (dest/r).mkdir()
    text=(source/r/'index.m3u8').read_text(encoding='utf-8')
    for old in sorted((source/r).glob('*.m4s')):
        number=int(old.stem.split('-')[-1]);new=f'seg-{number:06d}.m4s'
        shutil.copyfile(old,dest/r/new);text=text.replace(old.name,new)
    shutil.copyfile(source/r/'init.mp4',dest/r/'init.mp4')
    (dest/r/'index.m3u8').write_text(text,encoding='utf-8',newline='\n')
master='#EXTM3U\n#EXT-X-VERSION:7\n#EXT-X-INDEPENDENT-SEGMENTS\n'
for h,w,b in [(1080,1920,3400000),(720,1280,1800000),(480,854,1100000)]:
    master+=f'#EXT-X-STREAM-INF:BANDWIDTH={b},RESOLUTION={w}x{h},FRAME-RATE=29.970\n{h}p/index.m3u8\n'
(dest/'master.m3u8').write_text(master,encoding='utf-8',newline='\n')
for file in sorted(dest.rglob('*')):
    if file.is_file(): objects.append({'path':file.relative_to(dest).as_posix(),'size':file.stat().st_size,'sha256':hashlib.file_digest(file.open('rb'),'sha256').hexdigest()})
original['objects']=objects;original['producerCommit']=args.commit
original['packaging']='Filename/reference normalization plus master playlist only; media bytes unchanged.'
original['contractRevision']=None;original['consumerReceipt']=None
(dest/'inventory.json').write_text(json.dumps(original,ensure_ascii=False,indent=2),encoding='utf-8')
for name in ['HANDOFF.md','preview.png','machine-and-progress.md']:
    if (source/name).exists():shutil.copyfile(source/name,dest/name)
result=qa.verify(dest,args.seconds)
(dest/'local-validation.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
with zipfile.ZipFile(str(dest)+'.zip','x',compression=zipfile.ZIP_STORED,allowZip64=True) as z:
    for file in sorted(dest.rglob('*')):
        if file.is_file():z.write(file,file.relative_to(dest).as_posix())
archive=pathlib.Path(str(dest)+'.zip');digest=hashlib.file_digest(archive.open('rb'),'sha256').hexdigest()
archive.with_suffix('.zip.sha256').write_text(digest+'  '+archive.name+'\n')
print(json.dumps({'artifact':str(archive),'sha256':digest,'size':archive.stat().st_size,'consumerReceipt':None}))
