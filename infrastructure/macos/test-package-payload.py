#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Inspect real staged payloads without running Installer or touching an install."""
import argparse
import hashlib
import json
import pathlib
import subprocess
import tempfile
from packagepayload import prepare, SUPPORT
root=pathlib.Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('--artifacts',required=True,type=pathlib.Path);args=parser.parse_args()
bundle=args.artifacts/'io-helper/Unbound.dvcp.bundle'
runtime=json.loads((bundle/'Contents/Resources/build-manifest.json').read_text())
inputs={'version':(root/'VERSION').read_text().strip(),'commit':runtime['commit'],'runtime_manifest':runtime,'bundle':bundle}
with tempfile.TemporaryDirectory(prefix='opengate-payload-') as temporary:
    base=pathlib.Path(temporary);payload=base/'payload';scripts=base/'scripts'
    metadata=prepare(root,args.artifacts,inputs,payload,scripts)
    paths=(scripts/'paths.txt').read_text().splitlines()
    assert metadata['version']==inputs['version'] and metadata['architecture']=='universal'
    assert all('/Users/' not in path and '/Library/Application Support/Unbound' not in path for path in paths)
    assert not any('@VERSION@' in p.read_text() for p in scripts.iterdir())
    count=0
    for line in (scripts/'hashes.tsv').read_text().splitlines():
        expected,path=line.split('\t',1)
        assert hashlib.sha256((payload/path.lstrip('/')).read_bytes()).hexdigest()==expected
        count+=1
    assert count>30
    entries=json.loads((payload/SUPPORT/'Presets/Arrangements.json').read_text())['arrangements']
    assert [e['name'] for e in entries]==['Example Preset']
    workspace=json.loads((payload/SUPPORT/'Presets/Workspace.json').read_text())
    assert workspace['selected_arrangement']=='opengate.resolve-layout.edit'
    assert (payload/SUPPORT/'uninstall.command').is_file() and (payload/SUPPORT/'export-support.command').is_file()
    for path in scripts.iterdir():
        if path.suffix in ('.sh','.command') or path.name in ('preinstall','postinstall'):
            subprocess.run(['/bin/bash','-n',str(path)],check=True)
    print('PASS: real universal payload, exact inventory/hashes, current version, optional the support team preset, Resolve Layout default and maintenance scripts')
