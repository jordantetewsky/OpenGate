#!/usr/bin/env python3
"""Prepare a first-install preview without editing the normal workspace files."""
import argparse
import datetime
import json
import os
from pathlib import Path
import subprocess

def atomic_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary=path.with_suffix(path.suffix+'.new')
    temporary.write_text(json.dumps(value,indent=2)+'\n')
    os.replace(temporary,path)

def seed(directory, bundle):
    workspace=directory/'Workspace.json'; arrangements=directory/'Arrangements.json'
    if workspace.exists() or arrangements.exists():
        raise RuntimeError('Settings already exist; preserving them rather than resetting the profile.')
    chosen=next(e for e in bundle['arrangements'] if e['id']==bundle['default'])
    state=dict(chosen['workspace']); state['selected_arrangement']=chosen['id']
    # The immutable default is supplied by the runtime, never a writable record.
    legacy=[dict(entry) for entry in bundle['arrangements'] if not entry.get('read_only')]
    atomic_json(arrangements,{'version':1,'arrangements':legacy})
    atomic_json(workspace,state)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode',choices=['preview','normal','seed'])
    parser.add_argument('--base',type=Path,default=Path.home()/'Library/Application Support/Unbound')
    parser.add_argument('--bundle',type=Path,default=Path(__file__).resolve().parents[2]/'src/assets/presets/BundledArrangements.json')
    args=parser.parse_args()
    process=subprocess.run(['pgrep','-x','Resolve'],capture_output=True)
    if process.returncode!=1: raise SystemExit('Close Resolve before selecting or initializing a settings profile.')
    bundle=json.loads(args.bundle.read_text()); marker=args.base/'Profile.json'
    if args.mode=='seed': seed(args.base,bundle); return
    if args.mode=='preview':
        target=args.base/'Profiles/first-install-preview'
        if not (target/'Workspace.json').exists(): seed(target,bundle)
    if marker.exists():
        stamp=datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ')
        backup=args.base/'Profiles/checkpoints'/('Profile-'+stamp+'.json')
        backup.parent.mkdir(parents=True,exist_ok=True); backup.write_bytes(marker.read_bytes())
    atomic_json(marker,{'active':'first-install-preview' if args.mode=='preview' else ''})
    print('Selected',args.mode,'profile. Normal Workspace.json, Arrangements.json and Shortcuts.json were not edited.')

if __name__=='__main__': main()
