#!/usr/bin/env python3
"""Explicitly refresh the source lock after merging feature tips into integration."""
import json
import pathlib
import subprocess
root=pathlib.Path(__file__).resolve().parents[2]
def git(*args): return subprocess.check_output(['git','-C',str(root),*args],text=True).strip()
path=root/'infrastructure/macos/integration-features.json'
lock=json.loads(path.read_text()); entries={item['branch']:item for item in lock['features']}
for branch in git('for-each-ref','--format=%(refname:short)','refs/heads/feature/macos-*','refs/heads/experiment/macos-*').splitlines():
    commit=git('rev-parse',branch)
    if subprocess.run(['git','-C',str(root),'merge-base','--is-ancestor',commit,'HEAD']).returncode: continue
    entry=entries.get(branch,{'branch':branch,'scope':'research' if branch.startswith('experiment/') else 'implementation','acceptance':'pending'})
    if entry.get('commit') != commit: entry['acceptance']='pending'
    entry['commit']=commit; entries[branch]=entry
lock['features']=[entries[key] for key in sorted(entries)]
path.write_text(json.dumps(lock,indent=2)+'\n')
print('Locked',len(entries),'integrated feature tips; changed tips remain pending acceptance')
