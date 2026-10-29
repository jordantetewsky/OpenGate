#!/usr/bin/env python3
"""Hide existing arrangements reversibly; never delete records or alter hotkeys."""
import argparse,datetime,hashlib,json,os,shutil,subprocess
from pathlib import Path

def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest() if path.exists() else None

def prepare(base):
    base=Path(base);marker=base/'Profile.json'
    profile=json.loads(marker.read_text()).get('active','') if marker.exists() else ''
    directory=base/'Profiles/first-install-preview' if profile=='first-install-preview' else base
    workspace=directory/'Workspace.json';arrangements=directory/'Arrangements.json'
    state=json.loads(workspace.read_text()) if workspace.exists() else {}
    view=state.get('page_workspaces',{}).get('edit',{}).get('last_view',state)
    if view.get('groups') or view.get('native_arrangement') or any(p.get('floating') or 'native_hidden' in p for p in view.get('panels',{}).values()):
        raise RuntimeError('Choose Native Edit and quit Resolve before preparing the clean arrangement view.')
    document=json.loads(arrangements.read_text()) if arrangements.exists() else {'version':1,'arrangements':[]}
    if document.get('version')!=1 or not isinstance(document.get('arrangements'),list):raise RuntimeError('Unknown arrangement format; preserving it unchanged.')
    stamp=datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
    backup=base/'Profiles/checkpoints'/('before-clean-arrangements-'+stamp);backup.mkdir(parents=True)
    before={str(p):digest(p) for p in (marker,workspace,arrangements,directory/'Shortcuts.json')}
    for label,path in [('Profile.json',marker),('Workspace.json',workspace),('Arrangements.json',arrangements),('Shortcuts.json',directory/'Shortcuts.json')]:
        if path.exists():shutil.copy2(path,backup/label)
    original=[dict(e) for e in document['arrangements']]
    for entry in document['arrangements']:entry['hidden']=True
    if original:
        temporary=arrangements.with_name(arrangements.name+'.clean-start.tmp')
        temporary.write_text(json.dumps(document,indent=2)+'\n');os.replace(temporary,arrangements)
    for path in (marker,workspace,directory/'Shortcuts.json'):assert digest(path)==before[str(path)],str(path)
    assert [{k:v for k,v in e.items() if k!='hidden'} for e in original]==[{k:v for k,v in e.items() if k!='hidden'} for e in document['arrangements']]
    receipt={'operation':'hide-existing-arrangements','profile':profile or 'normal','backup':str(backup),'count':len(original),'workspace_unchanged':True,'shortcuts_unchanged':True,'records_preserved':True,'before_sha256':before,'after_arrangements_sha256':digest(arrangements)}
    (backup/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');return receipt

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--base',type=Path,default=Path.home()/'Library/Application Support/Unbound');args=parser.parse_args()
    if subprocess.run(['pgrep','-x','Resolve'],capture_output=True).returncode!=1:raise SystemExit('Resolve must be closed and its state verifiable.')
    print(json.dumps(prepare(args.base),indent=2))
if __name__=='__main__':main()
