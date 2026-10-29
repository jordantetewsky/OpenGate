#!/usr/bin/env python3
import json,os,runpy,subprocess,tempfile,sys
from types import SimpleNamespace as Obj
from pathlib import Path
root=Path(__file__).resolve().parents[3]
platform='windows' if '--windows' in sys.argv else 'macos'
if platform=='windows':
    subprocess.run([sys.executable,str(root/'infrastructure/shared/build-workflow-commands.py'),'--platform','windows'],check=True)
else:
    subprocess.run([sys.executable,str(root/'infrastructure/macos/build-workflow-commands.py')],check=True)
output=root/('build/'+platform+'/workflow-commands')
manifest=json.loads((output/'manifest.json').read_text())
assert len({entry['command'] for entry in manifest})==len(manifest)==34
with tempfile.TemporaryDirectory() as directory:
    old=os.environ.get('UNBOUND_LAUNCH_DIRECTORY'); os.environ['UNBOUND_LAUNCH_DIRECTORY']=directory
    try:
        calls=[]
        clip=Obj(GetTrackTypeAndIndex=lambda:['video',1],Stabilize=lambda:calls.append('stabilize') or True)
        timeline=Obj(GetSelectedClips=lambda:[clip])
        project=Obj(GetCurrentTimeline=lambda:timeline)
        resolve=Obj(GetCurrentPage=lambda:'edit',GetProjectManager=lambda:Obj(GetCurrentProject=lambda:project))
        for entry in manifest:
            runpy.run_path(str(output/entry['file']),init_globals={'resolve':resolve})
        requests=list((Path(directory)/'command-requests').glob('*.json'))
        assert len(requests)==33
        assert {json.loads(p.read_text())['command'] for p in requests}=={e['command'] for e in manifest if e['command']!='edit.inspector.stabilize'}
        results=list((Path(directory)/'command-results').glob('*.json'))
        assert calls==['stabilize'] and len(results)==1
        result=next(json.loads(p.read_text()) for p in results if json.loads(p.read_text())['command']=='edit.inspector.stabilize')
        assert result['success'] and result['completed']==1 and result['command']=='edit.inspector.stabilize'
        assert not list((Path(directory)/'command-requests').glob('*.tmp'))
        # Resolve's embedded Python may lack the optional ctypes extension.
        # The request must still be published for the filesystem wake-up path.
        had_ctypes='ctypes' in sys.modules;original_ctypes=sys.modules.get('ctypes')
        sys.modules['ctypes']=None
        try:
            runpy.run_path(str(output/manifest[0]['file']))
        finally:
            if had_ctypes:sys.modules['ctypes']=original_ctypes
            else:sys.modules.pop('ctypes',None)
        fallback=list((Path(directory)/'command-requests').glob('*.json'))
        assert len(fallback)==34
        stabilize=next(e for e in manifest if e['command']=='edit.inspector.stabilize')
        # No initialized Resolve context fails explicitly; it cannot fall back to
        # a legacy registry request or to the clip underneath the playhead.
        runpy.run_path(str(output/stabilize['file']))
        results=[json.loads(p.read_text()) for p in (Path(directory)/'command-results').glob('*.json')]
        assert len(results)==2 and any(r.get('reason')=='resolve-context-unavailable' for r in results)
        assert len(list((Path(directory)/'command-requests').glob('*.json')))==34
        # All four commands use the same read-only planner and never write media.
        Clip=runpy.run_path(str(root/'tests/core/operations/test_audio_channel.py'))['Clip']
        audio=Clip('a',1,5);media=Path(directory)/'fixture.wav';media.write_bytes(b'availability fixture')
        audio.path=str(media)
        audio.source_mapping['track_mapping']={str(i):{'channel_idx':[i],'type':'mono','mute':False} for i in (3,4,5)}
        timeline.GetSelectedClips=lambda:[audio];timeline.GetUniqueId=lambda:'timeline'
        project.GetUniqueId=lambda:'project'
        import time,uuid
        for entry in [e for e in manifest if e['command'].startswith('edit.audio.')]:
            token=str(uuid.uuid4())
            (Path(directory)/'audio-channel-plan-job.json').write_text(json.dumps({'protocol':'native-audio-plan-v3','token':token,'command':entry['command'],'issued_ms':int(time.time()*1000)}))
            runpy.run_path(str(output/entry['file']),init_globals={'resolve':resolve})
            plan=json.loads((Path(directory)/'audio-channel-plans'/(token+'.json')).read_text())
            assert plan['success'] and plan['phase']=='plan-only' and plan['changed']==0
            previous='previous' in entry['command'];all_source=entry['command'].endswith('-all-source')
            assert plan['items'][0]['after']==(5 if previous else (2 if all_source else 3))
            assert audio.channel()==1 and audio.writes==[]
        # A queued planner arriving after cancellation cannot replay the command.
        entry=next(e for e in manifest if e['command'].startswith('edit.audio.'))
        token=str(uuid.uuid4())
        (Path(directory)/'audio-channel-plan-job.json').write_text(json.dumps({'protocol':'native-audio-plan-v3','token':token,'command':entry['command'],'issued_ms':int(time.time()*1000),'cancelled':True}))
        runpy.run_path(str(output/entry['file']),init_globals={'resolve':resolve})
        assert not (Path(directory)/'audio-channel-plans'/(token+'.json')).exists()
        assert audio.writes==[]
        (Path(directory)/'audio-channel-plan-job.json').write_text(json.dumps({'protocol':'native-audio-plan-v3','token':token,'command':entry['command'],'issued_ms':int(time.time()*1000),'inactive':True}))
        runpy.run_path(str(output/entry['file']),init_globals={'resolve':resolve})
        assert not (Path(directory)/'audio-channel-plans'/(token+'.json')).exists()
        # A different late script cannot turn an existing inactive ticket into
        # a fresh registry request, and malformed tickets fail closed as well.
        different=next(e for e in manifest if e['command'].startswith('edit.audio.') and e['command']!=entry['command'])
        runpy.run_path(str(output/different['file']),init_globals={'resolve':resolve})
        (Path(directory)/'audio-channel-plan-job.json').write_text('{broken')
        runpy.run_path(str(output/different['file']),init_globals={'resolve':resolve})
        assert audio.writes==[]
        assert len(list((Path(directory)/'command-requests').glob('*.json')))==34
        if platform=='windows':
            runpy.run_path(str(output/'OpenGate.py'))
            first=(Path(directory)/'show-controls.request').read_text()
            runpy.run_path(str(output/'OpenGate.py'))
            second=(Path(directory)/'show-controls.request').read_text()
            assert first.startswith('workflow-') and second.startswith('workflow-') and first!=second
            launches=[json.loads(line) for line in (Path(directory)/'launches.jsonl').read_text().splitlines()]
            assert [item['request'] for item in launches]==[first,second]
            assert all(item['component']=='OpenGate' and item['event']=='workflow.launch' for item in launches)
            assert len(list(Path(directory).glob('*.request')))==1
    finally:
        if old is None: os.environ.pop('UNBOUND_LAUNCH_DIRECTORY',None)
        else: os.environ['UNBOUND_LAUNCH_DIRECTORY']=old
print('PASS: 33 registry requests, four read-only native audio plans and one stabilization API wrapper')
