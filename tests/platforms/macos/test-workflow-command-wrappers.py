#!/usr/bin/env python3
import json,os,runpy,subprocess,tempfile,sys
from types import SimpleNamespace as Obj
from pathlib import Path
root=Path(__file__).resolve().parents[3]
platform='windows' if '--windows' in sys.argv else 'macos'
colors='--color-manager' in sys.argv
extra=['--color-manager'] if colors else []
if platform=='windows':
    subprocess.run([sys.executable,str(root/'infrastructure/shared/build-workflow-commands.py'),'--platform','windows']+extra,check=True)
else:
    subprocess.run([sys.executable,str(root/'infrastructure/macos/build-workflow-commands.py')]+extra,check=True)
output=root/('build/'+platform+'/workflow-commands')
manifest=json.loads((output/'manifest.json').read_text())
count=39 if colors else 37
assert len({entry['command'] for entry in manifest})==len(manifest)==count
assert ({'color-manager.open','color-display.toggle-textures'} <= {e['command'] for e in manifest}) == colors
assert {'clip-color.timeline.red','clip-color.timeline.clear','clip-color.media-pool.red','clip-color.media-pool.clear'} <= {e['command'] for e in manifest}
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
        assert len(requests)==count-1
        assert {json.loads(p.read_text())['command'] for p in requests}=={e['command'] for e in manifest if e['command']!='edit.inspector.stabilize'}
        results=list((Path(directory)/'command-results').glob('*.json'))
        assert calls==['stabilize'] and len(results)==1
        result=json.loads(results[0].read_text())
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
        assert len(fallback)==count
        stabilize=next(e for e in manifest if e['command']=='edit.inspector.stabilize')
        # No initialized Resolve context fails explicitly; it cannot fall back to
        # a legacy registry request or to the clip underneath the playhead.
        runpy.run_path(str(output/stabilize['file']))
        results=[json.loads(p.read_text()) for p in (Path(directory)/'command-results').glob('*.json')]
        assert len(results)==2 and any(r.get('reason')=='resolve-context-unavailable' for r in results)
        assert len(list((Path(directory)/'command-requests').glob('*.json')))==count
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
print('PASS:',count-1,'atomic workflow requests and one direct selected-clip API command, without recursive requests')
